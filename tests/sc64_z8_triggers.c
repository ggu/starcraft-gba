/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free synthetic checks for the isolated Z8 trigger scheduler. Every
 * program here is authored; no source scenario rows are used. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../starcraft/sc64_z8_triggers.h"

#define CHECK(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        exit(1); \
    } \
} while (0)

enum { MAX_CONDITIONS = 256, MAX_ACTIONS = 512, LOG_LIMIT = 4096 };

typedef struct {
    Sc64Z8Trigger triggers[SC64_Z8_TRIGGER_LIMIT];
    Sc64Z8Condition conditions[MAX_CONDITIONS];
    Sc64Z8Action actions[MAX_ACTIONS];
    Sc64Z8SourceLocation locations[SC64_Z8_LOCATION_LIMIT];
    Sc64Z8Program program;
} Builder;

static void
b_init(Builder *b)
{
    memset(b, 0, sizeof(*b));
    b->program.triggers = b->triggers;
    b->program.conditions = b->conditions;
    b->program.actions = b->actions;
    b->program.locations = b->locations;
    b->program.map_pixel_width = 4096;
    b->program.map_pixel_height = 4096;
    b->program.location_count = 8;
    b->locations[0] = (Sc64Z8SourceLocation){0, 0, 4096, 4096, 0};
    for (unsigned i = 1; i < 8; ++i)
        b->locations[i] = (Sc64Z8SourceLocation){
            (u16)(100 * i), (u16)(100 * i), (u16)(100 * i + 64),
            (u16)(100 * i + 32), 0};
}

static unsigned
b_trigger(Builder *b, unsigned selector)
{
    unsigned i = b->program.trigger_count++;
    Sc64Z8Trigger *t = &b->triggers[i];
    t->first_condition = b->program.condition_count;
    t->first_action = b->program.action_count;
    t->enabled_players[selector] = 1;
    return i;
}

static Sc64Z8Condition *
b_cond(Builder *b, u8 opcode, u8 comparison, u32 amount)
{
    Sc64Z8Trigger *t = &b->triggers[b->program.trigger_count - 1];
    Sc64Z8Condition *c = &b->conditions[b->program.condition_count++];
    c->index = (u8)t->condition_count++;
    c->opcode = opcode;
    c->comparison = comparison;
    c->amount = amount;
    if (opcode == 2 || opcode == 3 || opcode == 5) c->flags = 0x10;
    return c;
}

static Sc64Z8Action *
b_act(Builder *b, u8 opcode)
{
    Sc64Z8Trigger *t = &b->triggers[b->program.trigger_count - 1];
    Sc64Z8Action *a = &b->actions[b->program.action_count++];
    a->index = (u8)t->action_count++;
    a->opcode = opcode;
    return a;
}

static Sc64Z8Action *
b_switch(Builder *b, unsigned index, u8 modifier)
{
    Sc64Z8Action *a = b_act(b, 13);
    a->group2 = index;
    a->modifier = modifier;
    return a;
}

static Sc64Z8Action *
b_wait(Builder *b, u32 ms)
{
    Sc64Z8Action *a = b_act(b, 4);
    a->time = ms;
    return a;
}

static Sc64Z8Condition *
b_switch_cond(Builder *b, unsigned index, u8 comparison)
{
    Sc64Z8Condition *c = b_cond(b, 11, comparison, 0);
    c->resource = (u8)index;
    return c;
}

typedef struct {
    char kind;          /* C count, A action, T timed, F find */
    u8 owner, phase;
    u16 row;
    u32 operand;
    Sc64Z8Rect rect;
    u8 has_rect;
} LogEntry;

typedef struct {
    const Builder *b;
    u32 count_by_row[MAX_CONDITIONS];
    int count_result, action_result, find_result, fail_row;
    int timed_eligible, timed_start_result, timed_finish_result;
    u32 timed_duration;
    s16 find_x, find_y;
    unsigned log_count;
    LogEntry log[LOG_LIMIT];
} World;

static void
world_init(World *w, const Builder *b)
{
    memset(w, 0, sizeof(*w));
    w->b = b;
    w->count_result = 1;
    w->action_result = 1;
    w->fail_row = -1;
    w->find_result = 1;
    w->timed_eligible = 1;
    w->timed_start_result = 1;
    w->timed_finish_result = 1;
    w->timed_duration = 1000;
}

static void
world_log(World *w, char kind, unsigned owner, unsigned row, u32 operand,
          const Sc64Z8Rect *rect, unsigned phase)
{
    CHECK(w->log_count < LOG_LIMIT);
    LogEntry *e = &w->log[w->log_count++];
    memset(e, 0, sizeof(*e));
    e->kind = kind;
    e->owner = (u8)owner;
    e->row = (u16)row;
    e->operand = operand;
    e->phase = (u8)phase;
    if (rect) { e->rect = *rect; e->has_rect = 1; }
}

static int
world_count(void *context, const Sc64Z8Condition *c, unsigned owner,
            const Sc64Z8Rect *rect, u32 *count)
{
    World *w = context;
    unsigned row = (unsigned)(c - w->b->conditions);
    CHECK(row < w->b->program.condition_count);
    world_log(w, 'C', owner, row, c->opcode, rect, 0);
    *count = w->count_by_row[row];
    return w->count_result;
}

static int
world_action(void *context, const Sc64Z8Action *a, unsigned owner,
             const Sc64Z8Rect *rect)
{
    World *w = context;
    unsigned row = (unsigned)(a - w->b->actions);
    CHECK(row < w->b->program.action_count);
    world_log(w, 'A', owner, row, a->opcode == 16 ? a->group2 : a->opcode,
              rect, 0);
    return w->fail_row < 0 || (int)row == w->fail_row ? w->action_result : 1;
}

static int
world_timed(void *context, int phase, const Sc64Z8Action *a, unsigned owner,
            const Sc64Z8Rect *rect, u32 *duration)
{
    World *w = context;
    unsigned row = (unsigned)(a - w->b->actions);
    world_log(w, 'T', owner, row, a->opcode, rect, (unsigned)phase);
    if (phase == SC64_Z8_TIMED_ELIGIBLE) return w->timed_eligible;
    if (phase == SC64_Z8_TIMED_START) {
        *duration = w->timed_duration;
        return w->timed_start_result;
    }
    return w->timed_finish_result;
}

static int
world_find(void *context, const Sc64Z8Action *a, unsigned owner,
           const Sc64Z8Rect *rect, s16 *x, s16 *y)
{
    World *w = context;
    world_log(w, 'F', owner, (unsigned)(a - w->b->actions), a->unit, rect, 0);
    *x = w->find_x;
    *y = w->find_y;
    return w->find_result;
}

static const Sc64Z8WorldOps OPS = {world_count, world_action, world_timed,
                                   world_find};

static Sc64Z8Setup
setup_owner(unsigned owner)
{
    Sc64Z8Setup s;
    memset(&s, 0, sizeof(s));
    s.active_mask = (u8)(1u << owner);
    s.force[owner] = 1;
    s.frame_ms = 42;
    return s;
}

/* Owners 1..3 with the Z8-shaped FORC arrangement: slot 3 is Force 1,
 * slot 1 Force 2, slot 2 Force 3; the inactive rest are Force 4. */
static Sc64Z8Setup
setup_z8_shape(void)
{
    Sc64Z8Setup s;
    memset(&s, 0, sizeof(s));
    s.active_mask = 0x0e;
    for (unsigned o = 0; o < 8; ++o) s.force[o] = 4;
    s.force[3] = 1;
    s.force[1] = 2;
    s.force[2] = 3;
    s.frame_ms = 42;
    return s;
}

static unsigned
count_log(const World *w, char kind, unsigned row)
{
    unsigned n = 0;
    for (unsigned i = 0; i < w->log_count; ++i)
        if (w->log[i].kind == kind && w->log[i].row == row) ++n;
    return n;
}

static int
log_index(const World *w, char kind, unsigned row, unsigned nth)
{
    for (unsigned i = 0; i < w->log_count; ++i)
        if (w->log[i].kind == kind && w->log[i].row == row && nth-- == 0)
            return (int)i;
    return -1;
}

static unsigned
row_of(const Builder *b, const Sc64Z8Action *a)
{
    return (unsigned)(a - b->actions);
}

static unsigned
cond_row(const Builder *b, const Sc64Z8Condition *c)
{
    return (unsigned)(c - b->conditions);
}

/* Tick until the next pass completes; return the frame it ran on. */
static u32
tick_to_pass(Sc64Z8SchedulerState *s, const Builder *b, World *w)
{
    u32 passes = s->passes;
    for (unsigned guard = 0; guard < 10000; ++guard) {
        CHECK(sc64_z8_tick(s, &b->program, &OPS, w, 0));
        if (s->passes != passes) return s->frames;
    }
    CHECK(!"no pass");
    return 0;
}

static const Sc64Z8Instance *
instance(const Sc64Z8SchedulerState *s, unsigned owner, unsigned trigger)
{
    for (unsigned i = 0; i < s->instance_count; ++i)
        if (s->instances[i].owner == owner && s->instances[i].trigger == trigger)
            return &s->instances[i];
    return 0;
}

static void
test_validation(void)
{
    Builder b;
    Sc64Z8Setup setup = setup_owner(0);
    Sc64Z8SchedulerState s;

    b_init(&b);
    b_trigger(&b, 0);
    b_cond(&b, 22, 0, 0);
    b_act(&b, 8);
    CHECK(sc64_z8_program_valid(&b.program));
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    /* Unsupported opcodes, operands and layouts fail closed. */
    static const u8 bad_actions[] = {5, 6, 14, 15, 22, 23, 24, 25, 39, 44, 46};
    for (unsigned i = 0; i < sizeof(bad_actions); ++i) {
        b.actions[0].opcode = bad_actions[i];
        CHECK(!sc64_z8_program_valid(&b.program));
    }
    b.actions[0].opcode = 13;
    b.actions[0].modifier = 11; /* randomize needs the native RNG */
    CHECK(!sc64_z8_program_valid(&b.program));
    b.actions[0].modifier = 4;
    b.actions[0].group2 = 256;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.actions[0].group2 = 255;
    CHECK(sc64_z8_program_valid(&b.program));
    b.actions[0].opcode = 16;
    b.actions[0].group2 = 0x31335241; /* an unrelated FourCC */
    CHECK(!sc64_z8_program_valid(&b.program));
    b.actions[0].group2 = SC64_Z8_AI_MVTE;
    CHECK(sc64_z8_program_valid(&b.program));
    b.actions[0].flags = 1; /* would pre-arm a wait */
    CHECK(!sc64_z8_program_valid(&b.program));
    b.actions[0].flags = 2; /* disabled: never dispatched */
    b.actions[0].opcode = 46;
    CHECK(sc64_z8_program_valid(&b.program));
    b.actions[0] = (Sc64Z8Action){0};
    b.actions[0].opcode = 38;
    b.actions[0].location = 1;
    b.actions[0].group2 = 9; /* destination past the location table */
    CHECK(!sc64_z8_program_valid(&b.program));
    b.actions[0].group2 = 2;
    CHECK(sc64_z8_program_valid(&b.program));
    b.actions[0].location = 9;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.actions[0].location = 1;

    static const u8 bad_conditions[] = {1, 4, 6, 13, 14, 15, 16, 21};
    for (unsigned i = 0; i < sizeof(bad_conditions); ++i) {
        b.conditions[0].opcode = bad_conditions[i];
        CHECK(!sc64_z8_program_valid(&b.program));
    }
    b.conditions[0].opcode = 2;
    b.conditions[0].comparison = 2; /* Set is not a numeric comparison */
    CHECK(!sc64_z8_program_valid(&b.program));
    b.conditions[0].comparison = 10;
    CHECK(sc64_z8_program_valid(&b.program));
    b.conditions[0].opcode = 11;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.conditions[0].comparison = 3;
    CHECK(sc64_z8_program_valid(&b.program));
    b.conditions[0].flags = 0x01;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.conditions[0].flags = 0;

    b.triggers[0].execution_flags = 4;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.triggers[0].execution_flags = 0;
    b.triggers[0].initial_cursor = 1;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.triggers[0].initial_cursor = 0;
    b.triggers[0].enabled_players[13] = 1; /* current-player selector */
    CHECK(!sc64_z8_program_valid(&b.program));
    b.triggers[0].enabled_players[13] = 0;
    b.triggers[0].enabled_players[17] = 1; /* same owner twice */
    CHECK(sc64_z8_program_valid(&b.program));
    CHECK(!sc64_z8_setup_valid(&setup, &b.program));
    CHECK(!sc64_z8_init(&s, &b.program, &setup));
    b.triggers[0].enabled_players[17] = 0;

    /* Rows must ascend in source slot order and cover the table. */
    b_act(&b, 9);
    b.actions[1].index = 0;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.actions[1].index = 64;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.actions[1].index = 5;
    CHECK(sc64_z8_program_valid(&b.program));
    b.program.action_count = 1;
    CHECK(!sc64_z8_program_valid(&b.program));
    b.program.action_count = 2;

    setup.frame_ms = 50; /* not a native speed-table entry */
    CHECK(!sc64_z8_setup_valid(&setup, &b.program));
    setup.frame_ms = 167;
    CHECK(sc64_z8_setup_valid(&setup, &b.program));
    setup.active_mask = 0x02; /* no instances for owner 1 */
    CHECK(!sc64_z8_setup_valid(&setup, &b.program));
}

static void
test_cadence(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_owner(0);

    b_init(&b);
    b_trigger(&b, 0);
    b_cond(&b, 22, 0, 0);
    b_act(&b, 8);
    b_act(&b, 3);
    b_trigger(&b, 0);
    b_cond(&b, 12, 0, 3); /* ElapsedTime AtLeast 3 */
    b_switch(&b, 9, 4);
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));
    CHECK(s.pass_counter == 1 && s.second_counter == 15);

    static const u32 expected[] = {2, 33, 64, 95, 126};
    for (unsigned i = 0; i < 5; ++i) {
        u32 frame = tick_to_pass(&s, &b, &w);
        CHECK(frame == expected[i]);
        CHECK(s.elapsed_seconds == frame / 16);
        CHECK(sc64_z8_switch_get(&s, 9) == (frame >= 64));
    }

    /* Paused frames freeze every counter. */
    Sc64Z8SchedulerState before = s;
    for (unsigned i = 0; i < 100; ++i)
        CHECK(sc64_z8_tick(&s, &b.program, &OPS, &w, 1));
    CHECK(!memcmp(&before, &s, sizeof(s)));

    /* Seconds advance on frames 16, 32, ... regardless of passes. */
    CHECK(sc64_z8_init(&s, &b.program, &setup));
    for (unsigned f = 1; f <= 48; ++f) {
        CHECK(sc64_z8_tick(&s, &b.program, &OPS, &w, 0));
        CHECK(s.elapsed_seconds == f / 16);
    }
}

static void
test_repeated_and_preserved(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_owner(0);

    b_init(&b);
    b_trigger(&b, 0);                       /* 0: one-shot */
    b_cond(&b, 22, 0, 0);
    Sc64Z8Action *once = b_act(&b, 8);
    b_trigger(&b, 0);                       /* 1: preserved, always true */
    b_cond(&b, 22, 0, 0);
    Sc64Z8Action *every = b_act(&b, 9);
    b_act(&b, 3);
    b_trigger(&b, 0);                       /* 2: preserved, count-driven */
    Sc64Z8Condition *gate = b_cond(&b, 2, 0, 1);
    Sc64Z8Action *gated = b_act(&b, 12);
    b_act(&b, 3);
    b_trigger(&b, 0);                       /* 3: one-shot, count-driven */
    Sc64Z8Condition *one_gate = b_cond(&b, 5, 0, 2);
    Sc64Z8Action *one_effect = b_act(&b, 26);
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    const unsigned gate_row = cond_row(&b, gate);
    const unsigned one_row = cond_row(&b, one_gate);
    static const u32 gate_counts[] = {0, 0, 1, 1, 0, 1};
    for (unsigned pass = 0; pass < 6; ++pass) {
        w.count_by_row[gate_row] = gate_counts[pass];
        w.count_by_row[one_row] = pass >= 3 ? 2 : 1;
        tick_to_pass(&s, &b, &w);
    }
    CHECK(count_log(&w, 'A', row_of(&b, once)) == 1);
    CHECK(count_log(&w, 'A', row_of(&b, every)) == 6);
    CHECK(count_log(&w, 'A', row_of(&b, gated)) == 3);
    CHECK(count_log(&w, 'C', gate_row) == 6);   /* rechecked every pass */
    CHECK(count_log(&w, 'A', row_of(&b, one_effect)) == 1);
    CHECK(count_log(&w, 'C', one_row) == 4);    /* stops once retired */
    CHECK(instance(&s, 0, 0)->flags == SC64_Z8_INST_DONE);
    CHECK(instance(&s, 0, 1)->flags == SC64_Z8_INST_PRESERVE);
    CHECK(instance(&s, 0, 3)->flags == SC64_Z8_INST_DONE);
}

static void
test_preserve_lifecycle_with_wait(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_owner(0);

    b_init(&b);
    b_trigger(&b, 0);
    Sc64Z8Condition *gate = b_cond(&b, 2, 0, 1);
    Sc64Z8Action *first = b_act(&b, 8);
    b_act(&b, 3);              /* Preserve before the wait still applies */
    b_wait(&b, 2000);
    Sc64Z8Action *after = b_act(&b, 9);
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));
    const unsigned gate_row = cond_row(&b, gate);

    w.count_by_row[gate_row] = 1;
    CHECK(tick_to_pass(&s, &b, &w) == 2);
    const Sc64Z8Instance *in = instance(&s, 0, 0);
    CHECK(in->flags == (SC64_Z8_INST_EXECUTING | SC64_Z8_INST_PRESERVE |
                        SC64_Z8_INST_ARMED));
    CHECK(in->cursor == 2 && s.waiting_mask == 1 && s.wait_ms[0] == 2000);

    /* While executing, conditions are not consulted again. */
    w.count_by_row[gate_row] = 0;
    CHECK(tick_to_pass(&s, &b, &w) == 33);
    CHECK(count_log(&w, 'C', gate_row) == 1);
    CHECK(count_log(&w, 'A', row_of(&b, after)) == 0);

    /* 2000 ms at 42 ms/frame expires on frame 50; the expiry makes the
     * next pass run on frame 51 instead of frame 64. */
    CHECK(tick_to_pass(&s, &b, &w) == 51);
    CHECK(count_log(&w, 'A', row_of(&b, after)) == 1);
    CHECK(in->flags == SC64_Z8_INST_PRESERVE && in->cursor == 0);
    CHECK(s.waiting_mask == 0);
    CHECK(tick_to_pass(&s, &b, &w) == 82);
    CHECK(count_log(&w, 'C', gate_row) == 2);
    CHECK(count_log(&w, 'A', row_of(&b, first)) == 1);

    /* Re-fires from slot 0 once its condition is true again. */
    w.count_by_row[gate_row] = 1;
    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'A', row_of(&b, first)) == 2);
    CHECK(in->cursor == 2 && (in->flags & SC64_Z8_INST_ARMED));
}

static void
test_simultaneous_conditions(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_owner(0);

    b_init(&b);
    b_trigger(&b, 0);
    Sc64Z8Condition *a0 = b_cond(&b, 2, 0, 1);
    Sc64Z8Condition *a1 = b_cond(&b, 3, 1, 4);
    a1->location = 2;
    Sc64Z8Action *e0 = b_act(&b, 8);
    b_trigger(&b, 0);
    Sc64Z8Condition *b0 = b_cond(&b, 2, 0, 1);
    Sc64Z8Condition *b1 = b_cond(&b, 2, 0, 1);
    Sc64Z8Action *e1 = b_act(&b, 9);
    b_trigger(&b, 0);
    Sc64Z8Condition *disabled = b_cond(&b, 2, 0, 1);
    disabled->flags |= 2;
    b_cond(&b, 22, 0, 0);
    Sc64Z8Condition *hidden = b_cond(&b, 23, 0, 0);
    hidden->index = 3; /* slot 2 is all-zero, so Never at slot 3 is unseen */
    Sc64Z8Action *e2 = b_act(&b, 12);
    Sc64Z8Action *e2_hidden = b_act(&b, 28);
    e2_hidden->index = 2;  /* slot 1 is all-zero: the list ends first */
    e2_hidden->location = 1;
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    /* Pass 1: first trigger fails on its second row, second short-circuits
     * on its first row, third passes through a disabled row and a gap. */
    w.count_by_row[cond_row(&b, a0)] = 1;
    w.count_by_row[cond_row(&b, a1)] = 5;
    tick_to_pass(&s, &b, &w);
    CHECK(w.log_count == 4);
    CHECK(w.log[0].kind == 'C' && w.log[0].row == cond_row(&b, a0));
    CHECK(w.log[1].kind == 'C' && w.log[1].row == cond_row(&b, a1));
    CHECK(w.log[1].has_rect && w.log[1].rect.left == 100);
    CHECK(w.log[2].kind == 'C' && w.log[2].row == cond_row(&b, b0));
    CHECK(w.log[3].kind == 'A' && w.log[3].row == row_of(&b, e2));
    CHECK(count_log(&w, 'C', cond_row(&b, b1)) == 0);
    CHECK(count_log(&w, 'C', cond_row(&b, disabled)) == 0);
    CHECK(count_log(&w, 'A', row_of(&b, e2_hidden)) == 0);
    CHECK(instance(&s, 0, 2)->flags == SC64_Z8_INST_DONE);

    /* Pass 2: both remaining triggers are true; actions follow source order. */
    w.count_by_row[cond_row(&b, a1)] = 4;
    w.count_by_row[cond_row(&b, b0)] = 1;
    w.count_by_row[cond_row(&b, b1)] = 3;
    unsigned mark = w.log_count;
    tick_to_pass(&s, &b, &w);
    int i0 = log_index(&w, 'A', row_of(&b, e0), 0);
    int i1 = log_index(&w, 'A', row_of(&b, e1), 0);
    CHECK(i0 >= (int)mark && i1 > i0);
}

static void
test_order_dependent_actions(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_z8_shape();

    b_init(&b);
    b_trigger(&b, 3);           /* 0: owner 3 sets 5 */
    b_cond(&b, 22, 0, 0);
    b_switch(&b, 5, 4);
    b_trigger(&b, 3);           /* 1: owner 3 sees 5 in the same pass */
    b_switch_cond(&b, 5, 2);
    Sc64Z8Action *same_pass = b_act(&b, 8);
    b_trigger(&b, 3);           /* 2: owner 3 waits for 6 */
    b_switch_cond(&b, 6, 2);
    Sc64Z8Action *next_pass = b_act(&b, 9);
    b_trigger(&b, 3);           /* 3: sets 6 after 2 was checked */
    b_cond(&b, 22, 0, 0);
    b_switch(&b, 6, 4);
    b_trigger(&b, 18);          /* 4: Force 1 = owner 3, waits for 7 */
    b_switch_cond(&b, 7, 2);
    Sc64Z8Action *cross_early = b_act(&b, 12);
    b_trigger(&b, 19);          /* 5: Force 2 = owner 1 sets 7 */
    b_cond(&b, 22, 0, 0);
    b_switch(&b, 7, 4);
    b_trigger(&b, 1);           /* 6: owner 1 waits for 8 */
    b_switch_cond(&b, 8, 2);
    Sc64Z8Action *cross_late = b_act(&b, 26);
    b_trigger(&b, 3);           /* 7: owner 3 sets 8 */
    b_cond(&b, 22, 0, 0);
    b_switch(&b, 8, 4);
    b_trigger(&b, 3);           /* 8: toggles twice in one list */
    b_cond(&b, 22, 0, 0);
    b_switch(&b, 9, 6);
    b_switch(&b, 9, 6);
    b_switch(&b, 10, 6);
    b_switch(&b, 10, 5);
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'A', row_of(&b, same_pass)) == 1);
    CHECK(count_log(&w, 'A', row_of(&b, next_pass)) == 0);
    CHECK(count_log(&w, 'A', row_of(&b, cross_early)) == 1);
    CHECK(count_log(&w, 'A', row_of(&b, cross_late)) == 0);
    CHECK(!sc64_z8_switch_get(&s, 9) && !sc64_z8_switch_get(&s, 10));
    int early = log_index(&w, 'A', row_of(&b, cross_early), 0);
    CHECK(w.log[early].owner == 3);
    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'A', row_of(&b, next_pass)) == 1);
    CHECK(count_log(&w, 'A', row_of(&b, cross_late)) == 1);
    CHECK(w.log[log_index(&w, 'A', row_of(&b, cross_late), 0)].owner == 1);
}

static void
test_outcomes(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_z8_shape();

    b_init(&b);
    b_trigger(&b, 1);           /* owner 1: Defeat, then Victory */
    b_cond(&b, 22, 0, 0);
    b_act(&b, 2);
    b_act(&b, 1);
    Sc64Z8Action *after1 = b_act(&b, 8);
    b_trigger(&b, 3);           /* owner 3: Victory, then Defeat */
    b_cond(&b, 22, 0, 0);
    b_act(&b, 1);
    b_act(&b, 2);
    b_trigger(&b, 2);           /* owner 2: Defeat only */
    b_cond(&b, 22, 0, 0);
    b_act(&b, 2);
    b_trigger(&b, 17);          /* every owner, preserved */
    Sc64Z8Condition *probe = b_cond(&b, 2, 0, 0);
    b_act(&b, 3);
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    tick_to_pass(&s, &b, &w);
    CHECK(s.outcome[1] == SC64_Z8_OUTCOME_VICTORY);
    CHECK(s.outcome[3] == SC64_Z8_OUTCOME_VICTORY);
    CHECK(s.outcome[2] == SC64_Z8_OUTCOME_DEFEAT);
    /* The pass still finishes after a terminal action. */
    CHECK(count_log(&w, 'A', row_of(&b, after1)) == 1);
    CHECK(count_log(&w, 'C', cond_row(&b, probe)) == 3);
    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'C', cond_row(&b, probe)) == 3);
    CHECK(sc64_z8_state_valid(&s, &b.program));
}

static void
test_owner_handling(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_z8_shape();

    b_init(&b);
    b_trigger(&b, 3);    /* 0 -> owner 3 */
    b_cond(&b, 22, 0, 0);
    b_act(&b, 8);
    b_act(&b, 3);
    b_trigger(&b, 18);   /* 1 -> Force 1 = owner 3 */
    b_cond(&b, 22, 0, 0);
    b_wait(&b, 5000);
    Sc64Z8Action *slow = b_act(&b, 9);
    b_trigger(&b, 19);   /* 2 -> Force 2 = owner 1 */
    b_cond(&b, 22, 0, 0);
    b_wait(&b, 100);
    Sc64Z8Action *fast = b_act(&b, 12);
    b_trigger(&b, 20);   /* 3 -> Force 3 = owner 2 */
    Sc64Z8Condition *per_owner = b_cond(&b, 2, 0, 1);
    per_owner->player = 13;
    b_act(&b, 26);
    b_trigger(&b, 17);   /* 4 -> every active owner */
    b_cond(&b, 22, 0, 0);
    b_act(&b, 28)->location = 1;
    b_trigger(&b, 5);    /* 5 -> inactive slot: no instance */
    b_cond(&b, 22, 0, 0);
    b_act(&b, 8);
    b_trigger(&b, 21);   /* 6 -> Force 4: only inactive slots */
    b_cond(&b, 22, 0, 0);
    b_act(&b, 8);
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    static const u8 expected[][2] = {
        {1, 2}, {1, 4}, {2, 3}, {2, 4}, {3, 0}, {3, 1}, {3, 4}};
    CHECK(s.instance_count == 7);
    for (unsigned i = 0; i < 7; ++i)
        CHECK(s.instances[i].owner == expected[i][0] &&
              s.instances[i].trigger == expected[i][1]);

    w.count_by_row[cond_row(&b, per_owner)] = 1;
    tick_to_pass(&s, &b, &w);
    /* Callback owners are instance owners, in ascending owner order. */
    static const u8 order[] = {1, 2, 2, 3, 3};
    CHECK(w.log_count == 6);
    CHECK(w.log[0].owner == 1 && w.log[1].owner == 2 && w.log[2].kind == 'A');
    CHECK(w.log[1].kind == 'C');
    for (unsigned i = 0, k = 0; i < w.log_count; ++i) {
        if (w.log[i].kind != 'A') continue;
        CHECK(k < sizeof(order) && w.log[i].owner == order[k++]);
    }
    CHECK(s.waiting_mask == ((1u << 1) | (1u << 3)));

    /* Owner 1's short wait does not wait for owner 3's long one. */
    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'A', row_of(&b, fast)) == 1);
    CHECK(count_log(&w, 'A', row_of(&b, slow)) == 0);
    CHECK(s.waiting_mask == (1u << 3));
    while (s.waiting_mask) CHECK(sc64_z8_tick(&s, &b.program, &OPS, &w, 0));
    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'A', row_of(&b, slow)) == 1);
}

static void
test_shared_waits(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_owner(0);

    b_init(&b);
    b_trigger(&b, 0);                     /* 0: starts late, long wait */
    Sc64Z8Condition *late = b_cond(&b, 2, 0, 1);
    b_wait(&b, 3000);
    Sc64Z8Action *x = b_act(&b, 8);
    b_trigger(&b, 0);                     /* 1: short wait armed first */
    b_cond(&b, 22, 0, 0);
    Sc64Z8Action *before = b_act(&b, 12);
    b_wait(&b, 100);
    Sc64Z8Action *y = b_act(&b, 9);
    b_trigger(&b, 0);                     /* 2: non-timed work still runs */
    b_cond(&b, 22, 0, 0);
    Sc64Z8Action *free_run = b_act(&b, 26);
    b_wait(&b, 42);
    Sc64Z8Action *z = b_act(&b, 28);
    z->location = 1;
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'A', row_of(&b, before)) == 1);
    CHECK(count_log(&w, 'A', row_of(&b, free_run)) == 1);
    CHECK(s.waiting_mask == 1 && s.wait_ms[0] == 100);
    CHECK(instance(&s, 0, 1)->flags & SC64_Z8_INST_ARMED);
    CHECK(!(instance(&s, 0, 2)->flags & SC64_Z8_INST_ARMED));

    /* 100 ms expires on the third decrement; before that pass, trigger 0
     * becomes true and arms its own wait ahead of trigger 1 in order. */
    w.count_by_row[cond_row(&b, late)] = 1;
    u32 frame = tick_to_pass(&s, &b, &w);
    CHECK(frame == 2 + 3 + 1);
    CHECK(s.wait_ms[0] == 3000);
    CHECK(instance(&s, 0, 0)->flags & SC64_Z8_INST_ARMED);
    CHECK(instance(&s, 0, 1)->flags & SC64_Z8_INST_ARMED);  /* retained */
    CHECK(count_log(&w, 'A', row_of(&b, y)) == 0);
    CHECK(sc64_z8_state_valid(&s, &b.program));

    while (s.waiting_mask) CHECK(sc64_z8_tick(&s, &b.program, &OPS, &w, 0));
    tick_to_pass(&s, &b, &w);
    /* Trigger 1 resumes past its already-served wait in the same pass as
     * trigger 0; trigger 2 then arms the next wait. */
    int ix = log_index(&w, 'A', row_of(&b, x), 0);
    int iy = log_index(&w, 'A', row_of(&b, y), 0);
    CHECK(ix >= 0 && iy > ix);
    CHECK(count_log(&w, 'A', row_of(&b, z)) == 0);
    CHECK(s.waiting_mask == 1 && s.wait_ms[0] == 42);
    /* A timer equal to the frame step expires on its first decrement. */
    CHECK(sc64_z8_tick(&s, &b.program, &OPS, &w, 0));
    CHECK(s.waiting_mask == 0 && s.pass_counter == 0);
    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'A', row_of(&b, z)) == 1);
}

static void
test_timed_presentation(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_owner(0);

    b_init(&b);
    b_trigger(&b, 0);
    b_cond(&b, 22, 0, 0);
    Sc64Z8Action *pan = b_act(&b, 10);
    pan->location = 3;
    Sc64Z8Action *after_pan = b_act(&b, 8);
    Sc64Z8Action *talk = b_act(&b, 7);
    talk->location = 1;
    talk->modifier = 9;
    talk->flags = 0x10;
    Sc64Z8Action *after_talk = b_act(&b, 9);
    Sc64Z8Action *no_loc = b_act(&b, 7);
    Sc64Z8Action *end = b_act(&b, 12);
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    w.timed_duration = SC64_Z8_WAIT_INDEFINITE;
    tick_to_pass(&s, &b, &w);
    CHECK(w.log_count == 2);
    CHECK(w.log[0].phase == SC64_Z8_TIMED_ELIGIBLE && w.log[1].phase == SC64_Z8_TIMED_START);
    CHECK(w.log[1].has_rect && w.log[1].rect.left == 200);
    CHECK(s.wait_ms[0] == SC64_Z8_WAIT_INDEFINITE);
    for (unsigned i = 0; i < 400; ++i)
        CHECK(sc64_z8_tick(&s, &b.program, &OPS, &w, 0));
    CHECK(s.waiting_mask == 1 && count_log(&w, 'A', row_of(&b, after_pan)) == 0);

    /* External completion releases it; the pass then follows in 2 frames. */
    CHECK(!sc64_z8_release_wait(&s, 1));
    u32 frame = s.frames;
    CHECK(sc64_z8_release_wait(&s, 0));
    w.timed_duration = 250;
    CHECK(tick_to_pass(&s, &b, &w) == frame + 2);
    CHECK(count_log(&w, 'A', row_of(&b, after_pan)) == 1);
    /* Eligibility was re-tested on every pass while it waited; FINISH runs
     * once, before the following action. */
    unsigned finishes = 0, eligible = 0;
    for (unsigned i = 0; i < w.log_count; ++i) {
        if (w.log[i].kind != 'T' || w.log[i].row != row_of(&b, pan)) continue;
        finishes += w.log[i].phase == SC64_Z8_TIMED_FINISH;
        eligible += w.log[i].phase == SC64_Z8_TIMED_ELIGIBLE;
    }
    CHECK(finishes == 1 && eligible == 2 + 400 / 31);
    CHECK(s.wait_ms[0] == 250);

    /* An ineligible presentation completes without effect even while the
     * owner waits, because eligibility is tested first. */
    u32 armed_frame = s.frames;
    while (s.waiting_mask) CHECK(sc64_z8_tick(&s, &b.program, &OPS, &w, 0));
    CHECK(s.frames == armed_frame + 6);   /* 250 ms at 42 ms */
    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'A', row_of(&b, after_talk)) == 1);
    CHECK(count_log(&w, 'T', row_of(&b, no_loc)) == 0);
    CHECK(count_log(&w, 'A', row_of(&b, end)) == 1);
    CHECK(instance(&s, 0, 0)->flags == SC64_Z8_INST_DONE);

    Builder b2;
    b_init(&b2);
    b_trigger(&b2, 0);
    b_cond(&b2, 22, 0, 0);
    b_wait(&b2, 1000);
    b_trigger(&b2, 0);
    b_cond(&b2, 22, 0, 0);
    Sc64Z8Action *skip = b_act(&b2, 7);
    skip->location = 1;
    Sc64Z8Action *skip_after = b_act(&b2, 8);
    world_init(&w, &b2);
    w.timed_eligible = 0;
    CHECK(sc64_z8_init(&s, &b2.program, &setup));
    tick_to_pass(&s, &b2, &w);
    CHECK(s.waiting_mask == 1);
    CHECK(count_log(&w, 'A', row_of(&b2, skip_after)) == 1);
    CHECK(count_log(&w, 'T', row_of(&b2, skip)) == 1);
}

static void
test_locations_and_world_effects(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_z8_shape();

    b_init(&b);
    b_trigger(&b, 3);
    Sc64Z8Condition *kills = b_cond(&b, 5, 0, 1);
    kills->player = 13;
    kills->unit = 154;
    Sc64Z8Action *move = b_act(&b, 38);
    move->location = 1;
    move->group1 = 1;
    move->group2 = 2;
    move->unit = 154;
    b_act(&b, 3);
    b_trigger(&b, 3);
    Sc64Z8Condition *bring = b_cond(&b, 3, 0, 1);
    bring->location = 2;
    Sc64Z8Action *create = b_act(&b, 11);
    create->location = 2;
    create->group1 = 19;
    create->unit = 74;
    create->flags = 0x10;
    Sc64Z8Action *invincible = b_act(&b, 43);
    invincible->location = 2;
    invincible->group1 = 1;
    invincible->modifier = 4;
    invincible->flags = 0x10;
    Sc64Z8Action *create_none = b_act(&b, 11);  /* location 0: no effect */
    Sc64Z8Action *move_none = b_act(&b, 38);
    move_none->location = 1;                    /* destination 0 */
    b_trigger(&b, 19);
    b_cond(&b, 22, 0, 0);
    Sc64Z8Action *ai[3];
    static const u32 fourcc[3] = {SC64_Z8_AI_ZER8, SC64_Z8_AI_ZE8T, SC64_Z8_AI_MVTE};
    for (unsigned i = 0; i < 3; ++i) {
        ai[i] = b_act(&b, 16);
        ai[i]->group2 = fourcc[i];
        ai[i]->location = 4 + i;
    }
    Sc64Z8Action *resources = b_act(&b, 26);
    resources->group1 = 19;
    resources->group2 = 8000;
    resources->modifier = 7;
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));

    /* Pass 1: kill count unmet; AI scripts keep their distinct FourCCs. */
    tick_to_pass(&s, &b, &w);
    CHECK(count_log(&w, 'F', row_of(&b, move)) == 0);
    int k = log_index(&w, 'C', cond_row(&b, kills), 0);
    CHECK(k >= 0 && !w.log[k].has_rect && w.log[k].owner == 3);
    for (unsigned i = 0; i < 3; ++i) {
        int at = log_index(&w, 'A', row_of(&b, ai[i]), 0);
        CHECK(at >= 0 && w.log[at].operand == fourcc[i] && w.log[at].owner == 1);
        CHECK(w.log[at].rect.left == (s16)(100 * (3 + i)));
    }
    CHECK(count_log(&w, 'A', row_of(&b, resources)) == 1);

    /* Pass 2: MoveLocation recentres location 2 on the anchor actor, and a
     * later Bring in the same pass already sees the moved rectangle. */
    w.count_by_row[cond_row(&b, kills)] = 1;
    w.count_by_row[cond_row(&b, bring)] = 1;
    w.find_x = 1000;
    w.find_y = 2000;
    tick_to_pass(&s, &b, &w);
    CHECK(s.locations[1].left == 968 && s.locations[1].right == 1032);
    CHECK(s.locations[1].top == 1984 && s.locations[1].bottom == 2016);
    int f = log_index(&w, 'F', row_of(&b, move), 0);
    CHECK(f >= 0 && w.log[f].rect.right == 4096);
    int c = log_index(&w, 'C', cond_row(&b, bring), 1);
    CHECK(c > f && w.log[c].rect.left == 968);
    int cr = log_index(&w, 'A', row_of(&b, create), 0);
    CHECK(cr > c && w.log[cr].rect.left == 968 && w.log[cr].owner == 3);
    CHECK(sc64_z8_create_count(create) == 1);
    int inv = log_index(&w, 'A', row_of(&b, invincible), 0);
    CHECK(inv > cr && w.log[inv].rect.top == 1984);
    CHECK(count_log(&w, 'A', row_of(&b, create_none)) == 0);
    CHECK(count_log(&w, 'F', row_of(&b, move_none)) == 0);

    /* No anchor: centre of the source location. Then clamp every edge. */
    struct { int found; s16 x, y; Sc64Z8Rect want; } cases[] = {
        {0, 0, 0, {2016, 2032, 2080, 2064}},
        {1, 10, 500, {0, 484, 64, 516}},
        {1, 4090, 500, {4031, 484, 4095, 516}},
        {1, 500, 5, {468, 0, 532, 32}},
        {1, 500, 4093, {468, 4063, 532, 4095}},
        {1, -300, -300, {0, 0, 64, 32}},
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        w.find_result = cases[i].found;
        w.find_x = cases[i].x;
        w.find_y = cases[i].y;
        tick_to_pass(&s, &b, &w);
        const Sc64Z8Rect *r = &s.locations[1];
        CHECK(r->left == cases[i].want.left && r->top == cases[i].want.top &&
              r->right == cases[i].want.right && r->bottom == cases[i].want.bottom);
    }
    CHECK(sc64_z8_state_valid(&s, &b.program));
}

static void
expect_fault(Builder *b, World *w, Sc64Z8SchedulerState *s, unsigned fault,
             unsigned owner, unsigned trigger, unsigned slot)
{
    u32 passes = s->passes;
    int ok = 1;
    for (unsigned i = 0; i < 64 && ok; ++i)
        ok = sc64_z8_tick(s, &b->program, &OPS, w, 0);
    CHECK(!ok && s->passes == passes);
    CHECK(s->fault == fault && s->fault_owner == owner &&
          s->fault_trigger == trigger && s->fault_slot == slot);
    CHECK(!sc64_z8_state_valid(s, &b->program));
    CHECK(!sc64_z8_tick(s, &b->program, &OPS, w, 0));
    CHECK(!sc64_z8_tick_prevalidated(s, &b->program, &OPS, w, 0));
    Sc64Z8SchedulerState copy;
    memset(&copy, 0x5a, sizeof(copy));
    Sc64Z8SchedulerState untouched = copy;
    CHECK(!sc64_z8_restore(&copy, s, &b->program));
    CHECK(!memcmp(&copy, &untouched, sizeof(copy)));
}

static void
test_unsupported_fails_explicitly(void)
{
    Builder b;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_owner(2);

    b_init(&b);
    b_trigger(&b, 2);
    b_cond(&b, 22, 0, 0);
    b_act(&b, 9);
    Sc64Z8Action *bad = b_act(&b, 11);
    bad->location = 1;
    Sc64Z8Action *never = b_act(&b, 8);

    static const int results[] = {-1, 0, 2};
    for (unsigned i = 0; i < 3; ++i) {
        world_init(&w, &b);
        CHECK(sc64_z8_init(&s, &b.program, &setup));
        w.fail_row = (int)row_of(&b, bad);
        w.action_result = results[i];
        expect_fault(&b, &w, &s, SC64_Z8_FAULT_ACTION, 2, 0, 1);
        /* Slot 0 ran; the failing slot was attempted; nothing after it. */
        CHECK(count_log(&w, 'A', 0) == 1 && count_log(&w, 'A', row_of(&b, bad)) == 1);
        CHECK(count_log(&w, 'A', row_of(&b, never)) == 0);
        CHECK(s.instances[0].flags == SC64_Z8_INST_EXECUTING);
    }

    /* Missing callbacks are unsupported, not silent successes. */
    static const Sc64Z8WorldOps none = {0, 0, 0, 0};
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));
    CHECK(sc64_z8_tick(&s, &b.program, &none, &w, 0));
    CHECK(!sc64_z8_tick(&s, &b.program, &none, &w, 0));
    CHECK(s.fault == SC64_Z8_FAULT_ACTION && s.fault_slot == 0);

    /* Count query failure. */
    Builder bc;
    b_init(&bc);
    b_trigger(&bc, 2);
    b_cond(&bc, 22, 0, 0);
    Sc64Z8Condition *kill = b_cond(&bc, 5, 0, 1);
    (void)kill;
    b_act(&bc, 8);
    world_init(&w, &bc);
    w.count_result = 0;
    CHECK(sc64_z8_init(&s, &bc.program, &setup));
    expect_fault(&bc, &w, &s, SC64_Z8_FAULT_CONDITION, 2, 0, 1);
    CHECK(w.log_count == 1);

    /* Timed START, FINISH and anchor failures. */
    Builder bt;
    b_init(&bt);
    b_trigger(&bt, 2);
    b_cond(&bt, 22, 0, 0);
    b_act(&bt, 10)->location = 1;
    Sc64Z8Action *mv = b_act(&bt, 38);
    mv->location = 1;
    mv->group2 = 2;
    world_init(&w, &bt);
    w.timed_start_result = 0;
    CHECK(sc64_z8_init(&s, &bt.program, &setup));
    expect_fault(&bt, &w, &s, SC64_Z8_FAULT_ACTION, 2, 0, 0);
    CHECK(s.waiting_mask == 0);

    world_init(&w, &bt);
    w.timed_duration = 100;
    CHECK(sc64_z8_init(&s, &bt.program, &setup));
    tick_to_pass(&s, &bt, &w);
    w.timed_finish_result = -1;
    expect_fault(&bt, &w, &s, SC64_Z8_FAULT_ACTION, 2, 0, 0);

    /* Eligibility changing under an armed presentation is not modelled. */
    world_init(&w, &bt);
    w.timed_duration = 100;
    CHECK(sc64_z8_init(&s, &bt.program, &setup));
    tick_to_pass(&s, &bt, &w);
    w.timed_eligible = 0;
    expect_fault(&bt, &w, &s, SC64_Z8_FAULT_ACTION, 2, 0, 0);

    world_init(&w, &bt);
    w.timed_eligible = 0;
    w.find_result = -1;
    CHECK(sc64_z8_init(&s, &bt.program, &setup));
    expect_fault(&bt, &w, &s, SC64_Z8_FAULT_ACTION, 2, 0, 1);

    /* Corrupted state reaching the checked runner is a state fault. */
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));
    s.instances[0].flags = SC64_Z8_INST_EXECUTING;
    CHECK(!sc64_z8_tick(&s, &b.program, &OPS, &w, 0));
    CHECK(s.fault == SC64_Z8_FAULT_STATE);
    CHECK(sc64_z8_init(&s, &b.program, &setup));
    s.frame_ms = 43;
    CHECK(!sc64_z8_tick_prevalidated(&s, &b.program, &OPS, &w, 0));
    CHECK(s.fault == SC64_Z8_FAULT_STATE);
}

/* A program that exercises every stateful path across a long run. */
static void
build_continuity(Builder *b)
{
    b_init(b);
    b_trigger(b, 3);                          /* 0 */
    b_cond(b, 12, 0, 2);
    b_switch(b, 1, 4);
    Sc64Z8Action *pan = b_act(b, 10);
    pan->location = 2;
    b_act(b, 8);
    b_trigger(b, 18);                         /* 1: preserved spawner */
    b_switch_cond(b, 1, 2);
    b_cond(b, 2, 1, 6);
    Sc64Z8Action *create = b_act(b, 11);
    create->location = 3;
    create->group1 = 19;
    create->unit = 74;
    b_wait(b, 1500);
    b_act(b, 3);
    b_trigger(b, 19);                         /* 2: owner 1 mover */
    b_cond(b, 22, 0, 0);
    b_act(b, 3);
    Sc64Z8Action *move = b_act(b, 38);
    move->location = 1;
    move->group1 = 1;
    move->group2 = 4;
    move->unit = 154;
    b_wait(b, 700);
    Sc64Z8Action *talk = b_act(b, 7);
    talk->location = 1;
    b_trigger(b, 20);                         /* 3: owner 2 toggles */
    b_cond(b, 3, 0, 1)->location = 4;
    b_switch(b, 2, 6);
    b_act(b, 3);
    b_trigger(b, 2);                          /* 4: owner 2, kill gate */
    b_cond(b, 5, 10, 6);
    b_switch_cond(b, 2, 3);
    b_act(b, 43)->location = 4;
    b_act(b, 1);
}

static void
continuity_world(World *w, const Builder *b, u32 frame)
{
    /* Deterministic world inputs as a function of the frame only. */
    for (unsigned r = 0; r < b->program.condition_count; ++r)
        w->count_by_row[r] = (frame / 37 + r * 3) % 7;
    w->find_x = (s16)((frame * 53) % 4200);
    w->find_y = (s16)((frame * 29) % 4200);
    w->find_result = (frame / 50) % 3 != 0;
    w->timed_duration = (frame / 90) % 2 ? SC64_Z8_WAIT_INDEFINITE : 600;
}

static void
continuity_step(Sc64Z8SchedulerState *s, const Builder *b, World *w, u32 frame)
{
    continuity_world(w, b, frame);
    /* A camera pan or similar completion arrives every 113 frames. */
    if (frame % 113 == 0)
        for (unsigned o = 0; o < 8; ++o)
            if (s->wait_ms[o] == SC64_Z8_WAIT_INDEFINITE &&
                (s->waiting_mask & (1u << o)))
                CHECK(sc64_z8_release_wait(s, o));
    CHECK(sc64_z8_tick_prevalidated(s, &b->program, &OPS, w, frame % 97 == 5));
    CHECK(sc64_z8_state_valid(s, &b->program));
}

static void
test_save_restore_continuity(void)
{
    static Builder b;
    static World reference, resumed;
    Sc64Z8SchedulerState s, r;
    Sc64Z8Setup setup = setup_z8_shape();
    enum { FRAMES = 900 };

    build_continuity(&b);
    world_init(&reference, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));
    unsigned saw_wait = 0, saw_indefinite = 0, saw_armed = 0, saw_preserve = 0;
    for (u32 f = 1; f <= FRAMES; ++f) {
        continuity_step(&s, &b, &reference, f);
        saw_wait |= s.waiting_mask;
        for (unsigned o = 0; o < 8; ++o)
            saw_indefinite |= s.wait_ms[o] == SC64_Z8_WAIT_INDEFINITE;
        for (unsigned i = 0; i < s.instance_count; ++i) {
            saw_armed |= s.instances[i].flags & SC64_Z8_INST_ARMED;
            saw_preserve |= s.instances[i].flags & SC64_Z8_INST_PRESERVE;
        }
    }
    CHECK(saw_wait && saw_indefinite && saw_armed && saw_preserve);
    CHECK(s.passes > 40 && reference.log_count > 120);
    CHECK(s.outcome[2] == SC64_Z8_OUTCOME_VICTORY && !s.outcome[1] && !s.outcome[3]);
    CHECK(s.locations[3].left != b.locations[3].left);

    /* Split at every frame: save a byte image, restore it over garbage
     * into a new state, and finish. The result must be identical. */
    for (u32 split = 0; split <= FRAMES; ++split) {
        Sc64Z8SchedulerState live;
        world_init(&resumed, &b);
        CHECK(sc64_z8_init(&live, &b.program, &setup));
        for (u32 f = 1; f <= split; ++f) continuity_step(&live, &b, &resumed, f);
        unsigned char image[sizeof(Sc64Z8SchedulerState)];
        memcpy(image, &live, sizeof(image));
        memset(&live, 0xa5, sizeof(live));
        memset(&r, 0xc3, sizeof(r));
        Sc64Z8SchedulerState loaded;
        memcpy(&loaded, image, sizeof(loaded));
        CHECK(sc64_z8_restore(&r, &loaded, &b.program));
        for (u32 f = split + 1; f <= FRAMES; ++f)
            continuity_step(&r, &b, &resumed, f);
        CHECK(!memcmp(&r, &s, sizeof(s)));
        CHECK(resumed.log_count == reference.log_count);
        CHECK(!memcmp(resumed.log, reference.log,
                      reference.log_count * sizeof(LogEntry)));
    }
}

static void
test_restore_rejects_corruption(void)
{
    Builder b, other;
    World w;
    Sc64Z8SchedulerState s;
    Sc64Z8Setup setup = setup_z8_shape();

    build_continuity(&b);
    world_init(&w, &b);
    CHECK(sc64_z8_init(&s, &b.program, &setup));
    for (u32 f = 1; f <= 240; ++f) continuity_step(&s, &b, &w, f);
    CHECK(s.waiting_mask);
    CHECK(sc64_z8_state_valid(&s, &b.program));

    unsigned armed = 0, idle = 0, done = 0;
    for (unsigned i = 0; i < s.instance_count; ++i) {
        if (s.instances[i].flags & SC64_Z8_INST_ARMED) armed = i;
        if (s.instances[i].flags == 0 ||
            s.instances[i].flags == SC64_Z8_INST_PRESERVE) idle = i;
        if (s.instances[i].flags == SC64_Z8_INST_DONE) done = i;
    }
    CHECK(s.instances[armed].flags & SC64_Z8_INST_ARMED);
    unsigned waiting_owner = s.instances[armed].owner;

    for (unsigned mutation = 0; mutation < 22; ++mutation) {
        Sc64Z8SchedulerState bad = s;
        switch (mutation) {
        case 0: bad.version ^= 1; break;
        case 1: bad.trigger_count++; break;
        case 2: bad.pass_counter = SC64_Z8_PASS_RELOAD + 1; break;
        case 3: bad.second_counter = SC64_Z8_SECOND_RELOAD + 1; break;
        case 4: bad.frame_ms = 41; break;
        case 5: bad.fault = SC64_Z8_FAULT_ACTION; break;
        case 6: bad.instances[idle].flags |= 0x10; break;
        case 7: bad.instances[idle].cursor = 1; break;
        case 8: bad.instances[armed].cursor = 0; break; /* not a timed slot */
        case 9: bad.instances[done].cursor = 1; break;
        case 10: bad.instances[done].flags |= SC64_Z8_INST_PRESERVE; break;
        case 11: bad.instances[armed].flags &= (u8)~SC64_Z8_INST_ARMED;
                 for (unsigned i = 0; i < bad.instance_count; ++i)
                     if (bad.instances[i].owner == waiting_owner)
                         bad.instances[i].flags &= (u8)~SC64_Z8_INST_ARMED;
                 break;
        case 12: bad.waiting_mask = 0; break; /* timer left behind */
        case 13: bad.outcome[1] = 1; break;
        case 14: bad.outcome[0] = SC64_Z8_OUTCOME_VICTORY; break;
        case 15: bad.instances[bad.instance_count].trigger = 1; break;
        case 16: { Sc64Z8Instance t = bad.instances[0];
                   bad.instances[0] = bad.instances[1];
                   bad.instances[1] = t; } break;
        case 17: bad.instance_count--; break;
        case 18: bad.locations[2].left = (s16)(bad.locations[2].right + 1); break;
        case 19: bad.locations[SC64_Z8_LOCATION_LIMIT - 1].top = 1; break;
        case 20: bad.force[3] = 2; break;
        case 21: bad.active_mask &= (u8)~(1u << 1); break;
        }
        CHECK(!sc64_z8_state_valid(&bad, &b.program));
        Sc64Z8SchedulerState out;
        memset(&out, 0x77, sizeof(out));
        Sc64Z8SchedulerState untouched = out;
        CHECK(!sc64_z8_restore(&out, &bad, &b.program));
        CHECK(!memcmp(&out, &untouched, sizeof(out)));
    }

    /* A state belongs to the program it was built from. */
    build_continuity(&other);
    other.actions[0].modifier = 5;
    CHECK(sc64_z8_state_valid(&s, &other.program));
    other.triggers[4].enabled_players[2] = 0;
    other.triggers[4].enabled_players[3] = 1;
    CHECK(!sc64_z8_state_valid(&s, &other.program));
}

int
main(void)
{
    test_validation();
    test_cadence();
    test_repeated_and_preserved();
    test_preserve_lifecycle_with_wait();
    test_simultaneous_conditions();
    test_order_dependent_actions();
    test_outcomes();
    test_owner_handling();
    test_shared_waits();
    test_timed_presentation();
    test_locations_and_world_effects();
    test_unsupported_fails_explicitly();
    test_save_restore_continuity();
    test_restore_rejects_corruption();
    printf("sc64_z8_triggers: ok (state %zu bytes, instance %zu, "
           "condition row %zu, action row %zu)\n",
           sizeof(Sc64Z8SchedulerState), sizeof(Sc64Z8Instance),
           sizeof(Sc64Z8Condition), sizeof(Sc64Z8Action));
    return 0;
}
