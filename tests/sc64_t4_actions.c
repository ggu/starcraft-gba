#include <assert.h>
#include <stdio.h>

#define SC64_T4_RUNTIME 1
#include "../starcraft/sc64_t4_actions.h"

static void
test_creation_count_exhaustive(void)
{
    for (unsigned raw = 0; raw <= 255; ++raw) {
        unsigned count = 0xdeadbeefu;
        assert(sc64_t4_creation_count(raw, &count));
        assert(count == (raw ? raw : 1));
    }
    unsigned count = 0x12345678u;
    assert(!sc64_t4_creation_count(256, &count));
    assert(count == 0x12345678u);
    assert(!sc64_t4_creation_count(0, NULL));
}

static void
test_doodad_initial_flags(void)
{
    unsigned enabled = 7;
    assert(sc64_t4_doodad_initial_enabled(0, &enabled) && enabled == 1);
    enabled = 7;
    assert(sc64_t4_doodad_initial_enabled(0x0001, &enabled) && enabled == 1);
    enabled = 7;
    assert(sc64_t4_doodad_initial_enabled(SC64_T4_THG2_DISABLED_FLAG,
                                          &enabled) && enabled == 0);
    enabled = 7;
    assert(!sc64_t4_doodad_initial_enabled(SC64_T4_THG2_SPRITE_FLAG,
                                           &enabled) && enabled == 7);
    enabled = 7;
    assert(!sc64_t4_doodad_initial_enabled(0x9001, &enabled) && enabled == 7);
    enabled = 7;
    assert(sc64_t4_doodad_initial_enabled(0x0fffu, &enabled) && enabled == 1);
    assert(!sc64_t4_doodad_initial_enabled(0, NULL));

    for (unsigned flags = 0; flags <= 0xffff; ++flags) {
        enabled = 0xdeadbeefu;
        int result = sc64_t4_doodad_initial_enabled((u16)flags, &enabled);
        if (flags & SC64_T4_THG2_SPRITE_FLAG) {
            assert(!result && enabled == 0xdeadbeefu);
        } else {
            assert(result);
            assert(enabled == !(flags & SC64_T4_THG2_DISABLED_FLAG));
        }
    }
}

static void
test_doodad_state_transition_exhaustive(void)
{
    for (unsigned current = 0; current <= 1; ++current) {
        for (unsigned modifier = 0; modifier <= 255; ++modifier) {
            unsigned next = 0xfeedu;
            int result = sc64_t4_doodad_set_state(0x0001, current,
                                                   modifier, &next);
            if (modifier == 0 || modifier == 6) {
                assert(result && next == !current);
            } else if (modifier == 4) {
                assert(result && next == 1);
            } else if (modifier == 5) {
                assert(result && next == 0);
            } else {
                assert(!result && next == 0xfeedu);
            }
        }
    }

    unsigned next = 7;
    for (unsigned current = 0; current <= 1; ++current) {
        for (unsigned modifier = 0; modifier <= 255; ++modifier) {
            next = 7;
            assert(!sc64_t4_doodad_set_state(SC64_T4_THG2_SPRITE_FLAG,
                                              current, modifier, &next));
            assert(next == 7);
        }
    }
    next = 0xabcdu;
    assert(!sc64_t4_doodad_set_state(0, 2, 0, &next));
    assert(next == 0xabcdu);
    assert(!sc64_t4_doodad_set_state(0, 1, 4, NULL));

    for (unsigned flags = 0; flags <= 0xffff; ++flags) {
        for (unsigned current = 0; current <= 1; ++current) {
            for (unsigned modifier = 0; modifier <= 6; ++modifier) {
                unsigned result_value = 0xbeefu;
                int result = sc64_t4_doodad_set_state(
                    (u16)flags, current, modifier, &result_value);
                int supported_modifier = modifier == 0 || modifier == 4 ||
                                         modifier == 5 || modifier == 6;
                int is_sprite = !!(flags & SC64_T4_THG2_SPRITE_FLAG);
                if (is_sprite || !supported_modifier) {
                    assert(!result && result_value == 0xbeefu);
                } else {
                    assert(result);
                    unsigned expected = (modifier == 0 || modifier == 6)
                        ? !current : (modifier == 4);
                    assert(result_value == expected);
                }
            }
        }
    }
}

int
main(void)
{
    test_creation_count_exhaustive();
    test_doodad_initial_flags();
    test_doodad_state_transition_exhaustive();
    puts("sc64 T4 native action operand tests passed");
    return 0;
}
