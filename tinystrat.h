// tinystrat header file

#include "data.h" // data_palettes_bin declaration


#define SCREEN_W 25
#define SCREEN_H 19

#define MENU_X 10
#define MENU_Y 5

#define MAX_UNITS 32

enum Player_type_enum {
    player_notused,
    player_human,
    player_cpu1,
    player_off, // abandoned game but still in
};


struct GameInfo {
	uint8_t map_id;

    uint8_t day; 

    uint8_t current_player; // 0-3

    int8_t cursor_unit;  // id of object under cursor, <0 : None 

    // per player info
    uint8_t player_type[4]; 
    uint8_t player_avatar[4];
    uint8_t resources[4][5]; // per color / resource id

    // global info
    object *units [MAX_UNITS];      // frame as unit type, ptr=0 if not allocated. palette as player+aleady moved (faded)
    object *units_health[MAX_UNITS];  // same position as unit but frame=health

    object *map;		// BG
    object *cursor;

} game_info;


// Units
#include "data.h"


static inline int unit_get_type  (object *unit) // return unit_id
{ 
	return (unit->fr)/8; 
} 

static inline void unit_set_type (object *unit, int unit_id) 
{ 
	unit->fr = unit_id*8; 
} 

// frame 0-1, direction nsew
static inline void unit_set_frame (object *unit, int direction, int frame)
{
	unit->fr = (unit->fr & ~7) | direction*2 | frame;
}

static inline int unit_get_palette (object *unit) 
{ 
	return (unit->b - (uintptr_t) data_palettes_bin ) / 512;
}
static inline void unit_set_palette(object *unit, int palette)
{
	unit->b = (uintptr_t)&data_palettes_bin[512*palette];  
}



static inline int  unit_get_player(object *unit) { 
	return unit_get_palette(unit)%4; 
}


// also reset moved
static inline void unit_set_player(object *unit, int player) 
{ 
	unit_set_palette(unit, player);
}  

static inline int  unit_has_moved (object *unit) 
{ 
	return unit_get_palette(unit)/4; 
}

static inline void unit_set_moved (object *unit) 
{ 
	unit_set_palette(unit, unit_get_player(unit)+4 );
}

static inline void unit_reset_moved (object *unit) 
{
	unit_set_palette(unit, unit_get_player(unit));
}

// Map
/*
map 0..N (id):
    icone/nom pour choix menu
    type : 2P, 3P, 4P -> deduit 
    campaign
    triggers : conditions -> 1 bitmap de conditions, un callback qui verifie et s'execute eventuellement, appelé a chaque tour - pas chaque frame)
        ou un gros ? si reuse
        faire un tableau depuis maps

    initial pos players, initial resources
    intro eventuelle : ds trigger

    map_get_terrain(position u16)
    map_get
*/

