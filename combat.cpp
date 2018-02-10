#include "unit.h"
#include "game.h"
#include "tinystrat.h"
#include "data.h"

static void combat_face_frame(object *face, uint8_t anim_id, uint8_t attacked)
{
    // set combat faces
    static const uint8_t face_anims[][2][2] = { // attacking, attacked ; 2 frames
        {{2,3},{4,5}},
        {{4,5},{2,3}},
        {{2,2},{4,4}},
        {{4,4},{2,2}},
        {{0,0},{0,0}}, // poker face
    };
    face->fr = face->fr / face_NB * face_NB; // reset to idle
    face->fr += face_anims[anim_id][attacked][(vga_frame/16)%2];
}

static void fight_animation( void )
{
    // wait keypress
    object fight_spr;
    sprite3_insert(&fight_spr, data_fight_200x200_spr,100,70,3);
    static const uint8_t fight_anim[8] = {0,1,2,1,3,0,2,1};
    for (int i=0;i<60;i++) {
        fight_spr.fr = fight_anim[(vga_frame/16)%8];
        wait_vsync(1);
    }
    blitter_remove(&fight_spr);
}

static void sprite_set_palette(object *o, int palette)
{
    o->b = (uintptr_t)&data_palettes_bin[1024*palette];
}

// fixme use it !
struct Opponent {
    object bg, sprite, life, face;

    Opponent(bool left, Unit &unit)
    {
        const uint8_t terrain = unit.terrain();
        const uint8_t player = unit.player();

        sprite3_insert(&bg,terrain_bg_table[terrain],left ? 0 : 200, left?-200:300,35);

        sprite3_insert(&sprite, data_units_16x16_spr, left ? -30 : 410, 180, 5 );
        // also other units ...
        sprite.d |= 1; // set render 2X
        sprite.h *= 2;
        sprite.fr = unit.type()*8 + (left ? 0 : 2); // facing left
        sprite_set_palette(&sprite, player);

        sprite3_insert(&life, data_bignum_16x24_spr, left ? 34 : 340,52,5);
        life.fr = unit.health();

        sprite3_insert(&face,data_faces_26x26_spr,left ? 0:360,52,5);
        face.fr = game_info.face_frame(player, face_idle);
        face.h  = 26;
    }

    ~Opponent()
    {
        blitter_remove(&bg);
        blitter_remove(&sprite);
        blitter_remove(&life);
        blitter_remove(&face);
    }
};


void combat (Unit &attacking, Unit &attacked)
{
    mod_jumpto(songorder_battle);

    // hide all units
    for (int i=0;i<MAX_UNITS;i++) {
        Unit &u = game_info.units[i];
        if (!!u) u.hide();
    }
    game_info.face.y += 1024;
    game_info.cursor.y += 1024;

    Opponent op_att (true, attacking);
    Opponent op_def (false, attacked);


    // intro
    for (int i=0;i<25;i++) {
        op_att.bg.y+=10;
        op_def.bg.y-=10;
        wait_vsync(1);
    }
    for (int i=0;i<80;i++) {
        op_att.sprite.x+=2;
        op_att.sprite.fr = (op_att.sprite.fr&~7)   + (vga_frame/8)%2;
        wait_vsync(1);
    }
    for (int i=0;i<80;i++) {
        op_def.sprite.fr  = (op_def.sprite.fr&~7) + 2 + (vga_frame/8)%2;
        op_def.sprite.x-=2;
        wait_vsync(1);
    }

    fight_animation();

    int dmg_given = attacking.do_attack(attacked);
    int dmg_received=0;
    if (attacked.health()>0) {
        if (attacked.can_attack(attacking))
            dmg_received = attacked.do_attack(attacking);
    }

    int anim_id;
    if (attacked.health()==0) {
        anim_id=0;
    } else if (attacking.health()==0) {
        anim_id=1;
    } else if (dmg_given>dmg_received) {
        anim_id=2;
    } else if (dmg_given>dmg_received) {
        anim_id=3;
    } else {
        anim_id=4;
    }

    // anim update scores + faces
    for (int i=0;i<15;i++) {
        // hide old scores
        op_att.life.x -=2;
        combat_face_frame(&op_att.face,anim_id,0);
        op_def.life.x +=2;
        combat_face_frame(&op_def.face,anim_id,1);
        wait_vsync(1);
    }

    op_att.life.fr = attacking.health();
    op_def.life.fr  = attacked.health();

    // set sprites to explosion if needed
    // idle anim of units / explosion

    for (int i=0;i<15;i++) {
        op_att.life.x+=2;
        op_def.life.x-=2;
        combat_face_frame(&op_att.face,anim_id,0);
        combat_face_frame(&op_def.face,anim_id,1);
        wait_vsync(1);
    }

    // wait for keypress
    for (int i=0;i<120;i++) {
        int pressed = gamepad_pressed();
        if (pressed && gamepad_A) break;
        combat_face_frame(&op_att.face,anim_id,0);
        combat_face_frame(&op_def.face,anim_id,1);
        wait_vsync(1);
    }

    // outro - quicker
    while (op_att.bg.y > -150 ) {
        op_att.bg.y-=16;
        op_def.bg.y+=16;
        wait_vsync(1);
    }

    // show all units again
    for (int i=0;i<MAX_UNITS;i++) {
        Unit &u = game_info.units[i];
        if (!!u) u.show();
    }

    // small anims on board if any unit just died
    if (attacked.health()==0) {
        // set frame to explosion
        game_info.unit_remove(&attacked);
    }
    if (attacking.health()==0) {
        game_info.unit_remove(&attacking);
    }

    game_info.cursor.y -= 1024; // show cursor
    mod_jumpto(songorder_song);

}
