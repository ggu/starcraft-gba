#ifndef SC64_P6_SAVE_H
#define SC64_P6_SAVE_H

/* Candidate-only P6 SRAM checks. Nothing here consults or mutates the live
 * Unit/Building arrays, so a rejected record leaves the current world intact. */
#if defined(SC64_P6_RUNTIME)


static int race_p6_save_owner_allowed(unsigned source, int owner)
{
    switch (source) {
    case 7: return owner == SIDE;
    case 1: case 2: case 5: return owner == RESCUABLE || owner == SIDE;
    case 3: return owner == ENEMY;
    case 6: return owner == 4;
    case 0: case 4: return owner == NEUTRAL;
    default: return 0;
    }
}

static int race_p6_save_prop_valid(const Building *q, int type, int raw)
{
    int source_hp;
    int destroyed_prop;
    if (!q || q->type != type || q->maxhp <= 0 || q->hp > q->maxhp) return 0;
    if (raw == SC64_P6_RAW_WALL_MISSILE_TRAP) {
        const Sc64P6AddedActorFact *f = sc64_p6_added_actor_fact(raw);
        if (!f) return 0;
        source_hp = f->hp;
        destroyed_prop = 1;
    } else {
        const Sc64Z5BuildingFact *f = sc64_z5_building_fact(raw);
        if (!f) return 0;
        source_hp = f->hp;
        destroyed_prop = raw == 203 || raw == 209 || raw == 211 || raw == 213;
    }
    return q->maxhp == source_hp &&
           (q->alive ? q->hp > 0 : destroyed_prop && q->hp <= 0);
}

static unsigned race_p6_save_started_creates(const Sc64P6PersistentState *p6,
                                              unsigned source, unsigned raw)
{
    unsigned count = 0;
    for (unsigned t = 0; t < SC64_P6_TRIGGER_COUNT; ++t) {
        const Sc64P6SourceTrigger *tr = &sc64_p6_source_triggers[t];
        int started = sc64_p6_trigger_started(&p6->runtime.model, t);
        unsigned cursor = p6->runtime.model.action_cursor[t];
        unsigned dispatched;
        if (!started) continue;
        dispatched = cursor ? cursor : tr->action_count;
        for (unsigned i = 0; i < dispatched; ++i) {
            const Sc64P6SourceAction *a = &sc64_p6_source_actions[tr->first_action + i];
            unsigned slot;
            if (a->opcode != SC64_P6_ACTION_CREATE_UNIT || a->unit != raw) continue;
            if (a->group1 <= 11u) slot = a->group1;
            else if (a->group1 == 13u) slot = 7u;
            else if (a->group1 >= 18u && a->group1 <= 21u) {
                unsigned force = a->group1 - 18u;
                slot = SC64_P6_SOURCE_SLOT_COUNT;
                for (unsigned j = 0; j < 8; ++j)
                    if (sc64_p6_source_forces[j] == force) {
                        if (slot != SC64_P6_SOURCE_SLOT_COUNT) {
                            slot = SC64_P6_SOURCE_SLOT_COUNT + 1u;
                            break;
                        }
                        slot = j;
                    }
            } else continue;
            if (slot == source) ++count;
        }
    }
    return count;
}

static int race_p6_save_candidate_valid(const RaceSave *candidate,
                                         unsigned width, unsigned height)
{
    const Sc64RaceMap *map = &sc64_race_maps[SC64_P6_GLOBAL_ID];
    u8 seen_units[MAX_UNITS] = {0};
    u8 seen_buildings[MAX_BUILDINGS] = {0};
    unsigned missing_units[SC64_P6_SOURCE_SLOT_COUNT] = {0};
    unsigned live = 0, queued = 0;
    if (!candidate || width != SC64_P6_MAP_COLS || height != SC64_P6_MAP_ROWS ||
        !race_p6_source_row_valid(map) ||
        !sc64_p6_persistent_state_valid(&candidate->p6)) return 0;
    if (candidate->state[0] || candidate->state[2] != SC64_P6_GLOBAL_ID ||
        candidate->state[1] != R_PROTOSS ||
        candidate->players[SIDE] != R_PROTOSS ||
        candidate->players[RESCUABLE] != R_PROTOSS ||
        candidate->players[ENEMY] != R_ZERG || candidate->players[4] != R_ZERG ||
        candidate->players[NEUTRAL] != R_TER ||
        candidate->campaign.mission != SC64_P6_GLOBAL_ID ||
        candidate->campaign.elapsed_frames != (u32)candidate->state[7] ||
        candidate->campaign.objective_stage != 0) return 0;
    if (candidate->p6.runtime.model.outcome == SC64_P6_RUNNING) {
        if (candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_ACTIVE ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (candidate->p6.runtime.model.outcome == SC64_P6_WON) {
        if (!candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_COMPLETE ||
            !(candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
            !(candidate->state[3] & (1 << SC64_P6_GLOBAL_ID))) return 0;
    } else if (candidate->p6.runtime.model.outcome == SC64_P6_LOST) {
        if (candidate->state[18] || !candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_FAILED ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else return 0;

    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        int type, owner, found = 0;
        if (raw == SC64_RAW_START_LOCATION ||
            (raw >= 176u && raw <= 178u) || raw == 188u) continue;
        type = sc64_race_unit_from_raw((int)raw);
        if (type >= 0) {
            owner = race_p6_live_owner_for_source(src->owner);
            if (src->owner >= SC64_P6_SOURCE_SLOT_COUNT || owner < 0) return 0;
            for (unsigned i = 0; i < MAX_UNITS; ++i) {
                Unit saved;
                if (!race_save_decode_unit(candidate, i, width * 16u, height * 16u,
                                           &saved)) return 0;
                if (!seen_units[i] && saved.raw_type == (int)raw &&
                    saved.raw_owner == src->owner && saved.type == type &&
                    race_p6_save_owner_allowed(src->owner, saved.owner)) {
                    seen_units[i] = 1;
                    found = 1; break;
                }
            }
            if (!found) ++missing_units[src->owner];
            continue;
        }
        type = sc64_race_building_from_raw((int)raw);
        if (type >= 0) {
            owner = race_p6_live_owner_for_source(src->owner);
            if (src->owner >= 8u || owner < 0) return 0;
            for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
                Building saved;
                if (!race_save_decode_building(candidate, i, &saved)) return 0;
                if (!seen_buildings[i] && saved.raw_type == (int)raw &&
                    saved.raw_owner == src->owner && saved.type == type &&
                    race_p6_save_owner_allowed(src->owner, saved.owner) &&
                    saved.x == src->x_px && saved.y == src->y_px) {
                    seen_buildings[i] = 1; found = 1; break;
                }
            }
            if (!found) return 0;
        } else return 0;
    }
    {
        unsigned rescued_missing = missing_units[1] + missing_units[2] +
                                   missing_units[5] + missing_units[7];
        unsigned rescued_deaths = candidate->p6.deaths_by_source[1] +
                                  candidate->p6.deaths_by_source[2] +
                                  candidate->p6.deaths_by_source[5] +
                                  candidate->p6.deaths_by_source[7];
        if (rescued_missing > rescued_deaths) return 0;
    }
    for (unsigned owner = 0; owner < 8; ++owner) {
        if (owner == 1 || owner == 2 || owner == 5 || owner == 7) continue;
        if (missing_units[owner] > candidate->p6.deaths_by_source[owner]) return 0;
    }

    /* Every THG2 unit-backed source prop has exactly one immutable candidate
     * identity. Door state is separate in the persisted row mask; trap damage
     * and cooldown are validated by the source fact/combat policy. */
    for (unsigned row = 0; row < SC64_P6_DOODAD_SOURCE_COUNT; ++row) {
        const Sc64P6Doodad *src = &sc64_p6_doodads[row];
        int type, found = 0;
        if (src->sprite_only) continue;
        type = sc64_race_building_from_raw((int)src->raw_type);
        if (type < 0 || src->raw_owner >= 8u) return 0;
        for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
            Building saved;
            if (!race_save_decode_building(candidate, i, &saved)) return 0;
            if (!seen_buildings[i] && saved.raw_type == (int)src->raw_type &&
                saved.raw_owner == src->raw_owner && saved.type == type &&
                race_p6_save_owner_allowed(src->raw_owner, saved.owner) &&
                saved.x == (int)(src->x / 2u) && saved.y == (int)(src->y / 2u) &&
                saved.flags == src->flags &&
                race_p6_save_prop_valid(&saved, type, (int)src->raw_type)) {
                seen_buildings[i] = 1; found = 1; break;
            }
        }
        if (!found) return 0;
    }

    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit saved;
        if (!race_save_decode_unit(candidate, i, width * 16u, height * 16u,
                                   &saved)) return 0;
#ifdef SC64_P6_STORM_ENABLED
        if (saved.raw_type == SC64_P6_STORM_RAW_TASSADAR &&
            !race_p6_storm_energy_valid(&saved)) return 0;
#endif
        if (saved.alive) {
            unsigned source;
            int raw = saved.raw_type;
            if (saved.type < 0 || saved.type >= SC_RU_COUNT) return 0;
            if (raw < 0 || saved.raw_owner < 0) {
                /* Normal human production is not a source-script actor and
                 * keeps canonical -1 provenance in this profile. */
                if (saved.owner != SIDE || raw != -1 || saved.raw_owner != -1)
                    return 0;
                ++live;
                continue;
            }
            if (raw >= 228 || saved.raw_owner >= 8 ||
                !race_p6_save_owner_allowed((unsigned)saved.raw_owner, saved.owner) ||
                sc64_race_unit_from_raw(raw) != saved.type) return 0;
            source = (unsigned)saved.raw_owner;
            if (!seen_units[i]) {
                if (source != 7 && !race_p6_save_started_creates(&candidate->p6,
                                                                  source, (unsigned)raw))
                    return 0;
            }
            if (RACE_SAVE_INV_U(candidate, i) > 1) return 0;
#if defined(SC64_P6_COMBAT_ENABLED)
            if (!race_p6_combat_saved_actor_valid(raw, (unsigned)saved.flags,
                    saved.order, saved.target, saved.shield_remainder)) return 0;
#endif
            ++live;
        }
    }
    for (unsigned source = 0; source < 8; ++source) {
        for (unsigned type = 0; type < SC_RU_COUNT; ++type) {
            unsigned start = 0, saved_count = 0;
            int raw = sc64_race_raw_unit((int)type);
            for (unsigned row = 0; row < map->unit_count; ++row) {
                const Sc64ImportedUnit *src = &map->units[row];
                if (src->owner == source && src->source_type == (unsigned)raw)
                    ++start;
            }
            if (raw < 0) continue;
            for (unsigned i = 0; i < MAX_UNITS; ++i) {
                Unit saved;
                if (!race_save_decode_unit(candidate, i, width * 16u,
                                           height * 16u, &saved)) return 0;
                if (saved.alive && saved.raw_owner == (int)source &&
                    saved.type == (int)type && saved.raw_type == raw) ++saved_count;
            }
            if (saved_count > start +
                race_p6_save_started_creates(&candidate->p6, source,
                                              (unsigned)raw)) return 0;
        }
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building saved;
        int was_source;
        if (!race_save_decode_building(candidate, i, &saved)) return 0;
        if (saved.raw_type == SC64_P6_RAW_WALL_MISSILE_TRAP &&
            saved.shield_remainder > SC64_P6_WALL_TRAP_COOLDOWN_GBA_TICKS) return 0;
        if (!saved.alive) continue;
        if (RACE_SAVE_INV_B(candidate, i) > 1) return 0;
        was_source = seen_buildings[i];
        if (!was_source) {
            /* New buildings can only be normal human-built actors; source
             * scripts in this mission create units and retain THG2 props. */
            if (saved.owner != SIDE ||
                !((saved.raw_owner == -1 &&
                   (saved.raw_type == -1 ||
                    sc64_race_raw_building(saved.type) == saved.raw_type)) ||
                  (saved.raw_owner == 7 &&
                   sc64_race_raw_building(saved.type) == saved.raw_type))) return 0;
        }
        for (int q = 0; q < saved.queue_count; ++q)
            queued += saved.queue[(saved.queue_head + q) % 5] == U_ZERGLING ? 2u : 1u;
    }
    if (live + queued + sc64_p6_remaining_creations(&candidate->p6.runtime) > MAX_UNITS)
        return 0;
    return 1;
}

#endif /* SC64_P6_RUNTIME */
#endif
