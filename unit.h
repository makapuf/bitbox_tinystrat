// Units
#include <cstdlib> // abs
#pragma once

#include "data.h" // palettes
#include "tinystrat.h"
#include "defs.h"

extern "C" { void sprite3_cpl_line_noclip (struct object *o); }
#define YLIFE 15

struct Unit : public object
{
    bool operator!() const { return line==nullptr; } // truth testing

    // return unit_type_id
    int  type () const { return fr/8; }
    void type (int typeunit_id) { fr = typeunit_id*8; };

    void palette (int palette) { b = (uintptr_t)    &data_palettes_bin[ 512*palette ]; }
    int  palette () const { return (b - (uintptr_t) &data_palettes_bin[0] ) / 512; }

    void    health(uint8_t health) { c = health; }
    uint8_t health() { return c; }

    int  player() const { return palette()%4; }
    void player(int player) { palette(player); } // also reset moved

    // frame 0-1, direction nsew
    void set_frame (int direction, int frame) { fr = (fr & ~7) | direction*2 | frame; }

    bool moved () const { return palette()/4 || type()==unit_flag; }
    void moved (bool moved) { palette( player() + (moved ? 4 : 0)); }

    // gets position of a unit - hidden or not
    uint16_t position () const { return (y%1024)/16 * SCREEN_W + x/16; }
    void     position (uint16_t pos) { x = (pos%SCREEN_W)*16; y = (pos/SCREEN_W)*16; }

    // get the terrain type of this position
    uint8_t terrain() const;

    // return resource type if can harvest, -1 if not
    int can_harvest() const;

    bool can_attack(const Unit &attacked) const
    {
        const int dist = mdistance(attacked);
        const int attack_type = type();
        return (dist >= unit_attack_range_table[attack_type][0] && \
            dist <= unit_attack_range_table[attack_type][1]);
    }

    // get manhattan distance between two units
    uint8_t mdistance(const Unit &other) const
    {
        const int ax = (x%1024)/16;
        const int ay = (y%1024)/16;

        const int bx = (other.x%1024)/16;
        const int by = (other.y%1024)/16;

        return abs(ax-bx)+abs(ay-by);
    }

    void hide () { y |= 1024; }
    void show () { y &= ~1024; }

    void info() const { // tostring ?
        if (!*this) message("UNIT INACTIVE !");
        message ("unit player:%d type:%s on terrain:%s x:%d y:%d\n",
                player(), unit_names[type()], terrain_names[terrain()],x/16,y/16
                );
    }

    /* ratio (0-255) of attack of a unit to another
    given attack types and attacked unit terrain defense
    */
    uint8_t attack_damage(const Unit &to) const
    {
        // base percentage attacking -> attacked types : 0-10
        int attack = unit_damage_table [type()][to.type()];
        // defense of the terrain the attacked unit is 0-5
        int defense = terrain_defense[to.terrain()];
        int damage = attack*256 / (defense+5);
        message("attack %d def %d res: %d\n",attack,defense,damage);
        return damage<=255 ? damage : 255;
    }

    // move unit animation
    void moveto(const char *path);

    // return damage given
    uint8_t do_attack(Unit &attacked)
    {
        message (" * attack\n");
        info();
        attacked.info();

        uint8_t damage = attack_damage(attacked) * health() / 256;

        // inflict damage
        message("inflicting %d damage\n",damage);

        uint8_t h = attacked.health();
        attacked.health(h>damage ? h-damage : 0); // do not remove unit yet, anims not done
        return damage;
    }

    // after combat is finished, call for each unit on board
    void die();

    // blit it
    static void graph_line(struct object *o) {
        // draw sprite 3
        sprite3_cpl_line_noclip(o);

        // overdraw life as object->c
        const int y = vga_line-o->y;
        const pixel_t color = RGB((10-o->c)*20,o->c*20+50,30);

        if (y==14)
            for (unsigned int i=0;i<o->c;i++)
                draw_buffer[o->x+3+i]=color;
    }
};


// updates cost_array and frontier
// from source, up to max_cost (included)
void update_pathfinding ( const Unit &source );

void get_possible_targets(Unit &attacking); // main

void combat (Unit &attacking_unit, Unit &attacked_unit);
