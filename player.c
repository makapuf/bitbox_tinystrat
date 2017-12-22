#include "tinystrat.h"
#include "defs.h"
#include "units.h"


#define MAX_PATH 16
char path[MAX_PATH]; // path = string of NSEW or NULL for abort
// fixme remove 

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
    
    // square mouse_cursor, move on click
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
    game_info.avatar_state = face_hud;

    // wait till button A pressed on one of my units.
    while(1) {
        do { 
            pressed  = gamepad_pressed();
            move_cursor(pressed);
            update_cursor_info();
            face_frame();

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

        if (game_info.cursor_unit==-1) { // check no unit on dest. tile/ within range

            // get target destination
            int dest = cursor_position();
            struct Cell dest_cell = cost_array[dest];
            message ("target distance : %d\n",dest_cell.cost);
            if (cell_isempty(dest_cell)) {
                message("Error, too far\n");
                continue;
            }

            // fixme build path
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
        const int attack_type = unit_get_type(attack_unit);

        if (dist >= unit_attack_range_table[attack_type][0] && \
            dist <= unit_attack_range_table[attack_type][1]) {
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


void human_game_turn()
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
        int select_id = select_unit();
        message("selected unit %d\n",select_id);

        if (game_info.finished_turn) break; // end of turn : exit loop

        update_pathfinding(select_id);

        /* FIXME : paint all vram wiith attainable tiles. */
        // Do it when B button pressed on unit 
        color_map_movement_range(select_id);
        // reload map

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
            action = menu(MENU_ATTACK,3); // only if there is anything to attack !
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
