// Units
#include "data.h"
#include "tinystrat.h"
#include "defs.h"

#include <stdlib.h> // abs

static inline void unit_remove(uint8_t uid) 
{
    if (!game_info.units[uid]) return;
    blitter_remove(game_info.units[uid]);
    game_info.units[uid] = 0;
    blitter_remove(game_info.units_health[uid]);
    game_info.units_health[uid] = 0;
}


static inline int unit_get_type  (int unit) // return unit_id
{ 
	return (game_info.units[unit]->fr)/8; 
} 

static inline void unit_set_type (int unit, int typeunit_id) 
{ 
	game_info.units[unit]->fr = typeunit_id*8; 
} 

// frame 0-1, direction nsew
static inline void unit_set_frame (int unit, int direction, int frame)
{
	game_info.units[unit]->fr = (game_info.units[unit]->fr & ~7) | direction*2 | frame;
}

static inline void sprite_set_palette(object *spr, int palette)
{
    spr->b = (uintptr_t)&data_palettes_bin[1024*palette];  
}

static inline int unit_get_palette (int unit) 
{ 
	return (game_info.units[unit]->b - (uintptr_t) data_palettes_bin ) / 1024;
}
static inline void unit_set_palette(int unit, int palette)
{
	sprite_set_palette(game_info.units[unit], palette);
}

static inline void unit_set_health(uint8_t unit_id, uint8_t health)
{
    game_info.units_health[unit_id]->fr = fr_misc_0 + health;
}

static inline uint8_t unit_get_health(uint8_t unit_id)
{
    return game_info.units_health[unit_id]->fr - fr_misc_0;
}

static inline int  unit_get_player(int unit) { 
	return unit_get_palette(unit)%4; 
}


// also reset moved
static inline void unit_set_player(int unit, int player) 
{ 
	unit_set_palette(unit, player);
}  

static inline int  unit_has_moved (int unit) 
{ 
	return unit_get_palette(unit)/4; 
}

static inline void unit_set_moved (int unit) 
{ 
	unit_set_palette(unit, unit_get_player(unit)+4 );
}

static inline void unit_reset_moved (int unit) 
{
	unit_set_palette(unit, unit_get_player(unit));
}

// get manhattan distance between two units
static inline uint8_t unit_get_mdistance(int a,int b) 
{
    const object *ua = game_info.units[a];
    const int ax = (ua->x%1024)/16;
    const int ay = (ua->y%1024)/16;

    const object *ub = game_info.units[b];
    const int bx = (ub->x%1024)/16;
    const int by = (ub->y%1024)/16;

    return abs(ax-bx)+abs(ay-by);
}

// gets position of a unit by ID - hidden or not
static inline uint16_t unit_get_pos(int id)
{
    const object *u = game_info.units[id];
    return (u->y%1024)/16 * SCREEN_W + u->x/16;
}

static inline void unit_set_pos(int id, int pos)
{
    object *u  = game_info.units[id];
    object *uh = game_info.units_health[id];
    u->x = (pos%SCREEN_W)*16;
    u->y = (pos/SCREEN_W)*16;
    uh->x = u->x;
    uh->y = u->y;
}


// get the terrain type the unit is on
static inline uint8_t unit_get_terrain (int unit)
{
    // modulo 1024 : OK hidden or not
    return tile_terrain[game_info.vram[unit_get_pos(unit)]];
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

void unit_info(uint8_t uid);
int unit_attack_damage(uint8_t from, uint8_t to);
uint8_t unit_new (uint8_t x, uint8_t y, uint8_t type, uint8_t player_id );
void unit_moveto(int unit_id, char *path);

#ifdef UNITS_IMPLEMENTATION

void unit_info(uint8_t uid) 
{
    message ("unit:%d - player:%d type:%s on terrain:%s\n", 
            uid, 
            unit_get_player(uid), 
            unit_names[unit_get_type(uid)],
            terrain_names[unit_get_terrain(uid)]
            );
}

/* 
percentage (0-255) of attack of a unit to another 
given attack types and attacked unit terrain defense
*/
int unit_attack_damage(uint8_t from, uint8_t to)
{
    // base percentage attacking -> attacked types : 0-10
    int attack = unit_damage_table [unit_get_type(from)][unit_get_type(to)] ; 

    // defense of the terrain the attacked unit is 0-5
    int defense = terrain_defense[unit_get_terrain(to)]; 
    int damage = attack*256 / (defense+5);
    message("attack %d def %d res: %d\n",attack,defense,damage);
    return damage;
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

    unit_set_type(uid,type);
    unit_set_player(uid,player_id);

    // top of prec, below mouse_cursor
    object *oh=sprite3_new(data_misc_16x16_spr, x*16, y*16, 4); 
    game_info.units_health[uid] = oh;

    unit_set_health(uid, 10); 

    message("init unit %d at %d,%d : typ %d color %d\n",uid, o->x, o->y, type, player_id);
    return uid;
}

void unit_moveto(int unit_id, char *path)
{
    // animate move to dest
    object *o  = game_info.units[unit_id];
    object *oh = game_info.units_health[unit_id];

    for (;*path;path++) {
        for (int i=0;i<16;i++){ 
            o->fr &= ~7;
            switch(*path) {
                case 'N' : o->y-=1; o->fr+=6; break;
                case 'S' : o->y+=1; o->fr+=4; break;
                case 'E' : o->x+=1; o->fr+=0; break;
                case 'W' : o->x-=1; o->fr+=2; break;
                default : message ("unknown character : %c\n",*path); break;
            }
            o->fr += (vga_frame/16)%2;
            oh->x = o->x;
            oh->y = o->y;
            wait_vsync(1);
        }
    }
    // reset to rest frame
    o->fr &= ~7;
}

#endif 