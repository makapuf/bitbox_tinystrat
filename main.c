#include "bitbox.h"
#include <string.h>


// embed data
#define DATA_IMPLEMENTATION
#include "data.h"
#undef DATA_IMPLEMENTATION

#define TINYSTRAT_IMPLEMENTATION
#include "tinystrat_defs.h"
#undef TINYSTRAT_IMPLEMENTATION

#include "lib/blitter/blitter.h"
/*
  TODO : passer les sprites en palettes ? couleurs, grise quand KO. forcer palette size ds outil
  bug : ne va pas a droite / a gauche deux fois trop ...
  use astar

 */

#include "tinystrat.h"


// object *mouse_cursor;
uint16_t vram[SCREEN_W*SCREEN_H];
uint16_t backup_vram[5*5]; // extra for menus

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
    game_info.cursor = sprite3_new(data_misc_16x16_spr, 0,1024,1); // jusr below mouser mouse_cursor
    #if 0
    mouse_cursor = sprite3_new(data_misc_spr, 0,0,0);
    mouse_cursor->fr=frame_misc_mouse;
    #endif 

}


// sine, -128:128 with angle from 0 to 64
int8_t sinus(int x)
{
    // 0- pi/2 sines in 16 steps
    static const uint8_t sine_table[] = {0, 12, 24, 37, 48, 60, 71, 81, 90, 98, 106, 112, 118, 122, 125, 127};
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

void load_level(int map)
{
    // copy level from flash to vram (direct copy) 
    memcpy(vram, &data_map_map[8+sizeof(vram)*map],sizeof(vram));
    
    for (int i=0;i<MAX_UNITS;i++) { // unit
        uint8_t *unit = &level_units[map-1][i][0];

        if (!unit[2]) break; // no type defined : stop

        // allocate sprite
        if (unit[2]==16) { 
            // special : flag - or put into units
        } else {

            object *o =sprite3_new(
                data_units_16x16_spr, 
                unit[0]*16,
                unit[1]*16,
                5 // below cursors
            ); 
            game_info.units[i] = o;
            unit_set_type(o,unit[2]-1);
            unit_set_player(o,unit[3]);

            object *oh=sprite3_new(
                data_misc_16x16_spr, 
                unit[0]*16,
                unit[1]*16,  
                4 // top of prec, below mouse_cursor
            );
            game_info.units_health[i] = oh;

            oh->fr = fr_misc_0+10; // default to health 10

            message("init unit %d,%d : typ %d color %d\n",o->x, o->y, unit[2]-1, unit[3]);
        }
    }
}


void leave_level()
{
    // release all blitter sprites, set to zero
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
    //load_level(0);
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
    wait_vsync(15);
    object *horse_r = sprite3_new(data_intro_horse_right_spr, 330,  50,10);
    wait_vsync(15);

/*
    data_intro_objects_left_spr
    data_intro_objects_right_spr
*/

    object *tiny = sprite3_new(data_intro_tiny_spr, -16,40,10);
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
        message("%d\n",i); // FIXME CLIPPING
        wars->y -= 16;
        tiny->x += 8;
        horse_l->x -= 16;
        horse_r->x += 16;
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


/* updates info about terrain under mouse_cursor 
   returns the unit under mouse_cursor if any or NULL
   */
void update_cursor_info(void)
{
    object * const cursor = game_info.cursor;

    // "blink" mouse_cursor
    cursor->fr=(vga_frame/32)%2 ? fr_unit_cursor : fr_unit_cursor2;

    // terrain under the mouse_cursor
    const uint16_t tile_id = vram[(cursor->y/16)*SCREEN_W + cursor->x/16];
    int terrain_id = tile_terrain[tile_id];
    vram[3]=tile_id; 
    // defense
    vram[6]=tile_zero + terrain_defense[terrain_id];

    // find unit under mouse_cursor / if any
    int8_t *oi = &game_info.cursor_unit;

    for (*oi=0;*oi<16;(*oi)++) {
        object *obj = game_info.units[*oi];
        if (obj && obj->x/16 == cursor->x/16 && obj->y/16 == cursor->y/16 ) {            
            return;
        }
    }
    
    // not found
    *oi= -1;
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

        object *u  = game_info.units[ game_info.cursor_unit ];
        if ( unit_get_player(u)==game_info.current_player && !unit_has_moved(u)) {
            return game_info.cursor_unit;
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
            
            object *o = game_info.units[select_id];
            o->y = (o->y/16)*16 + ((vga_frame/16)%2 ? 1 : 0);
            object *oh = game_info.units_health[select_id];
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

        if (game_info.cursor_unit==-1) {
            int dest = cursor_position();
            char *p = path;
            // DUMMY path : manhattan simple
            // use astar then
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
    
    object *bullet = sprite3_new(data_misc_16x16_spr,MENU_X*16,MENU_Y*16,0);
    
    // select option
    // wait for keypress / animate 
    const uint8_t bullet_animation[] = {
        fr_unit_bullet1,
        fr_unit_bullet2,
        fr_unit_bullet3,
        fr_unit_bullet2
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

void level_play()
{

    game_info.cursor->y=16;
    game_info.cursor->x=0;

    load_level(1);
    // cycle through players, turn by turn, start play, ...
    // reset game data
    for (int i=0;i<MAX_UNITS;i++) {
        if (game_info.units[i])
            unit_reset_moved(game_info.units[i]);
    }

    while (1) {
        int select_id = select_unit();
        message("selected unit %d\n",select_id);

        // show it
        char *path = select_destination(select_id);
        if (!path) continue;
        message("selected dest : %s\n",path);

        // animate move to dest
        object *o  = game_info.units[select_id];
        object *oh = game_info.units_health[select_id];

        uint16_t old_x = o->x, old_y = o->y;
        for (;*path;path++) {
            for (int i=0;i<16;i++){ 

                // fixme unit_set_frame(6,(vga_frame/16)%2), ...
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

        unit_set_moved (game_info.units[select_id]);
    }

    leave_level();

}

void bitbox_main()
{
    // load all
    game_init();
    intro();

    // menu : new game, about, ...
    level_play();

}
