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

#define SZ(x) (sizeof(x)/sizeof(x[0]))// size of array

// flags
//#define FLAG_INTRO
#define FLAG_NEXTPLAYER_ANIM
#define FLAG_WALK_ANIMATION

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

int menu (const char *choices[], int nb_choices, int x,int y);
void text (const char *txt, int face_id=-1, enum Face_State st=face_talk);

void wait_vsync(int);

#define GAMEPAD_DIRECTIONS (gamepad_up|gamepad_down|gamepad_left|gamepad_right)
uint16_t gamepad_pressed(void);

void human_game_turn(void);
void play_CPU (void);

// -- path finding structures and functions

struct Cell {
    unsigned cost:6;
    unsigned pos:10; // comes from (absolute)
} __attribute__((packed));

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
void sd_load(const char *filename, int slot);
extern uint8_t memory[2][16384];
