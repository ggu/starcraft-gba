#ifndef SC64_Z6_STORM_ADAPTER_H
#define SC64_Z6_STORM_ADAPTER_H

#if !defined(SC64_Z6_RUNTIME) || !defined(SC64_Z6_STORM_ENABLED)
#error "Z6 Storm adapter requires SC64_Z6_RUNTIME and SC64_Z6_STORM_ENABLED"
#endif
#if !defined(SC64_Z6_ABILITIES_COMPLETE)
#include "sc64_z6_abilities_generated.h"
#endif
#include "sc64_psionic_storm.h"

_Static_assert(SC64_Z6_ABILITIES_COMPLETE == 1u &&
               SC64_Z6_ABILITIES_SOURCE_GLOBAL_ID == SC64_Z6_GLOBAL_ID &&
               SC64_Z6_ABILITIES_PSIONIC_STORM_TECH_ID == 19u &&
               SC64_Z6_ABILITIES_SOURCE_OWNER == 3u &&
               SC64_Z6_ABILITIES_HERO_RAW_TYPE == 51u &&
               SC64_Z6_ABILITIES_PSIONIC_STORM_AVAILABLE == 1u &&
               SC64_Z6_ABILITIES_PSIONIC_STORM_RESEARCHED == 0u &&
               SC64_Z6_ABILITIES_PSIONIC_STORM_HERO_RESEARCH_EXEMPTION == 1u,
               "Z6 Storm requires authenticated ability facts");

/* This adapter is included after sc64_z6_runtime_adapter.h and the engine
 * Unit/Building declarations. The parent owns the persisted `storm` member,
 * save generation and gameplay hook sites. */
enum {
    SC64_Z6_STORM_RAW_KERRIGAN = SC64_Z6_ABILITIES_HERO_RAW_TYPE,
    SC64_Z6_STORM_SOURCE_OWNER = SC64_Z6_ABILITIES_SOURCE_OWNER
};

static u16 race_z6_storm_max_energy(int raw_type)
{
    return raw_type == SC64_Z6_STORM_RAW_KERRIGAN ?
        SC64_STORM_HERO_MAX_ENERGY : SC64_STORM_MAX_ENERGY;
}

static int race_z6_storm_caster(int slot)
{
    return slot >= 0 && slot < MAX_UNITS && u[slot].alive &&
           u[slot].owner == SIDE && u[slot].raw_owner == 3 &&
           u[slot].raw_type == SC64_Z6_STORM_RAW_KERRIGAN;
}

static int race_z6_storm_technology_available(void)
{
    return SC64_Z6_ABILITIES_PSIONIC_STORM_AVAILABLE != 0;
}

static int race_z6_storm_technology_researched(void)
{
    return SC64_Z6_ABILITIES_PSIONIC_STORM_HERO_RESEARCH_EXEMPTION != 0;
}

static int race_z6_storm_energy_valid(const Unit *q)
{
    if (!q || q->raw_type != SC64_Z6_STORM_RAW_KERRIGAN) return 0;
    return q->carry >= 0 &&
           q->carry <= (int)SC64_STORM_HERO_MAX_ENERGY * SC64_STORM_ENERGY_ONE &&
           q->work_timer == 0;
}

static int race_z6_storm_world_state_valid(void)
{
    if (!sc64_z6_persistent_state_valid(&race_z6) ||
        !race_z6_cloak_world_state_valid() ||
        !sc64_psionic_storm_state_valid(&race_z6.storm)) return 0;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        const Sc64PsionicStorm *effect = &race_z6.storm.storm[i];
        if (effect->active && (effect->x >= WORLD_W || effect->y >= WORLD_H)) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == SC64_Z6_STORM_RAW_KERRIGAN &&
            !race_z6_storm_energy_valid(&u[i])) return 0;
    return 1;
}

static int race_z6_storm_menu_available(void)
{
    unsigned selected = 0;
    int caster = -1;
    if (!race_z6_active() || race_human != R_ZERG || selected_building >= 0 ||
        !race_z6_storm_technology_available() ||
        !race_z6_storm_technology_researched()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || !u[i].selected) continue;
        if (!race_z6_storm_caster(i) || !race_z6_storm_energy_valid(&u[i]) ||
            u[i].carry < SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE) return 0;
        caster = i;
        if (++selected > 1) return 0;
    }
    return selected == 1 && caster >= 0;
}

static int race_z6_storm_cast_selected(int x, int y)
{
    int caster = -1;
    if (!race_z6_storm_menu_available() || x < 0 || y < 0 ||
        x >= WORLD_W || y >= WORLD_H || !race_z6_storm_world_state_valid()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].selected) { caster = i; break; }
    if (caster < 0) return 0;
    int cast=sc64_psionic_storm_cast(&race_z6.storm, (u16)x, (u16)y,
        (u16)u[caster].x, (u16)u[caster].y, &u[caster].carry,
        race_z6_storm_technology_researched());
    if (cast) {
        u[caster].order=ORDER_IDLE; u[caster].target=-1;
        u[caster].tx=u[caster].x; u[caster].ty=u[caster].y;
    }
    return cast;
}

/* Cloak owns Kerrigan's single energy clock. This advances only Storm effects
 * once per live simulation update, after cloak drain/regeneration. */
static int race_z6_storm_tick(void)
{
    Sc64PsionicStormPulse pulses[SC64_STORM_MAX_ACTIVE];
    u8 hit[MAX_UNITS];
    u8 pulse_count;
    if (!race_z6_active()) return 1;
    if (!race_z6_storm_world_state_valid()) return 0;
    if (race_z6.runtime.model.outcome != SC64_Z6_RUNNING) return 1;
    pulse_count = sc64_psionic_storm_step(&race_z6.storm, pulses);
    memset(hit, 0, sizeof(hit));
    for (unsigned p = 0; p < pulse_count; ++p) {
        for (int i = 0; i < MAX_UNITS; ++i) {
            unsigned actor_flags;
            int dx, dy, hp_q8, shields_q8;
            u8 target_class;
            if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i])) continue;
            actor_flags = sc64_game_unit_actor_flags(u[i].type);
            if (actor_flags & SC64_GAME_ACTOR_HIDDEN) continue;
            dx = (int)pulses[p].x - u[i].x;
            dy = (int)pulses[p].y - u[i].y;
            if (dx < -SC64_STORM_RADIUS_GBA_PX || dx > SC64_STORM_RADIUS_GBA_PX ||
                dy < -SC64_STORM_RADIUS_GBA_PX || dy > SC64_STORM_RADIUS_GBA_PX ||
                dx * dx + dy * dy > SC64_STORM_RADIUS_GBA_PX * SC64_STORM_RADIUS_GBA_PX)
                continue;
            target_class = (u8)sc64_psionic_storm_target_class(
                sc64_race_unit_flying(u[i].type),
                !!(actor_flags & SC64_GAME_ACTOR_BUILDING));
            hp_q8 = u[i].hp * SC64_STORM_ENERGY_ONE;
            shields_q8 = u[i].shield * SC64_STORM_ENERGY_ONE;
            if (!sc64_psionic_storm_apply_hit(target_class,
                    invincible_units[i] || (u[i].flags & 16) ||
                    (actor_flags & SC64_GAME_ACTOR_INVINCIBLE), &hit[i],
                    &hp_q8, &shields_q8)) continue;
            /* A hallucination takes twice the pulse's Ignore Armor damage.
             * Reuse shield-first clamping for its second half, while `hit`
             * still prevents another overlapping Storm pulse this tick. */
            if (race_z6_hallucination_actor(&u[i]) && hp_q8 > 0) {
                u8 extra_hit=0;
                (void)sc64_psionic_storm_apply_hit(target_class,0,&extra_hit,
                                                  &hp_q8,&shields_q8);
            }
            u[i].hp = hp_q8 / SC64_STORM_ENERGY_ONE;
            u[i].shield = shields_q8 / SC64_STORM_ENERGY_ONE;
            if (u[i].hp <= 0) {
                /* Z6 source-death accounting must run while source identity is intact. */
                race_z6_note_unit_death(&u[i]);
                u[i].hp = 0;
                u[i].alive = 0;
                u[i].selected = 0;
                if (u[i].owner == SIDE) supply_used -= unit_supply(u[i].type);
                if (!race_z6_hallucination_cleanup_dead(&u[i])) return 0;
                if (u[i].raw_type == 51 && u[i].raw_owner == 3) {
                    race_z6.cloak.active=0; race_z6.cloak.regen_remainder=0;
                }
            }
        }
    }
    return race_z6_storm_world_state_valid();
}

static void race_z6_storm_draw(void)
{
    if (!race_z6_active()) return;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        const Sc64PsionicStorm *effect = &race_z6.storm.storm[i];
        int x, y, r;
        if (!effect->active) continue;
        x = effect->x - camx;
        y = effect->y - camy + 12;
        r = SC64_STORM_RADIUS_GBA_PX;
        gfx_line(x, y-r, x+r, y, C_CYAN);
        gfx_line(x+r, y, x, y+r, C_CYAN);
        gfx_line(x, y+r, x-r, y, C_CYAN);
        gfx_line(x-r, y, x, y-r, C_CYAN);
        gfx_line(x-2, y, x+2, y, C_WHITE);
        gfx_line(x, y-2, x, y+2, C_WHITE);
    }
}

#endif
