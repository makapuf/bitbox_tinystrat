#include "bitbox.h"
#include <string.h>

#define DATA_IMPLEMENTATION
#include "data.h"
#undef DATA_IMPLEMENTATION
#define TINYSTRAT_IMPLEMENTATION
#include "tinystrat_defs.h"
#undef TINYSTRAT_IMPLEMENTATION

#include "lib/blitter/blitter.h"


#define SCREEN_W 25
#define SCREEN_H 19


object *cursor, *square_cursor, *wars;
uint16_t vram[SCREEN_W*SCREEN_H];

enum Player_type_enum {
	player_notused,
	player_human,
	player_cpu1,
};

struct PlayerInfo {
	uint8_t type; // player_type 
	uint8_t resources[5];	// gold to food
	
	object *units_o [16]; 	   // frame as unit type, ptr=0 if not allocated.?
	object *units_health [16]; // same position as unit but frame=health
};

struct GameInfo {
	uint8_t day;
	struct PlayerInfo player_info[4]; // by color
	object *map;
} game_info;

// global game element init
void game_init(void)
{
	game_info.map = tilemap_new (
		&data_tiles_bg_tset[4], // no palette
		0,0,
		TMAP_HEADER(SCREEN_W,SCREEN_H,TSET_16,TMAP_U16), 
		vram
		);
	// for game only, not intro 

	// init play ? level ?
	square_cursor = sprite3_new(data_misc_spr, 0,32,1); // jusr below mouser cursor
	cursor = sprite3_new(data_misc_spr, 0,0,0);
	cursor->fr=misc_mouse;

}

const char  * const sprite_colors[4] = {
	data_blue_spr,
	data_red_spr,
	data_yellow_spr,
	data_green_spr,
};

void load_level(int map)
{
	// copy level from flash to vram (direct copy)
	memcpy(vram, &data_map_map[8+sizeof(vram)*map],sizeof(vram));
	if (!map) return ;

	for (int ply=0;ply<4;ply++) {
		int nunits=0;
		for (int i=0;i<16;i++) {
			uint8_t *unit = &level_units[map-1][ply][i][0];
			if (unit[2]) {
				// allocate sprite
				if (unit[2]==16) { // special : flag - or put into units
				} else {
					object *o=sprite3_new(sprite_colors[ply], unit[0]*16,unit[1]*16,5); //below cursors
					o->fr = (unit[2]-1)*8;
					game_info.player_info[ply].units_o[nunits++] = o;
				}
			}
		}
	}
}


void leave_level()
{
	// release all blitter sprite

}

void intro()
{
	object *wars = sprite3_new(data_wars_spr, 0,-200,10);
	load_level(0);
	while (!GAMEPAD_PRESSED(0,start)) {
		if (wars->y<120)
			wars->y+=16;
		wait_vsync(1);

	}
	blitter_remove(wars);
}

void update_info_cursor(void)
{
	// "blink" cursor
	square_cursor->fr=(vga_frame/32)%2 ? cursor_cursor : cursor_cursor2;

	// update info about the terrain / units under the cursor
	const uint16_t tile_id = vram[(square_cursor->y/16)*SCREEN_W + square_cursor->x/16];
	int terrain_id = tile_terrain[tile_id];
	vram[3]=tile_id; 
	// name
	vram[4]=terrain_tiledef[terrain_id];
	vram[5]=terrain_tiledef[terrain_id]+1;
	// defense
	vram[6]=tile_zero + terrain_defense[terrain_id];
}

void game()
{

	load_level(1);
	// reset game data

	uint16_t gamepad_pressed, gamepad_previous=0;
	while(1) {
		gamepad_pressed  = gamepad_buttons[0] & ~gamepad_previous;
		gamepad_previous = gamepad_buttons[0];

		// mouse
		cursor->x = mouse_x; 
		cursor->y = mouse_y;
		if (cursor->x<0) cursor->x=0;
		if (cursor->y<0) cursor->y=0;
		if (cursor->x>VGA_H_PIXELS) cursor->x = VGA_H_PIXELS;
		if (cursor->y>VGA_V_PIXELS) cursor->y = VGA_V_PIXELS;		

		// square cursor, mouve on click
		if (mouse_buttons & mousebut_left) {	
			square_cursor->x = cursor->x & ~0xf;
			square_cursor->y = cursor->y & ~0xf;
		}
		
		if (gamepad_pressed & gamepad_left && square_cursor->x > 0) 
			square_cursor->x -= 16;
		if (gamepad_pressed & gamepad_right && square_cursor->x < VGA_H_PIXELS-16) 
			square_cursor->x += 16;
		if (gamepad_pressed & gamepad_up && square_cursor->y > 32) 
			square_cursor->y -= 16;
		if (gamepad_pressed & gamepad_down && square_cursor->y < VGA_V_PIXELS-16) 
			square_cursor->y += 16;

		update_info_cursor();



		wait_vsync(1);
	}

	leave_level();

}

void bitbox_main()
{
	// load all
	game_init();
	intro();
	game();

}
