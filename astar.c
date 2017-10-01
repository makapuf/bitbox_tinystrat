#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <stdlib.h> // rand
#include <stdio.h>

#define SCREEN_W 25
#define SCREEN_H 19
#define MAX_FRONTIER 32
#define MAX_COST 63
#define DUMMY_POS 1023

#define DEBUG

uint8_t terrains[SCREEN_W*SCREEN_H];
const uint8_t terrain_cost[8] = {0,1,0,16,0,2,1,0};

void display_terrain() 
{
	printf(" *** Terrain\n");
	for (int j=0;j<SCREEN_H;j++) {
		for (int i=0;i<SCREEN_W;i++) {
			uint8_t c = terrain_cost[terrains[j*SCREEN_W+i]];
			if (c == 16)
				printf("x ");
			else 
				printf("%d ",c);
		}
		printf("\n");
	}
}
void init() {
	for (int j=0;j<SCREEN_H;j++)
		for (int i=0;i<SCREEN_W;i++)
			terrains[j*SCREEN_W+i] = rand()%8;
}

static inline uint16_t cell(int cost, int pos)
{
	if (cost>63 || pos > 1023) {
		printf("Internal Error : cannot encode cell %d-%d\n",cost, pos);
		display_current();
		exit(1);
	}
	return cost<<10 | pos;
}
static inline int cost(uint16_t cell)
{
	return cell>>10;
}
static inline int pos(uint16_t cell)
{
	return cell & 1023;
}

// cell : cost:6 << 10 | position:10
static uint16_t current_tab[SCREEN_W* SCREEN_H];
static uint16_t frontier[MAX_FRONTIER];

void display_frontier()
{
	printf("Frontier : ");
	for (int i=0;i<MAX_FRONTIER;i++) {
		if (frontier[i]==0xffff) 
			printf("- ");
		else 
			printf("%d-%x ",cost(frontier[i]), pos(frontier[i]));
	}
	printf("\n");
}


void display_current() 
{
	printf(" *** Current \n");
	for (int j=0;j<SCREEN_H;j++) {
		for (int i=0;i<SCREEN_W;i++) {
			uint16_t c = current_tab[j*SCREEN_W+i];
			if (c==0xffff) 
				printf(" - ");
			else 
				printf("%d-%d,%d ",cost(c),pos(c)%SCREEN_W, pos(c)/SCREEN_W);
		}
		printf("\n");
	}
}

// build costs and return max cost. if returns -1, did not find it 
int astar(int source, int dest, int max_cost )
{
	for (int i=0;i<SCREEN_W*SCREEN_H;i++)
		current_tab[i] = cell(MAX_COST, DUMMY_POS);
	current_tab[source] = cell(0,DUMMY_POS);

	for (int i=1;i<MAX_FRONTIER;i++)
		frontier[i]=cell(MAX_COST,DUMMY_POS);
	frontier[0] = cell(0,source);

	while (1) {
		#ifdef DEBUG
		display_frontier();
		#endif

		// find smallest element in frontier
		int mini=0, minicost=MAX_COST;
		#ifdef DEBUG
		int used_frontier = 0;
		#endif 

		for (int i=0;i<MAX_FRONTIER;i++) {
			#ifdef DEBUG
			if (frontier[i] != 0xffff) ++used_frontier;
			#endif 
			if (cost(frontier[i])<minicost) {
				mini=i;
				minicost = cost(frontier[i]);
			}
		}
		// nothing in frontier and not found : error
		if (minicost==MAX_COST) return -1;
		#ifdef DEBUG
		printf("max frontier %d\n",used_frontier);
		#endif 
		

		// get & remove it
		uint16_t current = frontier[mini]; 
		frontier[mini] = cell(MAX_COST, DUMMY_POS);

		// found it ?
		if (pos(current) == dest) {
			return cost(current);
		}

		{
			int c = pos(current);
			// add cells around to frontier
			int c_x = c%SCREEN_W;
			int c_y = c/SCREEN_W;

			int newpos[4] = {0,0,0,0}; // new positions
			int nb      = 0;

			if (c_x>0)        newpos[nb++] = c-1;
			if (c_x<SCREEN_W-1) newpos[nb++] = c+1;
			if (c_y>0)        newpos[nb++] = c-SCREEN_W;
			if (c_y<SCREEN_H-1) newpos[nb++] = c+SCREEN_W;;

			#ifdef DEBUG
			printf("current %d\n", c);
			#endif 

			// for each possible neighbour of c
			for (int i=0;i<nb;i++) {
				int npos = newpos[i];

				int new_cost = cost(current_tab[c]) + terrain_cost[terrains[npos]];

				// frontier.append((nx,ny,new_cost + abs(nx-dst[0]) + abs(ny-dst[1])))
				if (new_cost < cost(current_tab[npos]) && new_cost<max_cost) {
					#ifdef DEBUG
					printf("append npos (%d,%d) - ncost %d old %d\n", npos%SCREEN_W, npos/SCREEN_W, new_cost, cost(current_tab[npos]));
					#endif 

					// append to frontier : find free cell, write to it. 
					for (int i=0;i<MAX_FRONTIER;i++) {
						// free cell found ? 
						if (frontier[i]==cell(MAX_COST,DUMMY_POS)) {
							#ifdef DEBUG
							printf("insert to frontier [%d]\n",i);
							#endif 

							// beware heuristic can be larger and overflow cost cell. use u32 for costs ? u16 + u8 ?
							unsigned  heuristic = abs(dest/SCREEN_W-npos/SCREEN_W) + abs(dest%SCREEN_W-npos%SCREEN_W);
							frontier[i] = cell(new_cost + heuristic, npos); // divide to avoid overflow
							break;
						}
					}
					if (i==MAX_FRONTIER) {
						printf("ERROR : not enough room in frontier !\n");
						return -1;
					}

					// update current table with new_cost, come_from = current
					current_tab[npos] = cell(new_cost,c);
				}
			}
		}
	} // while
}

void reconstruct_path(int src, int dst)
{
	do {
		printf("%d,%d\n", dst%SCREEN_W, dst/SCREEN_W);
		dst = pos(current_tab[dst]);
		if (current_tab[dst]==0xffff) {
			printf("Error !\n");
			exit(1);
		}
	} while(dst != src);
}

int main() {
	init();
	int dest = SCREEN_W*7+8;
	display_terrain();
	if (astar(2,dest,15)>=0) {
		printf("found !\n");
		display_current();		
		reconstruct_path(2,dest);
	} else
		printf("NOT found !\n");
	
	return 0;
}
