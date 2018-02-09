#include "tinystrat.h"
#include "defs.h"
#include "unit.h"
extern "C" {
#include "sdk/lib/blitter/blitter.h" // object
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
    game_info.cursor_unit = game_info.unit_at(cursor->x/16 + (cursor->y/16)*SCREEN_W);

    game_info.draw_hud();
}

void move_cursor(uint16_t gamepad_pressed)
{
    object * const cursor = game_info.cursor;

    if (gamepad_pressed && GAMEPAD_DIRECTIONS)
        play_sfx(sfx_move);

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

Unit* select_unit( void )
{
    game_info.grid.color_units();
    // wait till button A pressed on one of my units.
    while(1) {
        uint16_t pressed;
        do {
            pressed  = gamepad_pressed();
            move_cursor(pressed);
            update_cursor_info();
            wait_vsync(1);
        } while( !(pressed & gamepad_A) );

        play_sfx(sfx_select);
        if (game_info.cursor_unit<0) {
            int choice = menu(MENU_BG,4);
            switch (choice) // bg click fixme handle end / exit ?
            {
                case 4 :
                    game_info.finished_turn=1;
                    return nullptr;
                default : // help, options, save ...
                    break;
            }
        } else {
            Unit *u  = game_info.cursor_unit;
            if ( u->player()==game_info.current_player && !u->moved()) {
                return u;
            }
        }
    }
}


int select_destination( Unit * selected )
{
    int start = cursor_position();
    game_info.grid.color_movement_range();

    // wait till button A pressed and on an empty space
    while(1) {
        uint16_t pressed;
        while(1) {
            // animate selected unit
            selected->y = (selected->y/16)*16 + ((vga_frame/16)%2 ? 1 : 0);

            pressed = gamepad_pressed();
            move_cursor(pressed);
            update_cursor_info();
            wait_vsync(1);

            if (pressed & gamepad_A) break;
            if (pressed & gamepad_B) break;
        }

        // cancel
        if (pressed & gamepad_B)
            return 0;

        play_sfx(sfx_select);

        // get target destination
        int dest = cursor_position();
        if (dest == start)
            return dest; // do not move : nothing to check

        if (game_info.cursor_unit==nullptr) { // check no unit on dest.

            // check tile within range
            struct Cell dest_cell = cost_array[dest];
            message ("target distance : %d\n",dest_cell.cost);
            if (cell_isempty(dest_cell)) {
                message("Error, too far\n");
                // error sound
                continue;
            }

            return dest;
        } else {
            message("Error, not empty\n");
            // error sound
            continue;
        }
    }
    game_info.grid.clear();
}



// unit selected, already positioned to attack position
Unit * select_attack_target()
{
    game_info.grid.color_targets();

    // check any ?
    if (game_info.nbtargets==0) return nullptr;

    // select targets and display them.
    int choice=0; // index in targets
    while(1) {
        uint16_t pressed = gamepad_pressed();
        if (pressed & (gamepad_up | gamepad_right)) {
            choice++;
            play_sfx(sfx_move);
            if (choice>=game_info.nbtargets) choice -= game_info.nbtargets;
        } else if (pressed & (gamepad_down | gamepad_left)) {
            play_sfx(sfx_move);
            choice--;
            if (choice<0) choice += game_info.nbtargets;
        }

        Unit *u = game_info.targets[choice];
        game_info.cursor->x = u->x;
        game_info.cursor->y = u->y;
        game_info.cursor->fr = fr_unit_cursor + ((vga_frame/16)%2 ? 1 : 0);
        // update hud for targets

        wait_vsync(1);
        if (pressed & gamepad_A) {
            play_sfx(sfx_select);
            return game_info.targets[choice];
        }
        if (pressed & gamepad_B) return nullptr;
    }
    // update selected ?

}



void human_game_turn()
{
    char path[MAX_PATH]; // merge & remove ?

    message ("starting player %d turn\n", game_info.current_player);
    game_info.finished_turn = 0;

    game_info.cursor->y=16;
    game_info.cursor->x=0;

    for (int i=0;i<MAX_UNITS;i++) {
        Unit *u = game_info.units[i];
        if (u) u->moved(false);
    }

    game_info.harvest();

    while (1) {
        Unit *selected = select_unit();
        message("selected unit :\n"); selected->info();

        if (game_info.finished_turn)
            break; // end of turn : exit loop

        update_pathfinding(*selected);

        int dest = select_destination(selected);
        message("selected dest : %d\n",dest);
        if (!dest) continue;
        // save old position
        uint16_t old_pos = selected->position();

        reconstruct_path(dest, path);
        selected->moveto(path);

        // Action menu for post-move actions (Attack)

        game_info.get_possible_targets(*selected);

        int action;
        if (game_info.nbtargets>0) // if there is anything to attack
            action = menu(MENU_ATTACK,3);
        else
            action = menu(MENU_EMPTY,2)+1;


        message("action %d\n",action);
        switch(action)
        {
            case 0: // cancel
            case 3:
                // restore position
                selected->position(old_pos);
                break;

            case 1 : // attack
                {
                    Unit *tgt = select_attack_target();
                    game_info.grid.clear();

                    if (tgt>=0)
                        combat(*selected,*tgt);
                }
                selected->moved (true);
                break;

            case 2 : // wait : do nothing, set player moved.
                selected->moved (true);
                break;
        }

    }
    message("- End of turn\n");
}
