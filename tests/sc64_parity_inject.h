/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Parity command injector: semantic verbs for host parity scenarios.
 *
 * Include this header AFTER "../starcraft/game.c" and "sc64_route_purity.h".
 * Host test code only; nothing here is compiled into a ROM.
 *
 * Each verb drives the engine through the function the product's own command
 * path calls (spawn_unit, issue_order, stop_selected, the worker gather
 * assignment, race_queue_unit, race_start_build and, where the profile has
 * it, sc64_direct_attack_issue_target). No cursor, camera or menu state is
 * used. The order functions act on the selected human units and spread a
 * group into a formation, so a verb selects exactly one unit, issues the
 * command and restores the previous selection, the selected building and the
 * toast line the command shows.
 *
 * A verb returns 0 on success and -1 on a setup failure. A failure is counted
 * and its first message kept in sc64_pi_status; scenarios must treat any
 * failure as fatal, never as a skipped step. The product refusing a command
 * (no money, no supply, a bad site, a full queue, a missing prerequisite) is
 * a setup failure too: the scenario asked for something the game would not do.
 *
 * Every verb checks route_purity_hash() before and after: a command must not
 * build, age or evict a route field (tests/sc64_route_purity.h explains why).
 * One exception is the engine's own: spawn_unit() retires range-route work
 * that still names the slot it reuses (sc64_range_route_forget_target). The
 * spawn verb predicts that change exactly: inside a route-pure scope it runs
 * the same retirement for the slot spawn_unit() will take, hashes the result
 * and restores. The real spawn must take that slot and leave route state
 * either unchanged or equal to the prediction; a retirement is counted in
 * sc64_pi_status.route_retirements.
 *
 * Verbs and per-profile support:
 *   sc64_pi_spawn(owner, type, x, y)          any profile
 *   sc64_pi_move(slot, x, y)                  human-owned units only
 *   sc64_pi_attack_move(slot, x, y)           human-owned units only
 *   sc64_pi_attack(slot, target)              SC64_DIRECT_ATTACK: the direct
 *       target order. Without it the product has no targeted order, and the
 *       verb issues the product's own fallback, an attack-move to the target's
 *       position; sc64_pi_attack_mode() says which one a build uses.
 *   sc64_pi_stop(slot)                        any profile
 *   sc64_pi_harvest_minerals(slot, patch)     human-owned workers
 *   sc64_pi_harvest_gas(slot, refinery)       human-owned workers
 *   sc64_pi_group_move(slots, n, x, y)        human-owned units; one product
 *       group order: all n units selected at once, so issue_order() spreads
 *       them into its formation, as a player's box selection does
 *   sc64_pi_train(owner, producer, type)      SC64_RACES only
 *   sc64_pi_build(owner, worker, type, x, y)  SC64_RACES only
 * Orders for computer-owned units are not supported: their command path is
 * the AI's own, and the human order functions ignore them.
 */
#ifndef SC64_PARITY_INJECT_H
#define SC64_PARITY_INJECT_H
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef SC64_ROUTE_PURITY_H
#error "include tests/sc64_route_purity.h before sc64_parity_inject.h"
#endif

typedef struct {
    unsigned issued, failures, route_checks, dispatch_checks, route_retirements;
    char first_failure[192];
} Sc64ParityInjectStatus;
static Sc64ParityInjectStatus sc64_pi_status;

/* Called after each verb, success or failure, when a scenario wants a log. */
static void (*sc64_pi_on_verb)(const char *verb, int result, const char *detail);

static int sc64_pi_fail(const char *verb, const char *format, ...)
{
    char detail[160];
    va_list args;
    va_start(args, format);
    vsnprintf(detail, sizeof(detail), format, args);
    va_end(args);
    if (!sc64_pi_status.failures++)
        snprintf(sc64_pi_status.first_failure, sizeof(sc64_pi_status.first_failure),
                 "%s: %s", verb, detail);
    fprintf(stderr, "parity setup failure: %s: %s\n", verb, detail);
    if (sc64_pi_on_verb) sc64_pi_on_verb(verb, -1, detail);
    return -1;
}

static int sc64_pi_ok(const char *verb, const char *format, ...)
{
    char detail[160];
    va_list args;
    va_start(args, format);
    vsnprintf(detail, sizeof(detail), format, args);
    va_end(args);
    ++sc64_pi_status.issued;
    if (sc64_pi_on_verb) sc64_pi_on_verb(verb, 0, detail);
    return 0;
}

/* The route state the next verb must find. The runner sets it before a
 * scenario dispatch; only a spawn's verified retirement advances it. Every
 * verb checks it on entry, so scenario code between verbs is held too. */
static uint64_t sc64_pi_route_expect;
static int sc64_pi_route_expect_set;

static void sc64_pi_route_expect_begin(void)
{
    sc64_pi_route_expect = route_purity_hash();
    sc64_pi_route_expect_set = 1;
}

/* Called after a dispatch: the state must be what the verbs left. */
static void sc64_pi_route_expect_end(const char *where)
{
    ++sc64_pi_status.dispatch_checks;
    if (sc64_pi_route_expect_set && route_purity_hash() != sc64_pi_route_expect) {
        fprintf(stderr, "parity injector: route state changed %s\n", where);
        abort();
    }
    sc64_pi_route_expect_set = 0;
}

static void sc64_pi_route_entry(uint64_t now, const char *verb)
{
    if (sc64_pi_route_expect_set && now != sc64_pi_route_expect) {
        fprintf(stderr, "parity injector: route state changed before %s\n", verb);
        abort();
    }
}

static void sc64_pi_route_check(uint64_t before, const char *verb)
{
    ++sc64_pi_status.route_checks;
    sc64_pi_route_entry(before, verb);
    if (route_purity_hash() != before) {
        fprintf(stderr, "parity injector: %s changed route state\n", verb);
        abort();
    }
}

/* Selection and the UI state a command writes as a side effect. */
typedef struct {
    u8 selected[MAX_UNITS];
    int selected_building, toast_timer, toast_id, toast_advisor, mode, menu_open, menu_cursor;
} Sc64ParitySelection;

static void sc64_pi_select_only(Sc64ParitySelection *saved, int slot)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        saved->selected[i] = (u8)(u[i].selected != 0);
        u[i].selected = 0;
    }
    saved->selected_building = selected_building;
    saved->toast_timer = toast_timer;
    saved->toast_id = toast_id;
    saved->toast_advisor = toast_advisor;
    /* stop_selected() also leaves placement mode and closes the command menu. */
    saved->mode = mode;
    saved->menu_open = menu_open;
    saved->menu_cursor = menu_cursor;
    u[slot].selected = 1;
}

static void sc64_pi_restore(const Sc64ParitySelection *saved)
{
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = saved->selected[i];
    selected_building = saved->selected_building;
    toast_timer = saved->toast_timer;
    toast_id = saved->toast_id;
    toast_advisor = (u8)saved->toast_advisor;
    mode = saved->mode;
    menu_open = saved->menu_open;
    menu_cursor = saved->menu_cursor;
}

static int sc64_pi_live_unit(int slot)
{
    return slot >= 0 && slot < MAX_UNITS && u[slot].alive;
}

static int sc64_pi_check_commandable(const char *verb, int slot)
{
    if (!sc64_pi_live_unit(slot)) return sc64_pi_fail(verb, "slot %d is not a live unit", slot);
    if (u[slot].owner != SIDE)
        return sc64_pi_fail(verb, "slot %d is owned by %d; only human units take orders", slot, u[slot].owner);
    if (SC64_TRANSPORT_LOADED(&u[slot])) return sc64_pi_fail(verb, "slot %d is loaded in a transport", slot);
    return 0;
}

/* The route hash after spawn_unit() retires the range work of the slot it
 * will take (find_unit_slot), computed in a route-pure scope and undone.
 * *slot gets that slot. */
static uint64_t sc64_pi_predict_retirement(int *slot)
{
    uint64_t retired = route_purity_hash();
    *slot = find_unit_slot();
#ifdef SC64_RANGE_WORK
    if (*slot >= 0) {
        ROUTE_PURE_BEGIN();
        sc64_range_route_forget_target(*slot);
        retired = route_purity_hash();
        ROUTE_PURE_END();
    }
#endif
    return retired;
}

/* After a command that may create one unit through spawn_unit(): route state
 * must be unchanged or exactly the predicted retirement. */
static void sc64_pi_check_birth(const char *verb, uint64_t route, uint64_t retired, int born, int predicted)
{
    uint64_t now = route_purity_hash();
    ++sc64_pi_status.route_checks;
    if (now != route && (now != retired || born != predicted)) {
        fprintf(stderr, "parity injector: %s changed route state beyond retiring slot %d's range work "
                "(new unit in slot %d)\n", verb, predicted, born);
        abort();
    }
    if (now != route) ++sc64_pi_status.route_retirements;
    if (sc64_pi_route_expect_set) sc64_pi_route_expect = now;
}

static int sc64_pi_spawn(int owner, int type, int x, int y)
{
    uint64_t route = route_purity_hash(), retired;
    int slot, predicted;
    if (owner < 0 || owner > 4) return sc64_pi_fail("spawn", "owner %d out of range", owner);
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H)
        return sc64_pi_fail("spawn", "position %d,%d is off the map", x, y);
#ifdef SC64_RACES
    if (!sc64_race_unit_rule(type)) return sc64_pi_fail("spawn", "type %d has no unit rule", type);
#endif
    sc64_pi_route_entry(route, "spawn");
    retired = sc64_pi_predict_retirement(&predicted);
    slot = spawn_unit(owner, type, x, y);
    if (slot != predicted) {
        fprintf(stderr, "parity injector: spawn took slot %d, not the free slot %d\n", slot, predicted);
        abort();
    }
    sc64_pi_check_birth("spawn", route, retired, slot, predicted);
    if (slot < 0) return sc64_pi_fail("spawn", "no free unit slot for type %d", type);
    sc64_pi_ok("spawn", "slot=%d owner=%d type=%d x=%d y=%d", slot, owner, type, x, y);
    return slot;
}

static int sc64_pi_order(const char *verb, int slot, int order, int x, int y)
{
    Sc64ParitySelection saved;
    uint64_t route = route_purity_hash();
    int issued;
    if (sc64_pi_check_commandable(verb, slot)) return -1;
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H)
        return sc64_pi_fail(verb, "point %d,%d is off the map", x, y);
    sc64_pi_select_only(&saved, slot);
    issued = issue_order(order, x, y);
    sc64_pi_restore(&saved);
    sc64_pi_route_check(route, verb);
    if (issued != 1) return sc64_pi_fail(verb, "slot %d refused the order (%d issued)", slot, issued);
    return sc64_pi_ok(verb, "slot=%d x=%d y=%d", slot, x, y);
}

static int sc64_pi_move(int slot, int x, int y)
{
    return sc64_pi_order("move", slot, ORDER_MOVE, x, y);
}

/* Select exactly the given units (none twice), saving what the command may change. */
static int sc64_pi_select_group(const char *verb, Sc64ParitySelection *saved, const int *slots, int n)
{
    if (n < 1 || n > MAX_UNITS) return sc64_pi_fail(verb, "group of %d units", n);
    for (int k = 0; k < n; ++k) {
        if (sc64_pi_check_commandable(verb, slots[k])) return -1;
        for (int j = 0; j < k; ++j)
            if (slots[j] == slots[k]) return sc64_pi_fail(verb, "slot %d listed twice", slots[k]);
    }
    sc64_pi_select_only(saved, slots[0]);
    for (int k = 1; k < n; ++k) u[slots[k]].selected = 1;
    return 0;
}

static int sc64_pi_group_move(const int *slots, int n, int x, int y)
{
    Sc64ParitySelection saved;
    uint64_t route = route_purity_hash();
    int issued;
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H)
        return sc64_pi_fail("group_move", "point %d,%d is off the map", x, y);
    if (sc64_pi_select_group("group_move", &saved, slots, n)) return -1;
    issued = issue_order(ORDER_MOVE, x, y);
    sc64_pi_restore(&saved);
    sc64_pi_route_check(route, "group_move");
    if (issued != n) return sc64_pi_fail("group_move", "%d of %d units took the order", issued, n);
    return sc64_pi_ok("group_move", "n=%d x=%d y=%d", n, x, y);
}

static int sc64_pi_attack_move(int slot, int x, int y)
{
    return sc64_pi_order("attack_move", slot, ORDER_ATTACK_MOVE, x, y);
}

/* "direct" or "attack_move_fallback". */
static const char *sc64_pi_attack_mode(void)
{
#ifdef SC64_DIRECT_ATTACK
    return "direct";
#else
    return "attack_move_fallback";
#endif
}

/* target is a unit slot, or MAX_UNITS + a building slot. */
static int sc64_pi_attack(int slot, int target)
{
    int alive;
    if (target >= 0 && target < MAX_UNITS) alive = u[target].alive;
    else if (target >= MAX_UNITS && target < MAX_UNITS + MAX_BUILDINGS) alive = b[target - MAX_UNITS].alive;
    else return sc64_pi_fail("attack", "target %d out of range", target);
    if (!alive) return sc64_pi_fail("attack", "target %d is not alive", target);
#ifdef SC64_DIRECT_ATTACK
    {
        Sc64ParitySelection saved;
        uint64_t route = route_purity_hash();
        int issued;
        if (sc64_pi_check_commandable("attack", slot)) return -1;
        sc64_pi_select_only(&saved, slot);
        issued = sc64_direct_attack_issue_target(target);
        sc64_pi_restore(&saved);
        sc64_pi_route_check(route, "attack");
        if (issued != 1) return sc64_pi_fail("attack", "slot %d cannot attack target %d", slot, target);
        return sc64_pi_ok("attack", "slot=%d target=%d mode=direct", slot, target);
    }
#else
    return sc64_pi_order("attack", slot, ORDER_ATTACK_MOVE, target_x(target), target_y(target));
#endif
}

static int sc64_pi_stop(int slot)
{
    Sc64ParitySelection saved;
    uint64_t route = route_purity_hash();
    if (sc64_pi_check_commandable("stop", slot)) return -1;
    sc64_pi_select_only(&saved, slot);
    stop_selected();
    sc64_pi_restore(&saved);
    sc64_pi_route_check(route, "stop");
    if (u[slot].order != ORDER_IDLE
#ifdef SC64_EGG_RUNTIME
        && u[slot].type != SC64_EGG_ACTOR
#endif
        ) return sc64_pi_fail("stop", "slot %d kept order %d", slot, u[slot].order);
    return sc64_pi_ok("stop", "slot=%d", slot);
}

/* The assignment the product's A-on-minerals command makes for one worker. */
static int sc64_pi_harvest_minerals(int slot, int patch)
{
    Sc64ParitySelection saved;
    uint64_t route = route_purity_hash();
    int issued;
    if (sc64_pi_check_commandable("harvest_minerals", slot)) return -1;
    if (!IS_WORKER(u[slot].type)) return sc64_pi_fail("harvest_minerals", "slot %d is not a worker", slot);
    if (patch < 0 || patch >= MAX_MINERALS || m[patch].amount <= 0)
        return sc64_pi_fail("harvest_minerals", "patch %d is empty or out of range", patch);
    sc64_pi_select_only(&saved, slot);
#ifdef SC64_WORKER_CARGO_KIND
    issued = sc64_worker_mineral_issue_selected(patch);
#else
    issued = 0;
    if (!u[slot].carry) {
        u[slot].order = ORDER_MINE;
        u[slot].target = patch;
        u[slot].work_timer = 0;
        u[slot].path_timer = u[slot].path_retry = 0;
#ifdef SC64_NEAREST_RESOURCE_DEPOT
        u[slot].build = -1;
#endif
        issued = 1;
    }
#endif
    sc64_pi_restore(&saved);
    sc64_pi_route_check(route, "harvest_minerals");
    if (issued != 1) return sc64_pi_fail("harvest_minerals", "slot %d refused patch %d", slot, patch);
    return sc64_pi_ok("harvest_minerals", "slot=%d patch=%d", slot, patch);
}

/* The assignment the product's A-on-refinery command makes. */
static int sc64_pi_harvest_gas(int slot, int refinery)
{
    uint64_t route = route_purity_hash();
    if (sc64_pi_check_commandable("harvest_gas", slot)) return -1;
    if (!IS_WORKER(u[slot].type)) return sc64_pi_fail("harvest_gas", "slot %d is not a worker", slot);
    if (!building_complete(refinery) || b[refinery].owner != SIDE)
        return sc64_pi_fail("harvest_gas", "building %d is not a finished human building", refinery);
    if (nearest_refinery(b[refinery].x, b[refinery].y) != refinery)
        return sc64_pi_fail("harvest_gas", "building %d is not a usable refinery", refinery);
    sc64_worker_reset_repair_for_gather(&u[slot]);
    u[slot].work_timer = 0;
    u[slot].order = ORDER_GAS;
    u[slot].target = refinery;
#ifdef SC64_WORKER_CARGO_KIND
    sc64_worker_gather_pending_set(&u[slot], 1);
#else
    u[slot].carry = 0;
#endif
#ifdef SC64_NEAREST_RESOURCE_DEPOT
    u[slot].build = -1;
#endif
    sc64_pi_route_check(route, "harvest_gas");
    return sc64_pi_ok("harvest_gas", "slot=%d refinery=%d", slot, refinery);
}

#ifdef SC64_RACES
static int sc64_pi_train(int owner, int producer, int type)
{
    uint64_t route = route_purity_hash(), retired;
    int before, queued, predicted, born = -1;
    if (producer < 0 || producer >= MAX_BUILDINGS || !b[producer].alive)
        return sc64_pi_fail("train", "producer %d is not a live building", producer);
    before = b[producer].queue_count;
    sc64_pi_route_entry(route, "train");
    /* In Egg profiles race_queue_unit() can create the Egg at once through
     * spawn_unit(), which retires the reused slot's range work like a spawn. */
    retired = sc64_pi_predict_retirement(&predicted);
    queued = race_queue_unit(owner, producer, type);
    if (predicted >= 0 && u[predicted].alive) born = predicted;
    sc64_pi_check_birth("train", route, retired, born, predicted);
    if (!queued)
        return sc64_pi_fail("train", "owner %d producer %d refused type %d (minerals=%d gas=%d queue=%d)",
                            owner, producer, type, race_owner_minerals(owner), race_owner_gas(owner), before);
    return sc64_pi_ok("train", "owner=%d producer=%d type=%d", owner, producer, type);
}

static int sc64_pi_build(int owner, int worker, int type, int x, int y)
{
    uint64_t route = route_purity_hash();
    int started;
    if (!sc64_pi_live_unit(worker)) return sc64_pi_fail("build", "worker %d is not a live unit", worker);
    started = race_start_build(owner, worker, type, x, y);
    sc64_pi_route_check(route, "build");
    if (!started)
        return sc64_pi_fail("build", "owner %d worker %d refused type %d at %d,%d (minerals=%d gas=%d)",
                            owner, worker, type, x, y, race_owner_minerals(owner), race_owner_gas(owner));
    return sc64_pi_ok("build", "owner=%d worker=%d type=%d x=%d y=%d", owner, worker, type, x, y);
}
#endif

#endif
