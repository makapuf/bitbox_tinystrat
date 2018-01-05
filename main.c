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

static uint16_t ram_palette[256*2]; 

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

// gamepad 0 button pressed this frame ? 
// Autorepeat UDLR presses
// call exactly once per frame 

#define AUTOREPEAT_MASK (gamepad_up|gamepad_down|gamepad_left|gamepad_right)
uint16_t gamepad_pressed(void)
{
    static uint16_t gamepad_previous=0; 
    static uint8_t timer;

    uint16_t pressed = gamepad_buttons[0] & ~gamepad_previous;
    gamepad_previous = gamepad_buttons[0];

    if ((gamepad_buttons[0] & AUTOREPEAT_MASK)) {
        if (pressed & AUTOREPEAT_MASK) {
            timer=20;
        } else if (--timer==0) {
            // fire again
            pressed |= (gamepad_buttons[0] & AUTOREPEAT_MASK);
            timer=4;
        }
    } 

    return pressed; 
}

void intro()
{
    wait_vsync(15); // little pause

    object *bg = sprite3_new(data_intro_bg_spr, 0,0,200);

    #define BGPAL 64 // couples
    // replace with ram palette
    uint16_t *src_pal = (uint16_t*) bg->b; // in rom 
    bg->b = (uintptr_t) ram_palette; 


    // fade-in palette 
    for (int i=0;i<255;i+=3) {
        palette_fade(BGPAL, bg, src_pal, i);
        wait_vsync(1);
    }

    wait_vsync(15);
    
    // fixme enter left/right ...
    object *horse_l = sprite3_new(data_intro_horse_left_spr, 0,     50,10);
    wait_vsync(15);
    object *horse_r = sprite3_new(data_intro_horse_right_spr, 330,  50,10);
    wait_vsync(15);
    object *obj_l = sprite3_new(data_intro_objects_left_spr,    0,  30,7);
    wait_vsync(15);
    object *obj_r = sprite3_new(data_intro_objects_right_spr, 330,  30,7);
    wait_vsync(15);


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
    blitter_remove(obj_r);
    blitter_remove(obj_l);

    message("End of intro.\n");
}

#define MAP_HEADER_SZ 8

void load_map(int map) {
    memcpy(
        game_info.vram, 
        &data_map_map[MAP_HEADER_SZ+sizeof(game_info.vram)*map],
        sizeof(game_info.vram)
    );
}

void load_units(int map)
{
    // load / free unused units
    for (int i=0;i<MAX_UNITS;i++) { // unit
        const uint8_t *unit = &level_units[map][i][0];

        if (!unit[2]) {// no type defined : set as not used
            unit_remove(i);
        } else if (unit[2]==16) { 
            // special : flag
        } else {
            unit_new(unit[0],unit[1],unit[2]-1,unit[3]);
            game_info.player_type[unit[3]] = unit[3]==0 ? player_human : player_cpu0;
        }
    }
}

void get_possible_targets(int attack_unit)
{
    message("selecting attack target\n");
    // get targets
    game_info.nbtargets=0;
    for (int i=0; i<MAX_UNITS && game_info.nbtargets<8; i++) {
        if (!game_info.units[i]) continue; // unused unit, skip
        if (unit_get_player(i) == game_info.current_player) continue; // same player

        // unit within reach ?
        if (unit_can_attack(attack_unit,i)) {
            game_info.targets[game_info.nbtargets++]=i; 
            message("possible target : %d\n",i);
        } 
    }
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

    game_info.grid = grid_new();

    // for game only, not intro 

    // init play ? level ?
    game_info.cursor = sprite3_new(data_units_16x16_spr, 0,1024,1); 
    game_info.cursor->fr = fr_unit_cursor;

    for (int i=0;i<4;i++) {
        game_info.player_type[i]=player_notused;
        game_info.player_avatar[i]=i;

    }
    game_info.face = sprite3_new(data_faces_26x26_spr,0,-6,5);

    load_map(0);
    load_units(0);

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
    game_info.vram[17] = tile_zero+game_info.resources[game_info.current_player][resource_food];
    game_info.vram[19] = tile_zero+game_info.resources[game_info.current_player][resource_gold];
    game_info.vram[21] = tile_zero+game_info.resources[game_info.current_player][resource_wood];
    game_info.vram[23] = tile_zero+game_info.resources[game_info.current_player][resource_stone];

    // current player 
    game_info.vram[2] = tile_P1 + game_info.current_player;

    // set face hud
    game_info.face->fr = face_frame(game_info.current_player,face_idle);
    game_info.face->x = 0; 
    game_info.face->y = -6; 
    game_info.face->h = 21; 

}

// return damage
static int do_attack(int attacking_unit, int attacked_unit, bool right_to_left)
{   
    message("unit %d attacks %d,",attacking_unit,attacked_unit);
    unit_info(attacking_unit);
    unit_info(attacked_unit);

    uint8_t damage = unit_attack_damage(attacking_unit, attacked_unit) * unit_get_health(attacking_unit) / 256;

    // inflict damage
    message("inflicting %d damage\n",damage);
    uint8_t h = unit_get_health(attacked_unit);
    unit_set_health(attacked_unit, h>damage ? h-damage : 0); // do not remove unit yet, anims not done 
    return damage;
}


static const uint8_t resource_terrain[4]={ // per resource
    [resource_food]  = terrain_fields,
    [resource_gold]  = terrain_town,
    [resource_wood]  = terrain_forest,
    [resource_stone] = terrain_mountains,
};

void harvest()
{
    object *spr = sprite3_new(data_misc_16x16_spr,0,0,5);
    for (int i=0;i<MAX_UNITS;i++) {
        // for all my units
        if (!game_info.units[i] || unit_get_player(i)!=game_info.current_player) 
            continue;

        const int utype = unit_get_type(i);
        const int uterrain = unit_get_terrain(i);

        if ( utype == unit_farmer || utype == unit_farmer_f ) {
            // find resource for terrain
            for (int res=0;res<4;res++) {
                if (resource_terrain[res]==uterrain) {
                    message("terrain %d resource %d\n",uterrain,res);

                    // fixme SFX 
                    // little animation
                    spr->fr = fr_misc_food+res;
                    spr->x  = game_info.units[i]->x;
                    spr->y  = game_info.units[i]->y;

                    int tgt_x = 2+16*(16+res*2);
                    int tgt_y = 0;

                    for (int i=0;i<32;i++){
                        spr->x += (tgt_x - spr->x)/(32-i);
                        spr->y += (tgt_y - spr->y)/(32-i);
                        wait_vsync(1);
                    }
                    // update stat
                    game_info.resources [game_info.current_player][res]++;
                    draw_hud(); // refresh 
                }
            }
        }
    }
    blitter_remove(spr);
}

static void fight_animation( void )
{
    // wait keypress
    object *fight_spr = sprite3_new(data_fight_200x200_spr,100,70,3);
    static const uint8_t fight_anim[8] = {0,1,2,1,3,0,2,1};
    for (int i=0;i<60;i++) {
        fight_spr->fr = fight_anim[(vga_frame/16)%8];
        wait_vsync(1);
    }
    blitter_remove(fight_spr);
}

static void combat_face_frame(object *face, uint8_t anim_id, uint8_t attacked)
{
    // set combat faces
    static const uint8_t face_anims[][2][2] = { // attacking, attacked ; 2 frames
        {{2,3},{4,5}},
        {{4,5},{2,3}},
        {{2,2},{4,4}},
        {{4,4},{2,2}},
        {{0,0},{0,0}}, // poker face
    };
    face->fr = face->fr / face_NB * face_NB; // reset to idle
    face->fr += face_anims[anim_id][attacked][(vga_frame/16)%2];
}

// object opponent ?

void combat (int attacking_unit, int attacked_unit) 
{
    // hide all units
    for (int i=0;i<MAX_UNITS;i++)
        unit_hide(i);
    
    object *attacked_face = sprite3_new(data_faces_26x26_spr,360,52,5);
    object *attacking_face = game_info.face;

    game_info.cursor->y += 1024; // hide cursor
    
    uint8_t attacking_terrain = unit_get_terrain(attacking_unit);    
    message("attacking terrain of unit %d : %d\n",attacking_unit, attacking_terrain);
    object *bg_left  = sprite3_new(terrain_bg_table[attacking_terrain],  0,-200,35); 
    object *attacking_sprite = sprite3_new( data_units_16x16_spr, -30, 180, 5 );
    attacking_sprite->d |= 1; // set render 2X 
    attacking_sprite->h *= 2;
    attacking_sprite->fr = unit_get_type(attacking_unit)*8;
    object *attacking_life = sprite3_new(data_bignum_16x24_spr, 34,52,5);
    attacking_life->fr = unit_get_health(attacking_unit);
    uint8_t attacking_player = unit_get_player(attacking_unit);
    sprite_set_palette(attacking_sprite, attacking_player );    

    attacking_face->fr = face_frame(attacking_player,face_idle);
    attacking_face->x  = 0; 
    attacking_face->y  = 52; 
    attacking_face->h  = 26;

    uint8_t attacked_terrain = unit_get_terrain(attacked_unit);
    message("attacked terrain of unit %d : %d\n",attacked_unit, attacked_terrain);
    object *bg_right = sprite3_new(terrain_bg_table[attacked_terrain ],200, 300,35); 
    object *attacked_sprite = sprite3_new( data_units_16x16_spr, 410, 180, 5 );
    attacked_sprite->d |= 1; // set render 2X 
    attacked_sprite->h *= 2;
    attacked_sprite->fr = unit_get_type(attacked_unit)*8 + 2; // facing left
    object *attacked_life = sprite3_new(data_bignum_16x24_spr, 340,52,5);
    attacked_life->fr = unit_get_health(attacked_unit);
    uint8_t attacked_player = unit_get_player(attacked_unit);
    sprite_set_palette(attacked_sprite, attacked_player);

    attacked_face->fr = face_frame(attacked_player, face_idle);
    attacked_face->x  = 360;
    attacked_face->y  = 52;
    attacked_face->h  = 26;

    // also other units ...  

    // intro
    for (int i=0;i<25;i++) {
        bg_left ->y+=10;
        bg_right->y-=10;
        wait_vsync(1);        
    }
    for (int i=0;i<80;i++) {
        attacking_sprite->x+=2;
        attacking_sprite->fr = (attacking_sprite->fr&~7)   + (vga_frame/8)%2;
        wait_vsync(1);
    }
    for (int i=0;i<80;i++) {
        attacked_sprite->fr  = (attacked_sprite->fr&~7) + 2 + (vga_frame/8)%2;
        attacked_sprite->x-=2;
        wait_vsync(1);
    }

    fight_animation();

    int dmg_given = do_attack(attacking_unit, attacked_unit, true);
    int dmg_received=0;

    if (unit_get_health(attacked_unit)>0) {
        if (unit_can_attack(attacked_unit, attacking_unit))
            dmg_received = do_attack(attacked_unit, attacking_unit, false);
    }

    int anim_id;
    if (unit_get_health(attacked_unit)==0) {
        anim_id=0;
    } else if (unit_get_health(attacking_unit)==0) {
        anim_id=1;
    } else if (dmg_given>dmg_received) {
        anim_id=2;
    } else if (dmg_given>dmg_received) {
        anim_id=3;
    } else {
        anim_id=4;
    }

    // anim update scores + faces
    for (int i=0;i<15;i++) {
        // hide old scores
        attacking_life->x -=2;
        attacked_life->x +=2;
        combat_face_frame(attacking_face,anim_id,0);
        combat_face_frame(attacked_face,anim_id,1);
        wait_vsync(1);
    }

    attacking_life->fr = unit_get_health(attacking_unit);
    attacked_life->fr  = unit_get_health(attacked_unit);

    // set sprites to explosion if needed
    // idle anim of units / explosion

    for (int i=0;i<15;i++) {
        attacking_life->x+=2;
        attacked_life->x-=2;
        combat_face_frame(attacking_face,anim_id,0);
        combat_face_frame(attacked_face,anim_id,1);
        wait_vsync(1);
    }

    // wait for keypress
    while (1) {
        int pressed = gamepad_pressed();
        if (pressed && gamepad_A) break;
        combat_face_frame(attacking_face,anim_id,0);
        combat_face_frame(attacked_face,anim_id,1);
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
    blitter_remove(attacked_face);

    // show all units again
    for (int i=0;i<MAX_UNITS;i++)
        unit_show(i);

    // small anims on board if any unit just died
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


}

void ready_animation()
{
    // fixme music 
    // fixme stop at middle or something
    // fixme get ready + PLAYER 

    game_info.face->fr = face_frame(game_info.current_player, face_idle);
    game_info.face->y=134;
    game_info.face->h=26;
    object *next_spr = sprite3_new(data_next_player_spr,0,130,6);
    for (next_spr->x=-30;next_spr->x<400+100;next_spr->x+=4) {
        game_info.face->x=next_spr->x+2;
        wait_vsync(1);
    }
    blitter_remove(next_spr);
}

// main_menu : new game, about, ...
int main_menu()
{
    wait_vsync(15); // little pause

    object *bg = sprite3_new(data_main_menu_spr, 0,0,200);

    // replace with ram palette
    uint16_t *src_pal = (uint16_t*) bg->b; // in rom 
    bg->b = (uintptr_t) ram_palette; 

    // fade-in palette 
    for (int i=0;i<255;i+=3) {
        palette_fade(255, bg, src_pal, i);
        wait_vsync(1);
    }

    // wait keypress
    while (!GAMEPAD_PRESSED(0,start)); 


    // fade-out palette
    for (int i=0;i<255;i+=4) {
        palette_fade(255, bg, src_pal, 255-i);
        wait_vsync(1);
    }

    blitter_remove(bg);
    return 0;
}


void bitbox_main()
{

    intro();
    main_menu(); 

    // fixme select game level, parameters ...

    game_init();
    do {
        draw_hud();        
        ready_animation();
        switch (game_info.player_type[game_info.current_player]) {
            case player_human : human_game_turn(); break;
            case player_cpu0  : play_CPU0(); break;
            case player_notused : break;
            case player_off : break;
        }
        game_next_player();
    } while (!game_info.finished_game);
    leave_level(); // should be end of turn
}
