// tinystrat header file
#pragma once
#include <stdbool.h>

#include "sdk/lib/blitter/blitter.h"

#define SCREEN_W 25
#define SCREEN_H 19

#define MENU_X 10
#define MENU_Y 5

#define MAX_UNITS 32 // total max number of units on screen
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

struct GameInfo {
	uint8_t map_id;

    uint8_t day;
    unsigned finished_game:1;
    unsigned finished_turn:1;

    uint8_t current_player; // 0-3

    int8_t cursor_unit;  // id of object under cursor, <0 : None

    // per player info
    uint8_t player_type[4]; // replace with fnpointer ?
    uint8_t player_avatar[4];

    uint8_t resources[4][4]; // per color / resource id

    // global info
    object *units [MAX_UNITS];      // frame as unit type, ptr=0 if not allocated. palette as player+aleady moved (faded)
    //object *units_health[MAX_UNITS];  // same position as unit but frame=health

    uint16_t vram[SCREEN_W*SCREEN_H]; // map

    object *map;		// BG
    object *cursor;

    object *face;       // for avatar : player

    object *grid;

    uint8_t targets[8];
    int nbtargets;
} game_info;


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

// Faces
static inline int face_frame(int player, enum Face_State st)
{
    return game_info.player_avatar[player]*face_NB+st;
}

#define GAMEPAD_DIRECTIONS (gamepad_up|gamepad_down|gamepad_left|gamepad_right)
uint16_t gamepad_pressed(void);
void combat (int attacking_unit, int attacked_unit);
void draw_hud( void );
void harvest(void); // harvest for current player

void human_game_turn(void);
void play_CPU0 (void);

void get_possible_targets(int attacking_unit); // main

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

// updates cost_array and frontier
// from source, up to max_cost (included)
void update_pathfinding ( int source );

// once cost_array has been created, find the path from an array
void reconstruct_path(int dst, char *path);

// color whole map with attainable spots.
// must call update pathfinding before
void color_grid_movement_range();
void color_grid_units(void);
void color_grid_targets(void);

// grid display (grid.c)
object *grid_new(void);
void grid_empty();

// SFX
#define sfx_move 2
#define sfx_select 3
#define sfx_explode 4

#define songorder_song 0
#define songorder_battle 6
#define songorder_next 7

void play_sfx(int sfx_id);
void mod_jumpto(int order);
