#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> // abs
// --- Fake ---------------------------------------------------------
struct Player {
	uint8_t type; // type of player : 0 : absent, 1 : IA1 (should be an enum)
	int hq; // position of HQ 
};

struct Unit {
	uint8_t player_id;
	uint8_t pos_x, pos_y; 
	uint8_t life;
};

enum {unit_farmer, unit_farmer_f};

#define SCREEN_W 16
#define SCREEN_H 8
#define NB_UNITS 8
#define NB_PLAYERS 2

#define SCORE_OCCUPY 2
#define SCORE_HARVEST 10 // and future

struct Unit units[NB_UNITS] = {
};
struct Player players[NB_PLAYERS];

int unit_player(int n) { return units[n].player_id; }
int unit_pos(int n)    { return units[n].pos_x + units[n].pos_x * SCREEN_W;}
void unit_set_played (int n) {};
void unit_reset_played (int n) {};
void unit_move(int n, int pos) 
{		// move unit to new position
		// w/ anims
		units[n].pos_x = pos%SCREEN_W;
		units[n].pos_y = pos/SCREEN_W;
}
int unit_type(int n){return 1;}
// animate &co
void unit_harvest(int n) {}
// merge those three ?
void combat (int a, int b) {}
int attack_score(int a, int b) {return 1;}
int unit_can_attack(int player_id, int u) {return 1;}


const uint8_t terrain_type[] = {1}; //  tine_id to terrain_type_id
const uint8_t terrain_score[] = {1,2,3,4,5,6,4,5,4,7}; // terrain to score. defense is better, occupy resources
const uint8_t resource_harvest_type[] = {0,1,2,3,4,0,0,0,0}; // what you can harvest from each terrain, 0=nothing, 1=gold, ...
void wait_vsync(int n) {};

int distance(int pos, int pos2) {
	return abs(pos/SCREEN_W-pos2/SCREEN_W) + abs(pos%SCREEN_W-pos2%SCREEN_W);
}
uint8_t map[SCREEN_W*SCREEN_H];
void update_pathfinding(int unit_id) {}
int pathfinding_cell_empty(int n) { return 0; }

// -- end fake -------------------------------------------------


// refers to units and map. call repeatedly until action = surrender or stop
void surrender(int player_id)
{
	players[player_id].type = 0; // surrender
}

void play_AI_1 (int player_id, int target_player) // --> to player ? return when stops playing. does animations here.
{
	static int c=0;
	if (++c>=8) surrender(player_id); // fixme bah 

	// find target HQ unit 
	const int pos_hq = unit_pos(players[target_player].hq);

	// set not played
	for (int u=0;u<NB_UNITS;u++) {
		if (unit_player(u)==player_id)
			unit_reset_played(player_id);
	}

	// check all units in order and play them 
	for (int unit_id=0;unit_id<NB_UNITS;unit_id++) { // randomize order ?
		if (unit_player(unit_id)!=player_id) continue; // skip it, not me

		// current best action for this unit
		int best_score=-1;  // best score so far
		uint16_t best_pos;  // target position
		int best_target;    // target unit_id , -1 if wait and does not attack

		update_pathfinding(unit_id);

		// for all possible destinations
		for (int newpos=0;newpos<SCREEN_W*SCREEN_H;newpos++) {
			if (pathfinding_cell_empty(newpos)) continue; // pathfindng : cannot reach there : avoid.
			// fixme: not empty ? not possible ! 

			// depends of type of terrain, distance of next human QG (teams?)
			int position_score = terrain_score[terrain_type[map[newpos]]]; // defense, resources also useful to be on even if not taken.
			position_score += 40-distance(newpos, pos_hq); // distance : for this unit type from new position ??? can be expensive ..

			if (unit_type(unit_id)==unit_farmer || unit_type(unit_id)==unit_farmer_f) {
				// XXX resource utility ?
				int score = position_score + (resource_harvest_type[terrain_type[map[newpos]]]>0 ? SCORE_HARVEST : 0); 
				if (score>best_score) {
					best_score=score;
					best_target=-1;
					best_pos=newpos;
				}
			} else {
				// get all possible attack targets for this position (including -1, do not attack)
				for (int u=-1;u<NB_UNITS;u++) {
					int score = position_score;

					if (u>=0 && unit_can_attack(player_id,u)) {
						// simulate attack & update score accordingly
						score += attack_score(player_id,u);
					}

					if (score>best_score) {
						best_score=score;
						best_target = u;
						best_pos=newpos;
					}
				}		
			}
		}

		// -- now play it effectively
		unit_move(unit_id, best_pos);

		if (best_target>=0)
			combat(unit_id,best_target);
		else if (unit_type(unit_id) ==unit_farmer || unit_type(unit_id) == unit_farmer_f) 
			unit_harvest(unit_id); // animate

		unit_set_played(unit_id);
		// -- wait at least next frame until next AI ? - maybe less if already moved.
		wait_vsync(8);
	}

	// build units - start as : build most expensive possible ? - campaign = restrict
	// then : target + build it & select new target
}


int main(void)
{
	// init
	for (int i=0;i<NB_PLAYERS;i++)
		players[i].type=1;
	int current_player=0;

	while (1) {
		current_player = 1-current_player;
		printf("= player %d playing\n", current_player);

		play_AI_1(current_player, 1-current_player); 
		if (!players[current_player].type) break;
	}

	printf("Game finished.\n");
}