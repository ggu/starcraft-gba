/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void observe_hit(int target,int damage,int source_type,int owner);
#define SC64_TEST_HIT_OBSERVER observe_hit
#include "../starcraft/game.c"
static unsigned projectile_hits;
static void observe_hit(int target,int damage,int source_type,int owner)
{
    (void)target; (void)damage; (void)owner;
    if (source_type==U_P7_INTERCEPTOR) ++projectile_hits;
}

static void launch_p7(void)
{
    race_mode=0; race_selected_mission=SC64_P7_GLOBAL_ID;
    init_state();
    assert(screen==SCREEN_PLAY && !mission_unsupported_count);
    assert(sc64_pool_validate(&race_p7_projectiles));
    assert(sc64_pool_active_head(&race_p7_projectiles)==SC64_END);
    assert(race_save_game() && race_load_game());
    projectile_hits=0;
}

static void make_pair(int *shooter,int *target)
{
    *shooter=spawn_unit(SIDE,U_P7_INTERCEPTOR,64,64);
    *target=spawn_unit(ENEMY,U_MARINE,224,64);
    assert(*shooter>=0 && *target>=0);
    u[*target].raw_type=0; u[*target].raw_owner=0;
    assert(u[*shooter].raw_type==73 && u[*shooter].raw_owner==7);
    assert(unit_slot_supply(&u[*shooter])==0);
    assert(!race_p7_projectile_fire(*shooter,*target,64,65*512,64*512));
}

static void test_delayed_impact_and_save(void)
{
    int shooter,target,hp;
    launch_p7(); make_pair(&shooter,&target); hp=u[target].hp;
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    assert(u[target].hp==hp && !projectile_hits);
    assert(race_save_game());
    for (unsigned i=0;i<80 && !projectile_hits;++i) {
        game_update(0,0);
        assert(!race_p7_projectile_fault && screen==SCREEN_PLAY);
        assert(race_save_game() && race_load_game());
    }
    assert(projectile_hits==1 && u[target].hp==hp-6);
    assert(race_save_game() && race_load_game());
    for (unsigned i=0;i<10;++i) game_update(0,0);
    assert(projectile_hits==1 && !race_p7_projectile_fault);
    assert(sc64_pool_active_head(&race_p7_projectiles)==SC64_END);
}

static void run_projectile_ticks(unsigned count)
{
    /* Isolate slot-reuse damage from unrelated campaign armies. The primary
     * continuation test above exercises the complete game_update clock. */
    for (unsigned i=0;i<count;++i) {
        race_p7_projectile_source_tick();
        race_p7_projectile_clear_dead_references();
        assert(screen==SCREEN_PLAY && !race_p7_projectile_fault);
    }
}

static void test_dead_slots_are_not_new_targets(void)
{
    int shooter,target,replacement,hp;
    launch_p7(); make_pair(&shooter,&target);
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    hit_target(target,1000,U_ZEALOT,SIDE);
    assert(!u[target].alive);
    replacement=spawn_unit(ENEMY,U_MARINE,224,64);
    assert(replacement==target);
    u[target].raw_type=0; u[target].raw_owner=0; hp=u[target].hp;
    Sc64ProjectileRow row;
    assert(sc64_pool_get_row(&race_p7_projectiles,
        sc64_pool_active_head(&race_p7_projectiles),&row));
    assert(row.target_ref==511);
    run_projectile_ticks(40);
    assert(!projectile_hits && u[target].hp==hp);

    launch_p7(); make_pair(&shooter,&target); hp=u[target].hp;
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    hit_target(shooter,1000,U_MARINE,ENEMY);
    assert(!u[shooter].alive);
    replacement=spawn_unit(SIDE,U_PROBE,64,64);
    assert(replacement==shooter);
    assert(sc64_pool_get_row(&race_p7_projectiles,
        sc64_pool_active_head(&race_p7_projectiles),&row));
    assert(row.owner_ref==255 && row.raw_source_owner==7);
    assert(race_save_game() && race_load_game());
    run_projectile_ticks(40);
    assert(projectile_hits==1 && u[target].hp==hp-6);

    launch_p7(); make_pair(&shooter,&target);
    int building=spawn_build(ENEMY,B_HATCHERY,224,64,0);
    assert(building>=0); b[building].raw_type=131; b[building].raw_owner=0;
    assert(race_p7_projectile_fire(shooter,MAX_UNITS+building,64,64*512,64*512));
    hit_target(MAX_UNITS+building,10000,U_ZEALOT,SIDE);
    assert(!b[building].alive);
    assert(spawn_build(ENEMY,B_HATCHERY,224,64,0)==building);
    hp=b[building].hp;
    assert(sc64_pool_get_row(&race_p7_projectiles,
        sc64_pool_active_head(&race_p7_projectiles),&row));
    assert(row.target_ref==511);
    run_projectile_ticks(40);
    assert(!projectile_hits && b[building].hp==hp);
}

static void reject_without_live_mutation(const RaceSave *bad)
{
    static Unit before_u[MAX_UNITS];
    static Building before_b[MAX_BUILDINGS];
    static u16 before_pool[SC64_ARENA_WORDS];
    Sc64P7PersistentState before_p7=race_p7;
    int before_frame=frame,before_minerals=minerals,before_gas=gas;
    u32 before_seed=seed;
    memcpy(before_u,u,sizeof(u)); memcpy(before_b,b,sizeof(b));
    memcpy(before_pool,race_p7_projectile_host_words,sizeof(before_pool));
    save_write(bad,sizeof(*bad),SAVE_VERSION);
    assert(!race_load_game());
    assert(memcmp(before_u,u,sizeof(u))==0 && memcmp(before_b,b,sizeof(b))==0);
    assert(memcmp(before_pool,race_p7_projectile_host_words,sizeof(before_pool))==0);
    assert(memcmp(&before_p7,&race_p7,sizeof(before_p7))==0);
    assert(frame==before_frame && minerals==before_minerals && gas==before_gas &&
        seed==before_seed);
}

static void test_save_preflight_and_atomic_rejection(void)
{
    int shooter,target;
    static RaceSave good,bad;
    static u16 scratch_words[SC64_ARENA_WORDS];
    Sc64Arena scratch={scratch_words,SC64_ARENA_WORDS};
    launch_p7(); make_pair(&shooter,&target);
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    assert(race_save_game()); good=race_data;
    for (unsigned test=0;test<9;++test) {
        bad=good;
        if (test<5) {
            Sc64ProjectileRow row;
            assert(sc64_codec_decode(&scratch,bad.interceptor_projectiles,SC64_CODEC_BYTES));
            u8 id=sc64_pool_active_head(&scratch);
            assert(sc64_pool_get_row(&scratch,id,&row));
            if (test==0) { assert(!u[MAX_UNITS-1].alive); row.owner_ref=MAX_UNITS-1; }
            if (test==1) { assert(!b[MAX_BUILDINGS-1].alive); row.target_ref=MAX_UNITS+MAX_BUILDINGS-1; }
            if (test==2) row.pending_damage_event=1;
            if (test==3) { row.state=1; row.script_phase=2; row.remaining_time=-1; }
            if (test==4) row.raw_source_owner=8;
            assert(sc64_pool_set_row(&scratch,id,&row));
            assert(sc64_codec_encode(&scratch,bad.interceptor_projectiles,SC64_CODEC_BYTES));
        } else if (test==5) bad.interceptor_padding=1;
        else if (test==6) bad.interceptor_projectiles[2]|=0x80u;
        else {
            int slot=test==7?shooter:target;
            Unit actor=u[slot]; actor.raw_type=test==7?70:73;
            assert(sc64_p7_typed_unit_encode(&bad.units[slot],&actor,
                (unsigned)(race_cols*16),(unsigned)(race_rows*16)));
        }
        reject_without_live_mutation(&bad);
        save_write(&good,sizeof(good),SAVE_VERSION);
        assert(race_load_game());
    }
}

static void test_candidate_rejects_bad_projectile_target(void)
{
    static Unit before_u[MAX_UNITS];
    static Building before_b[MAX_BUILDINGS];
    static u16 before_pool[SC64_ARENA_WORDS];
    RaceLoadPlan plan={0};
    launch_p7();assert(race_save_game());
    assert(sc64_codec_validate(race_data.interceptor_projectiles,SC64_CODEC_BYTES));
    assert(race_load_preflight(&plan));
    memcpy(before_u,u,sizeof(u));memcpy(before_b,b,sizeof(b));
    memcpy(before_pool,race_p7_projectile_host_words,sizeof(before_pool));
    Sc64P7PersistentState before_p7=race_p7;
    int before_frame=frame,before_minerals=minerals,before_gas=gas;
    size_t old_count=race_p7_projectiles.word_count;
    race_p7_projectiles.word_count=SC64_ARENA_WORDS-1;
    assert(sc64_codec_validate(race_data.interceptor_projectiles,SC64_CODEC_BYTES));
    assert(!race_load_preflight(&plan));
    assert(!memcmp(before_u,u,sizeof(u)) && !memcmp(before_b,b,sizeof(b)));
    assert(!memcmp(before_pool,race_p7_projectile_host_words,sizeof(before_pool)));
    assert(!memcmp(&before_p7,&race_p7,sizeof(before_p7)));
    assert(frame==before_frame && minerals==before_minerals && gas==before_gas);
    race_p7_projectiles.word_count=old_count;
    assert(race_load_preflight(&plan));
}

static void test_pause_and_profile_matrix(void)
{
    int shooter,target;
    u8 before[SC64_CODEC_BYTES],after[SC64_CODEC_BYTES];
    launch_p7(); make_pair(&shooter,&target);
    assert(race_p7_projectile_fire(shooter,target,64,64*512,64*512));
    assert(race_p7_projectile_encode(before));
    game_update(KEY_START,KEY_START); assert(screen==SCREEN_PAUSE);
    u8 phase=race_p7.reserved[0]; int paused_frame=frame;
    for (unsigned i=0;i<20;++i) game_update(0,0);
    assert(race_p7_projectile_encode(after));
    assert(memcmp(before,after,sizeof(before))==0 && phase==race_p7.reserved[0] &&
        paused_frame==frame);
    /* Every map/race/bot choice must still initialize and save/load with the
     * new generation's canonical empty projectile packet. */
    for (int map=0;map<2;++map) for (int human=0;human<3;++human)
        for (int bots=1;bots<=3;++bots) {
            race_mode=1; race_human=human; race_bots=bots; race_skirmish_map=map;
            race_skirmish_init();
            assert(race_save_game() && race_load_game());
            assert(race_mode && race_human==human && race_bots==bots && race_skirmish_map==map);
            assert(sc64_pool_active_head(&race_p7_projectiles)==SC64_END);
        }
}

int main(void)
{
    game_init();
    assert(SC64_RACE_SAVE_GENERATION==41u && sizeof(RaceSave)==16292u);
    assert(sizeof(Unit)==56u && sizeof(Building)==48u);
    test_delayed_impact_and_save();
    test_dead_slots_are_not_new_targets();
    test_save_preflight_and_atomic_rejection();
    test_candidate_rejects_bad_projectile_target();
    test_pause_and_profile_matrix();
    puts("PASS P7 delayed impact/save, unit/building reuse, dead shooter, atomic rejection, pause and 18 skirmish setups");
    return 0;
}
