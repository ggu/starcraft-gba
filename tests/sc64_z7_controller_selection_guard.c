/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Controlled host transitions; this is not a physical player-input witness. */
#define SC64_Z7_GUARD_FIXTURE 1
#define main z7_controller_original_main
#include "sc64_z7_campaign_run.c"
#undef main

static int fixture_kind,fixture_updates,fixture_sensitive_after_change,fixture_inject;
static int fixture_drag;
static void z7_guard_fixture_update(u16 held,u16 pressed)
{
    if(fixture_updates>=fixture_inject&&((held|pressed)&(KEY_A|KEY_R)))
        fixture_sensitive_after_change++;
    fixture_updates++;
    /* Model only the ordinary cursor and empty-box gestures needed here. */
    if(held&(KEY_R|KEY_L)) {
        if(held&KEY_RIGHT)cx+=4;if(held&KEY_LEFT)cx-=4;
        if(held&KEY_DOWN)cy+=4;if(held&KEY_UP)cy-=4;
    }
    if(held&KEY_L)fixture_drag=1;
    else if(fixture_drag){selected_building=-1;fixture_drag=0;}
    if(fixture_updates==fixture_inject) {
        if(fixture_kind==0)screen=SCREEN_RESULT;
        if(fixture_kind==1)b[0].owner=ENEMY;
        if(fixture_kind==2)b[0].raw_type++;
        if(fixture_kind==3)b[0].type++;
        if(fixture_kind==4)b[0].alive=0;
        if(fixture_kind==5)b[0].raw_owner++;
    }
}
int main(void)
{
    trace_file=tmpfile();assert(trace_file);
    static const int injection_points[]={1,3,7};
    for(int k=0;k<6;k++)for(int present=0;present<2;present++)
    for(unsigned n=0;n<sizeof(injection_points)/sizeof(injection_points[0]);n++) {
        memset(b,0,sizeof(b));memset(u,0,sizeof(u));
        screen=SCREEN_PLAY;won=lost=0;menu_open=0;mode=MODE_NORMAL;
        selected_building=present?0:-1;cx=cy=0;prior_keys=0;host_ticks=0;
        b[0].alive=1;b[0].owner=SIDE;b[0].type=B_HATCHERY;
        b[0].raw_type=131;b[0].raw_owner=3;b[0].x=128;b[0].y=128;
        fixture_kind=k;fixture_inject=injection_points[n];fixture_drag=0;
        fixture_updates=fixture_sensitive_after_change=0;
        assert(!select_building_slot(0));
        assert(fixture_updates==fixture_inject&&fixture_sensitive_after_change==0);
        assert(!z7_producer_guard.active);
    }
    for(int k=0;k<6;k++) {
        screen=SCREEN_PLAY;won=lost=0;b[0].alive=1;b[0].owner=SIDE;
        b[0].type=B_HATCHERY;b[0].raw_type=131;b[0].raw_owner=3;
        fixture_kind=k;fixture_inject=1;fixture_drag=0;
        fixture_updates=fixture_sensitive_after_change=0;prior_keys=0;
        assert(z7_guard_begin(0));step(KEY_DOWN);tap(KEY_R);tap(KEY_A);
        assert(fixture_updates==1&&fixture_sensitive_after_change==0);
        z7_guard_end();assert(!z7_producer_guard.active);
    }
    fclose(trace_file);puts("Z7 controlled producer guard transitions PASS");return 0;
}
