/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Detached candidate proof; no historical guest SRAM is synthesized. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
static RaceSave candidate,snapshot_candidate;
int main(void)
{
    game_init();race_mode=1;race_human=R_TER;race_bots=1;race_skirmish_map=0;
    race_bot_races[0]=R_ZERG;init_state();
    int slot=spawn_unit(SIDE,U_SCV,128,128);assert(slot>=0);
    Unit worker=u[slot];worker.flags=0;worker.carry=8;worker.order=ORDER_GAS;
    assert(sc64_unit_save_encode(&candidate.units[0],&worker,1024,1024));
    const Sc64StageTagPair pairs[]={
        {SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,0},
        {SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION,0}
    };
    candidate.version=SC64_WORKER_CARGO_LEGACY_SAVE_VERSION;
    Sc64StageSaveAuth auth;assert(sc64_stage_save_authenticate(&auth,&candidate,pairs[0].outer,candidate.version,pairs,2));
    snapshot_candidate=candidate;
    assert(race_save_worker_cargo_prepare(&candidate,1024,1024,1,&auth));
    assert(!memcmp(&candidate,&snapshot_candidate,sizeof(candidate)));
    Unit decoded;assert(race_guardian_decode_authenticated(&candidate,0,1024,1024,&decoded,&auth));
    assert(decoded.carry==8 && sc64_worker_cargo_is_gas(&decoded));
    assert(!memcmp(&candidate,&snapshot_candidate,sizeof(candidate)));
    /* Modern feature0 has the same feature mask but must not migrate. */
    candidate.version=SAVE_VERSION;assert(sc64_stage_save_authenticate(&auth,&candidate,pairs[1].outer,candidate.version,pairs,2));
    snapshot_candidate=candidate;assert(race_save_worker_cargo_prepare(&candidate,1024,1024,0,&auth));
    assert(race_guardian_decode_authenticated(&candidate,0,1024,1024,&decoded,&auth));
    assert(!(decoded.flags & SC64_WORKER_CARGO_KIND_MASK));
    assert(!memcmp(&candidate,&snapshot_candidate,sizeof(candidate)));
    /* Invalid old cargo bits fail without changing candidate or output. */
    worker.flags=SC64_WORKER_CARGO_GAS;assert(sc64_unit_save_encode(&candidate.units[0],&worker,1024,1024));
    candidate.version=SC64_WORKER_CARGO_LEGACY_SAVE_VERSION;
    assert(sc64_stage_save_authenticate(&auth,&candidate,pairs[0].outer,candidate.version,pairs,2));
    snapshot_candidate=candidate;Unit sentinel={0};sentinel.hp=123;decoded=sentinel;
    assert(!race_save_worker_cargo_prepare(&candidate,1024,1024,1,&auth));
    assert(!race_guardian_decode_authenticated(&candidate,0,1024,1024,&decoded,&auth));
    assert(!memcmp(&decoded,&sentinel,sizeof(decoded)) && !memcmp(&candidate,&snapshot_candidate,sizeof(candidate)));
    puts("legacy cargo normalized only in authenticated temporary; candidate immutable");return 0;
}
