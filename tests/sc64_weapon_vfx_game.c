/* Real combat creates the transient effect; rendering does not delay damage. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

static void needle_shot(int type, int owner)
{
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b)); memset(tracers,0,sizeof(tracers));
    int shooter=spawn_unit(owner,type,64,48);
    int target=spawn_unit(owner==SIDE?ENEMY:SIDE,U_ZEALOT,112,48);
    assert(shooter>=0 && target>=0);
    int health=u[target].hp+u[target].shield;
    tick_combat(&u[shooter],target);
    assert(u[target].hp+u[target].shield<health);
    assert(tracers[0].life==5);
    assert(sc64_weapon_vfx_style(tracers[0].color)==SC64_WEAPON_VFX_STYLE_NEEDLE);
    assert(sc64_weapon_vfx_color(tracers[0].color)==C_GOLD);
    memset(visible,0xff,sizeof(visible)); camx=camy=0;
    int prior_x=-1;
    for (int life=4;life>=1;--life) {
        Sc64WeaponVfxSegment expected;
        update_tracers(); /* Same firing-update aging used before game_render. */
        assert(tracers[0].life==life);
        assert(sc64_weapon_vfx_segment(64,48,112,48,life,&expected));
        gfx_begin(C_BLACK); draw_tracers();
        assert(gfx_test_pixel(expected.head_x,expected.head_y+12)==C_WHITE);
        assert(gfx_test_pixel(64,60)==C_BLACK); /* No full source-to-target beam. */
        assert(expected.head_x>prior_x); prior_x=expected.head_x;
        unsigned lit=0;
        for (int x=64;x<=112;++x)
            for (int y=58;y<=61;++y) lit+=gfx_test_pixel(x,y)!=C_BLACK;
        assert(lit<=8);
    }
    update_tracers(); assert(!tracers[0].life);
}

int main(void)
{
    gba_init(); game_init();
    race_mode=1; race_human=R_ZERG; race_bots=1;
    race_skirmish_init(); screen=SCREEN_PLAY;
    _Static_assert(sizeof(Tracer)==6*sizeof(int),"visual metadata keeps tracer layout");
    needle_shot(U_HYDRALISK,SIDE);
    needle_shot(U_HYDRALISK,ENEMY);
    needle_shot(U_HUNTER_KILLER,SIDE);
    puts("Hydralisk/Hunter Killer moving needles preserve immediate damage and tracer layout");
    return 0;
}
