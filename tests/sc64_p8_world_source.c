/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional check against an already-prepared private P8 source descriptor. */
#include <assert.h>

#include "../starcraft/sc64_p8_world_adapter.h"

int main(void)
{
    Sc64Z8Rect locations[SC64_P8_SOURCE_LOCATION_COUNT];
    assert(sc64_p8_source_metadata_valid());
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
