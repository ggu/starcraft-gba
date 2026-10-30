/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free checks for the compact Z8 save state. The program is authored
 * (no source scenario rows): two owners, a preserved Wait, a MoveLocation and
 * switch traffic, so packing sees armed waits, cursors and a moved rect. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../starcraft/sc64_z8_state.h"

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } \
} while (0)

static Sc64Z8Trigger triggers[4];
static Sc64Z8Condition conditions[4];
static Sc64Z8Action actions[16];
static Sc64Z8SourceLocation locations[4] = {
    {0, 0, 4096, 4096, 0}, {100, 100, 164, 132, 0}, {300, 300, 364, 332, 0},
    {500, 500, 564, 532, 0}
};
static Sc64Z8Program program = {
    triggers, conditions, actions, locations, 0, 0, 0, 4, 4096, 4096
};

static Sc64Z8Action *act(unsigned trigger, u8 opcode)
{
    Sc64Z8Action *a = &actions[program.action_count++];
    a->opcode = opcode;
    a->index = (u8)triggers[trigger].action_count++;
    return a;
}

static void build(void)
{
    /* T0 (Force 1): wait, move location 3 onto the anchor, set switch 5,
     * preserve. T1 (Force 2): once switch 5 is set, wait and clear it. */
    triggers[0].enabled_players[SC64_Z8_SELECT_FORCE1] = 1;
    triggers[0].first_action = 0;
    conditions[0] = (Sc64Z8Condition){0, 0, 0, 0, 0, 0, 12, 0, 0, 0};
    triggers[0].condition_count = 1;
    act(0, 4)->time = 300;
    { Sc64Z8Action *a = act(0, 38); a->location = 1; a->unit = 42; a->group2 = 3; }
    { Sc64Z8Action *a = act(0, 13); a->group2 = 5; a->modifier = 4; }
    act(0, 3);
    triggers[1].enabled_players[SC64_Z8_SELECT_FORCE1 + 1] = 1;
    triggers[1].first_condition = 1;
    triggers[1].first_action = (u16)program.action_count;
    conditions[1] = (Sc64Z8Condition){0, 0, 0, 0, 0, 2, 11, 5, 0, 0};
    triggers[1].condition_count = 1;
    act(1, 4)->time = 700;
    { Sc64Z8Action *a = act(1, 13); a->group2 = 5; a->modifier = 5; }
    act(1, 3);
    program.trigger_count = 2;
    program.condition_count = 2;
}

static int count(void *c, const Sc64Z8Condition *x, unsigned o, const Sc64Z8Rect *r, u32 *n)
{ (void)c; (void)x; (void)o; (void)r; *n = 0; return 1; }
static int action(void *c, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r)
{ (void)c; (void)x; (void)o; (void)r; return 1; }
static int timed(void *c, int ph, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r, u32 *d)
{ (void)c; (void)ph; (void)x; (void)o; (void)r; *d = 0; return 1; }
static s16 anchor_x = 1000;
static int find(void *c, const Sc64Z8Action *x, unsigned o, const Sc64Z8Rect *r, s16 *px, s16 *py)
{ (void)c; (void)x; (void)o; (void)r; *px = anchor_x; *py = 800; return 1; }

int main(void)
{
    static Sc64Z8SchedulerState live, restored, scratch;
    static Sc64Z8PersistentState saved, bad;
    const Sc64Z8WorldOps ops = { count, action, timed, find };
    const Sc64Z8Setup setup = { 0x03u, { 1, 2, 0, 0, 0, 0, 0, 0 }, 42 };
    u8 moved[SC64_Z8_SAVE_MOVED_LOCATIONS];
    unsigned trips = 0, armed = 0, moved_seen = 0;

    build();
    CHECK(sc64_z8_program_valid(&program));
    CHECK(sc64_z8_moved_locations(&program, moved) == 1u && moved[0] == 3u);
    CHECK(sc64_z8_init(&live, &program, &setup));
    for (unsigned f = 0; f < 3000; ++f) {
        anchor_x = (s16)(1000 + (f / 100) * 8);
        CHECK(sc64_z8_tick(&live, &program, &ops, 0, 0));
        memset(&saved, 0xa5, sizeof(saved));
        CHECK(sc64_z8_state_pack(&saved, &live, &program));
        memset(&restored, 0x5a, sizeof(restored));
        CHECK(sc64_z8_state_unpack(&restored, &saved, &program, &setup, &scratch));
        CHECK(!memcmp(&restored, &live, sizeof(live)));
        armed += live.waiting_mask != 0;
        moved_seen += !sc64_z8_rect_is_source(&live.locations[2], &locations[2]);
        ++trips;
    }
    CHECK(armed && moved_seen);

    /* Each rejected candidate leaves the output untouched. */
    restored = live;
    bad = saved; bad.reserved = 1;
    CHECK(!sc64_z8_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.instance[live.instance_count] = 1;
    CHECK(!sc64_z8_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.instance[0] = 0x0fu;
    CHECK(!sc64_z8_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.moved[1].left = 1;
    CHECK(!sc64_z8_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.waiting_mask = 0x80u;
    CHECK(!sc64_z8_state_unpack(&restored, &bad, &program, &setup, &scratch));
    bad = saved; bad.outcome[5] = SC64_Z8_OUTCOME_VICTORY;
    CHECK(!sc64_z8_state_unpack(&restored, &bad, &program, &setup, &scratch));
    CHECK(!sc64_z8_state_unpack(&restored, &saved, &program, &setup, &restored));
    CHECK(!memcmp(&restored, &live, sizeof(live)));
    CHECK(sc64_z8_state_unpack(0, &saved, &program, &setup, &scratch));

    /* A non-mutable location that drifted cannot be packed. */
    restored = live;
    restored.locations[1].left += 2;
    CHECK(!sc64_z8_state_pack(&bad, &restored, &program));
    printf("Z8 compact state: %u round trips, %u bytes for an %u-byte scheduler\n",
           trips, (unsigned)sizeof(Sc64Z8PersistentState),
           (unsigned)sizeof(Sc64Z8SchedulerState));
    return 0;
}
