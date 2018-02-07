// Units
#include "data.h"
#include "tinystrat.h"
#include "defs.h"
#include "stdbool.h"

#include <stdlib.h> // abs

static inline void unit_remove(uint8_t uid)
{
    if (!game_info.units[uid]) return;
    blitter_remove(game_info.units[uid]);
    game_info.units[uid] = 0;
}

static inline bool unit_empty(int unit)
{
    return !game_info.units[unit];
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
    game_info.units[unit_id]->c = health;
}

static inline uint8_t unit_get_health(uint8_t unit_id)
{
    return game_info.units[unit_id]->c;
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
    u->x = (pos%SCREEN_W)*16;
    u->y = (pos/SCREEN_W)*16;
}

static inline bool unit_can_attack(int attacking, int attacked)
{
    const int dist = unit_get_mdistance(attacking,attacked);
    const int attack_type = unit_get_type(attacking);

    return (dist >= unit_attack_range_table[attack_type][0] && \
        dist <= unit_attack_range_table[attack_type][1]);
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
    }
}

static inline void unit_show(uint8_t uid)
{
    if (game_info.units[uid]) {
        game_info.units[uid]->y&= ~1024;
    }
}

// find ID of unit at this screen position, or -1 if not found
static inline int unit_at(int pos)
{
    for (int uid=0;uid<MAX_UNITS;uid++) {
        object *obj = game_info.units[uid];
        if (obj && obj->x/16 == pos%SCREEN_W && obj->y/16 == pos/SCREEN_W ) {
            return uid;
        }
    }
    return -1;
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

void sprite3_cpl_line_noclip (struct object *o);

#define YLIFE 15
static void unit_graph_line(struct object *o) {
    // draw sprite 3
    sprite3_cpl_line_noclip(o);

    // overdraw life as object->c
    const int y=vga_line-o->ry;
    if (y >= 16-o->c ) {
        draw_buffer[o->x+1]=RGB(y*20-100,305-y*20,0);
    }
    // fixme color green -> yellow -> red pixel colors gradient , not same

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
    o->line = unit_graph_line; // replace with unit drawing
    o->frame = 0; // do not change line draw - no clip

    game_info.units[uid] = o;

    unit_set_type(uid,type);
    unit_set_player(uid,player_id);
    unit_set_health(uid, 10);

    // message("init unit %d at %d,%d : typ %d color %d\n",uid, o->x, o->y, type, player_id);
    return uid;
}

void unit_moveto(int unit_id, char *path)
{
    // animate move to dest
    object *o  = game_info.units[unit_id];

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
            wait_vsync(1);
        }
    }
    // reset to rest frame
    o->fr &= ~7;
}

#endif