/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional P8 service bridge over the real bounded game pools. */
#include <assert.h>
#include <string.h>

#include "../starcraft/game.c"
#include "../starcraft/sc64_p8_world_adapter.h"

static unsigned live_units(void)
{
    unsigned count=0;
    for (unsigned i=0;i<MAX_UNITS;i++) count += u[i].alive != 0;
    return count;
}

int main(void)
{
    game_init();
    race_cols=128; race_rows=128;
    int slot=spawn_unit(SIDE,U_MARINE,100,120);
    assert(slot>=0);
    u[slot].raw_owner=1;
    u[slot].alive=1;
    /* game_init() does not select an imported campaign, so spawn_unit leaves
     * source provenance unset. Resolve the spawned engine enum through the
     * authored table rather than inventing a raw CHK unit ID. */
    u[slot].raw_type=(s16)sc64_race_raw_unit(u[slot].type);
    const unsigned raw_type=(unsigned)u[slot].raw_type;
    assert(raw_type<SC64_Z8_RAW_UNIT_LIMIT);
    invincible_units[slot]=0;
    const u32 flags_before=u[slot].flags;

    Sc64Z8WorldView live={0};
    sc64_z8_game_bind_actor_pool(&live);
    live.active_mask=(u8)(1u<<1);
    s8 owners[SC64_Z8_SOURCE_OWNER_LIMIT];
    for(unsigned i=0;i<SC64_Z8_SOURCE_OWNER_LIMIT;i++) owners[i]=-1;
    owners[1]=SIDE;
    Sc64Z8GameContext create_context;
    assert(sc64_z8_game_context_init(&create_context,owners,4096,4096));
    sc64_z8_game_bind_create_service(&live,&create_context);

    Sc64Z8Rect locations[SC64_P8_SOURCE_LOCATION_COUNT]={
        {0,0,4095,4095}
    };
    u8 switches[SC64_Z8_SWITCH_BYTES]={0};
    u32 elapsed=0;
    Sc64P8WorldContext p8={.actors=&live,.locations=locations,
        .location_count=SC64_P8_SOURCE_LOCATION_COUNT,
        .map_pixel_width=4096,.map_pixel_height=4096,
        .switches=switches,.elapsed_seconds=&elapsed};
    Sc64P8SourceWorldOps ops=sc64_p8_world_ops(&p8);
    Sc64P8SourceCondition command={.player=1,.amount=1,.unit=(u16)raw_type,
        .comparison=0,.opcode=2};
    assert(ops.condition(ops.context,&command,1)==1);

    Sc64P8SourceAction invincible={.location=1,.group1=1,
        .unit=(u16)raw_type,
        .opcode=43,.modifier=4};
    assert(ops.action(ops.context,&invincible,1,0)==SC64_P8_ACTION_COMPLETE);
    assert(invincible_units[slot]==1 && u[slot].flags==flags_before);

    /* Nonzero P8 UPRP selectors must reach a property-aware service. The
     * existing default-property Z8 game create callback rejects them without
     * allocating a unit instead of silently dropping the selector. */
    Sc64P8SourceAction create={.location=1,.group1=1,.group2=3,.unit=74,
        .opcode=11,.modifier=0,.flags=0x10};
    unsigned before=live_units();
    assert(ops.action(ops.context,&create,1,0)==-1);
    assert(live_units()==before);
    return 0;
}
