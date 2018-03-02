// tinystrat header file
#pragma once

extern "C" {
#include "sdk/lib/blitter/blitter.h" // object
}

#define SPRITE(x) (&data_##x [0])

#define MAX_UNITS 32 // total max number of units on screen

#define SCREEN_W 25
#define SCREEN_H 19

#define MENU_X 10
#define MENU_Y 5
#define MAX_PATH 16

enum Player_type_enum {
    player_notused,
    player_human,
    player_cpu0,
    player_off, // abandoned game but still present (no units)
};

enum Face_State {
    face_idle,
    face_talk,
    face_happy1,
    face_happy2,
    face_sad1,
    face_sad2,
    face_NB
};

// Map
/*
TODO : map 0..N (id):
    icone/nom pour choix menu
    type : 2P, 3P, 4P -> deduit
    campaign
    triggers : conditions -> 1 bitmap de conditions, un callback qui verifie et s'execute eventuellement, appelé a chaque tour - pas chaque frame)
        ou un gros ? si reuse
        faire un tableau depuis maps
        victory conditions

    initial pos players, initial resources
    intro eventuelle : ds trigger_fn

    map_get_terrain(position u16)
    map_get
*/

void load_map (int map_id); // (re) load map background

#define GAMEPAD_DIRECTIONS (gamepad_up|gamepad_down|gamepad_left|gamepad_right)
uint16_t gamepad_pressed(void);

void human_game_turn(void);
void play_CPU0 (void);

// -- path finding structures and functions

struct Cell {
    unsigned cost:6;
    unsigned pos:10; // comes from (absolute)
};
#define MAX_COST 63 // 6 bits
#define FRONTIER_SIZE 32 // max elements in frontier

bool cell_isempty(const struct Cell c);

extern struct Cell cost_array[SCREEN_W* SCREEN_H]; // cost, come_from
extern struct Cell frontier  [FRONTIER_SIZE];      // cost, position

// once cost_array has been created, find the path from an array
void reconstruct_path(int dst, char *path);

// Audio
#define sfx_move 2
#define sfx_select 3
#define sfx_explode 4

#define songorder_song 0
#define songorder_battle 6
#define songorder_next 7

void play_sfx(int sample_id);