/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercise the product journal selector with a checksummed malformed queue. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
void guardian_test_reset(void);
void guardian_test_corrupt(int slot);
void guardian_test_snapshot(void *dst);
static int accept_checksum_valid(const void *raw, u32 len, u32 tag, void *ctx)
{ (void)raw; (void)len; (void)tag; (void)ctx; return 1; }

int main(void)
{
    guardian_test_reset(); game_init(); race_mode=1; race_human=R_ZERG;
    race_bots=1; race_skirmish_map=1; race_bot_races[0]=R_TER; init_state();
    int site=spawn_build(SIDE,B_EXTRACTOR,sc64_gas_sites[0].x,sc64_gas_sites[0].y,0);
    assert(site>=0);
    int a=spawn_unit(SIDE,U_DRONE,b[site].x,b[site].y);
    int bslot=spawn_unit(SIDE,U_DRONE,b[site].x,b[site].y);
    assert(a>=0 && bslot>=0 && a!=bslot);
    u[a].target=u[bslot].target=site; u[a].order=u[bslot].order=ORDER_GAS;
    sc64_gas_service_begin(); race_update_resource_worker(&u[a]); race_update_resource_worker(&u[bslot]);
    assert(gas_service.head[site]==a+1 && gas_service.next[a]==bslot+1);
    assert(race_save_game());
    RaceLoadPlan plan={0}; GbaSaveSelection valid,chosen;
    assert(race_worker_cargo_select(&plan,&valid));
    assert(plan.gas_queue_tagged && race_gas_service_candidate.head[site]==a+1);
    /* Candidate selection normalizes carries. Rebuild the actual wire rows,
     * then duplicate rank 1; the normal product writer recomputes its checksum. */
    Sc64GasSaveContext ctx={&race_data,WORLD_W,WORLD_H};
    assert(sc64_gas_queue_save_encode(&ctx,&race_gas_service_candidate,
                                      race_gas_save_read,race_gas_save_write,race_gas_save_eligible));
    Unit bad; assert(race_save_decode_unit(&race_data,bslot,WORLD_W,WORLD_H,&bad));
    bad.carry=1<<SC64_GAS_QUEUE_CARRY_BITS;
    assert(race_save_encode_unit(&race_data,bslot,WORLD_W,WORLD_H,&bad));
    assert(race_impact_save_write(&valid));
    /* Verify independently that the newer, malformed product envelope has a
     * valid journal checksum and is the newest selectable bank. */
    static u8 checksum_scratch[RACE_IMPACT_RECORD_MAX];
    GbaSaveSelection checksum_newest;
    assert(save_select_prepared(checksum_scratch,sizeof checksum_scratch,
           race_zc_tags,sizeof race_zc_tags/sizeof race_zc_tags[0],
           accept_checksum_valid,0,&checksum_newest));
    assert(checksum_newest.slot!=valid.slot && checksum_newest.sequence>valid.sequence);
    memset(&plan,0,sizeof plan); assert(race_worker_cargo_select(&plan,&chosen));
    assert(chosen.slot==valid.slot && chosen.sequence==valid.sequence);
    assert(race_load_game());
    assert(gas_service.head[site]==a+1 && gas_service.next[a]==bslot+1);
    /* With only the malformed bank checksum-valid, selection must fail with
     * no changes to live units, buildings, queue, resources, or SRAM bytes. */
    guardian_test_corrupt(valid.slot);
    static Unit units_before[MAX_UNITS]; static Building buildings_before[MAX_BUILDINGS];
    static u8 sram_before[32768],sram_after[32768];
    memcpy(units_before,u,sizeof u); memcpy(buildings_before,b,sizeof b);
    Sc64GasServiceQueue queue_before=gas_service; int ore_before=minerals,gas_before=gas;
    int screen_before=screen, frame_before=frame, mission_before=race_selected_mission;
    unsigned seed_before=seed;
    guardian_test_snapshot(sram_before); assert(!race_load_game()); guardian_test_snapshot(sram_after);
    assert(!memcmp(units_before,u,sizeof u)); assert(!memcmp(buildings_before,b,sizeof b));
    assert(!memcmp(&queue_before,&gas_service,sizeof gas_service));
    assert(minerals==ore_before && gas==gas_before && screen==screen_before &&
           frame==frame_before && race_selected_mission==mission_before && seed==seed_before &&
           !memcmp(sram_before,sram_after,sizeof sram_before));
    puts("gas queue product journal: checksummed bad rank falls back and cannot publish");
    return 0;
}
