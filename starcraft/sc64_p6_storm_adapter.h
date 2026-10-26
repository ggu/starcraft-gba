#ifndef SC64_P6_STORM_ADAPTER_H
#define SC64_P6_STORM_ADAPTER_H

#if !defined(SC64_P6_RUNTIME) || !defined(SC64_P6_STORM_ENABLED)
#error "P6 Storm adapter requires SC64_P6_RUNTIME and SC64_P6_STORM_ENABLED"
#endif
#if !defined(SC64_P6_ABILITIES_COMPLETE)
#include "sc64_p6_abilities_generated.h"
#endif
#include "sc64_psionic_storm.h"

_Static_assert(SC64_P6_ABILITIES_COMPLETE == 1u &&
               SC64_P6_ABILITIES_SOURCE_GLOBAL_ID == SC64_P6_GLOBAL_ID &&
               SC64_P6_ABILITIES_PSIONIC_STORM_TECH_ID == 19u &&
               SC64_P6_ABILITIES_SOURCE_OWNER == 2u &&
               SC64_P6_ABILITIES_TASSADAR_RAW_TYPE == 79u &&
               SC64_P6_ABILITIES_PSIONIC_STORM_AVAILABLE == 1u &&
               SC64_P6_ABILITIES_PSIONIC_STORM_RESEARCHED == 1u,
               "P6 Storm requires authenticated ability facts");

/* This adapter is included after sc64_p6_runtime_adapter.h and the engine
 * Unit/Building declarations. The parent owns the persisted `storm` member,
 * save generation and gameplay hook sites. */
enum {
    SC64_P6_STORM_RAW_TASSADAR = SC64_P6_ABILITIES_TASSADAR_RAW_TYPE,
    SC64_P6_STORM_SOURCE_OWNER = SC64_P6_ABILITIES_SOURCE_OWNER
};

static u16 race_p6_storm_max_energy(int raw_type)
{
    return raw_type == SC64_P6_STORM_RAW_TASSADAR ?
        SC64_STORM_HERO_MAX_ENERGY : SC64_STORM_MAX_ENERGY;
}

static int race_p6_storm_caster(int slot)
{
    return slot >= 0 && slot < MAX_UNITS && u[slot].alive &&
           u[slot].owner == SIDE && u[slot].raw_type == SC64_P6_STORM_RAW_TASSADAR;
}

static int race_p6_storm_technology_available(void)
{
    return SC64_P6_ABILITIES_PSIONIC_STORM_AVAILABLE != 0;
}

static int race_p6_storm_technology_researched(void)
{
    return SC64_P6_ABILITIES_PSIONIC_STORM_RESEARCHED != 0;
}

static int race_p6_storm_energy_valid(const Unit *q)
{
    if (!q || q->raw_type != SC64_P6_STORM_RAW_TASSADAR) return 0;
    return q->carry >= 0 &&
           q->carry <= (int)SC64_STORM_HERO_MAX_ENERGY * SC64_STORM_ENERGY_ONE &&
           q->work_timer <= 4;
}

static int race_p6_storm_world_state_valid(void)
{
    if (!race_p6_runtime_state_valid() ||
        !sc64_psionic_storm_state_valid(&race_p6.storm)) return 0;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        const Sc64PsionicStorm *effect = &race_p6.storm.storm[i];
        if (effect->active && (effect->x >= WORLD_W || effect->y >= WORLD_H)) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == SC64_P6_STORM_RAW_TASSADAR &&
            !race_p6_storm_energy_valid(&u[i])) return 0;
    return 1;
}

#ifdef SC64_P6_STORM_POSTCHECK_LOCAL
/* The tick entry performs the full persistent-state/world preflight above.
 * During a tick, the adapter mutates only the bounded Storm state, Tassadar's
 * energy/remainder, actor HP/alive/selection and saturated source-death
 * counters. The death hooks only update those bounded counters, so they
 * never mutate the P6 runtime model. The model validator is
 * therefore redundant at return; retain every Storm-local invariant that the
 * public world validator checks, including live raw-79 energy rows of every
 * owner (not only the SIDE caster). */
static int race_p6_storm_post_state_valid(void)
{
    if (!sc64_psionic_storm_state_valid(&race_p6.storm)) return 0;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        const Sc64PsionicStorm *effect = &race_p6.storm.storm[i];
        if (effect->active && (effect->x >= WORLD_W || effect->y >= WORLD_H)) return 0;
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == SC64_P6_STORM_RAW_TASSADAR &&
            !race_p6_storm_energy_valid(&u[i])) return 0;
    return 1;
}
#endif

/* Call once after P6 source actors have been imported on a fresh mission.
 * Never call this during save restore: saved energy and remainder are state. */
static void race_p6_storm_initialize_source_units(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].raw_type != SC64_P6_STORM_RAW_TASSADAR) continue;
        u[i].carry = (int)((u32)SC64_STORM_HERO_MAX_ENERGY *
                           SC64_STORM_ENERGY_ONE *
                           SC64_P6_ABILITIES_TASSADAR_INITIAL_ENERGY_PERCENT / 100u);
        u[i].work_timer = 0;
    }
    sc64_psionic_storm_init(&race_p6.storm);
}

static int race_p6_storm_menu_available(void)
{
    unsigned selected = 0;
    int caster = -1;
    if (!race_p6_active() || race_human != R_PROTOSS || selected_building >= 0 ||
        !race_p6_storm_technology_available() ||
        !race_p6_storm_technology_researched()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].owner != SIDE || !u[i].selected) continue;
        if (!race_p6_storm_caster(i) || !race_p6_storm_energy_valid(&u[i]) ||
            u[i].carry < SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE) return 0;
        caster = i;
        if (++selected > 1) return 0;
    }
    return selected == 1 && caster >= 0;
}

static int race_p6_storm_cast_selected(int x, int y)
{
    int caster = -1;
    if (!race_p6_storm_menu_available() || x < 0 || y < 0 ||
        x >= WORLD_W || y >= WORLD_H || !race_p6_storm_world_state_valid()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].selected) { caster = i; break; }
    if (caster < 0) return 0;
    int cast=sc64_psionic_storm_cast(&race_p6.storm, (u16)x, (u16)y,
        (u16)u[caster].x, (u16)u[caster].y, &u[caster].carry,
        race_p6_storm_technology_researched());
    if (cast) {
        u[caster].order=ORDER_IDLE; u[caster].target=-1;
        u[caster].tx=u[caster].x; u[caster].ty=u[caster].y;
    }
    return cast;
}

/* The caller invokes this only on simulation ticks, with user_paused reflecting
 * the ordinary pause screen. P6 source has no PauseGame opcode; authored/user
 * pause therefore freezes this policy clock and Tassadar's energy. */
static int race_p6_storm_tick(int user_paused)
{
    Sc64PsionicStormPulse pulses[SC64_STORM_MAX_ACTIVE];
    u8 hit[MAX_UNITS], hit_props[MAX_BUILDINGS];
    u8 pulse_count;
    if (!race_p6_active()) return 1;
    if (!race_p6_storm_world_state_valid()) return 0;
    if (user_paused || race_p6.runtime.model.outcome != SC64_P6_RUNNING) return 1;
    for (int i = 0; i < MAX_UNITS; ++i) if (race_p6_storm_caster(i)) {
        u8 remainder = (u8)u[i].work_timer;
        int energy = sc64_psionic_storm_regen_q8(u[i].carry,
            SC64_STORM_HERO_MAX_ENERGY, &remainder);
        if (energy < 0) return 0;
        u[i].carry = energy;
        u[i].work_timer = remainder;
    }
    pulse_count = sc64_psionic_storm_step(&race_p6.storm, pulses);
    memset(hit, 0, sizeof(hit));
    memset(hit_props,0,sizeof(hit_props));
    for (unsigned p = 0; p < pulse_count; ++p) {
        for (int i = 0; i < MAX_UNITS; ++i) {
            unsigned actor_flags;
            int dx, dy, hp_q8, shields_q8;
            u8 target_class;
            if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i])) continue;
            actor_flags = sc64_game_unit_actor_flags(u[i].type);
            if ((actor_flags & SC64_GAME_ACTOR_HIDDEN) || SC64_CARRIER_STATE_HIDDEN(&u[i])) continue;
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
            u[i].hp = hp_q8 / SC64_STORM_ENERGY_ONE;
            u[i].shield = shields_q8 / SC64_STORM_ENERGY_ONE;
            if (u[i].hp <= 0) {
                /* P6 source-death accounting must run while source identity is intact. */
                race_p6_note_unit_death(&u[i]);
                u[i].hp = 0;
                u[i].alive = 0;
                u[i].selected = 0;
                if (u[i].owner == SIDE) supply_used -= unit_supply(u[i].type);
            }
        }
    }
    /* Native traps are NonBuilding units even though this engine stores
     * their stationary props in the Building pool. Storm can damage them. */
    for (unsigned p=0; p<pulse_count; ++p) {
        for (int i=0; i<MAX_BUILDINGS; ++i) {
            Building *q=&b[i]; int raw=q->raw_type;
            int hp_q8,shield_q8,dx,dy;
            if (!q->alive || !(raw==203 || raw==209 || raw==210 || raw==211 || raw==213))
                continue;
            dx=(int)pulses[p].x-q->x; dy=(int)pulses[p].y-q->y;
            if (dx < -SC64_STORM_RADIUS_GBA_PX || dx > SC64_STORM_RADIUS_GBA_PX ||
                dy < -SC64_STORM_RADIUS_GBA_PX || dy > SC64_STORM_RADIUS_GBA_PX ||
                dx*dx+dy*dy > SC64_STORM_RADIUS_GBA_PX*SC64_STORM_RADIUS_GBA_PX)
                continue;
            hp_q8=q->hp*SC64_STORM_ENERGY_ONE; shield_q8=q->shield*SC64_STORM_ENERGY_ONE;
            if (!sc64_psionic_storm_apply_hit(
                    SC64_STORM_TARGET_GROUND|SC64_STORM_TARGET_NONBUILDING,
                    invincible_buildings[i] || (q->flags&16u), &hit_props[i],
                    &hp_q8,&shield_q8)) continue;
            q->hp=hp_q8/SC64_STORM_ENERGY_ONE; q->shield=shield_q8/SC64_STORM_ENERGY_ONE;
            if (q->hp<=0) {
                race_p6_note_building_death(q); q->hp=0; q->alive=0;
                invalidate_path_grid();
            }
        }
    }
#ifdef SC64_P6_STORM_POSTCHECK_LOCAL
    return race_p6_storm_post_state_valid();
#else
    return race_p6_storm_world_state_valid();
#endif
}

static void race_p6_storm_draw(void)
{
    if (!race_p6_active()) return;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        const Sc64PsionicStorm *effect = &race_p6.storm.storm[i];
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
