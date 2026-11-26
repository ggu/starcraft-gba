/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_COMMAND_DELAY_H
#define SC64_COMMAND_DELAY_H

/* Player command latency for the ROM-derived profiles. StarCraft 64 changes
 * a unit's order 4 source frames after the player's Move press (T1 parity
 * captures: the order byte turns to Move 4 frames after the press on all
 * five Marines, in every capture). The engine applied the order on the next
 * update. A player Move or attack-move is now held SC64_COMMAND_DELAY_UPDATES
 * updates (3.2 source frames, so the order changes on the fourth frame)
 * with the selection it was given, then issued through issue_order().
 *
 * Bounded state: two pending commands, one selection bit per unit slot and
 * one order hash per slot. A third command while two wait applies the
 * oldest at once; commands run in the order given. A newer immediate player
 * command for a unit (see sc64_command_delay_input_end) removes it from the
 * pending ones, and a slot taken by a new unit leaves them too. save_game() applies
 * pending commands once the save will go ahead, so no save holds one; a
 * successful load or a new world drops them. Units that die or are loaded
 * meanwhile drop out, as issue_order() skips them.
 * Direct calls to issue_order() (tests, scripted controllers, AI) are not
 * delayed. Included by game.c before issue_order(). */
enum { SC64_COMMAND_DELAY_UPDATES = 4, SC64_COMMAND_DELAY_SLOTS = 2,
       SC64_COMMAND_DELAY_BYTES = (MAX_UNITS + 7) / 8 };

typedef struct {
    u32 due, seq;
    s16 tx, ty;
    u8 order, active;
    u8 selected[SC64_COMMAND_DELAY_BYTES];
} Sc64DelayedCommand;

static Sc64DelayedCommand sc64_delayed_commands[SC64_COMMAND_DELAY_SLOTS];
static u32 sc64_delayed_command_seq;
static int issue_order(int order, int tx, int ty);

/* The selected units issue_order() moves: the same skips it applies. */
static inline int
sc64_command_delay_mover(int i)
{
    if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i]) || SC64_SCARAB_CHILD(&u[i]) ||
        u[i].owner != SIDE || !u[i].selected) return 0;
#ifdef SC64_SHARED_NUKE_PROBE
    if (sc64_race_ordinary_ghost(u[i].type) && race_nuke_ghost_locked(i)) return 0;
#endif
#ifdef SC64_EGG_RUNTIME
    if (u[i].type == SC64_EGG_ACTOR) return 0;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_unit_actor_flags(u[i].type) &&
        !(sc64_game_unit_actor_flags(u[i].type) & SC64_GAME_ACTOR_MOVABLE)) return 0;
#endif
    return 1;
}

/* A newer command for a unit, a unit leaving its slot or a new unit taking
 * it: no pending command may reach that slot any more. */
static void
sc64_command_delay_forget(int slot)
{
    if ((unsigned)slot >= MAX_UNITS) return;
    for (int k = 0; k < SC64_COMMAND_DELAY_SLOTS; ++k)
        sc64_delayed_commands[k].selected[slot >> 3] &= (u8)~(1u << (slot & 7));
}

/* A delayed command reaches only units whose order the player has not
 * changed since. While a command waits, each update's input phase hashes
 * every unit's order before and after: a unit whose order changed there was
 * given a newer player command (Stop, gather, build, repair, boarding, a
 * spell...) and leaves the pending commands. Orders the simulation changes
 * (auto-acquire, AI) lie outside that window and do not cancel the
 * player's command. */
static u8 sc64_command_delay_seen[MAX_UNITS];
static u8 sc64_command_delay_watching;

static inline u8
sc64_command_delay_hash(const Unit *q)
{
    return (u8)((unsigned)q->order * 37u + (unsigned)q->target * 11u +
                (unsigned)q->tx * 3u + (unsigned)q->ty * 7u + (unsigned)q->alive);
}

static void __attribute__((noinline))
sc64_command_delay_input_begin(void)
{
    sc64_command_delay_watching =
        sc64_delayed_commands[0].active || sc64_delayed_commands[1].active;
    if (!sc64_command_delay_watching) return;
    for (int i = 0; i < MAX_UNITS; ++i) sc64_command_delay_seen[i] = sc64_command_delay_hash(&u[i]);
}

static void __attribute__((noinline))
sc64_command_delay_input_end(void)
{
    if (!sc64_command_delay_watching) return;
    sc64_command_delay_watching = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (sc64_command_delay_seen[i] != sc64_command_delay_hash(&u[i]))
            sc64_command_delay_forget(i);
}

/* Issue one pending command with its own selection; the player's current
 * selection, selected building and toast line are restored afterwards. */
static void __attribute__((noinline))
sc64_command_delay_apply(Sc64DelayedCommand *c)
{
    u8 now[SC64_COMMAND_DELAY_BYTES];
    int building = selected_building, line = toast_id, line_timer = toast_timer;
    memset(now, 0, sizeof now);
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (u[i].selected) now[i >> 3] |= (u8)(1u << (i & 7));
        u[i].selected = (c->selected[i >> 3] >> (i & 7)) & 1;
    }
    c->active = 0;
    (void)issue_order(c->order, c->tx, c->ty);
    for (int i = 0; i < MAX_UNITS; ++i) {
        u[i].selected = (now[i >> 3] >> (i & 7)) & 1;
        if (sc64_command_delay_watching) sc64_command_delay_seen[i] = sc64_command_delay_hash(&u[i]);
    }
    selected_building = building;
    toast_id = line;
    toast_timer = line_timer;
}

/* In the order given: every command runs, and a later one for the same
 * units runs last, as the N64 executes each command after its latency. */
static Sc64DelayedCommand *
sc64_command_delay_oldest(void)
{
    Sc64DelayedCommand *a = &sc64_delayed_commands[0], *b = &sc64_delayed_commands[1];
    if (!a->active) return b->active ? b : 0;
    if (!b->active) return a;
    return (s32)(a->seq - b->seq) < 0 ? a : b;
}

/* Run each simulation update before orders are processed. */
static void __attribute__((noinline))
sc64_command_delay_tick(void)
{
    Sc64DelayedCommand *c;
    while ((c = sc64_command_delay_oldest()) && (s32)((u32)frame - c->due) >= 0)
        sc64_command_delay_apply(c);
}

static void
sc64_command_delay_flush(void)
{
    Sc64DelayedCommand *c;
    while ((c = sc64_command_delay_oldest())) sc64_command_delay_apply(c);
}

static void
sc64_command_delay_reset(void)
{
    memset(sc64_delayed_commands, 0, sizeof sc64_delayed_commands);
    sc64_command_delay_watching = 0;
}

/* StarCraft 64 holds a Move order 1-7 source frames after the unit stops
 * (T1 parity captures: 15 Marine moves; Move, then one Guard tick, then
 * PlayerGuard), as if each unit's order is processed on its own phase. The
 * engine ends an arrived Move on the unit slot's 10-update phase (8 source
 * frames), 0-9 updates after arrival. The unit no longer moves meanwhile. */
enum { SC64_MOVE_ORDER_PHASE_UPDATES = 10 };
static inline int
sc64_move_order_due(const Unit *q)
{
    return ((unsigned)frame + (unsigned)(q - u)) % SC64_MOVE_ORDER_PHASE_UPDATES == 0u;
}

/* An arrived Move ends on its phase. Out of line: update_orders() is IWRAM
 * code with little room. */
static void __attribute__((noinline))
sc64_move_arrive(Unit *q)
{
    if (sc64_move_order_due(q)) q->order = ORDER_IDLE;
}

/* The player's Move or attack-move. Returns the number of selected units
 * the command will reach (the value callers use for their ping), or
 * issue_order()'s refusal at once when nothing is selected. */
static int
sc64_command_delay_order(int order, int tx, int ty)
{
    Sc64DelayedCommand *c;
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += sc64_command_delay_mover(i);
    if (!n) return issue_order(order, tx, ty);
    if (sc64_delayed_commands[0].active && sc64_delayed_commands[1].active)
        sc64_command_delay_apply(sc64_command_delay_oldest());
    c = !sc64_delayed_commands[0].active ? &sc64_delayed_commands[0] : &sc64_delayed_commands[1];
    memset(c, 0, sizeof *c);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (sc64_command_delay_mover(i)) c->selected[i >> 3] |= (u8)(1u << (i & 7));
    c->order = (u8)order;
    c->tx = (s16)tx;
    c->ty = (s16)ty;
    /* frame advances once more before this update's orders run. */
    c->due = (u32)frame + SC64_COMMAND_DELAY_UPDATES + 1u;
    c->seq = ++sc64_delayed_command_seq;
    c->active = 1;
    selected_building = -1;
    toast(0);
    return n;
}

#endif
