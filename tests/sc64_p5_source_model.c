#include <assert.h>
#include "../.local/sc64/zp-stage/generated/scenarios/sc64_p5_scenario_generated.h"
#include "../starcraft/sc64_p5_source_model.h"

int main(void)
{
    unsigned i, creates = 0;
    Sc64P5SourceEffect effect;
    assert(sc64_p5_source_model_valid());
    assert(SC64_P5_SOURCE_MAP_COLS == 128 && SC64_P5_SOURCE_MAP_ROWS == 128);
    for (i = 0; i < SC64_P5_SOURCE_ACTION_COUNT; ++i) {
        const Sc64P5SourceAction *a = &sc64_p5_source_actions[i];
        assert(sc64_p5_source_effect(a->trigger, a->index, &effect));
        if (a->opcode == SC64_P5_ACTION_CREATE_UNIT) {
            ++creates;
            assert(effect.count == 1); /* all eleven CHK modifiers are zero */
        } else {
            assert(effect.count == 0);
        }
    }
    assert(creates == 11);
    assert(sc64_p5_source_create_count(0) == 0);
    assert(sc64_p5_source_effect(4, 0, &effect));
    assert(effect.opcode == SC64_P5_ACTION_SET_INVINCIBILITY);
    assert(effect.raw_unit == 229 && effect.count == 0);
    assert(!sc64_p5_source_effect(11, 0, &effect));
    assert(!sc64_p5_source_effect(4, 3, &effect));
    {
        Sc64P5SourceAction fake = {0};
        fake.opcode = SC64_P5_ACTION_CREATE_UNIT;
        assert(sc64_p5_source_create_count(&fake) == 1);
        fake.modifier = 9;
        assert(sc64_p5_source_create_count(&fake) == 9);
        fake.opcode = SC64_P5_ACTION_SET_INVINCIBILITY;
        assert(sc64_p5_source_create_count(&fake) == 0);
    }
    return 0;
}
