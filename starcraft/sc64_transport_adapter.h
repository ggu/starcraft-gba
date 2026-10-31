#ifndef SC64_TRANSPORT_ADAPTER_H
#define SC64_TRANSPORT_ADAPTER_H

#if defined(SC64_T5_RUNTIME) || defined(SC64_P5_SHUTTLE_ENABLED) || defined(SC64_Z6_TRANSPORT_ENABLED) || defined(SC64_SHARED_BUNKER) || defined(SC64_SHARED_TRANSPORT)
#include "sc64_transport_rules.h"

#if defined(SC64_P5_SHUTTLE_ENABLED) && \
    (!defined(SC64_P5_RUNTIME) || !defined(SC64_P5_ACTORS))
#error "P5 Shuttle transport requires the P5 runtime and source actor mapping"
#endif

/* Cargo lives in the existing unit pool: an embarked actor retains its type,
 * HP, owner and supply, with ORDER_TRANSPORTED and target=carrier slot. No
 * extra per-actor or SRAM fields are allocated; the derived activity flag
 * below occupies one word. These are bounded GBA movement and placement
 * policies, not a reproduction of the original order machine. */
enum { SC64_TRANSPORT_BOARD_RADIUS = 16, SC64_TRANSPORT_UNLOAD_TICKS = 19 };
/* Derived each order tick; no cargo bookkeeping is scanned in the ordinary
 * no-transport opening/battle case. This flag is never save-persistent. */
static int race_transport_activity;

#ifdef SC64_Z6_TRANSPORT_ENABLED
static int race_z6_transport_overlord_ready(void);
#endif

#ifdef SC64_Z6_TRANSPORT_ENABLED
static int __attribute__((noinline)) race_transport_carrier_type(const Unit *q)
#else
static int race_transport_carrier_type(const Unit *q)
#endif
{
    if (!q) return 0;
#ifdef SC64_SHARED_TRANSPORT
    {
        int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type);
        if (sc64_transport_unconditional_carrier_raw((unsigned)raw) &&
            sc64_race_unit_from_raw(raw) == q->type) return 1;
        if (race_mode && raw == SC64_TRANSPORT_OVERLORD_RAW && q->type == U_OVERLORD) {
            int owner = sc64_sk_transport_owner_index(q->owner);
            if (owner >= 0 && race_skirmish_transport_tech.owner[owner].researched)
                return 1;
        }
    }
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (race_z6_active() && race_z6_transport_overlord_ready() &&
        q->type==U_OVERLORD && q->raw_type==42 && q->raw_owner==3 && q->owner==SIDE)
        return 1;
#endif
    /* Custom games may spawn a mapped native Dropship without source metadata.
     * A contradictory explicit source identity is never accepted. */
    if (q->type == U_DROPSHIP && race_mode &&
        sc64_race_unit_from_raw(SC64_TRANSPORT_DROPSHIP_RAW) == U_DROPSHIP &&
        (q->raw_type == -1 || q->raw_type == SC64_TRANSPORT_DROPSHIP_RAW))
        return 1;
#ifdef SC64_T5_RUNTIME
    if (q->type == U_DROPSHIP && q->raw_type == SC64_TRANSPORT_DROPSHIP_RAW)
        return 1;
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (race_p5_active() && q->type == U_P5_SHUTTLE &&
        q->raw_type == SC64_TRANSPORT_SHUTTLE_RAW) return 1;
#endif
    return 0;
}

static int race_transport_carrier_valid(int slot)
{
    return slot >= 0 && slot < MAX_UNITS && u[slot].alive &&
           race_transport_carrier_type(&u[slot]) &&
           !SC64_TRANSPORT_LOADED(&u[slot]);
}

static int race_transport_passenger_space(const Unit *q)
{
    unsigned space;
    int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type);
    if ((race_mode
#ifdef SC64_SHARED_TRANSPORT
         || 1
#endif
        ) && (raw < 0 ||
        (sc64_race_unit_from_raw(raw) != q->type &&
         !(raw == 19 && q->type == U_RAYNOR)))) return -1;
    if (!q->alive || (q->flags & 2) || sc64_race_unit_flying(q->type) ||
#ifdef SC64_SHARED_TRANSPORT
        !sc64_transport_shared_space_required_raw((unsigned)raw, &space) ||
#else
        !sc64_transport_space_required_raw((unsigned)raw, &space) ||
#endif
        space == SC64_TRANSPORT_SPACE_UNAVAILABLE) return -1;
    return (int)space;
}

/* Return occupied space, or -1 for a malformed/unsupported carrier. */
static int race_transport_cargo_space(int carrier)
{
    int total = 0;
    if (!race_transport_carrier_valid(carrier)) return -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && SC64_TRANSPORT_LOADED(&u[i]) && u[i].target == carrier) {
            int space = race_transport_passenger_space(&u[i]);
            if (space < 0 || u[i].owner != u[carrier].owner) return -1;
            total += space;
            if (total > SC64_TRANSPORT_DROPSHIP_SPACE) return -1;
        }
    return total;
}

/* Context command for ordinary custom games; existing campaign commands
 * continue to use their mission-specific predicates. */
static int __attribute__((unused)) race_skirmish_transport_menu_available(void)
{
    if (!race_mode || selected_building >= 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].selected && u[i].owner == SIDE &&
            race_transport_carrier_valid(i) &&
            race_transport_cargo_space(i) > 0) return 1;
    return 0;
}

#ifdef SC64_SHARED_TRANSPORT
static int race_shared_campaign_transport_menu_available(void)
{
    if (race_mode || selected_building >= 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].selected && u[i].owner == SIDE &&
            race_transport_carrier_valid(i) && race_transport_cargo_space(i) > 0)
            return 1;
    return 0;
}
#endif

#ifdef SC64_P5_SHUTTLE_ENABLED
static int race_p5_shuttle_menu_available(void)
{
    if (!race_p5_active() || selected_building >= 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].selected && race_transport_carrier_valid(i) &&
            u[i].type == U_P5_SHUTTLE && race_transport_cargo_space(i) > 0)
            return 1;
    return 0;
}
#endif

#ifdef SC64_Z6_TRANSPORT_ENABLED
static int race_z6_overlord_menu_available(void)
{
    if (!race_z6_active() || selected_building >= 0) return 0;
    for (int i=0; i<MAX_UNITS; ++i)
        if (u[i].selected && race_transport_carrier_valid(i) &&
            u[i].type==U_OVERLORD && race_transport_cargo_space(i)>0) return 1;
    return 0;
}
#endif

/* A transition invalidates the short-lived indexes: newly unloaded units
 * must be targetable immediately, and newly loaded units must disappear.
 * The existing exact scan fallbacks handle the remainder of this tick. */
static void race_transport_changed(void)
{
#ifdef SC64_CONTACT_MEMO_ENABLED
    SC64_CONTACT_MEMO_INVALIDATE();
#endif

    sc64_unit_bucket_active = 0;
    sc64_target_index_active = 0;
    target_lists_valid = 0;
}

static int race_transport_request_board(int passenger, int carrier)
{
    int used, space;
    if (passenger < 0 || passenger >= MAX_UNITS || passenger == carrier ||
        !race_transport_carrier_valid(carrier) ||
        !u[passenger].alive || SC64_TRANSPORT_LOADED(&u[passenger]) ||
        u[passenger].owner != u[carrier].owner) return 0;
    used = race_transport_cargo_space(carrier);
    space = race_transport_passenger_space(&u[passenger]);
    if (used < 0 || space < 0 || used + space > SC64_TRANSPORT_DROPSHIP_SPACE)
        return 0;
    u[passenger].order = ORDER_BOARD;
    u[passenger].target = carrier;
    u[passenger].path_timer = u[passenger].path_retry = 0;
    return 1;
}

static int race_transport_request_unload(int carrier, int x, int y)
{
    if (!race_transport_carrier_valid(carrier) ||
        race_transport_cargo_space(carrier) <= 0 ||
        x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return 0;
    u[carrier].order = ORDER_UNLOAD;
    u[carrier].target = -1;
    u[carrier].tx = x; u[carrier].ty = y;
    u[carrier].work_timer = 0;
    return 1;
}

static void race_transport_destroy_cargo(int carrier)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].target != carrier) continue;
        /* A pending passenger is still alive in the world. Cancel its
         * reference before the carrier slot can be saved or reused. */
        if (u[i].order == ORDER_BOARD) {
            u[i].order = ORDER_IDLE; u[i].target = -1;
            u[i].path_timer = u[i].path_retry = 0;
            continue;
        }
        if (SC64_TRANSPORT_LOADED(&u[i])) {
#ifdef SC64_P5_SHUTTLE_ENABLED
            /* Destroying a carrier kills its passengers in this authored
             * runtime. Count those deaths before clearing their source owner. */
            if (race_p5_active()) race_p5_note_unit_death(&u[i]);
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
            if (race_z6_active()) race_z6_note_unit_death(&u[i]);
#endif
#ifdef SC64_SCARAB_HUMAN_CONTROL_PROBE
            if (sc64_race_raw_unit(u[i].type)==83) race_scarab_human_parent_lost(i);
#endif
            if (u[i].owner == SIDE) supply_used -= unit_supply(u[i].type);
            u[i].hp = 0; u[i].alive = u[i].selected = 0;
            u[i].order = ORDER_IDLE; u[i].target = -1;
        }
    }
    race_transport_changed();
}

static int race_transport_land_point(int passenger, int x, int y, int *px, int *py)
{
    /* Test the center, then nearby rings. Placement never turns blocked
     * terrain into floor and never overlaps another ground actor/building. */
    for (int radius = 0; radius <= 16; radius += 8)
        for (int dy = -radius; dy <= radius; dy += 8)
            for (int dx = -radius; dx <= radius; dx += 8) {
                if (radius && FAST_ABS(dx) != radius && FAST_ABS(dy) != radius)
                    continue;
                int nx = x + dx, ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= WORLD_W || ny >= WORLD_H ||
                    !is_traversable(nx / 16, ny / 16, -1) ||
                    !unit_step_clear(&u[passenger], nx, ny)) continue;
                *px = nx; *py = ny; return 1;
            }
    return 0;
}

#ifdef SC64_SHARED_BUNKER
static void race_bunker_fire_unit(int unit, int slot);
#include "sc64_bunker_adapter.h"
#endif

/* Handle only transport orders; return 1 when the ordinary order loop must
 * leave this actor alone. Other units retain the existing engine behavior. */
static int __attribute__((noinline)) race_transport_tick_unit(int slot)
{
    Unit *q = &u[slot];
    if (SC64_TRANSPORT_LOADED(q)) {
#ifdef SC64_SHARED_BUNKER
        if (q->target>=MAX_UNITS) {
            if (q->cd>0) --q->cd;
            return race_bunker_tick_loaded(slot);
        }
#endif
        return 1;
    }
#ifdef SC64_SHARED_BUNKER
    if (q->order==ORDER_BOARD && q->target>=MAX_UNITS)
        return race_bunker_tick_board(slot);
#endif
    if (q->order == ORDER_BOARD) {
        int carrier = q->target, space = race_transport_passenger_space(q);
        int used = race_transport_cargo_space(carrier);
        if (!race_transport_carrier_valid(carrier) || space < 0 || used < 0 ||
            q->owner != u[carrier].owner ||
            used + space > SC64_TRANSPORT_DROPSHIP_SPACE) {
            q->order = ORDER_IDLE; q->target = -1; return 1;
        }
        if (dist2(q->x, q->y, u[carrier].x, u[carrier].y) >
            SC64_TRANSPORT_BOARD_RADIUS * SC64_TRANSPORT_BOARD_RADIUS) {
            move_toward(q, u[carrier].x, u[carrier].y);
        } else {
            q->order = ORDER_TRANSPORTED; q->selected = 0;
            q->x = u[carrier].x; q->y = u[carrier].y;
            q->path_timer = q->path_retry = 0;
            race_transport_changed();
        }
        return 1;
    }
    if (q->order != ORDER_UNLOAD) return 0;
    if (!race_transport_carrier_valid(slot) || race_transport_cargo_space(slot) <= 0) {
        q->order = ORDER_IDLE; q->target = -1; return 1;
    }
    if (dist2(q->x, q->y, q->tx, q->ty) > 4 * 4) {
        move_toward(q, q->tx, q->ty); return 1;
    }
    if (q->work_timer > 0) --q->work_timer;
    return 1;
}

static void race_transport_try_unload(int slot)
{
    Unit *q = &u[slot];
    if (!race_transport_carrier_valid(slot) || q->order != ORDER_UNLOAD ||
        q->work_timer || dist2(q->x, q->y, q->tx, q->ty) > 4 * 4) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && SC64_TRANSPORT_LOADED(&u[i]) && u[i].target == slot) {
            int x, y;
            if (!race_transport_land_point(i, q->x, q->y, &x, &y)) return;
            u[i].x = x; u[i].y = y;
            u[i].order = ORDER_IDLE; u[i].target = -1;
            u[i].path_timer = u[i].path_retry = 0;
            if (u[i].cd < SC64_TRANSPORT_UNLOAD_TICKS)
                u[i].cd = SC64_TRANSPORT_UNLOAD_TICKS;
            q->work_timer = SC64_TRANSPORT_UNLOAD_TICKS;
            race_transport_changed();
            return;
        }
}

static void race_transport_tick_after_orders(void)
{
    if (!race_transport_activity) return;
    /* Disembark after every actor has ticked, making the passenger's weapon
     * cooldown independent of whether its slot precedes the carrier's. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].order == ORDER_UNLOAD) race_transport_try_unload(i);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && SC64_TRANSPORT_LOADED(&u[i])) {
            int carrier = u[i].target;
#ifdef SC64_SHARED_BUNKER
            if (carrier>=MAX_UNITS) {
                int slot=sc64_bunker_target_decode(MAX_UNITS,MAX_BUILDINGS,carrier);
                if (slot<0) race_bunker_kill_passenger(i,-1);
                else if (!race_bunker_carrier_valid(slot) ||
                    !sc64_bunker_owner_matches(u[i].owner,u[i].raw_owner,
                        b[slot].owner,b[slot].raw_owner,race_mode,SIDE))
                    race_bunker_destroy_cargo(slot,-1);
                else {u[i].x=b[slot].x;u[i].y=b[slot].y;}
                continue;
            }
#endif
            if (!race_transport_carrier_valid(carrier) ||
                u[i].owner != u[carrier].owner) {
                /* Includes carriers removed by scenario effects or direct
                 * damage, before the campaign's hero/outcome queries. */
                race_transport_destroy_cargo(carrier);
            } else {
                u[i].x = u[carrier].x; u[i].y = u[carrier].y;
            }
        }
}

#endif
#endif
