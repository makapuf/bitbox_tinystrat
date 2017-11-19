// Units
#include "data.h"
#include "tinystrat.h"

static inline void unit_remove(uint8_t uid) 
{
    blitter_remove(game_info.units[uid]);
    game_info.units[uid] = 0;
    blitter_remove(game_info.units_health[uid]);
    game_info.units_health[uid] = 0;
}


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
	return (unit->b - (uintptr_t) data_palettes_bin ) / 1024;
}
static inline void unit_set_palette(object *unit, int palette)
{
	unit->b = (uintptr_t)&data_palettes_bin[1024*palette];  
}

static inline void unit_set_health(uint8_t unit_id, uint8_t health)
{
    game_info.units_health[unit_id]->fr = fr_misc_0 + health;
}

static inline uint8_t unit_get_health(uint8_t unit_id)
{
    return game_info.units_health[unit_id]->fr - fr_misc_0;
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

// get the terrain type the unit is on
static inline uint8_t unit_get_terrain (object *unit)
{
    // modulo 1024 : OK hidden or not
    const int tx = (unit->x%1024)/16;
    const int ty = (unit->y%1024)/16;
    return tile_terrain[game_info.vram[ty*SCREEN_W+tx]];
}

static inline void unit_hide(uint8_t uid) 
{
    if (game_info.units[uid]) {            
        game_info.units[uid]->y|=1024;
        game_info.units_health[uid]->y|=1024;
    }
}

static inline void unit_show(uint8_t uid)
{
    if (game_info.units[uid]) {
        game_info.units[uid]->y&= ~1024;
        game_info.units_health[uid]->y &= ~1024;
    }
}

void unit_info(uint8_t uid) 
{
    object *unit = game_info.units[uid];
    message ("unit:%d - player:%d type:%s on terrain:%s\n", 
            uid, 
            unit_get_player(unit), 
            unit_names[unit_get_type(unit)],
            terrain_names[unit_get_terrain(unit)]
            );
}

/* 
percentage (0-255) of attack of a unit to another 
given attack types and attacked unit terrain defense
*/
int unit_attack_damage(uint8_t from, uint8_t to)
{
    // base percentage attacking -> attacked types : 0-5
    int attack = unit_damage_table [unit_get_type(game_info.units[from])] \
        [unit_get_type(game_info.units[to])] ; 

    // defense of the terrain the attacked unit is on 1-5
    int defense = terrain_defense[unit_get_terrain(game_info.units[to])]; 
    message("attack %d def %d res: %d\n",attack,defense,attack*51 * (6-defense)/6);
    return attack*51 * (6-defense)/6;
}

// returns uid
uint8_t unit_new (uint8_t x, uint8_t y, uint8_t type, uint8_t player_id )
{
    // find empty unit
    int uid;

    for (uid=0;uid<MAX_UNITS;uid++) {
        if (!game_info.units[uid]) break;
    }

    if (uid==MAX_UNITS) {
        message ("no more free units\n");
        die(2,1);
    }

    // allocate sprite
    // put below cursors
    object *o =sprite3_new( data_units_16x16_spr, x*16, y*16, 5 ); 

    game_info.units[uid] = o;
    unit_set_type(o,type);
    unit_set_player(o,player_id);

    // top of prec, below mouse_cursor
    object *oh=sprite3_new(data_misc_16x16_spr, x*16, y*16, 4); 
    game_info.units_health[uid] = oh;

    unit_set_health(uid, 10); 

    message("init unit %d at %d,%d : typ %d color %d\n",uid, o->x, o->y, type, player_id);
    return uid;
}
