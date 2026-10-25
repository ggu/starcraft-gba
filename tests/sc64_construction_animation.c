#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "../starcraft/sc64_construction_animation.h"
#include "../starcraft/sc64_construction_source.h"

static const SC64ConstructionAnimStep intro_loop_steps[] = {
    { 0, 2 }, { 1, 3 }, { 2, 4 }, { 3, 5 }
};
static const SC64ConstructionAnimDesc intro_loop = {
    intro_loop_steps, 4, 2, 4
};

static const SC64ConstructionAnimStep hold_steps[] = {
    { 1, 1 }, { 2, 2 }
};
static const SC64ConstructionAnimDesc hold = {
    hold_steps, 2, SC64_CONSTRUCTION_HOLD_LAST, 3
};

static void expect(const SC64ConstructionAnimDesc *desc, uint32_t elapsed,
                   int expected_ok, unsigned expected_frame)
{
    unsigned frame = 99;
    int ok = sc64_construction_animation_frame(desc, elapsed, &frame);
    assert(ok == expected_ok);
    if (ok) {
        assert(frame == expected_frame);
        assert(frame < desc->frame_count);
    } else assert(frame == 99);
}

int main(void)
{
    SC64ConstructionAnimStep invalid_steps[4];
    SC64ConstructionAnimDesc invalid;
    unsigned output = 0;

    /* Exact step boundaries; intro frames occur once, then loop frames repeat. */
    expect(&intro_loop, 0, 1, 0);
    expect(&intro_loop, 1, 1, 0);
    expect(&intro_loop, 2, 1, 1);
    expect(&intro_loop, 4, 1, 1);
    expect(&intro_loop, 5, 1, 2);
    expect(&intro_loop, 6, 1, 2);
    expect(&intro_loop, 9, 1, 3);
    expect(&intro_loop, 10, 1, 3);
    expect(&intro_loop, 14, 1, 2);
    expect(&intro_loop, 15, 1, 2);
    expect(&intro_loop, 19, 1, 3);
    expect(&intro_loop, UINT32_MAX, 1, 3);

    /* Hold-last descriptor terminates at its final authored frame. */
    expect(&hold, 0, 1, 1);
    expect(&hold, 1, 1, 2);
    expect(&hold, 2, 1, 2);
    expect(&hold, UINT32_MAX, 1, 2);

    assert(!sc64_construction_animation_frame(&intro_loop, 0, NULL));
    assert(!sc64_construction_animation_frame(NULL, 0, &output));

    /* Invalid descriptors fail closed without emitting a frame. */
    invalid = intro_loop;
    invalid.steps = NULL;
    expect(&invalid, 0, 0, 99);
    invalid = intro_loop;
    invalid.step_count = 0;
    expect(&invalid, 0, 0, 99);
    invalid = intro_loop;
    invalid.loop_start = invalid.step_count;
    expect(&invalid, 0, 0, 99);
    invalid = intro_loop;
    invalid.frame_count = 0;
    expect(&invalid, 0, 0, 99);

    for (unsigned scenario = 0; scenario < 3; ++scenario) {
        for (unsigned i = 0; i < 4; ++i) invalid_steps[i] = intro_loop_steps[i];
        invalid = intro_loop;
        invalid.steps = invalid_steps;
        if (scenario == 0) invalid_steps[1].duration_source_updates = 0;
        if (scenario == 1) invalid_steps[2].frame = invalid.frame_count;
        if (scenario == 2) invalid.step_count = SC64_CONSTRUCTION_ANIM_MAX_STEPS + 1u;
        expect(&invalid, 3, 0, 99);
    }

    /* Accumulation overflow is rejected before any timing decision. */
    {
        const SC64ConstructionAnimStep overflow_steps[] = {
            { 0, UINT32_MAX }, { 1, 1 }
        };
        const SC64ConstructionAnimDesc overflow = {
            overflow_steps, 2, 1, 2
        };
        expect(&overflow, 0, 0, 99);
    }
    (void)INT_MAX; /* Large elapsed value is covered by UINT32_MAX cases above. */

    /* Stage boundaries and clock conversion remain stable across render/save
     * calls: the same saved progress always returns the same frame clock. */
    for (int duration = 1; duration <= 120; ++duration) {
        for (int race = 1; race <= 2; ++race) {
            for (int progress = 0; progress < duration; ++progress) {
                unsigned stage = 99, again = 99;
                uint32_t elapsed = UINT32_MAX, replay = UINT32_MAX;
                assert(sc64_construction_source_phase(race,progress,duration,
                                                       &stage,&elapsed));
                unsigned expected = race == 1 ?
                    ((int64_t)progress*5 > duration) +
                    ((int64_t)progress*5 > (int64_t)duration*2) +
                    ((int64_t)progress*5 > (int64_t)duration*3) :
                    ((int64_t)progress*4 > duration) +
                    ((int64_t)progress*2 > duration);
                assert(stage == expected);
                assert(sc64_construction_source_phase(race,progress,duration,
                                                       &again,&replay));
                assert(stage == again && elapsed == replay);
                assert(elapsed <= (uint32_t)progress);
            }
        }
    }
    {
        unsigned stage = 99;
        uint32_t elapsed = 99;
        assert(sc64_construction_source_phase(1,INT_MAX-1,INT_MAX,&stage,&elapsed));
        assert(stage == 3 && elapsed < INT_MAX);
        assert(sc64_construction_source_phase(2,INT_MAX-1,INT_MAX,&stage,&elapsed));
        assert(stage == 2 && elapsed < INT_MAX);
        assert(sc64_construction_source_phase(3,1,100,&stage,&elapsed));
        assert(stage==0 && elapsed==0);
        assert(sc64_construction_source_phase(3,99,100,&stage,&elapsed));
        assert(stage==0 && elapsed==79);
        assert(!sc64_construction_source_phase(1,-1,100,&stage,&elapsed));
        assert(!sc64_construction_source_phase(1,100,100,&stage,&elapsed));
        assert(!sc64_construction_source_phase(1,0,0,&stage,&elapsed));
        assert(!sc64_construction_source_phase(1,1,100,NULL,&elapsed));
        assert(!sc64_construction_source_phase(1,1,100,&stage,NULL));
    }
    return 0;
}
