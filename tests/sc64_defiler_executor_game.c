/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_DEFILER_EXECUTOR
#error "requires disabled executor"
#endif
static unsigned test_phase;
static void test_field_remaining_set(int i,unsigned remaining)
{ Sc64DarkSwarmEffect f=sc64_defiler_field_get(i); f.remaining=(u16)remaining;sc64_defiler_field_set(i,f); }
static void test_field_position_set(int i,int x,int y)
{ Sc64DarkSwarmEffect f=sc64_defiler_field_get(i); f.x=x;f.y=y;sc64_defiler_field_set(i,f); }
static int fields(void) {int n=0;for(int i=0;i<MAX_UNITS;i++)n+=(sc64_defiler_field_get(i).remaining>0 && sc64_defiler_field_get(i).remaining<=900);return n;}
static void click_cast(int caster,int x,int y)
{
    screen=SCREEN_PLAY;race_mode=1;selected_building=-1;u[caster].selected=1;
    SC64_DEFILER_ACTOR(caster).energy_q8=100u*256u;
    game_update(KEY_B,KEY_B);assert(menu_open);
    for(int n=0;n<64 && menu_cursor!=field_menu_count_base();n++) {game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);}
    assert(menu_cursor==field_menu_count_base());game_update(0,0);game_update(KEY_A,KEY_A);
    assert(mode==MODE_DARK_SWARM_STAGE);cx=x;cy=y;game_update(0,0);
    SC64_DEFILER_ACTOR(caster).heading=104;SC64_DEFILER_ACTOR(caster).energy_q8=100u*256u;
    frame=test_phase; /* Exercise each position in the GBA/source cadence. */
    game_update(KEY_A,KEY_A);assert(mode==MODE_NORMAL);
}
int main(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    int c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);
    click_cast(c,400,320);
    int debit=-1,bullet=-1,created=-1,retired=-1;unsigned before=frame;
    for(int n=0;n<30 && retired<0;n++) {
        game_update(0,0);
        if(debit<0 && SC64_DEFILER_ACTOR(c).energy_q8<100u*256u)debit=(int)(frame-before);
        if(bullet<0 && sc64_shared_projectiles.active)bullet=(int)(frame-before);
        if(created<0 && fields())created=(int)(frame-before);
        if(bullet>=0 && !sc64_shared_projectiles.active)retired=(int)(frame-before);
    }
    fprintf(stderr,"debit=%d bullet=%d field=%d retire=%d\n",debit,bullet,created,retired);
    assert(debit>=0 && bullet>debit && created>bullet && retired>created && fields()==1);
    int f=-1;for(int i=0;i<MAX_UNITS;i++)if(sc64_defiler_field_get(i).remaining)f=i;
    assert(f>=0 && !u[f].alive && !SC64_UNIT_SLOT_FREE(f));
#ifdef SC64_SHARED_DEFILER_ART_PROBE
    /* Exercise actual field drawing and HUD/world clipping on an initialized
     * graphics surface; private extracted pixels remain outside git. */
    camx=camy=0;memset(visible,0xff,sizeof(visible));
    test_field_remaining_set(f,880);
    const int art_x[5]={200,0,478,200,200},art_y[5]={110,110,110,0,214};
    for(int k=0;k<5;k++) {
        test_field_position_set(f,art_x[k],art_y[k]);
        gfx_begin(C_PANEL);draw_tracers();int painted=0;
        for(int y=0;y<160;y++)for(int x=0;x<240;x++) {
            int changed=gfx_test_pixel(x,y)!=C_PANEL;
            if(y<12 || y>=120)assert(!changed);
            painted+=changed;
        }
        assert(painted>0);
    }
#endif
    assert(!race_save_game() && !race_load_game());
    u[c].alive=0;assert(!race_save_game() && !race_load_game());
    unsigned remaining=sc64_defiler_field_get(f).remaining;
    for(int n=0;n<1300 && sc64_defiler_field_get(f).remaining<=900;n++)game_update(0,0);
    assert(remaining && !fields() && sc64_defiler_field_get(f).remaining==904 && !SC64_UNIT_SLOT_FREE(f));
#ifdef SC64_SHARED_DEFILER_ART_PROBE
    test_field_position_set(f,200,110);
    for(unsigned tail=904;tail>=901;tail--) {
        test_field_remaining_set(f,tail);
        gfx_begin(C_PANEL);draw_tracers();int painted=0;
        for(int y=12;y<120;y++)for(int x=0;x<240;x++)painted+=gfx_test_pixel(x,y)!=C_PANEL;
        assert(painted>0);
    }
    test_field_remaining_set(f,904);
#endif
    for(int n=0;n<30 && sc64_defiler_field_get(f).remaining;n++)game_update(0,0);
    assert(!sc64_defiler_field_get(f).remaining && SC64_UNIT_SLOT_FREE(f));
    /* Full bullet pool still spends the spell energy; it produces no field. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);
    for(int i=0;i<100;i++)assert(sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_INTERCEPTOR,(unsigned)i));
    click_cast(c,400,320);
    for(int n=0;n<30;n++)game_update(0,0);
    assert(SC64_DEFILER_ACTOR(c).energy_q8<100u*256u && !fields() && sc64_shared_projectiles.active==100);
    /* A released spell outlives its caster. Fill the actual unit pool before
     * impact; failed field allocation cannot refund the already spent energy. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);click_cast(c,400,320);
    for(int n=0;n<30 && !sc64_shared_projectiles.active;n++)game_update(0,0);
    assert(sc64_shared_projectiles.active==1);
    while(find_unit_slot()>=0)assert(spawn_unit(SIDE,U_ZERGLING,330,320)>=0);
    u[c].alive=0; /* use the freed actor slot as an ordinary actor allocation */
    assert(spawn_unit(SIDE,U_ZERGLING,330,320)==c);
    for(int n=0;n<20;n++)game_update(0,0);
    assert(!fields() && !sc64_shared_projectiles.active);
#ifdef SC64_SHARED_ACTOR_STATE_UNION_PROBE
    /* The released projectile can reuse its dead caster's slot as a field. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);click_cast(c,400,320);
    for(int n=0;n<30 && !sc64_shared_projectiles.active;n++)game_update(0,0);
    assert(sc64_shared_projectiles.active==1);
    while(find_unit_slot()>=0)assert(spawn_unit(SIDE,U_ZERGLING,330,320)>=0);
    assert(SC64_ACTOR_IS_DEFILER(c));u[c].alive=0;
    for(int n=0;n<30 && !fields();n++)game_update(0,0);
    assert(fields()==1 && sc64_defiler_field_get(c).remaining && !SC64_ACTOR_IS_DEFILER(c));
    test_field_remaining_set(c,1);
    for(int n=0;n<30 && sc64_defiler_field_get(c).remaining;n++)game_update(0,0);
    assert(SC64_UNIT_SLOT_FREE(c));
    assert(spawn_unit(SIDE,U_ZERGLING,330,320)==c && !SC64_ACTOR_IS_DEFILER(c));
#endif
    /* Caster loss after release does not erase the independent projectile. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);click_cast(c,400,320);
    for(int n=0;n<30 && !sc64_shared_projectiles.active;n++)game_update(0,0);
    assert(sc64_shared_projectiles.active==1);u[c].alive=0;
    assert(!race_save_game() && !race_load_game());
    for(int n=0;n<20 && !fields();n++)game_update(0,0);
    assert(fields()==1 && !race_save_game() && !race_load_game());
    /* Source directional oracle starts heading 104. Compare source-tick
     * transition samples, not the 60 Hz game-update counts. */
    const int dx[5]={80,0,-80,0,56},dy[5]={0,80,0,-80,56};
    const int expected[5]={2,2,4,5,1};
    for(int phase=0;phase<5;phase++) for(int k=0;k<5;k++) {
        test_phase=(unsigned)phase;
        game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
        c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);assert(c>=0);
        click_cast(c,320+dx[k],320+dy[k]);
        int t=sc64_battlecruiser_source_tick_due()?1:0;
        int d=SC64_DEFILER_ACTOR(c).phase?t:-1,b=-1,f=-1,r=-1;
        while(t<30 && r<0) {
            game_update(0,0);if(!sc64_battlecruiser_source_tick_due())continue;++t;
            if(d<0 && SC64_DEFILER_ACTOR(c).phase)d=t;
            if(b<0 && sc64_shared_projectiles.active)b=t;
            if(f<0 && fields())f=t;
            if(b>=0 && !sc64_shared_projectiles.active)r=t;
        }
        fprintf(stderr,"phase=%d dir=%d debit=%d bullet=%d field=%d retire=%d\n",phase,k,d,b,f,r);
        assert(d==expected[k] && b==d+3 && f==b+5 && r==f+1);
    }
    /* Controlled same-tick interleaving: a second already-started cast is
     * due to release as the first real UI-fired bullet finishes its script.
     * Seed only the second cast phase to isolate scheduler ordering. */
    test_phase=0;game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
    c=spawn_unit(SIDE,U_SHARED_DEFILER,320,320);
    int c2=spawn_unit(SIDE,U_SHARED_DEFILER,340,320);assert(c>=0 && c2>=0);
    click_cast(c,400,320);
    unsigned projectile_slot=100;
    for(int n=0;n<30 && projectile_slot==100;n++) {
        game_update(0,0);
        for(unsigned j=0;j<100;j++)if(sc64_defiler_projectile_at(j))projectile_slot=j;
    }
    assert(projectile_slot<100);
    while(sc64_defiler_projectile_at(projectile_slot)->age<5)game_update(0,0);
    while(sc64_shared_projectiles.active<100)
        assert(sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_INTERCEPTOR,0));
    SC64_DEFILER_ACTOR(c2).intent=1;SC64_DEFILER_ACTOR(c2).phase=1;
    SC64_DEFILER_ACTOR(c2).age=2;SC64_DEFILER_ACTOR(c2).target_x=400;SC64_DEFILER_ACTOR(c2).target_y=320;
    do {game_update(0,0);} while(!sc64_battlecruiser_source_tick_due());
    assert(SC64_DEFILER_ACTOR(c2).phase==2 && sc64_shared_projectiles.active==99);
    for(unsigned j=0;j<100;j++)assert(!sc64_defiler_projectile_at(j));
    puts("experimental actual UI cast, directional timing, contention, and timed reserved field passed");return 0;
}
