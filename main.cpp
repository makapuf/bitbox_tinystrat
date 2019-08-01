#include <string.h>
#include <stdlib.h> // rand

extern "C" {
#include "bitbox.h"
#include "lib/blitter/blitter.h"
#include "lib/mod/mod32.h"
}

#include "data.h" // palette

#include "defs.h"
#include "tinystrat.h"

#include "game.h"
#include "unit.h"
#include "grid.h"

#define SFX_CHANNEL (MOD_CHANNELS-1) // use last channel
#define SFX_VOLUME 64
#define SFX_NOTE    214

void play_sfx(int sample_id) {
    mod_play_note(sample_id,SFX_CHANNEL,SFX_VOLUME,SFX_NOTE);
}

Game game_info;

// sine, -128:128 with angle from 0 to 64
int8_t sinus(int x)
{
    // 0- pi/2 sines in 16 steps
    static const uint8_t sine_table[] = {
         0, 12,  24,  37,  48,  60,  71,  81,
        90, 98, 106, 112, 118, 122, 125, 127
    }; // fixme to mk_defs
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

static pixel_t ram_palette[256*sizeof(pixel_t)*2];

// fade to black a palette from a reference palette. value =0 means full black
void palette_fade(int palette_sz, object *ob, uint16_t *src_palette, uint8_t value)
{
    pixel_t *dst = (pixel_t*)ob->b;
    for (int i=0;i<palette_sz*2;i++) {
        // split into RGB u8
        uint16_t c = src_palette[i];

        unsigned int r = ((c>>10) & 31) * value / 32;
        unsigned int g = ((c>> 5) & 31) * value / 32;
        unsigned int b = ((c>> 0) & 31) * value / 32;

        // reassemble
        dst[i] = RGB(r,g,b);
    }
}

// gamepad 0 button pressed this frame ?
// Autorepeat UDLR presses
// call exactly once per frame

uint16_t gamepad_pressed(void)
{
    static uint16_t gamepad_previous=0;
    static uint8_t timer;

    uint16_t pressed = gamepad_buttons[0] & ~gamepad_previous;
    gamepad_previous = gamepad_buttons[0];

    if ((gamepad_buttons[0] & GAMEPAD_DIRECTIONS)) {
        if (pressed & GAMEPAD_DIRECTIONS) {
            timer=20;
        } else if (--timer==0) {
            // fire again
            pressed |= (gamepad_buttons[0] & GAMEPAD_DIRECTIONS);
            timer=4;
        }
    }

    return pressed;
}

void intro()
{
    wait_vsync(15); // little pause

    object bg;
    object horse_l, horse_r, obj_l, obj_r, tiny, wars;

    sprite3_load(&bg, SPRITE(intro_bg));
    sprite3_load(&horse_l, SPRITE(intro_horse_left));
    sprite3_load(&horse_r, SPRITE(intro_horse_right));
    sprite3_load(&obj_l,   SPRITE(intro_objects_left));
    sprite3_load(&obj_r,   SPRITE(intro_objects_right));
    sprite3_load(&tiny,    SPRITE(intro_tiny));
    sprite3_load(&wars,    SPRITE(intro_wars));

    blitter_insert(&bg, 0,0,200);

    // replace with ram palette
    uint16_t *src_pal = (uint16_t*) bg.b; // in rom : this is a 16bpp palette
    bg.b = (uintptr_t) ram_palette;

    // fade-in palette
    #define BGPAL 64 // couples
    for (int i=0;i<255;i+=8) {
        palette_fade(BGPAL, &bg, src_pal, i);
        wait_vsync();
    }

    wait_vsync(15);

    blitter_insert(&horse_l, 0,     50,10);
    wait_vsync(15);
    blitter_insert(&horse_r, 330,  50,10);
    wait_vsync(15);
    blitter_insert(&obj_l,  0,  30,7);
    wait_vsync(15);
    blitter_insert(&obj_r,330,  30,7);
    wait_vsync(15);

    blitter_insert(&tiny, -16,140,10);
    wait_vsync(30);
    for (int i=0;i<50;i++) {
        tiny.x += 4;
        wait_vsync();
    }

    wait_vsync(40);

    blitter_insert(&wars, 20,-200,10);
    while (!GAMEPAD_PRESSED(0,start)) {
        if (wars.y<140)
            wars.y+=16;
        else
            wars.y = 166+sinus(vga_frame)/32;
        wait_vsync();
    }

    // fade out bg & remove
    for (int i=0;i<256;i+=4) {
        wars.y -= 16;
        // FIXME CLIPPING
        // tiny->x += 8;
        // horse_l->x -= 16;
        // horse_r->x += 16;
        palette_fade(BGPAL, &bg, src_pal, 255-i);
        wait_vsync();
    }

    blitter_remove(&bg);
    blitter_remove(&wars);
    blitter_remove(&tiny);
    blitter_remove(&horse_l);
    blitter_remove(&horse_r);
    blitter_remove(&obj_r);
    blitter_remove(&obj_l);

    message("End of intro.\n");
}


// main_menu : new game, about, ... + menus
int main_menu()
{
    wait_vsync(15);

    object bg;
    sprite3_load(&bg, SPRITE(main_menu));
    blitter_insert(&bg, 0,0,200);

    // replace with ram palette
    uint16_t *src_pal = (uint16_t*) bg.b; // in rom
    bg.b = (uintptr_t) ram_palette;

    // fade-in palette
    for (int i=0;i<256;i+=4) {
        palette_fade(255, &bg, src_pal, i);
        wait_vsync();
    }

    // wait keypress
    const char * choices[NB_LEVELS];
    for (int i=0;i<NB_LEVELS;i++)
        choices[i] = level_info[i].name;

    int lvl;
    do {
        lvl = menu (choices, NB_LEVELS, 256,80);
    } while (lvl==-1);

    // fade-out palette
    for (int i=0;i<128;i+=4) {
        palette_fade(255, &bg, src_pal, 255-i*2);
        wait_vsync();
    }

    blitter_remove(&bg);
    return lvl;
}


void wait_vsync(int n)
{
    for (int i=0;i<n;i++) wait_vsync();
}

void debug_text();

extern "C" {
    void bitbox_main()
    {
        #ifdef FLAG_INTRO
        intro();
        #endif

        while (1) {
            int level= main_menu();

            // fixme select game parameters ...

            game_info.init();

            game_info.start_level(level);
            do {
                game_info.draw_hud();
                #ifdef FLAG_NEXTPLAYER_ANIM
                game_info.ready_animation();
                #endif
                switch (game_info.player_type[game_info.current_player]) {
                    case player_human : human_game_turn(); break;
                    case player_cpu0  : play_CPU(); break;
                    case player_notused : break;
                    case player_off : break;
                }
                game_info.next_player();
            } while (!game_info.finished_game);
            game_info.leave_level(); // should be end of turn
        }
    }
}
