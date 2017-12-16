/* faces */ 
#include <stdint.h>
#include "lib/blitter/blitter.h"
#include "tinystrat.h"
#include "data.h"

void face_init(void)
{
    game_info.face[0] = sprite3_new(data_faces_26x26_spr,0,-6,5);    
    game_info.face[1] = sprite3_new(data_faces_26x26_spr,0,-6,5);    
}

static int combat_adversary;

void set_adversary_face(int adv)
{
	combat_adversary = game_info.player_avatar[adv];
}

void face_frame(void)
{
	object *pl = game_info.face[0];
	object *adv = game_info.face[1]; // adversary

	// defaults
	pl->fr = game_info.player_avatar[game_info.current_player]*5; 
	pl->x  = 0; 
	pl->y  = 52; 
	pl->h  = 26;

	adv->fr = game_info.player_avatar[combat_adversary]*5; 
	adv->x	= 360;
	adv->y	= 50;

    // position
    switch (game_info.avatar_state) {
    	case face_hud : 
	    	pl->x = 0; 
	    	pl->y = -6; 
	    	pl->h = 21; 
			adv->y= 1024; // hide
	    	break;

	    case face_off : 
	    	pl->y = 1024; 
	    	break;

	    case face_idle : 
	    	break;

	    case face_laugh : 
	    	if (vga_frame/16%4==0) {
	    		pl->fr  += 2;
	    		adv->fr += 3;
	    	}
	    	break;

	    case face_cry : 
	    	if (vga_frame/16%4==0) {
	    		pl->fr  += 3;
	    		adv->fr += 2;
	    	}
	    	break;

	    case face_talk : 
	    	break;

	}
}
