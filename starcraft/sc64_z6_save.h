#ifndef SC64_Z6_SAVE_H
#define SC64_Z6_SAVE_H

/* Candidate-only validation for the Z6 large-map payload. This header is
 * shared save transaction passes the decoded campaign state explicitly.
 * Validation never changes the live Unit/Building arrays. */
#if defined(SC64_Z6_RUNTIME)

static int race_z6_save_building_type(unsigned raw)
{
    return sc64_race_building_from_raw((int)raw);
}

static int race_z6_save_source_map_valid(const Sc64RaceMap *map)
{
    static const u8 owner_modes[8] = {2,1,1,0,2,2,2,2};
    static const u8 owner_races[8] = {0,2,2,1,0,2,0,0};
    unsigned units = 0, buildings = 0, minerals_count = 0, geysers_count = 0;
    unsigned creates = 0;
    if (!map || map != &sc64_race_maps[SC64_Z6_GLOBAL_ID] ||
        map->width != SC64_Z6_MAP_COLS || map->height != SC64_Z6_MAP_ROWS ||
        map->human_slot != 3 || map->unit_count != 185 || !map->units ||
        !map->owner_modes || !map->owner_races ||
        !sc64_z6_runtime_metadata_valid()) return 0;
    for (unsigned slot = 0; slot < 8; ++slot)
        if (map->owner_modes[slot] != owner_modes[slot] ||
            map->owner_races[slot] != owner_races[slot]) return 0;
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        if (src->x_px >= map->width * 16u || src->y_px >= map->height * 16u)
            return 0;
        if (raw == SC64_RAW_START_LOCATION) continue;
        if (raw >= 176u && raw <= 178u) {
            if (src->owner != SC64_OWNER_NEUTRAL) return 0;
            ++minerals_count;
        } else if (raw == 188u) {
            if (src->owner != SC64_OWNER_NEUTRAL) return 0;
            ++geysers_count;
        } else if (sc64_race_unit_from_raw((int)raw) >= 0) {
            if (src->owner >= 8u) return 0;
            ++units;
        } else if (race_z6_save_building_type(raw) >= 0) {
            if (src->owner >= 8u) return 0;
            ++buildings;
        } else return 0;
    }
    for (unsigned i = 0; i < SC64_Z6_SOURCE_ACTION_COUNT; ++i) {
        const Sc64Z6SourceAction *a = &sc64_z6_source_actions[i];
        if (a->opcode == 11) {
            if (a->modifier || a->unit >= 228u || a->group1 >= 22u ||
                (a->group1 > 11u && (a->group1 < 13u || a->group1 > 21u))) return 0;
            ++creates;
        }
    }
    return units + creates <= MAX_UNITS && buildings <= MAX_BUILDINGS &&
           minerals_count <= MAX_MINERALS && geysers_count <= MAX_RACE_GEYSERS;
}

static int race_z6_save_expected_owner(const Sc64RaceMap *map, unsigned source)
{
    if (!map || source >= 8u) return -1;
    return source == map->human_slot ? SIDE : map->owner_modes[source];
}

static int race_z6_save_executor(unsigned trigger, const Sc64RaceMap *map,
                                unsigned *slot)
{
    unsigned found = SC64_Z6_SOURCE_SLOT_COUNT;
    if (!map || !slot || trigger >= SC64_Z6_TRIGGER_COUNT) return 0;
    for (unsigned p = 0; p < 27; ++p) {
        if (!sc64_z6_trigger_enabled(trigger, p)) continue;
        if (p < 8u) {
            if (found != SC64_Z6_SOURCE_SLOT_COUNT) return 0;
            found = p;
        } else if (p >= 18u && p <= 21u) {
            unsigned force = p - 18u;
            unsigned force_found = SC64_Z6_SOURCE_SLOT_COUNT;
            for (unsigned s = 0; s < 8; ++s) {
                if (sc64_z6_source_forces[s] != force) continue;
                if (force_found != SC64_Z6_SOURCE_SLOT_COUNT) return 0;
                force_found = s;
            }
            if (force_found >= 8u || found != SC64_Z6_SOURCE_SLOT_COUNT) return 0;
            found = force_found;
        } else return 0;
    }
    if (found >= 8u || !map->owner_modes) return 0;
    *slot = found;
    return 1;
}

static int race_z6_save_action_owner(const Sc64RaceMap *map,
                                    const Sc64Z6SourceAction *a,
                                    unsigned *source)
{
    unsigned slot;
    if (!map || !a || !source) return 0;
    if (a->group1 <= 7u) slot = a->group1;
    else if (a->group1 == 13u) {
        if (!race_z6_save_executor(a->trigger, map, &slot)) return 0;
    } else if (a->group1 >= 18u && a->group1 <= 21u) {
        unsigned force = a->group1 - 18u;
        unsigned found = 8u;
        for (unsigned s = 0; s < 8; ++s) {
            if (sc64_z6_source_forces[s] != force) continue;
            if (found < 8u) return 0;
            found = s;
        }
        if (found >= 8u) return 0;
        slot = found;
    } else return 0;
    *source = slot;
    return 1;
}

static unsigned race_z6_save_dispatched_creates(const Sc64Z6PersistentState *state,
                                                 unsigned source, unsigned raw)
{
    unsigned count = 0;
    for (unsigned t = 0; t < SC64_Z6_TRIGGER_COUNT; ++t) {
        const Sc64Z6SourceTrigger *tr = &sc64_z6_source_triggers[t];
        unsigned cursor, dispatched;
        if (!(state->runtime.model.started & ((u16)1u << t))) continue;
        cursor = state->runtime.model.action_cursor[t];
        dispatched = cursor ? cursor : tr->action_count;
        for (unsigned i = 0; i < dispatched; ++i) {
            const Sc64Z6SourceAction *a = &sc64_z6_source_actions[tr->first_action + i];
            unsigned owner;
            if (a->opcode != 11 || a->unit != raw ||
                !race_z6_save_action_owner(&sc64_race_maps[SC64_Z6_GLOBAL_ID],
                                          a, &owner)) continue;
            if (owner == source) ++count;
        }
    }
    return count;
}

static unsigned race_z6_save_remaining_creates(const Sc64Z6PersistentState *state)
{
    unsigned count = 0;
    for (unsigned t = 0; t < SC64_Z6_TRIGGER_COUNT; ++t) {
        const Sc64Z6SourceTrigger *tr = &sc64_z6_source_triggers[t];
        unsigned cursor, first;
        if (!(state->runtime.model.started & ((u16)1u << t))) first = 0;
        else {
            cursor = state->runtime.model.action_cursor[t];
            if (!cursor) continue;
            first = cursor;
        }
        for (unsigned i = first; i < tr->action_count; ++i)
            count += sc64_z6_source_actions[tr->first_action + i].opcode == 11;
    }
    return count;
}

static int race_z6_save_is_ai_product(int raw)
{
    return raw == 65 || raw == 66 || raw == 70 || raw == 83;
}

static int race_z6_save_candidate_valid(const RaceSave *candidate,
                                         const Sc64Z6PersistentState *state,
                                         unsigned width, unsigned height)
{
    const Sc64RaceMap *map = &sc64_race_maps[SC64_Z6_GLOBAL_ID];
    u8 seen_units[(MAX_UNITS + 7) / 8] = {0};
    u8 seen_buildings[(MAX_BUILDINGS + 7) / 8] = {0};
    u8 seen_minerals[(MAX_MINERALS + 7) / 8] = {0};
    u8 seen_geysers[(MAX_RACE_GEYSERS + 7) / 8] = {0};
    unsigned unit_deaths[SC64_Z6_SOURCE_SLOT_COUNT] = {0};
    unsigned building_deaths[SC64_Z6_SOURCE_SLOT_COUNT] = {0};
    unsigned live = 0, queued = 0, human_drone_missing = 0, source_geysers = 0;
    unsigned ai_dispatched;
    if (!candidate || !state || width != SC64_Z6_MAP_COLS ||
        height != SC64_Z6_MAP_ROWS ||
        !race_z6_save_source_map_valid(map) ||
        !sc64_z6_persistent_state_valid(state) ||
        candidate->version != SAVE_VERSION ||
        candidate->state[0] != 0 || candidate->state[1] != R_ZERG ||
        candidate->state[2] != SC64_Z6_GLOBAL_ID || candidate->state[7] < 0 ||
        candidate->state[16] < 0 || candidate->state[16] > MAX_RACE_GEYSERS ||
        candidate->campaign.mission != SC64_Z6_GLOBAL_ID ||
        candidate->campaign.elapsed_frames != (u32)candidate->state[7] ||
        candidate->campaign.objective_stage != 0 ||
        candidate->players[SIDE] != R_ZERG ||
        candidate->players[ENEMY] != R_PROTOSS ||
        candidate->campaign.phase > SC64_RACE_PHASE_FAILED ||
        !race_save_invincible_padding_valid(candidate)) return 0;
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (state->transport.ticks_remaining) {
        unsigned slot = (unsigned)state->transport.producer_slot_plus1 - 1u;
        Building producer;
        if (slot >= MAX_BUILDINGS ||
            !race_save_decode_building(candidate, slot, &producer) ||
            !producer.alive || producer.hp <= 0 || producer.under_construction ||
            producer.owner != SIDE || producer.raw_owner != 3 ||
            (producer.raw_type != 132 && producer.raw_type != 133) ||
            producer.type != race_z6_save_building_type((unsigned)producer.raw_type)) return 0;
    }
#endif
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit decoded;
        if (!race_save_decode_unit(candidate, i, width * 16u, height * 16u,
                                   &decoded) ||
            !race_z6_hallucination_unit_state_valid(&decoded,
                race_z6_save_expected_owner(map, 2))) return 0;
#ifdef SC64_Z6_CLOAK_ENABLED
        if (decoded.raw_type==51 && decoded.raw_owner==3 &&
            (decoded.carry<0 || decoded.carry>SC64_Z5_CLOAK_MAX_ENERGY_Q8 ||
             (!decoded.alive && (state->cloak.active || state->cloak.regen_remainder))))
            return 0;
#endif
    }
    for (unsigned owner = 0; owner < 5; ++owner)
        if (candidate->ore[owner] < 0 || candidate->ore[owner] > 65535 ||
            candidate->vespene[owner] < 0 || candidate->vespene[owner] > 65535)
            return 0;
    if (state->runtime.model.outcome == SC64_Z6_RUNNING) {
        if (candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_ACTIVE ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (state->runtime.model.outcome == SC64_Z6_WON) {
        if (!candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_COMPLETE ||
            !(candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY) ||
            !(candidate->state[3] & (1 << SC64_Z6_GLOBAL_ID))) return 0;
    } else if (state->runtime.model.outcome == SC64_Z6_LOST) {
        if (candidate->state[18] || !candidate->state[19] ||
            candidate->campaign.phase != SC64_RACE_PHASE_FAILED ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else return 0;

    /* Imported source units retain raw owner/type identity; coordinates and
       movement orders may change normally. Missing or dead source actors must
       be covered by their event-death counters. */
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        int type, owner, found = 0;
        if (raw == SC64_RAW_START_LOCATION || (raw >= 176u && raw <= 178u) ||
            raw == 188u) continue;
        type = sc64_race_unit_from_raw((int)raw);
        if (type >= 0) {
            owner = race_z6_save_expected_owner(map, src->owner);
            if (owner < 0) return 0;
            for (unsigned i = 0; i < MAX_UNITS; ++i) {
                Unit saved;
                if (!race_save_decode_unit(candidate, i, width * 16u,
                                           height * 16u, &saved)) return 0;
                if (!(seen_units[i >> 3] & (1u << (i & 7u))) &&
                    saved.raw_type == (int)raw && saved.raw_owner == src->owner &&
                    saved.type == type && saved.owner == owner) {
                    seen_units[i >> 3] |= (u8)(1u << (i & 7u));
                    found = 1;
                    if (!saved.alive) {
                        ++unit_deaths[src->owner];
                        if (raw==41u && src->owner==3u) ++human_drone_missing;
                    }
                    break;
                }
            }
            if (!found) {
                ++unit_deaths[src->owner];
                if (raw==41u && src->owner==3u) ++human_drone_missing;
            }
            continue;
        }
        type = race_z6_save_building_type(raw);
        if (type < 0 || src->owner >= 8u) return 0;
        owner = race_z6_save_expected_owner(map, src->owner);
        if (owner < 0) return 0;
        for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
            Building saved;
            if (!race_save_decode_building(candidate, i, &saved)) return 0;
            if (!(seen_buildings[i >> 3] & (1u << (i & 7u))) &&
                saved.raw_owner == src->owner && saved.owner == owner &&
                ((saved.raw_type == (int)raw && saved.type == type) ||
                 (raw == 131u && src->owner == 3u && owner == SIDE &&
                  (saved.raw_type == 132 || saved.raw_type == 133) &&
                  saved.type == race_z6_save_building_type((unsigned)saved.raw_type))) &&
                saved.x == src->x_px && saved.y == src->y_px) {
                seen_buildings[i >> 3] |= (u8)(1u << (i & 7u));
                found = 1;
                if (!saved.alive) ++building_deaths[src->owner];
                break;
            }
        }
        if (!found) ++building_deaths[src->owner];
    }
    for (unsigned source = 0; source < 8; ++source) {
        unsigned morph_allowance = source==3u ? state->human_drone_morphs : 0u;
        if (morph_allowance>human_drone_missing) morph_allowance=human_drone_missing;
        if (unit_deaths[source] + building_deaths[source] >
            state->deaths_by_source[source] + morph_allowance) return 0;
    }

    /* Resource nodes retain their source position/style even after their
       amount reaches zero; geyser rows are immutable map identities. */
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        unsigned raw = src->source_type;
        if (raw >= 176u && raw <= 178u) {
            int found = 0;
            for (unsigned i = 0; i < MAX_MINERALS; ++i) {
                Mineral saved;
                if (!race_save_decode_mineral(candidate, i, width * 16u,
                                              height * 16u, &saved)) return 0;
                if (!(seen_minerals[i >> 3] & (1u << (i & 7u))) &&
                    saved.style == (int)raw && saved.x == src->x_px &&
                    saved.y == src->y_px) {
                    seen_minerals[i >> 3] |= (u8)(1u << (i & 7u));
                    found = 1; break;
                }
            }
            if (!found) return 0;
        } else if (raw == 188u) ++source_geysers;
    }
    /* A fresh HOME load has no live map yet. Authenticate this candidate
       against the immutable map rather than the current world's count. */
    if (source_geysers > MAX_RACE_GEYSERS ||
        (unsigned)candidate->state[16] != source_geysers) return 0;
    for (unsigned i = 0; i < source_geysers; ++i) {
        int found = 0;
        unsigned ordinal = 0;
        for (unsigned row = 0; row < map->unit_count; ++row)
            if (map->units[row].source_type == 188u) {
                if (!(seen_geysers[ordinal >> 3] & (1u << (ordinal & 7u))) &&
                    candidate->geysers[i].x == map->units[row].x_px &&
                    candidate->geysers[i].y == map->units[row].y_px) {
                    seen_geysers[ordinal >> 3] |= (u8)(1u << (ordinal & 7u));
                    found = 1; break;
                }
                ++ordinal;
            }
        if (!found) return 0;
    }

    ai_dispatched = (state->runtime.model.started & (u16)(1u << 8)) &&
        (state->runtime.model.action_cursor[8] == 0 ||
         state->runtime.model.action_cursor[8] >= 2);
    if (!ai_dispatched && (state->ai_plan_ticks || state->ai_plan_phase)) return 0;

    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit saved;
        if (!race_save_decode_unit(candidate, i, width * 16u, height * 16u, &saved))
            return 0;
        if (!saved.alive) continue;
        if (saved.owner < SIDE || saved.owner > 4 || RACE_SAVE_INV_U(candidate, i) > 1)
            return 0;
        if (saved.raw_type == -1 && saved.raw_owner == -1) {
            if (saved.owner != SIDE) return 0;
        } else {
            int raw = saved.raw_type;
            unsigned source = (unsigned)saved.raw_owner;
            unsigned start_count = 0, saved_count = 0;
            if (raw < 0 || raw >= 228 || source >= 8u ||
                sc64_race_unit_from_raw(raw) != saved.type ||
                saved.owner != race_z6_save_expected_owner(map, source)) return 0;
            if (!(seen_units[i >> 3] & (1u << (i & 7u)))) {
                for (unsigned row = 0; row < map->unit_count; ++row)
                    if (map->units[row].source_type == (unsigned)raw &&
                        map->units[row].owner == source) ++start_count;
                for (unsigned j = 0; j < MAX_UNITS; ++j) {
                    Unit other;
                    if (!race_save_decode_unit(candidate, j, width * 16u,
                                               height * 16u, &other)) return 0;
                    if (other.alive && other.raw_type == raw &&
                        other.raw_owner == (int)source) ++saved_count;
                }
                unsigned scripted = race_z6_save_dispatched_creates(state, source,
                                                                     (unsigned)raw);
                unsigned ai_limit = source == SC64_Z6_AI_OWNER_SLOT &&
                    race_z6_save_is_ai_product(raw) && ai_dispatched ?
                    1u + candidate->campaign.elapsed_frames /
                        (SC64_Z6_AI_INTERVAL_TICKS) : 0u;
                /* Paid human products retain source slot 3. Heroes and
                 * scripted actors still obey the exact source census. */
                int human_product = source == 3u && saved.owner == SIDE &&
                    (raw == 37 || raw == 38 || raw == 39 || raw == 41 || raw == 42 ||
                     raw == 43 || raw == 44 || raw == 45 || raw == 46 || raw == 47);
                if (!human_product && saved_count > start_count + scripted + ai_limit) return 0;
            }
        }
        ++live;
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building saved;
        if (!race_save_decode_building(candidate, i, &saved)) return 0;
        if (!saved.alive) continue;
        if (RACE_SAVE_INV_B(candidate, i) > 1) return 0;
        if (!(seen_buildings[i >> 3] & (1u << (i & 7u)))) {
            int raw = saved.raw_type;
            if (saved.owner != SIDE || (saved.raw_owner != -1 && saved.raw_owner != 3) ||
                (raw != -1 && (raw < 0 || race_z6_save_building_type((unsigned)raw) != saved.type ||
                 sc64_race_building_rule(saved.type)->race != R_ZERG)))
                return 0;
        }
        for (int q = 0; q < saved.queue_count; ++q) {
            int type = saved.queue[(saved.queue_head + q) % 5];
            queued += type == U_ZERGLING ? 2u : 1u;
            if (saved.owner == ENEMY && saved.raw_owner == SC64_Z6_AI_OWNER_SLOT &&
                type != U_ZEALOT && type != U_DRAGOON && type != U_SCOUT &&
                type != U_REAVER) return 0;
        }
    }
    if (live + queued + race_z6_save_remaining_creates(state) > MAX_UNITS) return 0;
    return 1;
}

#endif /* SC64_Z6_RUNTIME */
#endif /* SC64_Z6_SAVE_H */
