/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef COMMAND_RING_H
#define COMMAND_RING_H

/* Hold-B command ring for the default profile (SC_COMMAND_RING).
 *
 * Aim the cursor first, hold B, steer with the D-pad and release B to act at
 * the cursor. Directions that lead to a submenu open it immediately, so
 * "hold B, left, up, release" picks Build > Depot. L backs out of a submenu;
 * A queues one more unit or upgrade while a training slot is highlighted. A
 * short tap of B without a direction still opens the classic command list,
 * and the simulation is paused while B is held, as it is for that list. The
 * ring only calls the commands the list already uses, so orders, costs and
 * rules are unchanged. */

#ifdef SC64_RACES
#error "SC_COMMAND_RING currently covers the default Terran command set only"
#endif

#define RING_SHOW_FRAMES 6  /* draw the ring after 0.2 s; quick flicks stay hidden */
#define RING_TAP_FRAMES 8   /* a shorter press with no direction opens the list */

enum { RING_ROOT, RING_BUILD, RING_TECH };
enum {
    RING_NONE, RING_MOVE, RING_ATTACK, RING_STOP, RING_SUB,
    RING_PLACE, RING_TRAIN, RING_RESEARCH
};

typedef struct {
    u8 kind, arg;
    const char *label;
} RingSlot;

static u8 ring_active, ring_frames, ring_page, ring_used;
static s8 ring_slot = -1;

/* Slots are up, right, down, left. */
static RingSlot
ring_entry(int page, int slot)
{
    static const RingSlot none = { RING_NONE, 0, 0 };
    static const RingSlot orders[4] = {
        { RING_MOVE, 0, "MOVE" }, { RING_ATTACK, 0, "ATTACK" },
        { RING_STOP, 0, "STOP" }, { RING_SUB, RING_BUILD, "BUILD" }
    };
    static const RingSlot build[4] = {
        { RING_PLACE, B_DEPOT, "DEPOT" }, { RING_PLACE, B_BARRACKS, "BARRACKS" },
        { RING_PLACE, B_REFINERY, "REFINERY" }, { RING_SUB, RING_TECH, "TECH" }
    };
    static const RingSlot tech[4] = {
        { RING_PLACE, B_ACADEMY, "ACADEMY" }, { RING_PLACE, B_ENG_BAY, "ENG BAY" },
        { RING_NONE, 0, 0 }, { RING_NONE, 0, 0 }
    };
    static const RingSlot barracks[4] = {
        { RING_TRAIN, U_MARINE, "MARINE" }, { RING_TRAIN, U_FIREBAT, "FIREBAT" },
        { RING_TRAIN, U_MEDIC, "MEDIC" }, { RING_NONE, 0, 0 }
    };
    static const RingSlot command_center = { RING_TRAIN, U_SCV, "SCV" };
    static const RingSlot upgrades[2] = {
        { RING_RESEARCH, 0, "WEAPONS" }, { RING_RESEARCH, 1, "ARMOR" }
    };
    if (slot < 0 || slot > 3) return none;
    if (page == RING_BUILD) return build[slot];
    if (page == RING_TECH) return tech[slot];
    if (selected_building >= 0) {
        const Building *q = &b[selected_building];
        if (!q->alive || q->under_construction) return none;
        if (q->type == B_BARRACKS) return barracks[slot];
        if (q->type == B_COMMAND) return slot == 0 ? command_center : none;
        if (q->type == B_ENG_BAY) return slot < 2 ? upgrades[slot] : none;
        return none;
    }
    if (!selected_count()) return none;
    /* Only workers can open the build pages. */
    if (slot == 3 && find_selected_worker() < 0) return none;
    return orders[slot];
}

static const char *
ring_title(void)
{
    if (ring_page == RING_BUILD) return "BUILD";
    if (ring_page == RING_TECH) return "BUILD TECH";
    if (selected_building >= 0) {
        int type = b[selected_building].type;
        return type == B_BARRACKS ? "BARRACKS" : type == B_COMMAND ? "COMMAND CENTER" :
            type == B_ENG_BAY ? "ENGINEERING BAY" : "NO COMMANDS";
    }
    return selected_count() ? "ORDERS" : "NOTHING SELECTED";
}

static void
ring_execute(RingSlot e)
{
    switch (e.kind) {
    case RING_MOVE:
        issue_order(ORDER_MOVE, cx, cy);
        toast(0);
        break;
    case RING_ATTACK:
        issue_order(ORDER_ATTACK_MOVE, cx, cy);
        toast(0);
        break;
    case RING_STOP:
        stop_selected();
        break;
    case RING_PLACE:
        /* The usual placement preview opens at the cursor; A confirms. */
        mode = MODE_BUILD;
        menu = e.arg;
        break;
    case RING_TRAIN:
        if (e.arg != U_SCV) train_type = e.arg;
        queue_unit(selected_building, e.arg);
        break;
    case RING_RESEARCH:
        start_research(e.arg);
        break;
    default:
        break;
    }
}

/* Returns 1 when the ring owns this frame's input; the simulation then waits,
 * as it does while the command list is open. */
static int
command_ring_input(u16 held, u16 pressed)
{
    static const u16 dirs[4] = { KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT };
    if (!ring_active) {
        if (!(pressed & KEY_B) || (pressed & KEY_L) || box_state || menu_open ||
            help || mode != MODE_NORMAL) return 0;
        ring_active = 1;
        ring_frames = 0;
        ring_page = RING_ROOT;
        ring_slot = -1;
        ring_used = 0;
        reset_selection_gestures();
        return 1;
    }
    reset_selection_gestures();
    if (ring_frames < 255) ++ring_frames;
    for (int d = 0; d < 4; ++d) {
        if (!(pressed & dirs[d])) continue;
        RingSlot e = ring_entry(ring_page, d);
        ring_used = 1;
        if (e.kind == RING_SUB) {
            ring_page = e.arg;
            ring_slot = -1;
        } else
            ring_slot = (s8)(e.kind == RING_NONE ? -1 : d);
    }
    if ((pressed & KEY_L) && ring_page != RING_ROOT) {
        ring_page = ring_page == RING_TECH ? RING_BUILD : RING_ROOT;
        ring_slot = -1;
        ring_used = 1;
    }
    if (pressed & KEY_A) {
        RingSlot e = ring_entry(ring_page, ring_slot);
        ring_used = 1;
        if (e.kind == RING_TRAIN || e.kind == RING_RESEARCH) ring_execute(e);
    }
    if (!(held & KEY_B)) {
        RingSlot e = ring_entry(ring_page, ring_slot);
        ring_active = 0;
        if (e.kind != RING_NONE)
            ring_execute(e);
        else if (!ring_used && ring_frames < RING_TAP_FRAMES) {
            menu_open = 1;
            menu_cursor = 0;
        }
    }
    return 1;
}

static int
command_ring_visible(void)
{
    return ring_active && ring_frames >= RING_SHOW_FRAMES;
}

static void
ring_draw_slot(int x, int y, int slot, RingSlot e)
{
    int cost = 0, gas_cost = 0, w, len = 0;
    const char *t;
    if (e.kind == RING_NONE) return;
    for (t = e.label; *t; ++t) ++len;
    if (e.kind == RING_PLACE) { cost = building_cost(e.arg); gas_cost = building_gas_cost(e.arg); }
    if (e.kind == RING_TRAIN) { cost = unit_cost(e.arg); gas_cost = unit_gas_cost(e.arg); }
    if (e.kind == RING_RESEARCH) cost = gas_cost = 100;
    w = len * 6 + (e.kind == RING_SUB ? 6 : 0) + 5;
    if (w < 38) w = 38;
    int h = cost ? 20 : 11;
    int bx = slot == 1 ? x + 12 : slot == 3 ? x - 12 - w : x - w / 2;
    int by = slot == 0 ? y - 12 - h : slot == 2 ? y + 12 : y - h / 2;
    int on = slot == ring_slot;
    gfx_rect(bx, by, w, h, C_PANEL);
    gfx_frame(bx, by, w, h, on ? C_GOLD : C_EDGE);
    gfx_text(bx + 3, by + 2, e.label, on ? C_WHITE : C_DIM);
    if (e.kind == RING_SUB) gfx_text(bx + 3 + len * 6, by + 2, ">", C_GOLD);
    if (cost) {
        gfx_num(bx + 3, by + 11, cost, minerals >= cost ? C_CYAN : C_RED);
        if (gas_cost) gfx_num(bx + 23, by + 11, gas_cost, gas >= gas_cost ? C_GREEN : C_RED);
    }
}

/* x, y: cursor screen position. The ring keeps clear of the screen edges. */
static void
command_ring_draw(int x, int y)
{
    if (!command_ring_visible()) return;
    x = limit(x, 68, 172);
    y = limit(y, 46, 86);
    gfx_frame(x - 3, y - 3, 7, 7, C_GOLD);
    for (int d = 0; d < 4; ++d) ring_draw_slot(x, y, d, ring_entry(ring_page, d));
}

/* Replaces the command panel text while the ring is shown. */
static int
command_ring_draw_hud(void)
{
    if (!command_ring_visible()) return 0;
    gfx_text(5, 126, ring_title(), C_WHITE);
    RingSlot e = ring_entry(ring_page, ring_slot);
    gfx_text(5, 137, e.kind == RING_TRAIN || e.kind == RING_RESEARCH ?
             "RELEASE B DO  A +1 MORE" :
             ring_page != RING_ROOT ? "RELEASE B DO  L BACK" :
             "D-PAD PICK  RELEASE B DO", C_GOLD);
    return 1;
}
#endif
