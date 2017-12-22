// tinystrat header file
#pragma once
#include <stdbool.h>

#include "sdk/lib/blitter/blitter.h"

#define SCREEN_W 25
#define SCREEN_H 19

#define MENU_X 10
#define MENU_Y 5

#define MAX_UNITS 32 // total max number of units on screen

enum Player_type_enum {
    player_notused,
    player_human,
    player_cpu1,
    player_off, // abandoned game but still present
};

enum Face_State {
    face_idle, 
    face_laugh,
    face_talk,
    face_cry,
    face_hud,
    face_off,
};

struct GameInfo {
	uint8_t map_id;

    uint8_t day; 
    unsigned finished_game:1;
    unsigned finished_turn:1;

    uint8_t current_player; // 0-3

    int8_t cursor_unit;  // id of object under cursor, <0 : None 

    // per player info
    uint8_t player_type[4]; 
    uint8_t player_avatar[4];
    // resources
    uint8_t food[4]; // per color / resource id
    uint8_t gold[4]; // per color / resource id
    uint8_t wood[4]; // per color / resource id
    uint8_t stone[4]; // per color / resource id

    // global info
    object *units [MAX_UNITS];      // frame as unit type, ptr=0 if not allocated. palette as player+aleady moved (faded)
    object *units_health[MAX_UNITS];  // same position as unit but frame=health
    
    uint16_t vram[SCREEN_W*SCREEN_H]; // map

    object *map;		// BG
    object *cursor;

    object *face[2];       // for avatars : player / adversary
    enum Face_State avatar_state; // current player 


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

// Faces
void face_init(void);
void face_frame(void);
void set_adversary_face(int); // set with player id

uint16_t gamepad_pressed(void);
void combat (int attacking_unit, int attacked_unit);
void draw_hud( void );

// human
void human_game_turn(void);


// -- path finding structures and functions

struct Cell {
    unsigned cost:6;
    unsigned pos:10;
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
void reconstruct_path(int src, int dst);

// color whole map with attainable spots.
// must call update pathfinding before
void color_map_movement_range();