/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional check against an already-prepared private P8 source descriptor. */
#include <assert.h>

#include "../starcraft/sc64_p8_world_adapter.h"

int main(void)
{
    Sc64Z8Rect locations[SC64_P8_SOURCE_LOCATION_COUNT];
    assert(sc64_p8_source_metadata_valid());
    unsigned seen_property[SC64_P8_SOURCE_PROPERTY_COUNT] = {0};
    for (unsigned i = 0; i < SC64_P8_SOURCE_ACTION_COUNT; ++i) {
        const Sc64P8SourceAction *action = &sc64_p8_source_actions[i];
        if (action->opcode != 11u || !action->group2) continue;
        const u8 *record = 0;
        assert(sc64_p8_source_property(action->group2, &record));
        assert(record == sc64_p8_source_properties[action->group2 - 1u]);
        ++seen_property[action->group2 - 1u];
    }
    assert(seen_property[0] && seen_property[1] && seen_property[2]);
    for (unsigned i = 0; i < SC64_P8_SOURCE_PROPERTY_COUNT; ++i)
        assert((seen_property[i] != 0u) ==
               (sc64_p8_source_property_used[i] != 0u));
    assert(sc64_p8_world_locations_init(locations,
                                        SC64_P8_SOURCE_LOCATION_COUNT));
    for (unsigned i = 0; i < SC64_P8_SOURCE_LOCATION_COUNT; ++i) {
        const Sc64P8SourceLocation *source = &sc64_p8_source_locations[i];
        assert(locations[i].left == (s16)source->left);
        assert(locations[i].top == (s16)source->top);
        assert(locations[i].right == (s16)source->right);
        assert(locations[i].bottom == (s16)source->bottom);
    }
    return 0;
}
