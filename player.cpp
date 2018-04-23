#include "tinystrat.h"
#include "defs.h"
#include "unit.h"
#include "game.h"

extern "C" {
#include "sdk/lib/blitter/blitter.h" // object
}
#include "surface.h"

void move_cursor(uint16_t gamepad_pressed)
{
    object * const cursor = &game_info.cursor;

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

char surface_data[SURFACE_BUFSZ(128,128)]; // 4k buffer

static const pixel_t surface_pal[] = {
    RGB(222,211,168),
    RGB(112,116,105),
    RGB(75,67,61),
    RGB(46,38,33),
};

static const uint8_t bullet_animation[] = {
    fr_misc_bullet1,
    fr_misc_bullet2,
    fr_misc_bullet3,
    fr_misc_bullet2
};

// returns a choice between 0 and nb_choices-1 or -1 to cancel
int menu (const char *choices[], int nb_choices, int x,int y)
{
    object menu, bullet;
    Surface surf { 80, 123, &surface_data };

    sprite3_load(&menu  , SPRITE(menu_border));
    sprite3_load(&bullet, SPRITE(misc_16x16));
    surf.setpalette (surface_pal);

    for (int i=0;i<nb_choices;i++) {
        surf.text(choices[i],0,16*i,data_font_fon);
    }

    blitter_insert(&surf, x+16,y+8,5);
    blitter_insert(&menu,    x,y  ,5);
    blitter_insert(&bullet,  x,y+6,0);

    // select option
    // wait for keypress / animate
    uint16_t pressed;

    uint8_t choice=0;
    while(1) {
        pressed=gamepad_pressed();
        if (pressed & gamepad_up) {
            if (choice==0)
                choice = nb_choices-1;
            else {
                for (int i=0;i<8;i++) {
                    bullet.y -=2;
                    wait_vsync();
                }
                choice--;
            }
        } else if (pressed & gamepad_down) {
            if (choice==nb_choices-1)
                choice = 0;
            else {
                for (int i=0;i<8;i++) {
                    bullet.y +=2;
                    wait_vsync();
                }
                choice++;
            }
        } else if (pressed & gamepad_A) {
            break;
        } else if (pressed & gamepad_B) {
            break;
        }

        bullet.fr = bullet_animation[(vga_frame/16)%4];
        bullet.y  =  y+6+choice*16;
        wait_vsync();
    }

    blitter_remove(&bullet);
    blitter_remove(&menu);
    blitter_remove(&surf);

    return (pressed & gamepad_A) ? choice : -1;
}


// face_id <0 -> no face
void text (const char *txt, int face_id, enum Face_State st)
{
    const int x=100, y=20;

    if (face_id>=0) {
        game_info.face.x = 100;
        game_info.face.y = 30;
        game_info.face.h = 26;
    }

    // appears from top
    object border, bullet;
    sprite3_load(&border, SPRITE(text_border));
    sprite3_load(&bullet, SPRITE(misc_16x16));

    Surface surf { 128, 128, &surface_data };

    surf.setpalette (surface_pal);

    surf.text(txt,0,0,data_font_mini_fon);
    blitter_insert(&surf, x+24,y+29 ,1);
    blitter_insert(&border,    x,y  ,1);
    blitter_insert(&bullet,x+120,y+164,0);

    // blink bullet
    while (! gamepad_pressed()) {
        bullet.fr = bullet_animation[(vga_frame/16)%4];
        if (face_id>=0) {
            game_info.face.fr = (vga_frame/32)%2 ? game_info.face_frame(face_id,st) : game_info.face_frame(face_id,face_idle);
        }
        wait_vsync();
    }

    blitter_remove(&border);
    blitter_remove(&surf);
    blitter_remove(&bullet);

    game_info.draw_hud(); // set face back
}

static const char *bg_menu[]={
    "\x7f  Help","\x80  Options","\x81  Save","\x82  End turn"
};

Unit* select_unit( void )
{
    game_info.grid.color_units();
    // wait till button A pressed on one of my units.
    while(1) {
        uint16_t pressed;
        do {
            pressed  = gamepad_pressed();
            move_cursor(pressed);
            game_info.update_cursor_info();
            game_info.bg_frame();
            wait_vsync();
        } while( !(pressed & gamepad_A) );

        play_sfx(sfx_select);
        if (game_info.cursor_unit==nullptr) {
            int choice = menu(bg_menu,SZ(bg_menu),50,50); // fixme right next to cursor 
            switch (choice) // bg click fixme handle end / exit ?
            {
                case 3 :
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
    int start = game_info.cursor_position();
    game_info.grid.color_movement_range();

    // wait till button A pressed and on an empty space
    while(1) {
        uint16_t pressed;
        while(1) {
            // animate selected unit
            selected->y = (selected->y/16)*16 + ((vga_frame/16)%2 ? 1 : 0);

            pressed = gamepad_pressed();
            move_cursor(pressed);
            game_info.update_cursor_info();
            wait_vsync();

            if (pressed & gamepad_A) break;
            if (pressed & gamepad_B) break;
        }

        // cancel
        if (pressed & gamepad_B)
            return 0;

        play_sfx(sfx_select);

        // get target destination
        int dest = game_info.cursor_position();
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
        game_info.cursor.x = u->x;
        game_info.cursor.y = u->y;
        game_info.cursor.fr = fr_unit_cursor + ((vga_frame/16)%2 ? 1 : 0);
        // update hud for targets

        wait_vsync();
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

    game_info.cursor.y=16;
    game_info.cursor.x=0;

    for (int i=0;i<MAX_UNITS;i++) {
        Unit *u = &game_info.units[i];
        if (u) u->moved(false);
    }

    game_info.harvest();

    while (1) {
        game_info.action();

        Unit *selected = select_unit();

        if (game_info.finished_turn)
            break; // end of turn : exit loop

        message("selected unit :\n");
        selected->info();

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

        static const char *menu_attack[]={"\x84 Attack","\x83 Wait","\x82 Cancel"};
        static const char *menu_empty []={"\x83 Wait","\x82 Cancel"};
        int action;
        if (game_info.nbtargets>0) // if there is anything to attack
            action = menu(menu_attack,SZ(menu_attack),50,50);
        else {
            action = menu(menu_empty ,SZ(menu_empty),50,50);
            if (action >=0) action++; // same actions as preceding menu
        }


        message("action %d\n",action);
        switch(action)
        {
            case -1: // cancel
            case 2:
                // restore position
                selected->position(old_pos);
                break;

            case 0 : // attack
                {
                    Unit *tgt = select_attack_target();
                    game_info.grid.clear();

                    if (tgt)
                        combat(*selected,*tgt);
                }
                selected->moved (true);
                break;

            case 1 : // wait : do nothing, set player moved.
                selected->moved (true);
                break;
        }

    }
    message("- End of turn\n");
    game_info.grid.clear(); // clear units left to play
}
