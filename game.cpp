#include <string.h>

#include "defs.h"
#include "game.h"
#include "unit.h"

extern "C" {
#include "lib/mod/mod32.h"
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

void Game::unit_remove(Unit *u)
{
    for (int i=0;i<MAX_UNITS;i++)
        if (units[i]==u) {
            blitter_remove(static_cast<object *>(u));
            units[i] = 0;
            break;
        }
}

void Game::next_player()
{
    int next=current_player;
    while (1) {
        next = (next+1)%4;
        if (player_type[next]!=player_notused && player_type[next]!=player_off)
            break;
    }
    if (next == current_player)
        finished_game = true;
    else
        current_player = next;
}

void Game::leave_level()
{
    // release all blitter sprites, set to zero
    // release player sprite
}

// global game element init - fixme make that a constructr ? would need dyn memory
void Game::init()
{
    map = tilemap_new (
        &data_tiles_bg_tset[4], // no palette
        0,0,
        TMAP_HEADER(SCREEN_W,SCREEN_H,TSET_16,TMAP_U16),
        vram
    );

    // for game only, not intro

    // init play ? level ?
    cursor = sprite3_new(data_units_16x16_spr, 0,1024,1);
    cursor->fr = fr_unit_cursor;

    for (int i=0;i<4;i++) {
        player_type[i]=player_notused;
        player_avatar[i]=i;

    }
    face = sprite3_new(data_faces_26x26_spr,0,-6,5);
}

#define MAP_HEADER_SZ 8

void load_map(int map) {
    memcpy(
        game_info.vram,
        &data_map_map[MAP_HEADER_SZ+sizeof(game_info.vram)*map],
        sizeof(game_info.vram)
    );
}

void load_units(int map)
{
    // load / free unused units
    for (int i=0;i<MAX_UNITS;i++) { // unit
        const uint8_t *unit = &level_units[map][i][0];

        if (!unit[2]) {// no type defined : set as not used
            game_info.units[i] = nullptr;
        } else if (unit[2]==16) {
            // special : flag
        } else {
            game_info.unit_new(unit[0],unit[1],unit[2]-1,unit[3]);
            game_info.player_type[unit[3]] = unit[3]==0 ? player_human : player_cpu0;
        }
    }
}

void Game::start_level(int level)
{
    load_mod(data_song_mod);
    load_map(level);
    load_units(level);
    game_info.finished_game = false;
}

void Game::get_possible_targets(Unit &attacking)
{
    message("selecting attack target\n");
    // get targets
    nbtargets=0;
    // for each target
    for (int i=0 ; i<MAX_UNITS ; i++) {
        Unit *u = units[i];
        if (!u) continue; // unused unit, skip

        if (u->player() == current_player) continue; // same player

        // unit within reach ?
        if (attacking.can_attack(*u)) {
            targets[nbtargets++]=u;
            message("possible target : \n");
            u->info();
        }

        // enough targets ?
        if (nbtargets == 8) break;
    }
}

void Game::harvest()
{
    object *spr = sprite3_new(data_misc_16x16_spr,0,0,5);
    for (int i=0;i<MAX_UNITS;i++) {
        Unit *u = units[i];
        // for all my units
        if (!u || u->player()!=current_player)
            continue;

        if ( u->type() == unit_farmer || u->type() == unit_farmer_f ) {
            // find resource for terrain
            for (int res=0;res<4;res++) {
                if (resource_terrain[res]==u->terrain()) {
                    message("terrain %d resource %d\n",u->terrain(),res);

                    // fixme SFX
                    // little animation
                    spr->fr = fr_misc_food+res;
                    spr->x  = units[i]->x;
                    spr->y  = units[i]->y;

                    int tgt_x = 2+16*(16+res*2);
                    int tgt_y = 0;

                    for (int i=0;i<32;i++){
                        spr->x += (tgt_x - spr->x)/(32-i);
                        spr->y += (tgt_y - spr->y)/(32-i);
                        wait_vsync(1);
                    }
                    // update stat
                    resources [current_player][res]++;
                    draw_hud(); // refresh
                }
            }
        }
    }
    blitter_remove(spr);
}



void Game::draw_hud()
{
    // resources
    vram[17] = tile_zero+resources[current_player][resource_food];
    vram[19] = tile_zero+resources[current_player][resource_gold];
    vram[21] = tile_zero+resources[current_player][resource_wood];
    vram[23] = tile_zero+resources[current_player][resource_stone];

    // current player
    vram[2] = tile_P1 + current_player;

    // set face hud
    face->fr = face_frame(current_player,face_idle);
    face->x = 0;
    face->y = -6;
    face->h = 21;
}

void Game::ready_animation()
{
    // fixme pause at middle 
    // fixme get ready + PLAYER
    mod_jumpto(songorder_next);

    face->fr = face_frame(current_player, face_idle);
    face->y=134;
    face->h=26;
    object *next_spr = sprite3_new(data_next_player_spr,0,130,6);
    for (next_spr->x=-30;next_spr->x<400+100;next_spr->x+=4) {
        face->x=next_spr->x+2;
        wait_vsync(1);
    }
    blitter_remove(next_spr);
    wait_vsync(60);
    mod_jumpto(songorder_song);
}


