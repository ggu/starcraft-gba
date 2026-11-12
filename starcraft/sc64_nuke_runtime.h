/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_RUNTIME_H
#define SC64_NUKE_RUNTIME_H

/* Opt-in world probe. The command and update paths use the same ownership
 * checks, including for campaign actors. No new state is written to SRAM. */
enum { SC64_NUKE_FLIGHT_LIMIT = 8, SC64_NUKE_NO_PAYER = 255 };
enum { SC64_NUKE_ASCENT_TICKS = 90, SC64_NUKE_ALERT_TICKS = 45 };
typedef struct {
    Sc64NukeFlight strike;
    u16 origin_x, origin_y, approach_x, approach_y;
    u8 approaching, impact_tick, ascent_ticks;
} Sc64NukeWorldFlight;
typedef struct {
    Sc64NukeSilo ammo;
    u8 payer;
} Sc64NukeWorldSilo;
static Sc64NukeWorldSilo race_nuke_silos[MAX_BUILDINGS];
static Sc64NukeWorldFlight race_nuke_flights[SC64_NUKE_FLIGHT_LIMIT];
static unsigned race_nuke_paid_silos, race_nuke_active_flights;

static void race_nuke_reset(void)
{
    memset(race_nuke_silos, 0, sizeof race_nuke_silos);
    memset(race_nuke_flights, 0, sizeof race_nuke_flights);
    race_nuke_paid_silos = race_nuke_active_flights = 0;
}

static int race_nuke_raw_building(int slot, unsigned raw)
{
    return (unsigned)slot < MAX_BUILDINGS && b[slot].alive &&
        sc64_race_raw_building(b[slot].type) == (int)raw;
}

static int race_nuke_parent_slot(int addon, unsigned parent_raw)
{
    if ((unsigned)addon >= MAX_BUILDINGS || !building_complete(addon)) return -1;
    int owner = b[addon].owner;
    if ((unsigned)owner >= 5 || owner == NEUTRAL) return -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (b[i].owner != owner || b[addon].x != b[i].x + 48 ||
            b[addon].y != b[i].y + 8 || !race_nuke_raw_building(i, parent_raw) ||
            !building_complete(i)) continue;
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (race_late_airborne(i)) continue;
#endif

#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        if (race_bc_facility_airborne(i)) continue;
#endif
        return i;
    }
    return -1;
}

static int race_nuke_silo_online(int slot, int owner)
{
    return (unsigned)owner < 5 && owner != NEUTRAL &&
        race_nuke_raw_building(slot, SC64_NUKE_SILO_RAW) && b[slot].owner == owner &&
        race_nuke_parent_slot(slot, 106) >= 0;
}

#ifdef SC64_FIELD_CONTROLS
/* Negative means no paid missile; zero or more is source progress. */
static int race_nuke_silo_status(int slot)
{
    if (!race_nuke_raw_building(slot, SC64_NUKE_SILO_RAW) || b[slot].owner != SIDE) return -1;
    const Sc64NukeWorldSilo *s = &race_nuke_silos[slot];
    if (s->payer != SIDE || !sc64_nuke_silo_supply(&s->ammo)) return -1;
    return SC64_NUKE_MISSILE_SOURCE_TICKS - (int)s->ammo.remaining;
}
#endif

#ifdef SC64_HAS_SPRITES
static int race_nuke_silo_working(int slot)
{
    unsigned remaining = race_nuke_silos[slot].ammo.remaining;
    return remaining && race_nuke_silo_online(slot, b[slot].owner) ?
        SC64_NUKE_MISSILE_SOURCE_TICKS - (int)remaining : -1;
}
#endif

static void race_nuke_clear_silo(int slot)
{
    Sc64NukeWorldSilo *s = &race_nuke_silos[slot];
    if (sc64_nuke_silo_supply(&s->ammo)) --race_nuke_paid_silos;
    s->ammo = (Sc64NukeSilo){0};
    s->payer = SC64_NUKE_NO_PAYER;
}

static unsigned race_nuke_addon_raw(int parent)
{
    int raw = sc64_race_raw_building(b[parent].type);
    return raw == 106 ? SC64_NUKE_SILO_RAW : raw == 116 ? SC64_NUKE_COVERT_RAW : 0;
}

static void race_nuke_snap_addon_landing(int parent, int *x, int *y, int (*valid)(int,int,int))
{
    unsigned addon_raw = race_nuke_addon_raw(parent);
    if (!addon_raw) return;
    int best = 8 * 8 + 1, px = *x, py = *y;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if ((b[i].owner != NEUTRAL && b[i].owner != b[parent].owner) ||
            !building_complete(i) || !race_nuke_raw_building(i, addon_raw)) continue;
        int ax = b[i].x - 48, ay = b[i].y - 8;
        int d = dist2(*x, *y, ax, ay);
        if (d >= best || !valid(parent, ax, ay)) continue;
        best = d; px = ax; py = ay;
    }
    *x = px; *y = py;
}

/* T8 places empty neutral add-ons as actual buildings. A completed landing,
 * not flight through the coordinate, claims the exact compatible add-on. */
static void race_nuke_relink_neutral_addon(int parent)
{
    unsigned addon_raw = race_nuke_addon_raw(parent);
    if (!building_complete(parent) || b[parent].owner == NEUTRAL || !addon_raw) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (b[i].owner != NEUTRAL || !building_complete(i) ||
            b[i].x != b[parent].x + 48 || b[i].y != b[parent].y + 8 ||
            !race_nuke_raw_building(i, addon_raw)) continue;
        if (race_nuke_silos[i].ammo.armed) race_nuke_silos[i].payer = (u8)b[parent].owner;
        b[i].owner = b[parent].owner;
        b[i].raw_owner = b[parent].raw_owner;
        target_lists_valid = 0;
        break;
    }
}

static int race_nuke_owned_addon(int parent)
{
    unsigned addon_raw = race_nuke_addon_raw(parent);
    if (!addon_raw || b[parent].owner == NEUTRAL || (b[parent].flags & 4)) return -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].owner == b[parent].owner && b[i].x == b[parent].x + 48 &&
            b[i].y == b[parent].y + 8 && race_nuke_raw_building(i, addon_raw)) return i;
    return -1;
}

static int race_nuke_addon_constructing(int parent)
{
    int slot = race_nuke_owned_addon(parent);
    return slot >= 0 && b[slot].under_construction;
}

static void race_nuke_detach_addon(int parent)
{
    int slot = race_nuke_owned_addon(parent);
    if (slot < 0 || b[slot].under_construction) return;
    Sc64NukeWorldSilo *s = &race_nuke_silos[slot];
    if (s->ammo.remaining) {
        int owner = s->payer;
        race_nuke_clear_silo(slot);
        race_add_owner_resources(owner, SC64_NUKE_MISSILE_ORE, SC64_NUKE_MISSILE_GAS);
    }
    /* A completed warhead keeps its payer while the Silo is neutral.
     * Ownership and supply move together when a parent reconnects. */
    b[slot].owner = NEUTRAL;
    b[slot].raw_owner = 11;
    if (selected_building == slot) selected_building = -1;
    target_lists_valid = 0;
}

static int race_nuke_reserved_supply(int owner)
{
    int result = 0;
    if (race_nuke_paid_silos)
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (race_nuke_silos[i].payer == owner && b[i].alive &&
                (b[i].owner == owner || (b[i].owner == NEUTRAL && race_nuke_silos[i].ammo.armed)))
                result += (int)sc64_nuke_silo_supply(&race_nuke_silos[i].ammo);
    if (race_nuke_active_flights)
        for (int i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i)
            if (race_nuke_flights[i].strike.owner == owner)
                result += (int)sc64_nuke_flight_supply(&race_nuke_flights[i].strike);
    return result;
}

/* Return a HUD feedback code so a failed purchase gives the same feedback
 * as an ordinary unit queue. One slot per complete attached Silo. */
static int race_nuke_arm(int slot, int owner)
{
    if (!race_nuke_silo_online(slot, owner)) return 8;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active() && !sc64_terran_late_source_product_available(
            sc64_terran_late_product_source_slot(owner, b[slot].raw_owner), SC64_NUKE_MISSILE_RAW))
        return 8;
#endif
    Sc64NukeWorldSilo *s = &race_nuke_silos[slot];
    if (sc64_nuke_silo_supply(&s->ammo)) return NUKE_TOAST_OCCUPIED;
    int used = race_owner_reserved_supply(owner), cap = race_owner_supply_cap(owner);
    if (cap - used < SC64_NUKE_MISSILE_SUPPLY) return 2;
    int ore = race_owner_minerals(owner), gas_amount = race_owner_gas(owner);
    if (ore < SC64_NUKE_MISSILE_ORE) return 1;
    if (gas_amount < SC64_NUKE_MISSILE_GAS) return 12;
    if (!sc64_nuke_train(&s->ammo, &ore, &gas_amount,
                         (unsigned)used, (unsigned)cap,
                         SC64_NUKE_MISSILE_SOURCE_TICKS)) return 2;
    s->payer = (u8)owner;
    ++race_nuke_paid_silos;
    race_add_owner_resources(owner, -SC64_NUKE_MISSILE_ORE, -SC64_NUKE_MISSILE_GAS);
    return 4;
}

static int race_nuke_cancel_arm(int slot, int owner)
{
    if (!race_nuke_silo_online(slot, owner)) return 0;
    Sc64NukeWorldSilo *s = &race_nuke_silos[slot];
    if (!s->ammo.remaining || s->payer != owner) return 0;
    race_nuke_clear_silo(slot);
    race_add_owner_resources(owner, SC64_NUKE_MISSILE_ORE, SC64_NUKE_MISSILE_GAS);
    return 1;
}

static int race_nuke_armed_slot(int owner)
{
    if (race_nuke_paid_silos)
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (race_nuke_silos[i].ammo.armed && race_nuke_silos[i].payer == owner &&
                race_nuke_silo_online(i, owner)) return i;
    return -1;
}

static int race_nuke_ghost_eligible(int slot, int owner)
{
    return (unsigned)slot < MAX_UNITS && u[slot].alive && u[slot].owner == owner &&
        sc64_race_ordinary_ghost(u[slot].type) && !SC64_TRANSPORT_LOADED(&u[slot]);
}

static Sc64NukeWorldFlight *race_nuke_track(int slot)
{
    if (race_nuke_active_flights)
        for (int i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i) {
            Sc64NukeWorldFlight *f = &race_nuke_flights[i];
            if ((f->approaching || f->strike.phase == SC64_NUKE_PAINTING) &&
                f->strike.spotter == slot) return f;
        }
    return 0;
}

static void race_nuke_retire(Sc64NukeWorldFlight *f)
{
    *f = (Sc64NukeWorldFlight){0};
    --race_nuke_active_flights;
}

static int race_nuke_abort(int slot)
{
    Sc64NukeWorldFlight *f = race_nuke_track(slot);
    if (!f) return 0;
    if (race_nuke_ghost_eligible(slot, f->strike.owner)) {
        u[slot].order = ORDER_IDLE;
        u[slot].target = -1;
    }
    if (f->strike.owner == SIDE) toast(NUKE_TOAST_ABORTED);
    race_nuke_retire(f);
    return 1;
}

static int race_nuke_ghost_locked(int slot)
{
    Sc64NukeWorldFlight *f = race_nuke_track(slot);
    return f && f->strike.phase == SC64_NUKE_PAINTING;
}

#if defined(SC64_HAS_SPRITES) && defined(SC64_UNIT_FACING)
static int race_nuke_draw_ghost(int slot, int x, int y)
{
    const Sc64NukeWorldFlight *f = race_nuke_track(slot);
    if (!f || f->strike.phase != SC64_NUKE_PAINTING) return 0;
    unsigned facing = (unsigned)sc64_facing_direction(f->strike.x - u[slot].x,
                                                      f->strike.y - u[slot].y);
    return sc64_nuke_ghost_paint_draw(facing, f->strike.age, x, y);
}
#endif

static int race_nuke_ghost_in_range(int slot, int x, int y)
{
    /* Half-scale world. This uses base sight until Ocular Implants has a
     * real owner research row and the native boundary is measured. */
    const int radius = SC64_NUKE_SOURCE_SIGHT / 2;
    return dist2(u[slot].x, u[slot].y, x, y) <= radius * radius;
}

/* Project the owned missile's Q8 acceleration onto the half-scale map.
 * The bounded ascent/descent never reaches the source speed ceiling. */
static unsigned race_nuke_travel(unsigned ticks)
{
    return SC64_NUKE_ACCEL_Q8 * ticks * (ticks + 1) / (2 * 512);
}

static unsigned race_nuke_travel_ticks(unsigned distance, unsigned limit_ticks)
{
    unsigned tick = 1;
    while (tick < limit_ticks && race_nuke_travel(tick) < distance) ++tick;
    return tick;
}

static int race_nuke_commit(Sc64NukeWorldFlight *f, int silo)
{
    /* Nominal OpenBW order timers plus the ROM's Warpin waits. Account for
     * a short climb near the northern edge and a map-clamped reentry. */
    unsigned rise = race_nuke_travel_ticks((unsigned)b[silo].y, SC64_NUKE_ASCENT_TICKS);
    unsigned drop = f->strike.y < 160 ? f->strike.y : 160;
    unsigned fall = race_nuke_travel_ticks(drop > 5 ? drop - 5 : 0, SC64_NUKE_ASCENT_TICKS);
    const Sc64NukeCadence cadence = {
        (u16)(rise + SC64_NUKE_WARP_SOURCE_TICKS), (u16)fall,
        (u16)(rise < SC64_NUKE_ALERT_TICKS ? rise : SC64_NUKE_ALERT_TICKS)
    };
    int slot = f->strike.spotter;
    if (!sc64_nuke_launch(&race_nuke_silos[silo].ammo, &f->strike,
                          f->strike.owner, (unsigned)slot,
                          f->strike.x, f->strike.y, cadence)) return 0;
    /* sc64_nuke_launch clears the ammo before the wrapper resets its payer. */
    --race_nuke_paid_silos;
    race_nuke_clear_silo(silo);
    f->origin_x = (u16)b[silo].x;
    f->origin_y = (u16)b[silo].y;
    f->ascent_ticks = (u8)rise;
    f->approaching = 0;
    u[slot].order = ORDER_IDLE;
    u[slot].target = -1;
    return 1;
}

static int race_nuke_issue(int slot, int owner, int x, int y)
{
    if ((unsigned)x >= (unsigned)WORLD_W || (unsigned)y >= (unsigned)WORLD_H ||
        !race_nuke_ghost_eligible(slot, owner) || race_nuke_track(slot)) return 0;
    int silo = race_nuke_armed_slot(owner);
    if (silo < 0) return 0;
    for (int i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i) {
        Sc64NukeWorldFlight *f = &race_nuke_flights[i];
        if (f->strike.phase || f->approaching || f->impact_tick) continue;
        f->strike.owner = (u8)owner;
        f->strike.spotter = (u16)slot;
        f->strike.x = (u16)x;
        f->strike.y = (u16)y;
        ++race_nuke_active_flights;
        if (race_nuke_ghost_in_range(slot, x, y)) {
            if (race_nuke_commit(f, silo)) return 1;
            race_nuke_retire(f);
            return 0;
        }
        /* Approach uses the existing route finder and stops at cast range.
         * The Silo is revalidated at commit, so approaching cannot reserve
         * or double-spend a missile another Ghost has already fired. */
        f->approaching = 1;
        f->approach_x = (u16)x;
        f->approach_y = (u16)y;
        u[slot].order = ORDER_MOVE;
        u[slot].tx = x; u[slot].ty = y;
        u[slot].target = -1;
        u[slot].path_timer = 0; u[slot].path_retry = 0;
        return 1;
    }
    return 0;
}

static int race_nuke_menu_action(void)
{
    int slot = selected_building;
    if (slot >= 0) {
        if (!race_nuke_silo_online(slot, SIDE)) return NUKE_MENU_NONE;
        const Sc64NukeSilo *s = &race_nuke_silos[slot].ammo;
        if (s->remaining) return NUKE_MENU_CANCEL_ARM;
        return s->armed ? NUKE_MENU_ARMED : NUKE_MENU_ARM;
    }
    int ghost = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].selected && race_nuke_ghost_eligible(i, SIDE)) {
            if (race_nuke_track(i)) return NUKE_MENU_ABORT;
            ghost = 1;
        }
    return ghost && race_nuke_armed_slot(SIDE) >= 0 ? NUKE_MENU_CAST : NUKE_MENU_NONE;
}

static const char *race_nuke_menu_label(int action)
{
    switch (action) {
    case NUKE_MENU_ARM: return "ARM NUCLEAR MISSILE";
    case NUKE_MENU_CANCEL_ARM: return "CANCEL MISSILE";
    case NUKE_MENU_CAST: return "NUCLEAR STRIKE";
    case NUKE_MENU_ABORT: return "ABORT STRIKE";
    case NUKE_MENU_ARMED: return "MISSILE ARMED";
    default: return "";
    }
}

static int race_nuke_issue_selected(int x, int y)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].selected && race_nuke_issue(i, SIDE, x, y)) return 1;
    return 0;
}

static void race_nuke_tick(void)
{
    /* Classic source data runs at 24 Hz. Keep approach/cast opportunity on
     * every game update, but advance paid and flight clocks on source ticks. */
    int due = (unsigned)frame % 5u != 0u;
    if (race_nuke_paid_silos && due)
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
            Sc64NukeWorldSilo *s = &race_nuke_silos[i];
            if (!sc64_nuke_silo_supply(&s->ammo)) continue;
            if (!race_nuke_raw_building(i, SC64_NUKE_SILO_RAW) ||
                (b[i].owner != s->payer && !(b[i].owner == NEUTRAL && s->ammo.armed))) {
                race_nuke_clear_silo(i);
                continue;
            }
            if (s->ammo.remaining && race_nuke_silo_online(i, s->payer) &&
                sc64_nuke_train_step(&s->ammo) && s->payer == SIDE) toast(NUKE_TOAST_ARMED);
        }
    if (!race_nuke_active_flights) return;
    for (int i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i) {
        Sc64NukeWorldFlight *f = &race_nuke_flights[i];
        if (f->impact_tick) {
            if (due && ++f->impact_tick >
                SC64_NUKE_IMPACT_FRAME_COUNT * SC64_NUKE_IMPACT_SOURCE_TICKS) race_nuke_retire(f);
            continue;
        }
        int slot = f->strike.spotter, owner = f->strike.owner;
        if (f->approaching) {
            if (!race_nuke_ghost_eligible(slot, owner) || u[slot].order != ORDER_MOVE ||
                u[slot].tx != f->approach_x || u[slot].ty != f->approach_y) {
                race_nuke_retire(f);
            } else if (race_nuke_ghost_in_range(slot, f->strike.x, f->strike.y)) {
                int silo = race_nuke_armed_slot(owner);
                if (silo < 0 || !race_nuke_commit(f, silo)) (void)race_nuke_abort(slot);
            }
            continue;
        }
        if (!due || !f->strike.phase) continue;
        unsigned event = sc64_nuke_flight_step(&f->strike, race_nuke_ghost_eligible(slot, owner));
        if (event & SC64_NUKE_EVENT_WARNING) {
            toast(NUKE_TOAST_WARNING);
            game_audio_nuclear_warning();
        }
        if (event & SC64_NUKE_EVENT_ABORT) race_nuke_retire(f);
        if (event & SC64_NUKE_EVENT_IMPACT) {
            int x = f->strike.x, y = f->strike.y;
            f->impact_tick = 1;
            race_nuke_apply_blast(x, y, owner, -1);
        }
    }
}

static void race_nuke_draw(void)
{
    if (!race_nuke_active_flights) return;
    for (int i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i) {
        const Sc64NukeWorldFlight *f = &race_nuke_flights[i];
        const Sc64NukeFlight *strike = &f->strike;
        if (!strike->phase && !f->impact_tick) continue;
        int x = strike->x - camx, y = strike->y - camy + 12;
        if (f->impact_tick) {
            if (strike->owner == SIDE || currently_visible(strike->x, strike->y)) {
                unsigned tick = f->impact_tick - 1;
                if (tick / SC64_NUKE_BURST_SOURCE_TICKS < SC64_NUKE_BURST_FRAME_COUNT)
                    sc64_nuke_draw_frame(&sc64_nuke_burst_frames[tick / SC64_NUKE_BURST_SOURCE_TICKS],
                                         x, y);
                sc64_nuke_draw_frame(&sc64_nuke_impact_frames[(f->impact_tick - 1) /
                                                             SC64_NUKE_IMPACT_SOURCE_TICKS],
                                     x, y + SC64_NUKE_IMPACT_SOURCE_Y / 2);
            }
            continue;
        }
        if (strike->owner == SIDE || currently_visible(strike->x, strike->y)) {
            if (strike->phase == SC64_NUKE_PAINTING)
                sc64_nuke_draw_frame(&sc64_nuke_dot_frames[(strike->age / SC64_NUKE_DOT_SOURCE_TICKS) %
                                                          SC64_NUKE_DOT_FRAME_COUNT], x, y);
            if (strike->phase == SC64_NUKE_DESCENDING) {
                int height = (strike->y < 160 ? strike->y : 160) -
                    (int)race_nuke_travel(strike->age - strike->cadence.paint);
                sc64_nuke_draw_frame(&sc64_nuke_missile_frames[1], x, y - height);
            }
        }
        if (strike->phase == SC64_NUKE_PAINTING &&
            strike->age < f->ascent_ticks &&
            (strike->owner == SIDE || currently_visible(f->origin_x, f->origin_y))) {
            int height = (int)race_nuke_travel(strike->age);
            sc64_nuke_draw_frame(&sc64_nuke_missile_frames[0],
                                 f->origin_x - camx, f->origin_y - camy + 12 - height);
        }
    }
}
#endif
