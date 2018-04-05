#pragma once

#include "tinystrat.h"
#include "grid.h"
#include "unit.h"


struct Game {
	uint8_t map_id;
    uint8_t level; // current level
    uint8_t day;
    bool finished_game;
    bool finished_turn;

    uint8_t current_player; // 0-3

    Unit *cursor_unit;  // ref to object under cursor, <0 : None

    // per player info
    uint8_t player_type[4]; // replace with fnpointer ?
    uint8_t player_avatar[4];

    uint8_t resources[4][4]; // per color / resource id

    // global info
    Unit units[MAX_UNITS];      // frame as unit type, set line=0 if not used. palette as player+aleady moved (faded)

    uint16_t vram[SCREEN_W*SCREEN_H]; // map

    object map;		// BG
    object cursor;
    object face;       // for avatar : player

    Grid grid;

    // temp for current targets -> use a next_target() method ?
    Unit *targets[8];
    int nbtargets;

    Unit *unit_at(int pos);
	Unit *unit_new (uint8_t x, uint8_t y, uint8_t type, uint8_t player_id );
	void  unit_remove(Unit *u);

	void harvest(); // harvest for current player

    /* updates info about terrain under mouse_cursor
       returns the unit under mouse_cursor if any or NULL
       */
    void update_cursor_info(void);

	void init();
	void next_player();
	void start_level(int level);
	void leave_level();
    void load_map();
    void load_units(); // also declares which player is what

	void get_possible_targets(Unit &attacking);

	int face_frame(int player, enum Face_State st) { return player_avatar[player]*face_NB+st; }
	void ready_animation();
	void draw_hud();

    void bg_frame();

    // gets next unit of mine starting from this one, or first if 'from' is null
    // returns nullptr if none found
    Unit *myunits ( Unit *from );

    // get cursor position on tilemap
    inline int cursor_position (void) {
        return (cursor.y/16)*SCREEN_W + cursor.x/16;
    }
};

extern Game game_info;
