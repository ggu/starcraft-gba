/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free checks for the Z9/Z10 scheduler extensions and compact save
 * state. The program is authored (no source scenario rows). Force 1 sets a
 * countdown and a switch after a Wait; once the countdown expires it removes
 * a unit, runs an AI script and moves a location. Force 2 reacts to a
 * death census by adding to the countdown. Every tick packs and unpacks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../starcraft/sc64_zerg_late_state.h"

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } \
} while (0)

#define SCRIPT 0x41756c56u /* "VluA" */

static Sc64Z8Trigger triggers[4];
static Sc64Z8Condition conditions[8];
static Sc64Z8Action actions[16];
static Sc64Z8SourceLocation locations[4] = {
    {0, 0, 4096, 4096, 0}, {100, 100, 164, 132, 0}, {300, 300, 364, 332, 0},
    {500, 500, 564, 532, 0}
};
static const u32 scripts[1] = { SCRIPT };
static Sc64ZlProgram program = {
    { triggers, conditions, actions, locations, 0, 0, 0, 4, 4096, 4096 },
    scripts, 1
};

static Sc64Z8Action *act(unsigned trigger, u8 opcode)
{
    Sc64Z8Action *a = &actions[program.rows.action_count++];
    a->opcode = opcode;
    a->index = (u8)triggers[trigger].action_count++;
    return a;
}

static Sc64Z8Condition *cond(unsigned trigger, u8 opcode, u8 comparison)
{
    Sc64Z8Condition *c = &conditions[program.rows.condition_count++];
    c->opcode = opcode;
    c->comparison = comparison;
    c->index = (u8)triggers[trigger].condition_count++;
    return c;
}

static void build(void)
{
    triggers[0].enabled_players[SC64_Z8_SELECT_FORCE1] = 1;
    cond(0, 12, 0);
    { Sc64Z8Action *a = act(0, 14); a->modifier = SC64_ZL_MOD_SET; a->time = 3; }
    act(0, 4)->time = 300;
    { Sc64Z8Action *a = act(0, 13); a->group2 = 1; a->modifier = 4; }

    triggers[1].enabled_players[SC64_Z8_SELECT_FORCE1] = 1;
    triggers[1].first_condition = (u16)program.rows.condition_count;
    triggers[1].first_action = (u16)program.rows.action_count;
    cond(1, 1, 1);
    cond(1, 11, 2)->resource = 1;
    { Sc64Z8Action *a = act(1, 25); a->location = 2; a->unit = 41; a->group1 = 18; }
    { Sc64Z8Action *a = act(1, 16); a->location = 1; a->group2 = SCRIPT; }
    { Sc64Z8Action *a = act(1, 38); a->location = 1; a->unit = 41; a->group2 = 3; }
    { Sc64Z8Action *a = act(1, 13); a->group2 = 2; a->modifier = 4; }

    triggers[2].enabled_players[SC64_Z8_SELECT_FORCE1 + 1] = 1;
    triggers[2].first_condition = (u16)program.rows.condition_count;
    triggers[2].first_action = (u16)program.rows.action_count;
    { Sc64Z8Condition *c = cond(2, 15, 0); c->player = 1; c->unit = 229; c->amount = 1; }
    { Sc64Z8Action *a = act(2, 14); a->modifier = SC64_ZL_MOD_ADD; a->time = 5; }
    { Sc64Z8Action *a = act(2, 13); a->group2 = 3; a->modifier = 4; }
    program.rows.trigger_count = 3;
}

static unsigned removes, ai_runs, deaths = 0;
static int count(void *c, const Sc64Z8Condition *x, unsigned o, const Sc64Z8Rect *r, u32 *n)
{ (void)c; (void)x; (void)o; (void)r; *n = 0; return 1; }
static int action(void *c, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r)
{ (void)c; (void)x; (void)o; (void)r; return 1; }
static int timed(void *c, int ph, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r, u32 *d)
{ (void)c; (void)ph; (void)x; (void)o; (void)r; *d = 0; return 1; }
static int find(void *c, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r, s16 *px, s16 *py)
{ (void)c; (void)x; (void)o; (void)r; *px = 1200; *py = 800; return 1; }
static int run_ai(void *c, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r)
{ (void)c; (void)o; ++ai_runs; return x->group2 == SCRIPT && r ? 1 : -1; }
static int remove_at(void *c, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r)
{ (void)c; (void)o; ++removes; return x->unit == 41 && r && r->left == 100 ? 1 : -1; }
static int death_census(void *c, const Sc64Z8Condition *x, unsigned o, u32 *n)
{ (void)c; (void)o; *n = deaths; return x->player == 1 ? 1 : -1; }

int main(void)
{
    static Sc64ZlSchedulerState live, restored, scratch;
    static Sc64ZlPersistentState saved, bad;
    static Sc64ZlProgram broken;
    static Sc64Z8Action broken_actions[16];
    const Sc64ZlWorldOps ops = { { count, action, timed, find }, run_ai, remove_at, death_census };
    const Sc64Z8Setup setup = { 0x03u, { 1, 2, 0, 0, 0, 0, 0, 0 }, 42 };
    unsigned trips = 0, max_countdown = 0, fired_at = 0;

    build();
    CHECK(sc64_zl_program_valid(&program));
    /* An unlisted FourCC or a bad countdown modifier invalidates the program. */
    broken = program;
    memcpy(broken_actions, actions, sizeof(actions));
    broken.rows.actions = broken_actions;
    broken_actions[4].group2 = 0x12345678u;
    CHECK(!sc64_zl_program_valid(&broken));
    memcpy(broken_actions, actions, sizeof(actions));
    broken_actions[0].modifier = 4;
    CHECK(!sc64_zl_program_valid(&broken));

    CHECK(sc64_zl_init(&live, &program, &setup));
    CHECK(live.core.instance_count == 3u);
    for (unsigned f = 0; f < 3000; ++f) {
        if (f == 1500) deaths = 1;
        CHECK(sc64_zl_tick(&live, &program, &ops, 0, 0));
        if (live.countdown > max_countdown) max_countdown = live.countdown;
        if (!fired_at && sc64_z8_switch_get(&live.core, 2)) fired_at = f;
        memset(&saved, 0xa5, sizeof(saved));
        CHECK(sc64_zl_state_pack(&saved, &live, &program));
        memset(&restored, 0x5a, sizeof(restored));
        CHECK(sc64_zl_state_unpack(&restored, &saved, &program, &setup, &scratch));
        CHECK(!memcmp(&restored, &live, sizeof(live)));
        ++trips;
    }
    /* T0 set 3 s; T1 fired once it expired (about 48 native frames later);
     * the death census then added 5 s, which counted back down to zero. */
    CHECK(max_countdown == 5u && live.countdown == 0u);
    CHECK(fired_at > 48u && fired_at < 200u);
    CHECK(removes == 1u && ai_runs == 1u && sc64_z8_switch_get(&live.core, 3));
    CHECK(!sc64_z8_rect_is_source(&live.core.locations[2], &locations[2]));

    /* Each rejected candidate leaves the output untouched. */
    restored = live;
    bad = saved; bad.countdown = SC64_ZL_COUNTDOWN_LIMIT + 1u;
    CHECK(!sc64_zl_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.instance[live.core.instance_count] = 1;
    CHECK(!sc64_zl_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.instance[0] = 0x0fu;
    CHECK(!sc64_zl_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.moved[1].left = 1;
    CHECK(!sc64_zl_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.outcome[5] = SC64_Z8_OUTCOME_VICTORY;
    CHECK(!sc64_zl_state_unpack(&restored, &bad, &program, &setup, &scratch));
    CHECK(!sc64_zl_state_unpack(&restored, &saved, &program, &setup, &restored));
    CHECK(!memcmp(&restored, &live, sizeof(live)));
    CHECK(sc64_zl_state_unpack(0, &saved, &program, &setup, &scratch));

    /* A failing world callback faults the scheduler instead of succeeding. */
    {
        const Sc64ZlWorldOps no_remove = { { count, action, timed, find }, run_ai, 0, death_census };
        Sc64ZlSchedulerState s;
        deaths = 0;
        CHECK(sc64_zl_init(&s, &program, &setup));
        for (unsigned f = 0; f < 400 && !s.core.fault; ++f)
            (void)sc64_zl_tick(&s, &program, &no_remove, 0, 0);
        CHECK(s.core.fault == SC64_Z8_FAULT_ACTION && s.core.fault_trigger == 1u);
    }
    printf("late Zerg compact state: %u round trips, %u bytes for an %u-byte scheduler\n",
           trips, (unsigned)sizeof(Sc64ZlPersistentState),
           (unsigned)sizeof(Sc64ZlSchedulerState));
    return 0;
}
