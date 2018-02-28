/* test ai */
#include "stdlib.h" // rand : make myrand ?
#include "tinystrat.h"
#include "unit.h"
#include "game.h"

static const int8_t neighbours[]={
	+1,-1,
	+SCREEN_W, -SCREEN_W,
	SCREEN_W+1,SCREEN_W-1,
	-SCREEN_W+1,-SCREEN_W-1
};

// dummy CPU play
void play_CPU0 ()
{
    char path[MAX_PATH]; // merge & remove ?

    for (int i=0;i<MAX_UNITS;i++) {
		Unit& u = game_info.units[i];
        if (!!u) u.moved(false);
    }
    game_info.harvest();

	for (int unit_id=0;unit_id<NB_UNITS;unit_id++) { // randomize order ?
		Unit &u = game_info.units[unit_id];
		if (!u || u.player() != game_info.current_player)
			continue;

		update_pathfinding(u);

		// find an enemy player in range, go towards it
		int tgt_unit;
		for (tgt_unit=0;tgt_unit<MAX_UNITS;tgt_unit++) {
			Unit &tgt = game_info.units[tgt_unit];

			if (!tgt || tgt.player() == game_info.current_player)
				continue;

			int pos = tgt.position();

			// find free neighbour place
			for (int n=0;n<8;n++) {
				const int dest = pos+neighbours[n];
				// fixme not if range is not 1!
				// not if cannot attack !

				if (!cell_isempty(cost_array[dest]) && game_info.unit_at(dest)==nullptr) {
					// go there
			        reconstruct_path(dest, path);
			        if (*path=='!') continue; // error, cannot move there
			        u.moveto(path);
			        for (int i=0;i<20;i++) wait_vsync();
					combat(u,tgt);
					// done for this unit
					goto end_unit_turn;
				}
			}
		}

		// no unit in sight
		if (tgt_unit==MAX_UNITS) {
			// do nothing
		}

end_unit_turn:
		u.moved(true);
	}

}