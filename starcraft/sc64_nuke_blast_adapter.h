/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_BLAST_ADAPTER_H
#define SC64_NUKE_BLAST_ADAPTER_H
#if !defined(SC64_RACES) || !defined(SC64_SIEGE_SPLASH)
#error "Nuclear impact uses the shared stock rectangle and explosive damage adapters"
#endif
#include "sc64_nuke_rules.h"

static void
race_nuke_apply_blast(int x, int y, int owner, int missile_slot)
{
    if ((unsigned)owner >= 5 || x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H)
        return;
    for (int target = 0; target < MAX_UNITS + MAX_BUILDINGS; ++target) {
        int raw, center_x, center_y, max_hp, buried = 0, shield_loss, hp_loss;
        int *shield;
        if (target == missile_slot || !sc64_siege_target_alive(target) ||
            sc64_siege_hidden_or_immune(target))
            continue;
        if (target < MAX_UNITS) {
            Unit *q = &u[target];
            if (SC64_TRANSPORT_LOADED(q) || SC64_SCARAB_HIDDEN(q) ||
                SC64_CARRIER_STATE_HIDDEN(q))
                continue;
            raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type);
            center_x = q->x; center_y = q->y;
            max_hp = q->maxhp; shield = &q->shield;
            buried = sc64_siege_burrowed(target);
        } else {
            Building *q = &b[target - MAX_UNITS];
            raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
            center_x = q->x; center_y = q->y;
            max_hp = q->maxhp; shield = &q->shield;
        }
        int distance = sc64_siege_distance(raw, center_x, center_y, x, y);
        const Sc64StockUnitProfile *stock = sc64_stock_unit_profile(raw);
        if (distance < 0 || max_hp <= 0 || !stock) continue;
        unsigned packet = sc64_nuke_damage_packet_q8((unsigned)max_hp, stock->shields,
                                                      (unsigned)distance, buried);
        if (!packet || packet > INT_MAX) continue;
#ifdef SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT
        hit_target_impl_ex(target, (int)packet, -1, owner, SC64_STOCK_DMG_EXPLOSIVE,
                            SC64_HIT_PACKET_Q8 | SC64_HIT_ALLOW_SPLASH_BURROW);
#else
        hp_loss = sc64_siege_resolve(target, packet, &shield_loss);
        *shield -= shield_loss;
        if (hp_loss)
            hit_target_impl_ex(target, hp_loss, -1, owner, SC64_STOCK_DMG_EXPLOSIVE,
                SC64_HIT_PRECOMPUTED_HP | SC64_HIT_ALLOW_SPLASH_BURROW |
                SC64_HIT_SKIP_HALLUCINATION_SCALE);
#endif
    }
}
#endif
