#include <string.h>

#include "defs.h"
#include "game.h"
#include "unit.h"

extern "C" {
#include "lib/mod/mod32.h"
#include "lib/blitter/blitter.h"
}

void text(int x, int y, const char *txt);

// find ID of unit at this screen position, or -1 if not found
Unit *Game::unit_at(int pos)
{
    for (int uid=0;uid<MAX_UNITS;uid++) {
        Unit *u = &units[uid];
        if (!!(*u) && u->x/16 == pos%SCREEN_W && u->y/16 == pos/SCREEN_W ) {
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
    Unit *u = &units[uid];
    sprite3_load(u, SPRITE(units_16x16));
    blitter_insert(u, x*16, y*16, 5 ); // put below cursors
    u->line = Unit::graph_line; // replace with unit drawing
    u->frame = 0; // do not change line draw - no clip

    u->type(type);
    u->player(player_id);
    u->health(10);

    return u;
}

void Game::unit_remove(Unit *u)
{
    for (int i=0;i<MAX_UNITS;i++)
        if (&units[i]==u) {
            blitter_remove(u);
            units[i].line = 0;
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

void Game::ready_animation()
{
    // fixme get ready + PLAYER
    mod_jumpto(songorder_next);

    face.fr = face_frame(current_player, face_idle);
    face.y=134;
    face.h=26;


    object next_spr;
    sprite3_load(&next_spr, SPRITE(next_player));
    blitter_insert(&next_spr,-60,130,1);
    for (int t=-80;t<80;t++) {
        int x = (VGA_H_PIXELS-next_spr.w)/2+t*t*t/2048;
        next_spr.x = x;

        face.x = x+2;
        game_info.bg_frame();
        wait_vsync();
    }

    blitter_remove(&next_spr);
    draw_hud();

    for (int i=0;i<60;i++) {
        game_info.bg_frame();
        wait_vsync();
    }
    mod_jumpto(songorder_song);
}

void Game::leave_level()
{
    // release all blitter sprites, set to zero
    // release player sprite
    grid.hide();
}

void Game::init()
{
    tilemap_init (&map,
        &data_tiles_bg[ 4 ], // no palette
        0,0,
        TMAP_HEADER(SCREEN_W,SCREEN_H,TSET_16,TMAP_U16),
        vram
    );

    blitter_insert(&map,0,0,100);

    // init play ? level ?
    sprite3_load(&cursor, SPRITE(units_16x16));
    blitter_insert(&cursor, 0,1024,1);
    cursor.fr = fr_unit_cursor;

    for (int i=0;i<4;i++) {
        player_type[i]=player_notused;
        player_avatar[i]=i;
    }

    sprite3_load(&face, SPRITE(faces_26x26));
    blitter_insert(&face,0,-6,0);
}

#define MAP_HEADER_SZ 8

void Game::load_map() {
    memcpy(
        vram,
        &data_map [ MAP_HEADER_SZ+sizeof(vram)*level ],
        sizeof(vram)
    );

    // copy background from first level over
    memcpy( vram, &data_map [MAP_HEADER_SZ], SCREEN_W*2 );
}

void Game::load_units()
{
    // load / free unused units
    for (int i=0;i<MAX_UNITS;i++) { // unit
        const uint8_t *unit = &level_info[level].units[i][0];

        if (!unit[2]) {// no type defined : set as not used
            units[i].line = nullptr;
        } else if (unit[2]==16) {
            // special : flag
        } else {
            unit_new(unit[0],unit[1],unit[2]-1,unit[3]);
            player_type[unit[3]] = unit[3]==0 ? player_human : player_cpu0;
            //player_type[unit[3]] = player_cpu0;
        }
    }
}


void Game::start_level(int _level)
{
    level=_level;
    message ("starting level %d \n",level);

    load_mod(&data_song_mod);
    load_map();
    load_units(); // also sets player type
    grid.show();

    text (100,20,level_info[level].intro);
    finished_game = false;
}

void Game::get_possible_targets(Unit &attacking)
{
    message("selecting attack target\n");
    // get targets
    nbtargets=0;
    // for each target
    for (int i=0 ; i<MAX_UNITS ; i++) {
        Unit& u = units[i];
        if (!u) continue; // unused unit, skip

        if (u.player() == current_player) continue; // same player

        // unit within reach ?
        if (attacking.can_attack(u)) {
            targets[nbtargets++]=&u;
            message("possible target : \n");
            u.info();
        }

        // enough targets ?
        if (nbtargets == 8) break;
    }
}

void Game::harvest()
{
    object spr;
    sprite3_load(&spr,SPRITE(misc_16x16));
    blitter_insert(&spr,0,0,5);
    for (int i=0;i<MAX_UNITS;i++) {
        Unit &u = units[i];
        // for all my units
        if (!u || u.player()!=current_player)
            continue;

        // fixme use can_harvest
        if ( u.type() == unit_farmer || u.type() == unit_farmer_f ) {
            // find resource for terrain
            for (int res=0;res<4;res++) {
                if (resource_terrain[res]==u.terrain()) {
                    message("terrain %d resource %d\n",u.terrain(),res);

                    // fixme SFX
                    // little animation
                    spr.fr = fr_misc_food+res;
                    spr.x  = units[i].x;
                    spr.y  = units[i].y;

                    int tgt_x = 2+16*(16+res*2);
                    int tgt_y = 0;

                    for (int i=0;i<32;i++){
                        spr.x += (tgt_x - spr.x)/(32-i);
                        spr.y += (tgt_y - spr.y)/(32-i);
                        wait_vsync();
                    }
                    // update stat
                    resources [current_player][res]++;
                    draw_hud(); // refresh
                }
            }
        }
    }
    blitter_remove(&spr);
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
    face.fr = face_frame(current_player,face_idle);
    face.x = 0;
    face.y = -6;
    face.h = 21;
}

/* updates info about terrain under mouse_cursor
   returns the unit under mouse_cursor if any or NULL
   */
void Game::update_cursor_info(void)
{
    // "blink" mouse_cursor
    cursor.fr=(vga_frame/32)%2 ? fr_unit_cursor : fr_unit_cursor2;

    // terrain under the mouse_cursor
    const uint16_t tile_id = vram[game_info.cursor_position()];
    int terrain_id = tile_terrain[tile_id];
    vram[3]=tile_id;
    // defense info
    vram[6]=tile_zero + terrain_defense[terrain_id];

    // resource tile
    vram[7] = 162;
    for (int r=0;r<4;r++) {
        if (resource_terrain[r]==terrain_id) {
            vram[7] = tile_gold + r;
            break;
        }
    }


    // find unit under mouse_cursor - if any
    cursor_unit = unit_at(cursor_position());

    draw_hud();
}


Unit *Game::myunits ( Unit *from )
{
    for (Unit *u = from ? from+1 : &units[0] ; u != &units[MAX_UNITS] ; u++) {
        if (!!u && u->player() == current_player)
            return u;
    }
    return nullptr;
}


// search for an animated frame -> to function returning next or zero
static uint8_t find_next_anim_tile(uint8_t tile_id)
{
    for (int anim=0;anim<NB_TILES_ANIMATIONS;anim++)
        for (int frame=0;frame<4;frame++) {
            if ( tile_id == anim_tiles[anim][frame] ) {
                if (frame==3 || anim_tiles[anim][frame+1]==0) {
                    return anim_tiles[anim][0];
                } else {
                    return anim_tiles[anim][frame+1];
                }
            }
        }
    return 0;
}

// update background animations & wait next frame
void Game::bg_frame()
{
    // update one line per frame
    const int line = vga_frame%32;
    if (line==0 || line>=SCREEN_H) return;

    for (int i = SCREEN_W*line ; i < SCREEN_W*(line+1) ; i++)
    {
        uint8_t next=find_next_anim_tile(vram[i]);
        if (next) vram[i] = next;
    }
}