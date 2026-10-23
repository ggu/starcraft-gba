#ifndef SC64_T4_CREATION_ADAPTER_H
#define SC64_T4_CREATION_ADAPTER_H

#ifdef SC64_T4_SCENARIO_RUNTIME

/* Support only the verified Create Unit with Properties rows in the T4 map.
 * UPRP number 1 is the first 20-byte record (index 0); zero means default
 * creation on the N64 path. The generic property helper must not be treated as
 * proof that other UPRP masks or special fields are understood. */
static int
race_t4_create_action(const Sc64T4Action *action, unsigned trigger_owner)
{
    const Sc64T4UnitProperties *properties = 0;
    Sc64T4QueryRect area;
    unsigned count, property_index = 0;
    int raw_type, type, runtime_owner, x, y, slot;
    int conditional_revealer;

    if (!action || trigger_owner >= 8 || action->opcode != 11) return -1;

    conditional_revealer = action->unit == 101 && action->location == 9 &&
        action->argument == 1 && action->modifier == 1 &&
        action->flags == 0x1c;
    if (conditional_revealer) {
        raw_type = 101;
    } else if ((action->unit == 218 && action->location == 32 &&
                action->argument == 0 && action->modifier == 0 &&
                action->flags == 0x10) ||
               (action->unit == 101 &&
                (action->location == 20 || action->location == 22 ||
                 action->location == 23) && action->argument == 0 &&
                action->modifier == 0 && action->flags == 0x10)) {
        raw_type = action->unit;
    } else {
        return -1;
    }

    /* All retained Create Unit actions target direct source player 0. */
    if (action->player != 0) return -1;
    runtime_owner = sc64_t4_runtime_owner(0);
    if (runtime_owner != SIDE) return -1;

    if (!sc64_t4_creation_count(action->modifier, &count) || count != 1)
        return -1;
    if (action->argument) {
        if (!conditional_revealer ||
            !sc64_t4_creation_property_index(action->argument,
                                              &property_index) ||
            property_index >= 64 ||
            !sc64_t4_source_setup.property_used[property_index]) return -1;
        properties = &sc64_t4_properties[property_index];
        /* The only used property row has an HP-only override at 100 percent.
         * valid_special advertises fields, but no special flag is requested. */
        if (properties->valid_special != 0x18 ||
            properties->valid_fields != 0x02 || properties->owner != 0xff ||
            properties->hp_percent != 100 || properties->shield_percent != 0 ||
            properties->energy_percent != 0 || properties->resources != 0 ||
            properties->hangar_count != 0 || properties->special != 0 ||
            properties->unused != 0) return -1;
    } else if (conditional_revealer) {
        return -1;
    }

    if (!race_t4_effect_location(&race_t4, action->location, &area)) return -1;
    /* Source pixels use twice the engine's world-pixel scale. All
     * four retained targets are non-solid special actors; use the bounded
     * location center as the deterministic placement. */
    x = ((area.left + area.right) / 2) / 2;
    y = ((area.top + area.bottom) / 2) / 2;
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return -1;
    type = sc64_race_unit_from_raw((unsigned)raw_type);
    if (type < 0 || !sc64_race_t4_actor_flags(type)) return -1;

    /* Reserve before changing any world state. This action family creates one
     * unit, so a single free slot is its complete all-or-nothing reservation. */
    slot = find_unit_slot();
    if (slot < 0) return 0;
    slot = spawn_unit(runtime_owner, type, x, y);
    if (slot < 0) return 0;
    u[slot].raw_type = raw_type;
    u[slot].raw_owner = 0;
    if (properties) {
        u[slot].hp = u[slot].maxhp * properties->hp_percent / 100;
        if (u[slot].hp < 1) u[slot].hp = 1;
    }
    /* spawn_unit applies the type's intrinsic invincibility. UPRP special
     * bits are zero in the only supported explicit record. */
    return 1;
}

#endif /* SC64_T4_SCENARIO_RUNTIME */

#endif
