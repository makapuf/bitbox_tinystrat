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


void find_next_best_dummy(int &npos, Unit * &nunit)
{
	// find an enemy player in range, go towards it

	nunit=nullptr; // by default dont attack

	// all units not mine
	for (int tgt_unit=0;tgt_unit<MAX_UNITS;tgt_unit++) {
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
				npos  = dest;
				nunit = &tgt;
				return;
			}
		}
	}
}

// best action by heuristic
// updates target, pos
void find_next_best(Unit &u, int &npos, Unit * &target)
{
	int original_pos = u.position(); // save position

	// by default stay here
	int best_score = u.can_harvest()<0 ? 0 : 10;
	npos = original_pos;
	target = nullptr;

	// foreach possible move destination
	for (int dest=SCREEN_W;dest<SCREEN_H*SCREEN_W;dest++) {
		if (cell_isempty(cost_array[dest]))
			continue; // cannot reach
		if (game_info.unit_at(dest)!=nullptr)
			continue; // not free

		// "move" it to new destination - should not be visible
		u.position(dest);

<<<<<<< HEAD
		const int harvest_score = u.can_harvest()<0 ? 0 : 10;
=======
		int harvest_score = 0; // harvesting position

        if ( u.type() == unit_farmer || u.type() == unit_farmer_f ) {
            // find resource for terrain
            for (int res=0;res<4;res++)
                if (resource_terrain[res]==tile_terrain[game_info.vram[dest]]) // terrain on destination
                	harvest_score = 10;
        }
>>>>>>> 60dc692d2f20136be48911e58a61d743a805f372

		const int proxymity_score = 0; // closer to my own units
		const int target_proximity_score = 0; // closer to castle

		const int score = harvest_score + proxymity_score + target_proximity_score;

		// XXX within direct range of being attacked (sum of damage possibly taken) ?

		// try doing nothing
		if ( score > best_score ) {
			message (
				"best so far : harvest %d, proxymity %d, target %d \n",
				harvest_score,
				proxymity_score,
				target_proximity_score
				);

			best_score=score;
			npos=dest;
			target=nullptr;
		}

		// -- for each target in range : try to attack
		for (int i=0 ; i<MAX_UNITS ; i++) {
			Unit& tgt = game_info.units[i];
			if (!tgt) continue; // unused unit, skip
			if (tgt.player() == game_info.current_player) continue; // same player

			int damage=0;
			if (u.can_attack(tgt)) {   // unit within reach ?
				// get score of potential attack
				damage = u.attack_damage(tgt) * u.health() / 256;
				// retaliate
				if (tgt.health()>damage && tgt.can_attack(u)) {   // unit within reach ?
					damage -= u.attack_damage(tgt) * (u.health()-damage) / 256; // anticipate future damage
				}
			}

			// destroyed ? bad
			if (score + damage*3 > best_score ) {
				best_score = score + damage;
				npos=dest;
				target=&tgt;
			}
		}
	}

	// restore unit position
	u.position(original_pos);
}


// CPU play
void play_CPU ()
{
	char path[MAX_PATH]; // merge & remove ?

	for (int i=0;i<MAX_UNITS;i++) {
		Unit& u = game_info.units[i];
		if (!!u) u.moved(false);
	}
	game_info.harvest(); // to main ?

	for ( Unit *u = game_info.myunits(0) ; u ; u=game_info.myunits(u) ) {

		wait_vsync(); // thinking ...
		update_pathfinding(*u);

		int next_pos=0; // target position found by AI
		Unit *target;   // target attack found by AI (or null if pause here).

		find_next_best(*u, next_pos, target);

		// go there
		reconstruct_path(next_pos, path);
		if (*path=='!') continue; // error, cannot move there
		u->moveto(path);

		for (int i=0;i<20;i++) wait_vsync(); // little pause
		if (target)
			combat(*u,*target);

		u->moved(true);
	}
}



