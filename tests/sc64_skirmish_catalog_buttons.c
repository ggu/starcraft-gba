/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual game_update input regression for catalog construction and production.
 * Setup grants a bank and, for Protoss, completed prerequisite structures;
 * every tested build, add-on and train operation uses ordinary buttons. */
#define main sc64_catalog_controller_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main
#include <string.h>

static int row_for(unsigned raw)
{
    for (int i=0;i<field_menu_count();++i) {
        Sc64SkirmishMenuRow row;
        if (sc64_skirmish_menu_at((unsigned)race_human,(unsigned)i,&row) &&
            row.record && row.record->raw_id==raw) return i;
    }
    return -1;
}
static int find_raw_building(unsigned raw)
{
    for (int i=0;i<MAX_BUILDINGS;i++)
        if (b[i].alive && b[i].owner==SIDE && sc64_race_raw_building(b[i].type)==(int)raw) return i;
    return -1;
}
static int count_raw_units(unsigned raw)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&sc64_race_raw_unit(u[i].type)==(int)raw) n++;
    return n;
}
static int test_site(int type,int worker,int need_addon,int *outx,int *outy)
{
    for(int dy=-160;dy<=160;dy+=16) for(int dx=-160;dx<=160;dx+=16) {
        int x=race_base_x[0]+dx,y=race_base_y[0]+dy;
        field_ui_site(type,&x,&y);
        int sx=x,sy=y;
        if(!race_build_request_valid(SIDE,worker,type,&sx,&sy,0)) continue;
        if(need_addon && !can_place(sc64_race_building_from_raw(120),sx+48,sy+8)) continue;
        *outx=sx;*outy=sy;return 1;
    }
    return 0;
}
static int build_by_buttons(unsigned raw,int need_addon)
{
    int type=sc64_race_building_from_raw((int)raw),worker=-1,x=0,y=0;
    for(int i=0;i<MAX_UNITS;i++)
        if(build_worker_available(i,SIDE,-1) && sc64_race_raw_unit(u[i].type)==(race_human==R_TER?7:64)) {worker=i;break;}
    assert(worker>=0 && type>=0 && test_site(type,worker,need_addon,&x,&y));
    assert(select_unit(worker));
    int row=row_for(raw); assert(row>=0 && choose_menu_command(row));
    assert(mode==MODE_BUILD && menu==type);
    move_cursor(x,y); assert(abs(cx-x)<=1 && abs(cy-y)<=1);
    tap(KEY_A);
    if (!(mode==MODE_NORMAL && (u[worker].order==ORDER_BUILD_PENDING || u[worker].order==ORDER_BUILD))) fprintf(stderr,"placement fail raw%u mode%d order%d worker%d selected%d cx%d cy%d site%d,%d minerals%d gas%d tick%u\n",raw,mode,u[worker].order,worker,u[worker].selected,cx,cy,x,y,minerals,gas,host_ticks);
    assert(mode==MODE_NORMAL && (u[worker].order==ORDER_BUILD_PENDING || u[worker].order==ORDER_BUILD));
    int slot=-1;
    for(unsigned t=0;t<6000;t++) {
        slot=find_raw_building(raw);
        if(slot>=0 && building_complete(slot)) return slot;
        assert(screen==SCREEN_PLAY && !won && !lost);
        input(0);
    }
    fprintf(stderr,"build timeout raw%u worker%d order%d slot%d tick%u\n",raw,worker,u[worker].order,slot,host_ticks);abort();
}
static void produce_by_buttons(int producer,unsigned raw)
{
    int type=sc64_race_unit_from_raw((int)raw),before=count_raw_units(raw);
    assert(type>=0 && select_building(producer));
    for(int i=0;i<24 && train_type!=type;i++) tap(KEY_R);
    assert(train_type==type);
    int old_minerals=minerals,old_gas=gas;
    tap(KEY_A);
    assert(b[producer].queue_count==1 && b[producer].queue[b[producer].queue_head]==type);
    const Sc64RaceUnitRule *r=sc64_race_unit_rule(type);
    assert(r && minerals==old_minerals-r->minerals && gas==old_gas-r->gas);
    for(unsigned t=0;t<5000 && count_raw_units(raw)==before;t++) {
        assert(screen==SCREEN_PLAY && !won && !lost);input(0);
    }
    assert(count_raw_units(raw)==before+1);
}
int main(int argc,char **argv)
{
    int race=argc>1?atoi(argv[1]):R_TER;
    assert(race==R_TER || race==R_PROTOSS);
    if ((race==R_TER && (sc64_race_building_from_raw(120)<0 || sc64_race_unit_from_raw(5)<0)) ||
        (race==R_PROTOSS && (sc64_race_building_from_raw(167)<0 || sc64_race_unit_from_raw(70)<0))) {
        printf("catalog button chain unavailable: %s identity missing in this profile\n",race==R_TER?"Terran":"Protoss");
        return 77;
    }
    option_map=0;option_human=race;option_bots=1;option_bot_races[0]=(race+1)%3;
    match_limit=30000;gba_init();assert(launch_match());
    /* Isolated synthetic starting state: adequate funds, quiet workers, and
     * Protoss prerequisites only. No actor changes after input testing starts. */
    minerals=gas=5000;
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive&&u[i].owner==SIDE&&sc64_race_is_worker(u[i].type)) u[i].order=ORDER_IDLE;
    if(race==R_PROTOSS) {
        assert(spawn_build(SIDE,B_PYLON,race_base_x[0]+112,race_base_y[0],0)>=0);
        assert(spawn_build(SIDE,B_GATEWAY,race_base_x[0]+160,race_base_y[0]+96,0)>=0);
        assert(spawn_build(SIDE,B_CYBERNETICS_CORE,race_base_x[0]+160,race_base_y[0]-96,0)>=0);
    }
    if(race==R_TER) {
        assert(sc64_race_building_from_raw(120)>=0 && sc64_race_unit_from_raw(5)>=0);
        int barracks=build_by_buttons(111,0);assert(barracks>=0);
        int factory=build_by_buttons(113,1);assert(factory>=0 && select_building(factory));
        int row=row_for(120);assert(row>=0 && choose_menu_command(row));
        int addon=find_raw_building(120);assert(addon>=0 && b[addon].under_construction);
        for(unsigned t=0;t<4000 && !building_complete(addon);t++) input(0);
        assert(building_complete(addon) && race_skirmish_attached(factory,120,1));
        produce_by_buttons(factory,5);
    } else {
        assert(sc64_race_building_from_raw(167)>=0 && sc64_race_unit_from_raw(70)>=0);
        int gate=build_by_buttons(167,0);produce_by_buttons(gate,70);
    }
    printf("catalog button chain %s passed at tick %u\n",race==R_TER?"Terran":"Protoss",host_ticks);
    return 0;
}
