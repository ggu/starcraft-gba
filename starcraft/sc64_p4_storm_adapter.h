#ifndef SC64_P4_STORM_ADAPTER_H
#define SC64_P4_STORM_ADAPTER_H

#if !defined(SC64_P4_RUNTIME) || !defined(SC64_P4_STORM_ENABLED)
#error "P4 Storm adapter requires SC64_P4_RUNTIME and SC64_P4_STORM_ENABLED"
#endif

#define SC64_P4_STORM_HIGH_TEMPLAR_RAW 67

static u16 race_p4_storm_max_energy(int raw_type)
{
    return raw_type == SC64_P4_RAW_TASSADAR ?
        SC64_STORM_HERO_MAX_ENERGY : SC64_STORM_MAX_ENERGY;
}

static int race_p4_storm_caster(int i)
{
    return i >= 0 && i < MAX_UNITS && u[i].alive && u[i].owner == SIDE &&
        (u[i].raw_type == SC64_P4_STORM_HIGH_TEMPLAR_RAW ||
         u[i].raw_type == SC64_P4_RAW_TASSADAR);
}

static int race_p4_storm_technology_available(void)
{
    return sc64_p4_source_storm_effective_available != 0;
}

static int race_p4_storm_technology_researched(void)
{
    return sc64_p4_source_storm_effective_researched != 0;
}

static int race_p4_storm_energy_valid(const Unit *q)
{
    u16 maximum;
    if (!q || (q->raw_type != SC64_P4_STORM_HIGH_TEMPLAR_RAW &&
               q->raw_type != SC64_P4_RAW_TASSADAR)) return 0;
    maximum = race_p4_storm_max_energy(q->raw_type);
    return q->carry >= 0 && q->carry <= (int)maximum * SC64_STORM_ENERGY_ONE &&
           q->work_timer <= 4;
}

static int race_p4_storm_world_state_valid(void)
{
    if (!race_p4_runtime_state_valid(&race_p4) ||
        !sc64_psionic_storm_state_valid(&race_p4.storm)) return 0;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i)
        if (race_p4.storm.storm[i].active &&
            (race_p4.storm.storm[i].x >= WORLD_W ||
             race_p4.storm.storm[i].y >= WORLD_H)) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive &&
            (u[i].raw_type == SC64_P4_STORM_HIGH_TEMPLAR_RAW ||
             u[i].raw_type == SC64_P4_RAW_TASSADAR) &&
            !race_p4_storm_energy_valid(&u[i])) return 0;
    return 1;
}

static void race_p4_storm_init_actor(int i)
{
    unsigned percent;
    if (i < 0 || i >= MAX_UNITS) return;
    if (u[i].raw_type == SC64_P4_STORM_HIGH_TEMPLAR_RAW)
        percent = sc64_p4_source_energy_percent_67;
    else if (u[i].raw_type == SC64_P4_RAW_TASSADAR)
        percent = sc64_p4_source_energy_percent_79;
    else return;
    u[i].carry = (int)((u32)race_p4_storm_max_energy(u[i].raw_type) *
                       SC64_STORM_ENERGY_ONE * percent / 100u);
    u[i].work_timer = 0;
}

static void race_p4_storm_initialize_source_units(void)
{
    sc64_psionic_storm_init(&race_p4.storm);
    for (int i = 0; i < MAX_UNITS; ++i) race_p4_storm_init_actor(i);
}

static int race_p4_storm_menu_available(void)
{
    unsigned selected = 0;
    int caster = -1;
    if (!race_p4_active() || race_human != R_PROTOSS || selected_building >= 0 ||
        !race_p4_storm_technology_available() ||
        !race_p4_storm_technology_researched()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE && u[i].selected) {
        if (!race_p4_storm_caster(i) || !race_p4_storm_energy_valid(&u[i]) ||
            u[i].carry < SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE) return 0;
        caster = i;
        if (++selected > 1) return 0;
    }
    return selected == 1 && caster >= 0;
}

static int race_p4_storm_cast_selected(int x, int y)
{
    int caster = -1;
    if (!race_p4_storm_menu_available() || x < 0 || y < 0 ||
        x >= WORLD_W || y >= WORLD_H ||
        !race_p4_storm_world_state_valid()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].selected) { caster = i; break; }
    if (caster < 0) return 0;
    return sc64_psionic_storm_cast(&race_p4.storm, (u16)x, (u16)y,
        (u16)u[caster].x, (u16)u[caster].y, &u[caster].carry,
        race_p4_storm_technology_researched());
}

/* Called from P4's normal mission tick. The effect clocks and caster energy
 * stop during authored PauseGame while its script cursor continues. */
static int race_p4_storm_tick(void)
{
    Sc64PsionicStormPulse pulses[SC64_STORM_MAX_ACTIVE];
    u8 hit[MAX_UNITS];
    u8 count;
    if (!race_p4_active()) return 1;
    if (!race_p4_storm_world_state_valid()) return 0;
    if (race_p4.source_paused) return 1;
    for (int i = 0; i < MAX_UNITS; ++i) if (race_p4_storm_caster(i)) {
        u8 remainder = (u8)u[i].work_timer;
        int energy = sc64_psionic_storm_regen_q8(u[i].carry,
            race_p4_storm_max_energy(u[i].raw_type), &remainder);
        if (energy < 0) return 0;
        u[i].carry = energy;
        u[i].work_timer = remainder;
    }
    count = sc64_psionic_storm_step(&race_p4.storm, pulses);
    memset(hit, 0, sizeof(hit));
    for (unsigned p = 0; p < count; ++p) {
        for (int i = 0; i < MAX_UNITS; ++i) {
            u8 klass;
            int hp_q8, shields_q8, dx, dy;
            unsigned flags;
            if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i]) ||
                (flags = sc64_game_unit_actor_flags(u[i].type)) & SC64_GAME_ACTOR_HIDDEN)
                continue;
            dx = (int)pulses[p].x - u[i].x;
            dy = (int)pulses[p].y - u[i].y;
            if (dx < -SC64_STORM_RADIUS_GBA_PX || dx > SC64_STORM_RADIUS_GBA_PX ||
                dy < -SC64_STORM_RADIUS_GBA_PX || dy > SC64_STORM_RADIUS_GBA_PX ||
                dx * dx + dy * dy > SC64_STORM_RADIUS_GBA_PX * SC64_STORM_RADIUS_GBA_PX)
                continue;
            klass = (u8)sc64_psionic_storm_target_class(
                sc64_race_unit_flying(u[i].type), !!(flags & SC64_GAME_ACTOR_BUILDING));
            hp_q8 = u[i].hp * SC64_STORM_ENERGY_ONE;
            shields_q8 = u[i].shield * SC64_STORM_ENERGY_ONE;
            if (!sc64_psionic_storm_apply_hit(klass,
                    invincible_units[i] || (u[i].flags & 16) ||
                    (flags & SC64_GAME_ACTOR_INVINCIBLE), &hit[i],
                    &hp_q8, &shields_q8)) continue;
            u[i].hp = hp_q8 / SC64_STORM_ENERGY_ONE;
            u[i].shield = shields_q8 / SC64_STORM_ENERGY_ONE;
            if (u[i].hp <= 0) {
                u[i].hp = 0; u[i].alive = 0; u[i].selected = 0;
                if (u[i].owner == SIDE) supply_used -= unit_supply(u[i].type);
            }
        }
    }
    return 1;
}

static void race_p4_storm_draw(void)
{
    if (!race_p4_active()) return;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        const Sc64PsionicStorm *e = &race_p4.storm.storm[i];
        int x, y, r;
        if (!e->active) continue;
        x = e->x - camx; y = e->y - camy + 12;
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
