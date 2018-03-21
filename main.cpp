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
#include "surface.h"

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
    for (int i=0;i<15;i++)
        wait_vsync(); // little pause

    object bg;
    object horse_l, horse_r, obj_l, obj_r, tiny, wars;

    sprite3_load(&bg, SPRITE(intro_bg));
    sprite3_load(&horse_l, SPRITE(intro_horse_left));
    sprite3_load(&horse_r, SPRITE(intro_horse_right));
    sprite3_load(&obj_l,   SPRITE(intro_objects_left));
    sprite3_load(&obj_r,   SPRITE(intro_objects_right));
    sprite3_load(&tiny,    SPRITE(intro_tiny));
    sprite3_load(&wars,    SPRITE(intro_wars));

    #define BGPAL 64 // couples
    // replace with ram palette
    uint16_t *src_pal = (uint16_t*) bg.b; // in rom
    bg.b = (uintptr_t) ram_palette;
    blitter_insert(&bg, 0,0,200);

    // fade-in palette
    for (int i=0;i<255;i+=3) {
        palette_fade(BGPAL, &bg, src_pal, i);
        wait_vsync();
    }

    for (int i=0;i<15;i++)
        wait_vsync();

    blitter_insert(&horse_l, 0,     50,10);
    for (int i=0;i<15;i++)
        wait_vsync();
    blitter_insert(&horse_r, 330,  50,10);
    for (int i=0;i<15;i++)
        wait_vsync();
    blitter_insert(&obj_l,  0,  30,7);
    for (int i=0;i<15;i++)
        wait_vsync();
    blitter_insert(&obj_r,330,  30,7);
    for (int i=0;i<15;i++)
        wait_vsync();

    blitter_insert(&tiny, -16,140,10);
    for (int i=0;i<30;i++)
        wait_vsync();
    for (int i=0;i<50;i++) {
        tiny.x += 4;
        wait_vsync();
    }

    for (int i=0;i<40;i++)
        wait_vsync();

    blitter_insert(&wars, 20,-200,10);
    while (!GAMEPAD_PRESSED(0,start)) {
        if (wars.y<140)
            wars.y+=16;
        else
            wars.y = 166+sinus(vga_frame)/32;
        wait_vsync();
    }

    // fade out bg & remove
    for (int i=0;i<255;i+=4) {
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

char surface_data[SURFACE_BUFSZ(144,176)];
pixel_t surface_pal[] = {
    RGB(205,201,185),
    RGB(92,86,77),
    RGB(71,65,57),
    RGB(46,38,33),
};

// main_menu : new game, about, ...
int main_menu()
{
    for (int i=0;i<15;i++)
        wait_vsync(); // little pause

    object bg;
    sprite3_load(&bg, SPRITE(main_menu));

    // replace with ram palette
    uint16_t *src_pal = (uint16_t*) bg.b; // in rom
    bg.b = (uintptr_t) ram_palette;

    blitter_insert(&bg, 0,0,200);

    // fade-in palette
    for (int i=0;i<256;i+=4) {
        palette_fade(255, &bg, src_pal, i);
        wait_vsync();
    }

    // menu
    Surface surf { 144, 176, &surface_data };
    surf.setpalette (surface_pal);
    for (int i=0;i<3;i++)
        surf.fillrect (10,10+40*i,100,30+40*i,i);
    blitter_insert(&surf, 240,80,50);
    // wait keypress
    while (!GAMEPAD_PRESSED(0,start));

    // fade-out palette
    for (int i=0;i<256;i+=4) {
        palette_fade(255, &bg, src_pal, 255-i);
        wait_vsync();
    }

    blitter_remove(&bg);
    blitter_remove(&surf); // destructor ?
    return 0;
}


extern "C" {
    void bitbox_main()
    {

        //intro();
        main_menu();

        // fixme select game level, parameters ...

        game_info.init();

        // play_level(0)
        game_info.start_level(0);
        do {
            game_info.draw_hud();
            game_info.ready_animation();
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
