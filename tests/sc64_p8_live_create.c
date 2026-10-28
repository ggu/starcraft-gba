/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional source-derived P8 CreateUnit check over the real game pools. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/game.c"
#include "../starcraft/sc64_p8_world_adapter.h"
#include "../starcraft/sc64_p8_game_bridge.h"

static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];
static u8 saved_invincible[MAX_UNITS];
static u8 saved_invincible_buildings[MAX_BUILDINGS];
static Unit restore_units[MAX_UNITS];
static Building restore_buildings[MAX_BUILDINGS];
static u8 restore_invincible[MAX_UNITS];
static u8 restore_invincible_buildings[MAX_BUILDINGS];

static unsigned live_units(void)
{
    unsigned count=0;
    for (unsigned i=0;i<MAX_UNITS;i++) count += u[i].alive != 0;
    return count;
}

static unsigned live_buildings(void)
{
    unsigned count=0;
    for (unsigned i=0;i<MAX_BUILDINGS;i++) count += b[i].alive != 0;
    return count;
}

static unsigned executor_for_trigger(unsigned trigger)
{
    assert(trigger < SC64_P8_TRIGGER_LIMIT);
    for (unsigned owner=0;owner<8;owner++)
        if (sc64_p8_source_triggers[trigger].enabled_players[owner])
            return owner;
    return 0;
}

int main(void)
{
    game_init();
    race_cols=SC64_P8_SOURCE_MAP_COLS;
    race_rows=SC64_P8_SOURCE_MAP_ROWS;

    Sc64Z8WorldView world={0};
    sc64_z8_game_bind_actor_pool(&world);
    world.active_mask=0xffu;
    Sc64Z8GameContext create_context;
    s8 source_to_live[SC64_Z8_SOURCE_OWNER_LIMIT];
    for (unsigned owner=0;owner<SC64_Z8_SOURCE_OWNER_LIMIT;owner++)
        source_to_live[owner]=owner<8u ? SIDE : -1;
    assert(sc64_z8_game_context_init(&create_context,source_to_live,
            SC64_P8_SOURCE_MAP_COLS*32u,SC64_P8_SOURCE_MAP_ROWS*32u));
    sc64_p8_game_bind_default_create_service(&world,&create_context);

    Sc64Z8Rect locations[SC64_P8_SOURCE_LOCATION_COUNT];
    assert(sc64_p8_world_locations_init(locations,
                                       SC64_P8_SOURCE_LOCATION_COUNT));
    Sc64P8WorldContext context={.actors=&world,.locations=locations,
        .location_count=SC64_P8_SOURCE_LOCATION_COUNT,
        .map_pixel_width=SC64_P8_SOURCE_MAP_COLS*32u,
        .map_pixel_height=SC64_P8_SOURCE_MAP_ROWS*32u};
    sc64_p8_game_bind_property_create_service(&context,&create_context);
    Sc64P8SourceWorldOps ops=sc64_p8_world_ops(&context);
    assert(sc64_p8_source_metadata_valid());

    unsigned default_creates=0, property_creates[SC64_P8_SOURCE_PROPERTY_COUNT]={0};
    unsigned before_count=live_units();
    for (unsigned i=0;i<SC64_P8_SOURCE_ACTION_COUNT;i++) {
        const Sc64P8SourceAction *action=&sc64_p8_source_actions[i];
        if (action->opcode!=11u) continue;
        unsigned executor=executor_for_trigger(action->trigger);
        unsigned before=live_units();
        if (action->group2) {
            const u8 *record=0;
            assert(sc64_p8_source_property(action->group2,&record) && record);
            ++property_creates[action->group2-1u];
            unsigned buildings_before=live_buildings();
            u8 was_alive[MAX_UNITS], was_building[MAX_BUILDINGS];
            for (unsigned unit=0;unit<MAX_UNITS;unit++)
                was_alive[unit]=(u8)(u[unit].alive!=0);
            for (unsigned building=0;building<MAX_BUILDINGS;building++)
                was_building[building]=(u8)(b[building].alive!=0);
            assert(ops.action(ops.context,action,executor,0)==
                   SC64_P8_ACTION_COMPLETE);
            if (action->unit==196u) {
                assert(live_buildings()==buildings_before+1u);
                assert(live_units()==before);
                unsigned found=MAX_BUILDINGS;
                for (unsigned building=0;building<MAX_BUILDINGS;building++)
                    if (!was_building[building] && b[building].alive) {
                        assert(found==MAX_BUILDINGS);
                        found=building;
                    }
                assert(found<MAX_BUILDINGS);
                int type=sc64_race_building_from_raw(196);
                assert(type>=0 && b[found].type==type &&
                       b[found].raw_type==196 && b[found].raw_owner==2 &&
                       b[found].owner==SIDE && b[found].hp==b[found].maxhp &&
                       !invincible_buildings[found]);
            } else {
                assert(live_units()==before+1u);
                unsigned found=MAX_UNITS;
                for (unsigned unit=0;unit<MAX_UNITS;unit++)
                    if (!was_alive[unit] && u[unit].alive) {
                        assert(found==MAX_UNITS);
                        found=unit;
                    }
                assert(found<MAX_UNITS);
                int type=sc64_race_unit_from_raw(action->unit);
                assert(type>=0 && u[found].type==type &&
                       u[found].raw_type==(s16)action->unit &&
                       u[found].raw_owner==2 && u[found].owner==SIDE &&
                       u[found].hp==u[found].maxhp);
                assert(u[found].shield==sc64_race_unit_rules[type].shields);
                if (action->group2==3u) {
                    assert(u[found].carry==SC64_STORM_HERO_MAX_ENERGY *
                                            SC64_STORM_ENERGY_ONE);
                    assert(invincible_units[found]);
                } else {
                    assert(u[found].carry==0 && !invincible_units[found]);
                }
            }
            continue;
        }

        u8 was_alive[MAX_UNITS];
        for (unsigned unit=0;unit<MAX_UNITS;unit++)
            was_alive[unit]=(u8)(u[unit].alive!=0);
        assert(ops.action(ops.context,action,executor,0)==
               SC64_P8_ACTION_COMPLETE);
        assert(live_units()==before+1u);
        unsigned found=MAX_UNITS;
        for (unsigned unit=0;unit<MAX_UNITS;unit++)
            if (!was_alive[unit] && u[unit].alive) {
                assert(found==MAX_UNITS);
                found=unit;
            }
        assert(found<MAX_UNITS);
        assert(u[found].raw_type==(s16)action->unit &&
               u[found].raw_owner>=0 && u[found].raw_owner<8 &&
               u[found].owner==SIDE);
        ++default_creates;
    }
    assert(default_creates>0 && live_units()==before_count+default_creates+
           property_creates[0]+property_creates[2]);
    for (unsigned property=0;property<SC64_P8_SOURCE_PROPERTY_COUNT;property++) {
        if (sc64_p8_source_property_used[property])
            assert(property_creates[property]>0);
        else assert(property_creates[property]==0);
    }

    /* The native source's three nonzero selectors are exercised against live
     * pools with their source-derived fields and owner preserved. */
    assert(property_creates[0] && property_creates[1] && property_creates[2]);

    /* Unsupported record drift is rejected before either pool changes. */
    {
        const Sc64P8SourceAction *source=0;
        const u8 *record=0;
        u8 altered[SC64_P8_SOURCE_PROPERTY_BYTES];
        for (unsigned i=0;i<SC64_P8_SOURCE_ACTION_COUNT;i++)
            if (sc64_p8_source_actions[i].opcode==11u &&
                sc64_p8_source_actions[i].group2==3u) {
                source=&sc64_p8_source_actions[i];
                break;
            }
        assert(source && sc64_p8_source_property(3,&record));
        memcpy(altered,record,sizeof(altered));
        altered[7]=99u;
        memcpy(saved_units,u,sizeof(u));
        memcpy(saved_buildings,b,sizeof(b));
        memcpy(saved_invincible,invincible_units,sizeof(invincible_units));
        memcpy(saved_invincible_buildings,invincible_buildings,
               sizeof(invincible_buildings));
        assert(sc64_p8_game_create_with_property(&create_context,source,2,1,3,
                    altered,&locations[source->location-1u])==-1);
        assert(memcmp(saved_units,u,sizeof(u))==0 &&
               memcmp(saved_buildings,b,sizeof(b))==0 &&
               memcmp(saved_invincible,invincible_units,
                      sizeof(invincible_units))==0 &&
               memcmp(saved_invincible_buildings,invincible_buildings,
                      sizeof(invincible_buildings))==0);
    }

    /* Exhausted real pools must reject before spawn writes any actor state. */
    for (unsigned property=1;property<=3;property++) {
        const Sc64P8SourceAction *source=0;
        const u8 *record=0;
        unsigned raw=property==1 ? 78u : property==2 ? 196u : 79u;
        for (unsigned i=0;i<SC64_P8_SOURCE_ACTION_COUNT;i++)
            if (sc64_p8_source_actions[i].opcode==11u &&
                sc64_p8_source_actions[i].group2==property) {
                source=&sc64_p8_source_actions[i];
                break;
            }
        assert(source && source->unit==raw &&
               sc64_p8_source_property(property,&record));
        memcpy(restore_units,u,sizeof(u));
        memcpy(restore_buildings,b,sizeof(b));
        memcpy(restore_invincible,invincible_units,sizeof(invincible_units));
        memcpy(restore_invincible_buildings,invincible_buildings,
               sizeof(invincible_buildings));
        if (property==2) {
            for (unsigned i=0;i<MAX_BUILDINGS;i++) b[i].alive=1;
        } else {
            for (unsigned i=0;i<MAX_UNITS;i++) u[i].alive=1;
        }
        memcpy(saved_units,u,sizeof(u));
        memcpy(saved_buildings,b,sizeof(b));
        memcpy(saved_invincible,invincible_units,sizeof(invincible_units));
        memcpy(saved_invincible_buildings,invincible_buildings,
               sizeof(invincible_buildings));
        assert(sc64_p8_game_create_with_property(&create_context,source,2,1,
                    property,record,&locations[source->location-1u])==-1);
        assert(memcmp(saved_units,u,sizeof(u))==0 &&
               memcmp(saved_buildings,b,sizeof(b))==0 &&
               memcmp(saved_invincible,invincible_units,
                      sizeof(invincible_units))==0 &&
               memcmp(saved_invincible_buildings,invincible_buildings,
                      sizeof(invincible_buildings))==0);
        memcpy(u,restore_units,sizeof(u));
        memcpy(b,restore_buildings,sizeof(b));
        memcpy(invincible_units,restore_invincible,sizeof(invincible_units));
        memcpy(invincible_buildings,restore_invincible_buildings,
               sizeof(invincible_buildings));
    }
    puts("sc64_p8_live_create: defaults and three exact UPRP creates pass");
    return 0;
}
