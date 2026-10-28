/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional check against an already-prepared private P8 source descriptor. */
#include <assert.h>

#include "../starcraft/sc64_p8_world_adapter.h"

static unsigned source_u16(const u8 *p)
{ return (unsigned)p[0] | ((unsigned)p[1] << 8); }

static u32 source_u32(const u8 *p)
{ return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24); }

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
    unsigned fields_02=0, fields_06=0, fields_0e=0;
    unsigned special_10=0, special_18=0;
    for (unsigned i = 0; i < SC64_P8_SOURCE_PROPERTY_COUNT; ++i) {
        if (!sc64_p8_source_property_used[i]) continue;
        const u8 *p=sc64_p8_source_properties[i];
        unsigned valid_special=source_u16(p);
        unsigned valid_fields=source_u16(p+2);
        unsigned special=source_u16(p+14);
        assert(valid_special==0x10u || valid_special==0x18u);
        assert(valid_fields==0x02u || valid_fields==0x06u ||
               valid_fields==0x0eu);
        assert(p[4]==0xffu && p[5]==100u);
        assert(p[6]==((valid_fields&0x04u)?100u:0u));
        assert(p[7]==((valid_fields&0x08u)?100u:0u));
        assert(source_u32(p+8)==0u && source_u16(p+12)==0u &&
               source_u32(p+16)==0u);
        assert(special==0u || special==0x10u);
        fields_02 += valid_fields==0x02u;
        fields_06 += valid_fields==0x06u;
        fields_0e += valid_fields==0x0eu;
        special_10 += valid_special==0x10u;
        special_18 += valid_special==0x18u;
    }
    assert(fields_02==1u && fields_06==1u && fields_0e==1u);
    assert(special_10==1u && special_18==2u);
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
