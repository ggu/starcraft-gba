/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only predecessor journals; never seed guest SRAM. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef SC64_COMPAT_GAME_SOURCE
#define SC64_COMPAT_GAME_SOURCE "../starcraft/game.c"
#endif
#include SC64_COMPAT_GAME_SOURCE
extern int sc64_test_sram_transfer(const char *,int);
extern void guardian_test_retag(int,u32,u32);
extern void guardian_test_snapshot(void *);
#ifdef SC64_COMPAT_WRITER
static void press(unsigned key) { game_update((u16)key,(u16)key);game_update(0,0); }
static void write_predecessor(const char *path,int mission)
{
    game_init();int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,mission);
    assert((ordinal==5 || ordinal==6
#ifdef SC64_Z9_RUNTIME
            || ordinal==7 || ordinal==8
#endif
           ) && race_front.page==SC_FRONT_HOME);press(KEY_A);
    for(int i=0;i<ordinal/2;i++)press(KEY_RIGHT);
    if(ordinal&1)press(KEY_DOWN);press(KEY_A);assert(race_front.page==SC_FRONT_BRIEFING);press(KEY_START);
    assert(screen==SCREEN_PLAY && !race_mode && race_selected_mission==mission);
    if (mission==SC64_Z7_GLOBAL_ID) {
        int hatch=spawn_build(SIDE,B_HATCHERY,420,420,0);assert(hatch>=0);
        b[hatch].raw_owner=3;b[hatch].raw_type=131;b[hatch].larva=3;
        minerals=race_minerals[SIDE]=1000;gas=race_gas[SIDE]=1000;
        assert(race_queue_unit(SIDE,hatch,U_DRONE));
    }
    assert(race_save_game() && sc64_test_sram_transfer(path,1));
    printf("HOST predecessor mission=%d inner=%08x outer=%08x\n",mission,(unsigned)SAVE_VERSION,(unsigned)SC64_TRANSPORT_SAVE_VERSION);
}
#else
static int find_egg(void)
{
    for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].type==U_Z7_EGG &&
        u[i].owner==SIDE && u[i].target==U_DRONE && u[i].raw_type==36 && u[i].raw_owner==3) return i;
    return -1;
}
static void verify_loaded(int mission)
{
    assert(!race_mode && race_selected_mission==mission && screen==SCREEN_PLAY);
    if(mission==SC64_Z7_GLOBAL_ID) {int egg=find_egg();assert(egg>=0 && u[egg].work_timer==sc64_z7_egg_duration(U_DRONE));}
}
static void read_predecessor(const char *path,int mission)
{
    static unsigned char before[32768],after[32768];
    static Unit original_units[MAX_UNITS];static Building original_buildings[MAX_BUILDINGS];
    game_init();assert(sc64_test_sram_transfer(path,0));
#if defined(SC64_SHARED_EGG_CUSTOM_SERVICE) || defined(SC64_SHARED_EGG_LATE_SERVICE)
    RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION &&
           plan.egg_auth.inner==SC64_WORKER_CARGO_LEGACY_SAVE_VERSION &&
           plan.egg_auth.candidate==&race_data && plan.egg_auth.features==0);
    /* A token from another candidate, or one whose bound inner version no
     * longer agrees, cannot authorize even a detached row decode. */
    Sc64StageSaveAuth wrong=plan.egg_auth;Unit decoded;
    unsigned width=(unsigned)sc64_race_maps[mission].width*16u;
    unsigned height=(unsigned)sc64_race_maps[mission].height*16u;
    wrong.candidate=&plan;
    assert(!race_save_decode_unit_authenticated(&race_data,0,width,height,&decoded,&wrong));
    wrong=plan.egg_auth;wrong.inner^=1u;
    assert(!race_save_decode_unit_authenticated(&race_data,0,width,height,&decoded,&wrong));
    /* A checksum-valid mismatched outer/inner pair cannot load or mutate SRAM
     * or the live unit/building world. This mutation is HOST fixture only. */
    guardian_test_retag(selected.slot,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION);
    guardian_test_snapshot(before);memcpy(original_units,u,sizeof(u));memcpy(original_buildings,b,sizeof(b));
    assert(!race_load_game());guardian_test_snapshot(after);
    assert(!memcmp(before,after,sizeof(before)) && !memcmp(original_units,u,sizeof(u)) && !memcmp(original_buildings,b,sizeof(b)));
    assert(sc64_test_sram_transfer(path,0));
#endif
    assert(race_load_game());verify_loaded(mission);
#if defined(SC64_SHARED_EGG_CUSTOM_SERVICE) || defined(SC64_SHARED_EGG_LATE_SERVICE)
    assert(race_worker_cargo_select(&plan,&selected));
    assert(plan.egg_auth.inner==SC64_WORKER_CARGO_LEGACY_SAVE_VERSION && race_data.version==plan.egg_auth.inner);
    /* Save a newer current candidate then make only its tag pair invalid.
     * Selection must fall back to the unchanged older pre-cargo slot. */
    assert(race_save_game());assert(race_worker_cargo_select(&plan,&selected));
#ifdef SC64_SHARED_EGG_LATE_SERVICE
    if (mission==SC64_Z8_GLOBAL_ID || mission==SC64_Z9_GLOBAL_ID) {
        assert(selected.tag==SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION);
        guardian_test_retag(selected.slot,SC64_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_SAVE_VERSION);
    } else
#endif
    {
        assert(selected.tag==SC64_TRANSPORT_SAVE_VERSION);
        guardian_test_retag(selected.slot,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    }
    guardian_test_snapshot(before);assert(race_load_game());verify_loaded(mission);guardian_test_snapshot(after);
    assert(!memcmp(before,after,sizeof(before)));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION &&
           plan.egg_auth.inner==SC64_WORKER_CARGO_LEGACY_SAVE_VERSION &&
           plan.egg_auth.candidate==&race_data && race_data.version==plan.egg_auth.inner);
#endif
    printf("HOST pre-cargo mission %d normal restore%s PASS\n",mission,
#if defined(SC64_SHARED_EGG_CUSTOM_SERVICE) || defined(SC64_SHARED_EGG_LATE_SERVICE)
           ", mismatched-tag rejection and older-slot fallback"
#else
           ""
#endif
    );
}
#endif
int main(int argc,char **argv)
{
    assert(argc==3);int n=atoi(argv[2]);assert(n==6 || n==7
#ifdef SC64_Z9_RUNTIME
        || n==8 || n==9
#endif
    );
    int mission=n==6?SC64_Z6_GLOBAL_ID:SC64_Z7_GLOBAL_ID;
#ifdef SC64_Z9_RUNTIME
    if(n==8) mission=SC64_Z8_GLOBAL_ID;
    if(n==9) mission=SC64_Z9_GLOBAL_ID;
#endif
#ifdef SC64_COMPAT_WRITER
    write_predecessor(argv[1],mission);
#else
    read_predecessor(argv[1],mission);
#endif
    return 0;
}
