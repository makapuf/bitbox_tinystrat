#include <string.h>
#include <stdbool.h>
#include <stdlib.h> // rand

#include "bitbox.h"
#include "lib/blitter/blitter.h"

// embed data
#define DATA_IMPLEMENTATION
#include "data.h"
#undef DATA_IMPLEMENTATION

#define DEFS_IMPLEMENTATION
#include "defs.h"
#undef DEFS_IMPLEMENTATION


#include "tinystrat.h"
#include "units.h"



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

    game_info.face = sprite3_new(data_faces_26x26_spr,0,-6,5);
}

// global game element init
void game_init(void)
{
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


/* updates info about terrain under mouse_cursor 
   returns the unit under mouse_cursor if any or NULL
   */
void update_cursor_info(void)
{
    object * const cursor = game_info.cursor;

    // "blink" mouse_cursor
    cursor->fr=(vga_frame/32)%2 ? fr_unit_cursor : fr_unit_cursor2;

    // terrain under the mouse_cursor    
    const uint16_t tile_id = game_info.vram[(cursor->y/16)*SCREEN_W + cursor->x/16];
    int terrain_id = tile_terrain[tile_id];
    game_info.vram[3]=tile_id; 
    // defense info
    game_info.vram[6]=tile_zero + terrain_defense[terrain_id];

    // find unit under mouse_cursor - if any
    int8_t *oi = &game_info.cursor_unit;
    for (*oi=0;*oi<MAX_UNITS;(*oi)++) {
        object *obj = game_info.units[*oi];
        if (obj && obj->x/16 == cursor->x/16 && obj->y/16 == cursor->y/16 ) {            
            break;
        }
    }
    if (*oi==MAX_UNITS) *oi=-1; // not found

    draw_hud();
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
    if (gamepad_pressed & gamepad_up && cursor->y > 16) 
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
// call exactly once per frame 
uint16_t gamepad_pressed(void)
{
    static uint16_t gamepad_previous=0; 
    uint16_t pressed = gamepad_buttons[0] & ~gamepad_previous;
    gamepad_previous = gamepad_buttons[0];

    return pressed; 
}


// returns a choice between 1 and nb_choices or zero to cancel
unsigned int menu(int menu_id, int nb_choices)
{
    object *menu   = sprite3_new(data_menus_88x82_spr,MENU_X*16,MENU_Y*16,  1);
    object *bullet = sprite3_new(data_units_16x16_spr,MENU_X*16,MENU_Y*16+8,0);

    // unroll menu ?
    menu->fr = menu_id;

    // select option
    // wait for keypress / animate 
    const uint8_t bullet_animation[] = {
        fr_unit_bullet1,
        fr_unit_bullet2,
        fr_unit_bullet3,
        fr_unit_bullet2
    };
    uint16_t pressed;

    uint8_t choice=0;
    while(1) {
        pressed=gamepad_pressed();
        if (pressed & gamepad_up) {
            if (choice==0)
                choice = nb_choices-1;
            else 
                choice--;
        } else if (pressed & gamepad_down) {
            if (choice==nb_choices-1)
                choice = 0;
            else 
                choice++;
        } else if (pressed & gamepad_A) {
            break;
        } else if (pressed & gamepad_B) {
            break;
        }

        bullet->fr = bullet_animation[(vga_frame/16)%4];
        bullet->y  =  MENU_Y*16+8+choice*16;
        wait_vsync(1);
    }

    blitter_remove(bullet);
    blitter_remove(menu);

    return (pressed & gamepad_A) ? choice+1 : 0;
}

int select_unit( void )
{
    uint16_t pressed;

    // wait till button A pressed on one of my units.
    while(1) {
        do { 
            pressed  = gamepad_pressed();
            move_cursor(pressed);
            update_cursor_info();

            wait_vsync(1);

        } while( !(pressed & gamepad_A) );

        if (game_info.cursor_unit<0) {
            int choice = menu(MENU_BG,4);
            switch (choice) // bg click fixme handle end / exit ?
            {
                case 4 : 
                    game_info.finished_turn=1; 
                    message("plop\n");
                    return -1; // 
                default : // help, options, save ...
                    break;
            }
        } else {
            int u  = game_info.cursor_unit;
            if ( unit_get_player(u)==game_info.current_player && !unit_has_moved(u)) {
                return game_info.cursor_unit;
            }
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
            
            object *o = game_info.units[select_id];
            object *oh = game_info.units_health[select_id];            
            o->y = (o->y/16)*16 + ((vga_frame/16)%2 ? 1 : 0);
            oh->y = (oh->y/16)*16 + ((vga_frame/16)%2 ? 1 : 0);

            move_cursor(pressed);
            update_cursor_info();

            wait_vsync(1);
            if (pressed & gamepad_A) break;
            if (pressed & gamepad_B) break;
        }

        // cancel
        if (pressed & gamepad_B)
            return 0;
        message("%d\n",game_info.cursor_unit);

        if (game_info.cursor_unit==-1) {
            int dest = cursor_position();
            char *p = path;
            // DUMMY path : manhattan simple
            // use astar then
            while (start != dest) {
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

// unit selected, already positioned to attack position
int select_attack_target(int attack_unit)
{
    message("selecting attack target\n");
    // get targets
    uint8_t targets[8];
    int nbtargets=0;
    for (int i=0; i<MAX_UNITS && nbtargets<8; i++) {
        if (!game_info.units[i]) continue; // unused unit, skip
        if (unit_get_player(i) == game_info.current_player) continue; // same player
        // unit within reach 
        const int dist = unit_get_mdistance(attack_unit,i);
        if (dist >= unit_attack_range_table[attack_unit][0] && \
            dist <= unit_attack_range_table[attack_unit][1]) {
            targets[nbtargets++]=i; 
            message("possible target : %d\n",i);
        }
    }
    // check any ?
    if (nbtargets==0) return -1;

    // select targets and display them.
    int choice=0; 
    while(1) { 
        uint16_t pressed = gamepad_pressed();
        if (pressed & (gamepad_up | gamepad_right)) {
            choice++;
            if (choice>=nbtargets) choice -= nbtargets;
        } else if (pressed & (gamepad_down | gamepad_left)) {
            choice--;
            if (choice<0) choice += nbtargets;
        }

        object *o = game_info.units[targets[choice]];
        game_info.cursor->x = o->x;
        game_info.cursor->y = o->y;
        game_info.cursor->fr = fr_unit_cursor + ((vga_frame/16)%2 ? 1 : 0);
        // update hud for targets

        wait_vsync(1);
        if (pressed & gamepad_A) return targets[choice];
        if (pressed & gamepad_B) return -1;
    }
    // update selected ? 

}

// true if unit died
static void do_attack(int attacking_unit, int attacked_unit, bool right_to_left)
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
    object *attacking_sprite = sprite3_new( data_units_16x16_spr, 30, 130, 5 );
    attacking_sprite->d |= 1; // set render 2X 
    attacking_sprite->h *= 2;
    attacking_sprite->fr = unit_get_type(attacking_unit)*8;
    object *attacking_life = sprite3_new(data_bignum_16x24_spr, 10,220,5);
    attacking_life->fr = unit_get_health(attacked_unit);

    uint8_t attacked_terrain = unit_get_terrain(attacked_unit);
    message("attacked terrain %d\n",attacked_terrain);
    object *bg_right = sprite3_new(terrain_bg_table[attacked_terrain ],200, 300,35); 
    object *attacked_sprite = sprite3_new( data_units_16x16_spr, 330, 130, 5 );
    attacked_sprite->d |= 1; // set render 2X 
    attacked_sprite->h *= 2;
    // fixme set player color
    attacked_sprite->fr = unit_get_type(attacked_unit)*8 + 3; // facing left
    object *attacked_life = sprite3_new(data_bignum_16x24_spr, 380,220,5);
    attacked_life->fr = unit_get_health(attacking_unit);

    // also other units ...  

    // intro
    for (int i=0;i<25;i++) {
        bg_left ->y+=10;
        bg_right->y-=10;
        wait_vsync(1);        
    }
    for (int i=0;i<25;i++) {
        attacking_sprite->x+=2;
        wait_vsync(1);        
    }
    for (int i=0;i<25;i++) {
        attacked_sprite->x-=2;
        wait_vsync(1);        
    }


    // wait keypress
    object *fight_spr = sprite3_new(data_fight_200x200_spr,100,50,3);
    static const uint8_t fight_anim[8] = {0,1,2,1,3,0,2,1};
    while (1) {
        int pressed = gamepad_pressed();
        if (pressed && gamepad_A) break;
        fight_spr->fr = fight_anim[(vga_frame/16)%8];
        wait_vsync(1);
    }
    blitter_remove(fight_spr);

    do_attack(attacking_unit, attacked_unit, true);

    if (unit_get_health(attacked_unit)>0)
        do_attack(attacked_unit, attacking_unit, false);

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


void game_turn()
{
    message ("starting player %d turn\n", game_info.current_player);
    game_info.finished_turn = 0;

    game_info.cursor->y=16;
    game_info.cursor->x=0;

    // cycle through players, turn by turn, start play, ...
    // reset game data
    for (int i=0;i<MAX_UNITS;i++) {
        if (game_info.units[i])
            unit_reset_moved(i);
    }

    while (1) {
        // to face/general code ?
        game_info.face->fr = game_info.player_avatar[game_info.current_player]*5; 
        game_info.face->x = 0; 
        game_info.face->y = -6; 
        game_info.face->h = 21; 

        int select_id = select_unit();
        message("selected unit %d\n",select_id);

        if (game_info.finished_turn) break; // end of turn : exit loop

        // show it
        char *path = select_destination(select_id);
        if (!path) continue; // if cannot reach select destination, reselect unit
        message("selected dest : %s\n",path);

        // animate move to dest
        object *o  = game_info.units[select_id];
        object *oh = game_info.units_health[select_id];

        uint16_t old_x = o->x, old_y = o->y;
        for (;*path;path++) {
            for (int i=0;i<16;i++){ 
                o->fr &= ~7;
                switch(*path) {
                    case 'N' : o->y-=1; o->fr+=6; break;
                    case 'S' : o->y+=1; o->fr+=4; break;
                    case 'E' : o->x+=1; o->fr+=0; break;
                    case 'W' : o->x-=1; o->fr+=2; break;
                    default : message ("unknown character : %c\n",*path); break;
                }
                o->fr += (vga_frame/16)%2;
                oh->x = o->x;
                oh->y = o->y;
                wait_vsync(1);
            }
        }
        // reset to rest frame
        o->fr &= ~7;

        // mini menu action : attack / harvest / nothing if not available
        const int selected_type = unit_get_type(select_id);
        const int selected_terrain = unit_get_terrain(select_id);

        int action;
        if ( selected_type == unit_farmer || selected_type == unit_farmer_f ) {
            if (
                selected_terrain == terrain_fields || 
                selected_terrain == terrain_forest || 
                selected_terrain == terrain_mountains || 
                selected_terrain == terrain_town 
            ) {
                action = menu(MENU_HARVEST,3);
                if (action==1) action = 4;
               } else {
                action = menu(MENU_EMPTY,2);
                if (action==1) action = 2;
               }
        } else {
            action = menu(MENU_ATTACK,3); // if there is anything to attack ? 
        }


        switch(action)
        {
            case 0: // cancel : rewind position
                o->x = old_x;
                o->y = old_y;
                oh->x = o->x;
                oh->y = o->y;
                continue;
                break;
            case 1 : // attack
                {
                    int tgt = select_attack_target(select_id);
                    if (tgt>=0) 
                        combat(select_id,tgt); 
                }
                break;
            case 2 : // wait
                break;
            case 4 : // harvest 
                // fixme little animation : move resource sprite to upper bar 
                switch (selected_terrain) {
                    case terrain_fields    : game_info.food [game_info.current_player]++; break;
                    case terrain_town      : game_info.gold [game_info.current_player]++; break;
                    case terrain_forest    : game_info.wood [game_info.current_player]++; break;
                    case terrain_mountains : game_info.stone[game_info.current_player]++; break;
                }

                break;
        }
        if (!action) {
        }
        
        unit_set_moved (select_id);
    }
    message("- End of turn\n");

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

void bitbox_main()
{
    //intro();

    // main_menu : new game, about, ...
    game_init();
    do {
        game_turn();
        game_next_player();
    } while (!game_info.finished_game);
    leave_level(); // should be end of turn
}
