#include "defs.h"
#include "game.h"
#include "unit.h"

void Game::unit_remove(Unit *u)
{
    for (int i=0;i<MAX_UNITS;i++)
        if (units[i]==u) {
            blitter_remove(static_cast<object *>(u));
            units[i] = 0;
            break;
        }
}

// find ID of unit at this screen position, or -1 if not found
Unit *Game::unit_at(int pos)
{
    for (int uid=0;uid<MAX_UNITS;uid++) {
        Unit *u = units[uid];
        if (u && u->x/16 == pos%SCREEN_W && u->y/16 == pos/SCREEN_W ) {
            return u;
        }
    }
    return nullptr;
}

Unit *Game::unit_new (uint8_t x, uint8_t y, uint8_t type, uint8_t player_id )
{
    // find first empty unit
    int uid;

    for (uid=0;uid<MAX_UNITS;uid++) {
        if (!units[uid]) break;
    }

    if (uid==MAX_UNITS) {
        message ("no more free units\n");
        die(2,1);
    }

    // allocate sprite
    // put below cursors
    object *o =sprite3_new( data_units_16x16_spr, x*16, y*16, 5 );
    o->line = Unit::graph_line; // replace with unit drawing
    o->frame = 0; // do not change line draw - no clip
    Unit *u = static_cast<Unit*>(o);


    units[uid] = u;


    u->type(type);
    u->player(player_id);
    u->health(10);

    // message("init unit %d at %d,%d : typ %d color %d\n",uid, o->x, o->y, type, player_id);
    return u;
}
