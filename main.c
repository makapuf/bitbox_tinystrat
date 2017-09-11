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

#define MENU_X 10
#define MENU_Y 5


// object *mouse_cursor;
uint16_t vram[SCREEN_W*SCREEN_H];
uint16_t backup_vram[5*5]; // extra for menus

enum Player_type_enum {
	player_notused,
	player_human,
	player_cpu1,
};

struct PlayerInfo {
	uint8_t type; // player_type 
	uint8_t resources[5];	// gold -> food
	
	object *units_o [16]; 	   // frame as unit type, ptr=0 if not allocated.
	object *units_health [16]; // same position as unit but frame=health

	uint16_t has_moved; // object has moved this turn.
};

struct GameInfo {
	uint8_t day;
	uint8_t current_player; // 0-3
	int8_t cursor_unit_pl;  // player id of object under cursor, <0 : None 
	int8_t cursor_unit_id;  // id in current player object array. 
	struct PlayerInfo player_info[4]; // by color
	object *map;
	object *cursor;

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
	game_info.cursor = sprite3_new(data_misc_spr, 0,1024,1); // jusr below mouser mouse_cursor
	#if 0
	mouse_cursor = sprite3_new(data_misc_spr, 0,0,0);
	mouse_cursor->fr=frame_misc_mouse;
	#endif 

}

const char *const sprite_colors[4] = {
	data_blue_spr,
	data_red_spr,
	data_yellow_spr,
	data_green_spr,
};

void load_level(int map)
{
	// copy level from flash to vram (direct copy)
	memcpy(vram, &data_map_map[8+sizeof(vram)*map],sizeof(vram));
	if (!map) return;

	for (int ply=0;ply<4;ply++) {
		int nunits=0;
		for (int i=0;i<16;i++) {
			uint8_t *unit = &level_units[map-1][ply][i][0];
			if (unit[2]) {
				// allocate sprite
				if (unit[2]==16) { 
					// special : flag - or put into units
				} else {
					object *o =sprite3_new(sprite_colors[ply], unit[0]*16,unit[1]*16,5); // below cursors
					o->fr = (unit[2]-1)*8;
					game_info.player_info[ply].units_o[nunits] = o;

					object *oh=sprite3_new(data_misc_spr, unit[0]*16,unit[1]*16,4); // top of prec, below mouse_cursor
					oh->fr = frame_number_0+10; // default to health 10
					game_info.player_info[ply].units_health[nunits] = oh;

					nunits++;

					message("init unit %d,%d : %d\n",o->x, o->y, unit[2]-1);

				}
			}
		}
	}
}


void leave_level()
{
	// release all blitter sprites


}

void intro()
{
	object *tiny = sprite3_new(data_tiny_spr, -16,40,10);

	load_level(0);

	wait_vsync(30);
	
	for (int i=0;i<100;i++) {
		tiny->x += 2;
		wait_vsync(1);
	}
	wait_vsync(60);

	object *wars = sprite3_new(data_wars_spr, 0,-200,10);
	while (!GAMEPAD_PRESSED(0,start)) {
		if (wars->y<120)
			wars->y+=16;
		wait_vsync(1);

	}
	blitter_remove(wars);
	blitter_remove(tiny);
}


/* updates info about terrain under mouse_cursor 
   returns the unit under mouse_cursor if any or NULL
   */
void update_cursor_info(void)
{
	object * const cursor = game_info.cursor;

	// "blink" mouse_cursor
	cursor->fr=(vga_frame/32)%2 ? frame_cursor_cursor : frame_cursor_cursor2;

	// terrain under the mouse_cursor
	const uint16_t tile_id = vram[(cursor->y/16)*SCREEN_W + cursor->x/16];
	int terrain_id = tile_terrain[tile_id];
	vram[3]=tile_id; 
	// name
	vram[4]=terrain_tiledef[terrain_id];
	vram[5]=terrain_tiledef[terrain_id]+1;
	// defense
	vram[6]=tile_zero + terrain_defense[terrain_id];

	// find unit under mouse_cursor / if any
	int8_t *op = &game_info.cursor_unit_pl;
	int8_t *oi = &game_info.cursor_unit_id;

	object *obj;
	for (*op=0;*op<4;(*op)++) {
		for (*oi=0;*oi<16;(*oi)++) {
			obj = game_info.player_info[*op].units_o[*oi];
			if (obj && obj->x/16 == cursor->x/16 && obj->y/16 == cursor->y/16 ) {
				vram[11] = unit_tiledef[obj->fr/8];
				vram[12] = unit_tiledef[obj->fr/8]+1;
				return;
			}
		}
	}
	// not found
	*op = *oi= -1;
	vram[11] = tile_wood+7; 
	vram[12] = tile_wood+1;
}

void move_cursor(uint16_t gamepad_pressed)
{
	// mouse
	#if 0
	mouse_cursor->x = mouse_x; 
	mouse_cursor->y = mouse_y;
	if (mouse_cursor->x<0) mouse_cursor->x=0;
	if (mouse_cursor->y<0) mouse_cursor->y=0;
	if (mouse_cursor->x>VGA_H_PIXELS) mouse_cursor->x = VGA_H_PIXELS;
	if (mouse_cursor->y>VGA_V_PIXELS) mouse_cursor->y = VGA_V_PIXELS;		
	
	// square mouse_cursor, mouve on click
	if (mouse_buttons & mousebut_left) {	
		cursor->x = mouse_cursor->x & ~0xf;
		cursor->y = mouse_cursor->y & ~0xf;
	}
	#endif 
	object * const cursor = game_info.cursor;
	if (gamepad_pressed & gamepad_left && cursor->x > 0) 
		cursor->x -= 16;
	if (gamepad_pressed & gamepad_right && cursor->x < VGA_H_PIXELS-16) 
		cursor->x += 16;
	if (gamepad_pressed & gamepad_up && cursor->y > 32) 
		cursor->y -= 16;
	if (gamepad_pressed & gamepad_down && cursor->y < VGA_V_PIXELS-16) 
		cursor->y += 16;
}

// get cursor position on tilemap
int cursor_position (void)
{
	object *const c = game_info.cursor;
	return (c->y/16)*SCREEN_W + c->x/16;
}

// gamepad 0 button pressed this frame ? 
uint16_t gamepad_pressed(void)
{
	static uint16_t gamepad_previous=0;	
	uint16_t pressed = gamepad_buttons[0] & ~gamepad_previous;
	gamepad_previous = gamepad_buttons[0];

	return pressed;	
}


// wait till button A pressed and on one of my units.
int select_unit( void )
{
	uint16_t pressed;

	while(1) {
		do { 
			pressed  = gamepad_pressed();
			move_cursor(pressed);
			update_cursor_info();

			wait_vsync(1);
		} while( !(pressed & gamepad_A) );

		if (
			game_info.cursor_unit_pl == game_info.current_player &&
			// check has not moved yet 
			!(game_info.player_info[game_info.current_player].has_moved & 1<<game_info.cursor_unit_id)
			) {
				return game_info.cursor_unit_id;
		} else {
			continue;
		}
	}
}


#define MAX_PATH 16
char path[MAX_PATH]; // path = string of NSEW or NULL for abort
char * select_destination( int select_id )
{
	int start = cursor_position();

	uint16_t pressed;
	// wait till button A pressed and on an empty space
	while(1) {
		while(1) { 
			pressed = gamepad_pressed();

			// animate selected unit / health
			
			object *o = game_info.player_info[game_info.current_player].units_o[select_id];
			o->y = (o->y/16)*16 + ((vga_frame/16)%2 ? 1 : 0);
			o = game_info.player_info[game_info.current_player].units_health[select_id];
			o->y = (o->y/16)*16 + ((vga_frame/16)%2 ? 1 : 0);

			move_cursor(pressed);
			update_cursor_info();

			wait_vsync(1);
			if (pressed & gamepad_A) break;
			if (pressed & gamepad_B) break;
		}

		// cancel
		if (pressed & gamepad_B)
			return 0;

		if (game_info.cursor_unit_pl==-1) {
			int dest = cursor_position();
			char *p = path;
			// DUMMY path : manhattan simple
			while (start != dest) {
				message ("%d->%d\n",start, dest);
				if (dest/SCREEN_W-start/SCREEN_W >0) {
					*p++ = 'S';
					start += SCREEN_W;
				} else if (dest/SCREEN_W-start/SCREEN_W< 0) {
					*p++ = 'N';
					start -= SCREEN_W;
				} else if (dest-start < 0) {
					*p++ = 'W';
					start -= 1;
				} else if (dest-start > 0) {
					*p++ = 'E';
					start += 1;
				}
			}
			*p='\0';
			return path;

		} else {
			// error sound
			continue;
		}
	}
}

// fixme attack pas tjs dispo, harvest pour certains .. 
const uint16_t action_menu_tiles[6] = {287,290,291,294,297,298};
int menu_action()
{
	// backup vram & place menu tiles
	for (int j=0;j<2;j++)
		for (int i=0;i<3;i++) {
			backup_vram[j*3+i]=vram[(MENU_Y+j)*SCREEN_W+MENU_X+i];
			vram[(MENU_Y+j)*SCREEN_W+MENU_X+i] = action_menu_tiles[j*3+i]+1;
		}
	
	object *bullet = sprite3_new(data_misc_spr,MENU_X*16,MENU_Y*16,0);
	
	// select option
	// wait for keypress / animate 
	const uint8_t bullet_animation[] = {
		frame_misc_bullet1,
		frame_misc_bullet2,
		frame_misc_bullet3,
		frame_misc_bullet2
	};

	uint16_t pressed;
	while(1) {
		pressed=gamepad_pressed();
		if (pressed & gamepad_up) {
			bullet->y -= 16;
			if (bullet->y < MENU_Y*16) bullet->y = MENU_Y*16+16;
		} else if (pressed & gamepad_down) {
			bullet->y += 16;
			if (bullet->y > MENU_Y*16+16) bullet->y = MENU_Y*16;
		} else if (pressed & gamepad_A) {
			break;
		} else if (pressed & gamepad_B) {
			break;
		}

		bullet->fr = bullet_animation[(vga_frame/16)%4];

		wait_vsync(1);
	}

	// restore vram
	for (int j=0;j<2;j++)
		for (int i=0;i<3;i++) {
			vram[(MENU_Y+j)*SCREEN_W+MENU_X+i]= backup_vram[j*3+i];
		}

	int res = bullet->y/16-MENU_Y;
	blitter_remove(bullet);

	return (pressed & gamepad_A) ? res : -1;
}

void game()
{

	game_info.cursor->y=32;
	game_info.cursor->x=0;

	load_level(1);
	// cycle through players, tunr by turn, start play, ...
	// reset game data
	game_info.player_info[game_info.current_player].has_moved = 0;
	while (1) {
		int select_id = select_unit();
		message("selected unit %d\n",select_id);

		// show it
		char *path = select_destination(select_id);
		if (!path) continue;
		message("selected dest : %s\n",path);

		// animate move to dest
		object *o  = game_info.player_info[game_info.current_player].units_o[select_id];
		object *oh = game_info.player_info[game_info.current_player].units_health[select_id];
		uint16_t old_x = o->x, old_y = o->y;
		for (;*path;path++) {
			for (int i=0;i<16;i++){ 
				o->fr &= ~7;
				switch(*path) {
					case 'N' : o->y-=1; o->fr+=6; break;
					case 'S' : o->y+=1; o->fr+=4; break;
					case 'E' : o->x+=1; o->fr+=0; break;
					case 'W' : o->x-=1; o->fr+=2; break;
				}
				o->fr += (vga_frame/16)%2;
				oh->x = o->x;
				oh->y = o->y;
				wait_vsync(1);
			}
		}
		// reset to rest frame
		o->fr &= ~7;

		// mini menu action : attack / harvest / 
		int action = menu_action();
		if (action==-1) {
			// rewind position
			o->x = old_x;
			o->y = old_y;
			oh->x = o->x;
			oh->y = o->y;
			continue;
		}

		game_info.player_info[game_info.current_player].has_moved |= 1<<select_id;
	}


	leave_level();

}

void bitbox_main()
{
	// load all
	game_init();
	intro();
	// menu
	game();

}
