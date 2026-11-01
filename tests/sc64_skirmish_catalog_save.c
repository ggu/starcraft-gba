/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercise the integrated catalog through the profile-selected SRAM codec. */
#define main skirmish_runtime_fixture_main
#include "sc64_skirmish_runtime.c"
#undef main

/* Qualified canonical profile tags, intentionally literal to detect drift. */
#if defined(SC64_SHOWCASE_ENABLED)
#define EXPECTED_BASE_SAVE_TAG 0x53435058u
#elif SC64_CAMPAIGN_RACE == 0
#define EXPECTED_BASE_SAVE_TAG 0x53435442u
#elif SC64_CAMPAIGN_RACE == 1
#define EXPECTED_BASE_SAVE_TAG 0x53435a50u
#elif SC64_CAMPAIGN_RACE == 2
#define EXPECTED_BASE_SAVE_TAG 0x5343505bu
#else
#error "Expected a canonical save profile"
#endif
#ifdef SC64_SKIRMISH_ROSTER
_Static_assert(SAVE_VERSION == (EXPECTED_BASE_SAVE_TAG | 0x80000000u), "roster save identity drift");
#else
_Static_assert(SAVE_VERSION == EXPECTED_BASE_SAVE_TAG, "default save identity drift");
#endif

static void save_start_race(int race)
{
    game_init(); race_mode=1; race_human=race; race_bots=1; init_state();
    assert(screen==SCREEN_PLAY && race_mode && race_player[SIDE]==race);
    minerals=gas=5000;
}

static void roundtrip(void)
{
    assert(race_save_game());
    assert(race_load_game());
}

static void worker_construction(int race, int worker_raw, int raw)
{
    save_start_race(race);
    int worker = find_worker_raw(SIDE, worker_raw), type = building_raw(raw);
    if (worker < 0 || type < 0) { printf("SKIP construction race=%d raw=%d\n",race,raw); return; }
    int x=0,y=0;
    assert(race_skirmish_site_for(SIDE,type,&x,&y));
    u[worker].x=x-52; u[worker].y=y;
    assert(race_start_build(SIDE,worker,type,x,y));
    assert(u[worker].order==ORDER_BUILD_PENDING);
    roundtrip();
    assert(u[worker].order==ORDER_BUILD_PENDING && u[worker].target==type && u[worker].tx==x && u[worker].ty==y);
    u[worker].x=x;u[worker].y=y;race_pending_build_tick(worker);
    int slot=-1;
    for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive && b[i].owner==SIDE && b[i].type==type && b[i].under_construction && b[i].x==x && b[i].y==y) { assert(slot<0);slot=i; }
    assert(slot>=0);int progress=b[slot].build_progress;
    roundtrip();
    assert(b[slot].alive && b[slot].type==type && b[slot].under_construction && b[slot].build_progress==progress);
    if(race==R_TER) assert(u[worker].alive && u[worker].order==ORDER_BUILD && u[worker].target==slot);
    if(race==R_ZERG) assert(!u[worker].alive);
    if(race==R_PROTOSS) assert(u[worker].alive && u[worker].order==ORDER_IDLE);
    printf("construction save race=%d raw=%d PASS\n",race,raw);
}

static void concurrent_foundations(void)
{
    save_start_race(R_TER);
    int type=building_raw(109), workers[2]={-1,-1}, slots[2]={-1,-1};
    assert(type>=0);
    for(int i=0,n=0;i<MAX_UNITS && n<2;i++)
        if(u[i].alive && u[i].owner==SIDE && sc64_race_raw_unit(u[i].type)==7) workers[n++]=i;
    assert(workers[0]>=0 && workers[1]>=0);
    for(int k=0;k<2;k++) {
        int found=0;
        for(int y=96;y<WORLD_H-64 && !found;y+=24)
            for(int x=96;x<WORLD_W-64 && !found;x+=24) {
                int sx=x,sy=y;
                if(!race_build_request_valid(SIDE,workers[k],type,&sx,&sy,0)) continue;
                u[workers[k]].x=sx-52;u[workers[k]].y=sy;
                if(!race_start_build(SIDE,workers[k],type,sx,sy)) continue;
                u[workers[k]].x=sx;u[workers[k]].y=sy;race_pending_build_tick(workers[k]);
                for(int n=0;n<MAX_BUILDINGS;n++)
                    if(b[n].alive && b[n].under_construction && b[n].type==type && b[n].x==sx && b[n].y==sy) slots[k]=n;
                found=slots[k]>=0;
            }
        assert(found && u[workers[k]].order==ORDER_BUILD && u[workers[k]].target==slots[k]);
    }
    assert(slots[0]!=slots[1]);
    roundtrip();
    for(int k=0;k<2;k++) assert(b[slots[k]].alive && b[slots[k]].under_construction && u[workers[k]].order==ORDER_BUILD && u[workers[k]].target==slots[k]);
    puts("concurrent foundations and worker associations PASS");
}

static void addon_queue(void)
{
    save_start_race(R_TER);
    if(building_raw(113)<0 || building_raw(120)<0 || unit_raw(5)<0) { puts("SKIP addon");return; }
    int a=fixture_building_raw(SIDE,113,300,300),b2=fixture_building_raw(SIDE,113,500,300);
    assert(a>=0 && b2>=0);
    int addon=fixture_building_raw(SIDE,120,b[a].x+48,b[a].y+8);
    assert(addon>=0 && race_skirmish_attached(a,120,1) && !race_skirmish_attached(b2,120,1));
    assert(race_queue_unit(SIDE,a,unit_raw(5)));
    int product=b[a].queue[b[a].queue_head],head=b[a].queue_head;
    roundtrip();
    assert(race_skirmish_attached(a,120,1) && !race_skirmish_attached(b2,120,1));
    assert(b[a].queue_count==1 && b[a].queue_head==head && b[a].queue[head]==product);
    puts("addon association and queued product PASS");
}

static RaceSave invalid_save;
static Unit units_before[MAX_UNITS];
static Building buildings_before[MAX_BUILDINGS];

static void malformed_building_rejected(void)
{
    save_start_race(R_TER);
    int slot=find_building_raw(SIDE,106);
    assert(slot>=0 && race_save_game());
    invalid_save=race_data;
#if defined(SC64_P7_TYPED_COMPACT_ENABLED)
    invalid_save.buildings[slot].bytes[24] |= 0x80;
#elif defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    invalid_save.buildings[slot].bytes[27] |= 0x80;
#else
#error "This fixture requires a canonical compact building profile"
#endif
    save_write(&invalid_save,sizeof(invalid_save),SAVE_VERSION);
    memcpy(units_before,u,sizeof(u)); memcpy(buildings_before,b,sizeof(b));
    int old_minerals=minerals,old_gas=gas;
    assert(!race_load_game());
    assert(!memcmp(units_before,u,sizeof(u)) && !memcmp(buildings_before,b,sizeof(b)));
    assert(minerals==old_minerals && gas==old_gas);
    puts("malformed building save rejected atomically PASS");
}

static void foreign_save_rejected(void)
{
    save_start_race(R_TER);
    assert(race_save_game());
    invalid_save=race_data;
    memcpy(units_before,u,sizeof(u)); memcpy(buildings_before,b,sizeof(b));
    int old_minerals=minerals,old_gas=gas;
    /* A save made in the opposite roster mode has a different SRAM tag. */
    save_write(&invalid_save,sizeof(invalid_save),SAVE_VERSION ^ 0x80000000u);
    save_write(&invalid_save,sizeof(invalid_save),SAVE_VERSION ^ 0x80000000u);
    assert(!race_load_game());
    assert(!memcmp(units_before,u,sizeof(u)) && !memcmp(buildings_before,b,sizeof(b)));
    assert(minerals==old_minerals && gas==old_gas);
    /* A matching outer tag cannot hide a mismatched embedded version. */
    invalid_save.version=SAVE_VERSION ^ 0x80000000u;
    save_write(&invalid_save,sizeof(invalid_save),SAVE_VERSION);
    save_write(&invalid_save,sizeof(invalid_save),SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(units_before,u,sizeof(u)) && !memcmp(buildings_before,b,sizeof(b)));
    assert(minerals==old_minerals && gas==old_gas);
    printf("save identity %08x rejects opposite roster mode atomically PASS\n",(unsigned)SAVE_VERSION);
}

static void morphs(void)
{
    const int before_raw[]={131,132,141}, after_raw[]={132,133,137};
    for(int j=0;j<3;j++) {
        save_start_race(R_ZERG);
        if(building_raw(before_raw[j])<0 || building_raw(after_raw[j])<0) { printf("SKIP morph raw=%d\n",after_raw[j]);continue; }
        int slot=fixture_building_raw(SIDE,before_raw[j],300,300);
        assert(slot>=0);
        /* Fixtures satisfy the real prerequisite checks, then use the actual morph command. */
        assert(fixture_building_raw(SIDE,142,400,300)>=0);
        assert(fixture_building_raw(SIDE,138,460,300)>=0);
        assert(fixture_building_raw(SIDE,133,520,300)>=0);
        b[slot].larva=2;b[slot].larva_timer=211;
        selected_building=slot;
        const Sc64SkirmishRecord *row=build_record(R_ZERG,after_raw[j]);
        assert(row && race_skirmish_choose_record(row));
        assert(b[slot].under_construction && b[slot].type==building_raw(after_raw[j]));
        int cap=race_owner_supply_cap(SIDE),larva=b[slot].larva,timer=b[slot].larva_timer,progress=b[slot].build_progress;
        roundtrip();
        assert(b[slot].under_construction && b[slot].type==building_raw(after_raw[j]) && b[slot].build_progress==progress);
        assert(b[slot].larva==larva && b[slot].larva_timer==timer && race_owner_supply_cap(SIDE)==cap);
        printf("morph save raw=%d PASS\n",after_raw[j]);
    }
}

int main(void)
{
    setbuf(stdout, NULL); gba_init();
    worker_construction(R_TER,7,109);
    worker_construction(R_ZERG,41,142);
    worker_construction(R_PROTOSS,64,156);
    concurrent_foundations();addon_queue();morphs();malformed_building_rejected();foreign_save_rejected();
    return 0;
}
