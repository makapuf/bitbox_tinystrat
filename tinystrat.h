// tinystrat header file
#pragma once
#define SCREEN_W 25
#define SCREEN_H 19

#define MENU_X 10
#define MENU_Y 5

#define MAX_UNITS 32

enum Player_type_enum {
    player_notused,
    player_human,
    player_cpu1,
    player_off, // abandoned game but still present
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
    object *face;   // commander 

} game_info;


// Map
/*
map 0..N (id):
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

