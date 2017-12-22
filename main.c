#include <string.h>
#include <stdbool.h>
#include <stdlib.h> // rand

#include "bitbox.h"
#include "lib/blitter/blitter.h"
#include "lib/mod/mod32.h" 

// embed data
#define DATA_IMPLEMENTATION
#include "data.h"
#undef DATA_IMPLEMENTATION

#define DEFS_IMPLEMENTATION
#include "defs.h"
#undef DEFS_IMPLEMENTATION

#define UNITS_IMPLEMENTATION
#include "units.h"

#include "tinystrat.h"

// sine, -128:128 with angle from 0 to 64
int8_t sinus(int x)
{
    // 0- pi/2 sines in 16 steps
    static const uint8_t sine_table[] = {
         0, 12,  24,  37,  48,  60,  71,  81,
        90, 98, 106, 112, 118, 122, 125, 127
    };
    x %= 64;

    if (x<16)
        return sine_table[x];
    else if (x<32)
        return sine_table[31-x];
    else if (x<48)
        return -sine_table[x-32];
    else
        return -sine_table[63-x];
}


// fade to black a palette from a reference palette. value =0 means full black
void palette_fade(int palette_sz, object *ob, uint16_t *src_palette, uint8_t value)
{
    uint16_t *dst = (uint16_t*)ob->b;
    for (int i=0;i<palette_sz*2;i++) {
        // split into RGB u8
        uint16_t c = src_palette[i];
        unsigned int r = ((c>>10) & 31) * value / 256; 
        unsigned int g = ((c>> 5) & 31) * value / 256;
        unsigned int b = ((c>> 0) & 31) * value / 256;

        // reassemble 
        dst[i] = r<<10 | g << 5 | b;
    }
}

void intro()
{
    object *bg = sprite3_new(data_intro_bg_spr, 0,0,200);

    #define BGPAL 64 // couples
    // replace with ram palette
    uint16_t *src_pal = (uint16_t*) bg->b; // in rom 
    static uint16_t ram_palette[BGPAL*2]; 
    bg->b = (uintptr_t) ram_palette; 


    // fade-in palette 
    for (int i=0;i<255;i+=3) {
        palette_fade(BGPAL, bg, src_pal, i);
        wait_vsync(1);
    }

    wait_vsync(15);
    object *horse_l = sprite3_new(data_intro_horse_left_spr, 0,     50,10);
    // enter left/right
    wait_vsync(15);
    object *horse_r = sprite3_new(data_intro_horse_right_spr, 330,  50,10);
    wait_vsync(15);

/*
    data_intro_objects_left_spr
    data_intro_objects_right_spr
*/

    object *tiny = sprite3_new(data_intro_tiny_spr, -16,140,10);
    wait_vsync(30);
    for (int i=0;i<50;i++) {
        tiny->x += 4;
        wait_vsync(1);
    }
    wait_vsync(40);

    object *wars = sprite3_new(data_intro_wars_spr, 20,-200,10);
    while (!GAMEPAD_PRESSED(0,start)) {
        if (wars->y<140)
            wars->y+=16;
        else 
            wars->y = 166+sinus(vga_frame)/32;
        if (GAMEPAD_PRESSED(0,up)) wars->y--;
        if (GAMEPAD_PRESSED(0,down)) wars->y++;
        wait_vsync(1);
    }

    // fade out bg & remove 
    for (int i=0;i<255;i+=4) {
        wars->y -= 16;
        // FIXME CLIPPING
        // tiny->x += 8;
        // horse_l->x -= 16;
        // horse_r->x += 16;
        palette_fade(BGPAL, bg, src_pal, 255-i);
        wait_vsync(1);
    }

    blitter_remove(bg);
    blitter_remove(wars);
    blitter_remove(tiny);
    blitter_remove(horse_l);
    blitter_remove(horse_r);
    message("End of intro.\n");
}

#define MAP_HEADER_SZ 8
void load_level(int map)
{
    // copy level from flash to vram (direct copy) 
    memcpy(game_info.vram, &data_map_map[MAP_HEADER_SZ+sizeof(game_info.vram)*map],sizeof(game_info.vram));
    
    for (int i=0;i<MAX_UNITS;i++) { // unit
        const uint8_t *unit = &level_units[map][i][0];

        if (!unit[2]) break; // no type defined : stop here

        if (unit[2]==16) { 
            // special : flag , or not 
        } else {
            unit_new(unit[0],unit[1],unit[2]-1,unit[3]);
            game_info.player_type[unit[3]] = player_human; // by default..
        }
    }

    face_init();
}

// global game element init
void game_init(void)
{
    load_mod(data_song_mod); 

    game_info.map = tilemap_new (
        &data_tiles_bg_tset[4], // no palette
        0,0,
        TMAP_HEADER(SCREEN_W,SCREEN_H,TSET_16,TMAP_U16), 
        game_info.vram
        );
    // for game only, not intro 

    // init play ? level ?
    game_info.cursor = sprite3_new(data_units_16x16_spr, 0,1024,1); 
    game_info.cursor->fr = fr_unit_cursor;

    for (int i=0;i<4;i++) {
        game_info.player_type[i]=player_notused;
        game_info.player_avatar[i]=i;

    }

    load_level(0);

    game_info.finished_game=0;
}

void leave_level()
{
    // release all blitter sprites, set to zero
    // release player sprite
}

void draw_hud( void )
{
    // resources
    game_info.vram[17] = tile_zero+game_info.gold [game_info.current_player];
    game_info.vram[19] = tile_zero+game_info.stone[game_info.current_player];
    game_info.vram[21] = tile_zero+game_info.wood [game_info.current_player];
    game_info.vram[23] = tile_zero+game_info.food [game_info.current_player];

    // current player 
    game_info.vram[2] = tile_P1 + game_info.current_player;
}

// return damage
static int do_attack(int attacking_unit, int attacked_unit, bool right_to_left)
{   
    message("unit %d attacks %d,",attacking_unit,attacked_unit);
    unit_info(attacking_unit);
    unit_info(attacked_unit);

    uint8_t damage = unit_attack_damage(attacking_unit, attacked_unit) * unit_get_health(attacking_unit) / 256;
    // fixme show animations !
    // fixme show happy / sad faces / very happy /sad & shake

    // inflict damage
    message("inflicting %d damage\n",damage);
    uint8_t h = unit_get_health(attacked_unit);
    unit_set_health(attacked_unit, h>damage ? h-damage : 0); // do not remove unit yet, anims not done 
    return damage;
}

// gamepad 0 button pressed this frame ? 
// call exactly once per frame 
uint16_t gamepad_pressed(void)
{
    static uint16_t gamepad_previous=0; 
    uint16_t pressed = gamepad_buttons[0] & ~gamepad_previous;
    gamepad_previous = gamepad_buttons[0];

    return pressed; 
}


void combat (int attacking_unit, int attacked_unit) 
{
    // hide all units
    for (int i=0;i<MAX_UNITS;i++)
        unit_hide(i);

    game_info.cursor->y += 1024; // hide cursor
    
    uint8_t attacking_terrain = unit_get_terrain(attacking_unit);
    message("attacking terrain %d\n",attacking_terrain);
    object *bg_left  = sprite3_new(terrain_bg_table[attacking_terrain],  0,-200,35); 
    object *attacking_sprite = sprite3_new( data_units_16x16_spr, -30, 150, 5 );
    attacking_sprite->d |= 1; // set render 2X 
    attacking_sprite->h *= 2;
    attacking_sprite->fr = unit_get_type(attacking_unit)*8;
    object *attacking_life = sprite3_new(data_bignum_16x24_spr, 34,52,5);
    attacking_life->fr = unit_get_health(attacking_unit);
    game_info.avatar_state = face_idle;
    sprite_set_palette(attacking_sprite, unit_get_player(attacking_unit));

    uint8_t attacked_terrain = unit_get_terrain(attacked_unit);
    message("attacked terrain %d\n",attacked_terrain);
    object *bg_right = sprite3_new(terrain_bg_table[attacked_terrain ],200, 300,35); 
    object *attacked_sprite = sprite3_new( data_units_16x16_spr, 410, 150, 5 );
    attacked_sprite->d |= 1; // set render 2X 
    attacked_sprite->h *= 2;
    attacked_sprite->fr = unit_get_type(attacked_unit)*8 + 3; // facing left
    object *attacked_life = sprite3_new(data_bignum_16x24_spr, 340,52,5);
    attacked_life->fr = unit_get_health(attacking_unit);
    sprite_set_palette(attacked_sprite, unit_get_player(attacked_unit));

    set_adversary_face(unit_get_player(attacked_unit));

    // also other units ...  

    // intro
    for (int i=0;i<25;i++) {
        face_frame();
        bg_left ->y+=10;
        bg_right->y-=10;
        wait_vsync(1);        
    }
    for (int i=0;i<80;i++) {
        attacking_sprite->x+=2;
        face_frame();
        wait_vsync(1);        
    }
    for (int i=0;i<80;i++) {
        attacked_sprite->x-=2;
        face_frame();
        wait_vsync(1);        
    }


    // wait keypress
    object *fight_spr = sprite3_new(data_fight_200x200_spr,100,50,3);
    static const uint8_t fight_anim[8] = {0,1,2,1,3,0,2,1};
    for (int i=0;i<60;i++) {
        fight_spr->fr = fight_anim[(vga_frame/16)%8];
        wait_vsync(1);
    }
    blitter_remove(fight_spr);

    int dmg_given = do_attack(attacking_unit, attacked_unit, true);

    if (unit_get_health(attacked_unit)>0) {
        int dmg_received = do_attack(attacked_unit, attacking_unit, false);
        game_info.avatar_state = dmg_given>dmg_received ? face_laugh : face_cry;
    } else {
        game_info.avatar_state = face_laugh;        
    }

    // idle animation : anim update scores, faces
    for (int i=0;i<15;i++) {
        // hide old scores
        attacking_life->x -=2;
        attacked_life->x +=2;
        face_frame();
        wait_vsync(1);
    }
    attacking_life->fr = unit_get_health(attacking_unit);
    attacked_life->fr  = unit_get_health(attacked_unit);
    for (int i=0;i<15;i++) {
        attacking_life->x+=2;
        attacked_life->x-=2;
        face_frame();
        wait_vsync(1);
    }

    // wait for keypress
    while (1) {
        int pressed = gamepad_pressed();
        if (pressed && gamepad_A) break;
        face_frame();
        wait_vsync(1);
    }

    // outro - quicker
    while (bg_left->y > -150 ) {
        bg_left ->y-=16;
        bg_right->y+=16;
        wait_vsync(1);
    }

    blitter_remove(bg_left);
    blitter_remove(bg_right);
    blitter_remove(attacking_sprite);
    blitter_remove(attacked_sprite);
    blitter_remove(attacking_life);
    blitter_remove(attacked_life);

    // show all units again
    for (int i=0;i<MAX_UNITS;i++)
        unit_show(i);

    // small anims if any unit just died
    if (unit_get_health(attacked_unit)==0) {
        // set frame to explosion 
        unit_remove(attacked_unit);
    }
    if (unit_get_health(attacking_unit)==0) {
        // 
        unit_remove(attacking_unit);
    }

    game_info.cursor->y -= 1024; // show cursor
}



void game_next_player()
{
    int next=game_info.current_player;
    while (1) {
        next = (next+1)%4;
        if (game_info.player_type[next]!=player_notused && game_info.player_type[next]!=player_off)
            break;
    }
    if (next == game_info.current_player)
        game_info.finished_game = 1;
    else
        game_info.current_player = next;

    object *next_spr = sprite3_new(data_next_player_spr,0,130,0);
    for (next_spr->x=-30;next_spr->x<400+100;next_spr->x+=4) {
        wait_vsync(1);
    }
    blitter_remove(next_spr);
    // also move player face ?
}

void bitbox_main()
{
    //intro();

    // main_menu : new game, about, ...
    game_init();
    do {
        human_game_turn();
        game_next_player();
    } while (!game_info.finished_game);
    leave_level(); // should be end of turn
}
