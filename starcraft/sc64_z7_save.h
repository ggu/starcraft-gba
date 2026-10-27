#ifndef SC64_Z7_SAVE_H
#define SC64_Z7_SAVE_H

/* Pure candidate validation for the seventh Zerg map.  This deliberately
 * authenticates the canonical imported map and decoded candidate rather than
 * consulting the live Unit/Building arrays; the save loader can therefore
 * reject a corrupt record before it mutates the running world. */
#if defined(SC64_Z7_RUNTIME)

static int race_z7_save_source_owner(const Sc64RaceMap *map, unsigned slot)
{
    if (!map || slot >= 8u || !map->owner_modes ||
        map->human_slot >= 8u) return -1;
    return slot == map->human_slot ? SIDE : map->owner_modes[slot];
}

static int race_z7_save_map_valid(const Sc64RaceMap *map)
{
    unsigned units=0, buildings=0, minerals=0, geysers_count=0,
             extractors=0, markers=0;
    if (!race_z7_map_shape_valid(map) || !race_z7_source_row_counts(map)) return 0;
    for (unsigned row=0; row<map->unit_count; ++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        unsigned raw=src->source_type;
        if (src->x_px >= (unsigned)map->width*16u ||
            src->y_px >= (unsigned)map->height*16u) return 0;
        if (raw==214u) { ++markers; continue; }
        if (raw>=176u && raw<=178u) {
            if (src->owner!=SC64_OWNER_NEUTRAL) return 0;
            ++minerals; continue;
        }
        if (raw==188u) {
            if (src->owner!=SC64_OWNER_NEUTRAL) return 0;
            ++geysers_count; continue;
        }
        if (raw==149u) {
            if (src->owner!=4u) return 0;
            ++extractors;
        }
        if (src->owner>=8u) return 0;
        if (sc64_race_unit_from_raw((int)raw)>=0) ++units;
        else if (sc64_race_building_from_raw((int)raw)>=0) ++buildings;
        else return 0;
    }
    return units==113u && buildings==57u && minerals==60u &&
           geysers_count==1u && extractors==5u && markers==0u &&
           units<=MAX_UNITS && buildings<=MAX_BUILDINGS &&
           minerals<=MAX_MINERALS && geysers_count<=MAX_RACE_GEYSERS;
}

static int race_z7_save_source_unit_count(const Sc64RaceMap *map,
                                           unsigned source, unsigned raw)
{
    int count=0;
    for (unsigned row=0; row<map->unit_count; ++row)
        if (map->units[row].owner==source && map->units[row].source_type==raw)
            ++count;
    return count;
}

static int race_z7_save_source_building_match(const Sc64RaceMap *map,
                                               const Building *saved,
                                               unsigned row)
{
    const Sc64ImportedUnit *src=&map->units[row];
    unsigned raw=src->source_type;
    int owner=race_z7_save_source_owner(map,src->owner);
    int type=sc64_race_building_from_raw((int)raw);
    if (owner<0 || type<0 || saved->raw_owner!=(int)src->owner ||
        saved->owner!=owner || saved->x!=src->x_px || saved->y!=src->y_px)
        return 0;
    if (saved->raw_type==(int)raw && saved->type==type) return 1;
    /* A source Zerg Hatchery can morph in place while retaining its source
     * provenance and position.  No other building identity may change. */
    if (src->owner==3u && raw==131u && owner==SIDE &&
        (saved->raw_type==132 || saved->raw_type==133) &&
        sc64_race_building_from_raw(saved->raw_type)==saved->type) return 1;
    /* Z7 Creep Colony morphs retain source provenance and position while
       changing the source building identity in place. */
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    if (src->owner==3u && raw==SC64_Z7_CREEP_RAW && owner==SIDE &&
        (saved->raw_type==SC64_Z7_SUNKEN_RAW ||
         saved->raw_type==SC64_Z7_SPORE_RAW) &&
        sc64_race_building_from_raw(saved->raw_type)==saved->type) return 1;
#endif
    return 0;
}

static int race_z7_save_gas_source_row(const Sc64RaceMap *map,int x,int y,
                                       unsigned *row_out)
{
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        if ((src->source_type==188u && src->owner==SC64_OWNER_NEUTRAL) ||
            (src->source_type==149u && src->owner==4u)) {
            if (src->x_px==(unsigned)x && src->y_px==(unsigned)y) {
                if (row_out) *row_out=row;
                return 1;
            }
        }
    }
    return 0;
}

static int race_z7_save_candidate_has_gas(const RaceSave *candidate,
                                          int x,int y)
{
    unsigned count=(unsigned)candidate->state[16];
    for (unsigned i=0;i<count;++i)
        if (candidate->geysers[i].x==x && candidate->geysers[i].y==y) return 1;
    return 0;
}

static int race_z7_save_candidate_has_live_extractor(const RaceSave *candidate,
                                                      int x,int y)
{
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building saved;
        if (!race_save_decode_building(candidate,i,&saved)) return 0;
        if (saved.alive && saved.raw_type==149 && saved.raw_owner==4 &&
            saved.type==B_EXTRACTOR && saved.x==x && saved.y==y) return 1;
    }
    return 0;
}

static int race_z7_save_human_unit_type(int type)
{
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(type);
    /* Keep untrainable scenario heroes and map-only actors from being
     * fabricated as human production. Their exact imported source rows are
     * still accepted through provenance matching. */
    return rule && rule->race==R_ZERG && rule->producer<SC_RB_COUNT &&
           rule->build_frames>0;
}

static unsigned race_z7_save_ai_started(const Sc64Z7PersistentState *state);

static unsigned race_z7_save_enemy_ling_bound(
    const RaceSave *candidate,const Sc64Z7PersistentState *state)
{
    unsigned count=(unsigned)race_z7_save_source_unit_count(
        &sc64_race_maps[SC64_Z7_GLOBAL_ID],4u,37u);
    if (race_z7_save_ai_started(state)) {
        unsigned pulses=candidate->campaign.elapsed_frames /
                        SC64_Z7_AI_PERIOD_TICKS + 1u;
        count += pulses * 2u; /* one paid egg yields two Zerglings */
    }
    return count;
}

#ifdef SC64_Z7_EGG_PRODUCTION
static int race_z7_save_egg_valid(const Sc64RaceMap *map, const Unit *saved,
                                   unsigned width, unsigned height)
{
    int source_owner, duration;
    if (!map || !saved || saved->type != U_Z7_EGG || saved->raw_type != 36 ||
        saved->raw_owner < 0 || saved->raw_owner >= 8 ||
        (saved->raw_owner != 3 && saved->raw_owner != 4) ||
        !sc64_z7_egg_product_valid(saved->target) ||
        saved->order != ORDER_IDLE || saved->build != -1 ||
        saved->maxhp != 200 || saved->hp > 200 ||
        saved->armor != 10 || saved->shield != 0 ||
        saved->shield_remainder != 0 || saved->carry != 0 || saved->cd != 0 ||
        saved->path_timer != 0 || saved->path_retry != 0 ||
        saved->tx != saved->x || saved->ty != saved->y ||
        (saved->alive && saved->hp <= 0) || saved->flags != 0 ||
        saved->x < 0 || saved->y < 0 ||
        (unsigned)saved->x >= width * 16u ||
        (unsigned)saved->y >= height * 16u) return 0;
    source_owner = race_z7_save_source_owner(map, (unsigned)saved->raw_owner);
    if (source_owner < 0 || source_owner != saved->owner ||
        map->owner_races[saved->raw_owner] != R_ZERG) return 0;
    /* The authored Z7 enemy production policy only queues Zerglings. Human
     * players may use any product accepted by the ordinary larva rules. */
    if (saved->raw_owner == 4 && saved->target != U_ZERGLING) return 0;
    duration = sc64_z7_egg_duration(saved->target);
    if (duration <= 0 || saved->work_timer > duration ||
        (saved->alive && saved->work_timer == 0)) return 0;
    return 1;
}

static unsigned race_z7_save_live_enemy_ling_eggs(const RaceSave *candidate,
                                                    unsigned width,
                                                    unsigned height)
{
    unsigned count = 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit saved;
        if (!race_save_decode_unit(candidate, i, width * 16u, height * 16u,
                                   &saved)) return ~0u;
        if (saved.alive && saved.type == U_Z7_EGG && saved.raw_owner == 4 &&
            saved.target == U_ZERGLING) ++count;
    }
    return count;
}
#endif

static int race_z7_save_human_building_type(int type)
{
    const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
    return rule && rule->race==R_ZERG;
}

static int race_z7_save_building_type_matches_source(unsigned source,int raw,
                                                       int owner,int type)
{
    return sc64_race_building_from_raw(raw)==type ||
        (source==3u && raw==131 && owner==SIDE &&
         (type==B_LAIR || type==B_HIVE))
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
        || (source==3u && raw==SC64_Z7_CREEP_RAW && owner==SIDE &&
            (type==B_SUNKEN_COLONY || type==B_SPORE_COLONY))
#endif
        ;
}

static int race_z7_save_unused_unit(const Unit *unit)
{
    static const Unit zero={0};
    return unit && !memcmp(unit,&zero,sizeof(zero));
}

static int race_z7_save_unused_building(const Building *building)
{
    static const Building zero={0};
    return building && !memcmp(building,&zero,sizeof(zero));
}

static unsigned race_z7_save_ai_started(const Sc64Z7PersistentState *state)
{
    return state && state->runtime.ai_script_mask!=0;
}

static unsigned race_z7_save_remaining_script_creates(
    const Sc64Z7PersistentState *state)
{
    (void)state;
    /* The authenticated Z7 trigger table has no Create Unit actions.  The
     * bounded authored AI queues paid Zerglings directly and is checked below. */
    return 0;
}

static int race_z7_save_candidate_valid(const RaceSave *candidate,
                                         const Sc64Z7PersistentState *state,
                                         unsigned width, unsigned height)
{
    const Sc64RaceMap *map;
    u8 seen_units[(MAX_UNITS+7u)/8u]={0};
    u8 seen_building_rows[32]={0};
    u8 seen_minerals[(MAX_MINERALS+7u)/8u]={0};
    /* Candidate-validation census only: no persistent or BSS storage. The
     * owner/raw tables count every decoded slot, including retained-dead and
     * canonical zero rows, exactly as the original repeated scans did. */
    u8 unit_owner_meta[MAX_UNITS], unit_raw_meta[MAX_UNITS];
    u8 unit_type_meta[MAX_UNITS], unit_player_meta[MAX_UNITS];
    u8 unit_flags[MAX_UNITS];
    u8 unit_counts[8][228]={{0}};
    unsigned live=0,queued=0,source_geysers=0,source_extractors=0;
    unsigned live_human_source_actors=0,live_enemy_source_buildings=0;
    unsigned live_enemy_ling_eggs=0;
    unsigned enemy_ling_bound;
    _Static_assert(MAX_UNITS <= 255, "save census counters must fit u8");

    map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    for (unsigned row=0;row<map->unit_count;++row) {
        source_geysers+=map->units[row].source_type==188u;
        source_extractors+=map->units[row].source_type==149u &&
                           map->units[row].owner==4u;
    }

    if (!candidate || !state || width!=SC64_Z7_MAP_COLS ||
        height!=SC64_Z7_MAP_ROWS || !race_z7_save_map_valid(
            &sc64_race_maps[SC64_Z7_GLOBAL_ID]) ||
        !sc64_z7_persistent_state_valid(state) ||
        candidate->version!=SAVE_VERSION || candidate->state[0]!=0 ||
        candidate->state[1]!=R_ZERG || candidate->state[2]!=SC64_Z7_GLOBAL_ID ||
        candidate->state[7]<0 || candidate->state[16]<0 ||
        candidate->state[16]>MAX_RACE_GEYSERS ||
        candidate->campaign.mission!=SC64_Z7_GLOBAL_ID ||
        candidate->campaign.elapsed_frames!=(u32)candidate->state[7] ||
        candidate->campaign.objective_stage!=0 ||
        candidate->players[SIDE]!=R_ZERG || candidate->players[ENEMY]!=R_ZERG ||
        candidate->campaign.phase>SC64_RACE_PHASE_FAILED ||
        !race_save_invincible_padding_valid(candidate)) return 0;

    if (state->runtime.model.outcome==SC64_Z7_RUNNING) {
        if (candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase!=SC64_RACE_PHASE_ACTIVE ||
            (candidate->campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (state->runtime.model.outcome==SC64_Z7_WON) {
        if (!candidate->state[18] || candidate->state[19] ||
            candidate->campaign.phase!=SC64_RACE_PHASE_COMPLETE ||
            !(candidate->campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY) ||
            !(candidate->state[3]&(1<<SC64_Z7_GLOBAL_ID))) return 0;
    } else if (state->runtime.model.outcome==SC64_Z7_LOST) {
        if (candidate->state[18] || !candidate->state[19] ||
            candidate->campaign.phase!=SC64_RACE_PHASE_FAILED ||
            (candidate->campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else return 0;

    for (unsigned owner=0; owner<5; ++owner)
        if (candidate->ore[owner]<0 || candidate->ore[owner]>65535 ||
            candidate->vespene[owner]<0 || candidate->vespene[owner]>65535) return 0;

    enemy_ling_bound=race_z7_save_enemy_ling_bound(candidate,state);
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit saved;
        int unused;
        if (!race_save_decode_unit(candidate,i,width*16u,height*16u,&saved)) return 0;
        unused=race_z7_save_unused_unit(&saved);
        unit_flags[i]=(u8)((saved.alive?1u:0u)|(unused?2u:0u)
#ifdef SC64_Z7_EGG_PRODUCTION
            | ((saved.type==U_Z7_EGG || saved.raw_type==36)?4u:0u)
#endif
            );
        unit_owner_meta[i]=saved.raw_owner>=0 && saved.raw_owner<8
            ? (u8)saved.raw_owner : 255u;
        unit_raw_meta[i]=saved.raw_type>=0 && saved.raw_type<228
            ? (u8)saved.raw_type : 255u;
        unit_type_meta[i]=saved.type>=0 && saved.type<255 ? (u8)saved.type : 255u;
        unit_player_meta[i]=saved.owner>=0 && saved.owner<255 ? (u8)saved.owner : 255u;
        if (unit_owner_meta[i]<8u && unit_raw_meta[i]<228u) {
            if (unit_counts[unit_owner_meta[i]][unit_raw_meta[i]]==255u) return 0;
            ++unit_counts[unit_owner_meta[i]][unit_raw_meta[i]];
        }
#ifdef SC64_Z7_EGG_PRODUCTION
        if (saved.alive && saved.type==U_Z7_EGG && saved.raw_owner==4 &&
            saved.target==U_ZERGLING) ++live_enemy_ling_eggs;
#endif
        /* The old second pass validated every retained source row, including
         * dead records. Do that once here so later row matching can use the
         * exact decoded metadata without another full decode. */
        if (!unused) {
#ifdef SC64_Z7_EGG_PRODUCTION
            if (saved.type==U_Z7_EGG || saved.raw_type==36) {
                if (!race_z7_save_egg_valid(map,&saved,width,height)) return 0;
                continue;
            }
#endif
            if (saved.raw_owner>=0 || saved.raw_type>=0) {
                int raw=saved.raw_type; unsigned source=(unsigned)saved.raw_owner;
                if (saved.raw_owner<0 || saved.raw_owner>=8 || raw<0 || raw>=228 ||
                    sc64_race_unit_from_raw(raw)!=saved.type ||
                    saved.owner!=race_z7_save_source_owner(map,source) ||
                    !sc64_race_unit_rule(saved.type) ||
                    sc64_race_unit_rule(saved.type)->race!=map->owner_races[source]) return 0;
            } else if (saved.alive && (saved.raw_owner!=-1 || saved.raw_type!=-1)) return 0;
            if (saved.alive && saved.raw_owner==-1 && saved.owner!=SIDE) return 0;
        }
    }

    for (unsigned i=0; i<MAX_UNITS; ++i) {
        Unit saved;
        if (!race_save_decode_unit(candidate,i,width*16u,height*16u,&saved) ||
            RACE_SAVE_INV_U(candidate,i)>1u) return 0;
#ifdef SC64_Z7_EGG_PRODUCTION
        if (saved.type == U_Z7_EGG || saved.raw_type == 36) {
            if (!race_z7_save_egg_valid(map, &saved, width, height)) return 0;
            if (!saved.alive) continue;
            ++live;
            if (saved.owner == SIDE && saved.raw_owner == 3)
                ++live_human_source_actors;
            continue;
        }
#endif
        if (!saved.alive) continue;
        ++live;
        if (saved.owner==SIDE && saved.raw_owner==3)
            ++live_human_source_actors;
        if (saved.owner<SIDE || saved.owner>4 || saved.type<0 ||
            saved.type>=SC_RU_COUNT) return 0;
        if (saved.raw_type==-1 && saved.raw_owner==-1) {
            if (saved.owner!=SIDE || !race_z7_save_human_unit_type(saved.type)) return 0;
        } else {
            int raw=saved.raw_type;
            unsigned source=(unsigned)saved.raw_owner;
            int expected;
            if (raw<0 || raw>=228 || source>=8u ||
                sc64_race_unit_from_raw(raw)!=saved.type) return 0;
            expected=race_z7_save_source_owner(map,source);
            if (saved.owner!=expected || !sc64_race_unit_rule(saved.type) ||
                sc64_race_unit_rule(saved.type)->race!=map->owner_races[source]) return 0;
        }
    }

    /* Apply the original per-actor provenance totals after the single census
     * pass has counted every slot. These are the same predicate boundaries as
     * the former nested decode loops. */
    for (unsigned i=0;i<MAX_UNITS;++i) {
        unsigned source=unit_owner_meta[i], raw=unit_raw_meta[i];
        int alive=(unit_flags[i]&1u)!=0, egg=(unit_flags[i]&4u)!=0;
        if (!alive || egg || source>=8u || raw>=228u) continue;
        if (source==3u && unit_player_meta[i]==SIDE &&
            race_z7_save_human_unit_type(unit_type_meta[i])) continue;
        if (source==4u && raw==37u) continue;
        if (unit_counts[source][raw]>
            (unsigned)race_z7_save_source_unit_count(map,source,raw)) return 0;
    }
    for (unsigned i=0;i<MAX_UNITS;++i) {
        unsigned source=unit_owner_meta[i], raw=unit_raw_meta[i];
        if ((unit_flags[i]&1u) && !(unit_flags[i]&4u) && source==4u &&
            raw<228u && raw!=37u) {
            unsigned initial=(unsigned)race_z7_save_source_unit_count(map,4u,raw);
            if (!initial || unit_counts[4][raw]>initial) return 0;
        } else if ((unit_flags[i]&1u) && !(unit_flags[i]&4u) &&
                   source==4u && raw==37u) {
            if (unit_counts[4][37]>enemy_ling_bound) return 0;
#ifdef SC64_Z7_EGG_PRODUCTION
            if (live_enemy_ling_eggs >
                (~0u-unit_counts[4][37])/2u ||
                unit_counts[4][37]+live_enemy_ling_eggs*2u>enemy_ling_bound) return 0;
#endif
        }
    }

    for (unsigned row=0; row<map->unit_count; ++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        unsigned raw=src->source_type;
        if (raw==214u || (raw>=176u && raw<=178u) || raw==188u ||
            sc64_race_unit_from_raw((int)raw)<0) continue;
        for (unsigned i=0;i<MAX_UNITS;++i) {
            unsigned source_owner=unit_owner_meta[i], source_raw=unit_raw_meta[i];
            int saved_type=unit_type_meta[i], saved_owner=unit_player_meta[i];
            if (!(seen_units[i>>3]&(1u<<(i&7u))) &&
                source_owner==(unsigned)src->owner && source_raw==raw &&
                saved_type==sc64_race_unit_from_raw((int)raw) &&
                saved_owner==race_z7_save_source_owner(map,src->owner)) {
                seen_units[i>>3]|=(u8)(1u<<(i&7u));
                break;
            }
        }
    }

    /* Bound every authenticated source roster even when a dead slot was
     * retained. The only source-owner growth is the authored AI's paid Ling
     * policy; human products are separately permitted by Zerg type/race. */
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        unsigned raw=src->source_type;
        if (sc64_race_unit_from_raw((int)raw)<0 || src->owner==3u) continue;
        /* The canonical Z7 map validator fixes unit_count=231 and rejects
         * unit owners >=8/unknown raw IDs before this loop; keep local bounds
         * explicit at the census lookup as a fail-closed invariant. */
        if (src->owner>=8u || raw>=228u) return 0;
        unsigned total=unit_counts[src->owner][raw];
        unsigned initial=(src->owner==4u && raw==37u)
            ? enemy_ling_bound
            : (unsigned)race_z7_save_source_unit_count(map,src->owner,raw);
        if (total>initial) return 0;
    }

#ifdef SC64_Z7_EGG_PRODUCTION
    /* Raw-36 Eggs are real retained unit slots, but are not source-map
     * placements. Validate their provenance above and bound the enemy's
     * unfinished Ling obligations against the same authored AI pulse budget. */
    {
        unsigned enemy_lings=unit_counts[4][37];
        if (live_enemy_ling_eggs > (~0u-enemy_lings)/2u ||
            enemy_lings+live_enemy_ling_eggs*2u>enemy_ling_bound) return 0;
    }
#endif

    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building saved;
        if (!race_save_decode_building(candidate,i,&saved) ||
            RACE_SAVE_INV_B(candidate,i)>1u) return 0;
        if (!saved.alive) continue;
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
        if (!race_z7_colony_morph_state_valid(&saved)) return 0;
#endif
        if (saved.owner==SIDE && saved.raw_owner==3)
            ++live_human_source_actors;
        if (saved.owner==ENEMY && saved.raw_owner==4)
            ++live_enemy_source_buildings;
        if (saved.owner<SIDE || saved.owner>4 || saved.type<0 ||
            saved.type>=SC_RB_COUNT) return 0;
        if (saved.raw_type==-1 && saved.raw_owner==-1) {
            if (saved.owner!=SIDE || !race_z7_save_human_building_type(saved.type)) return 0;
        } else {
            int raw=saved.raw_type;
            unsigned source=(unsigned)saved.raw_owner;
            int expected;
            if (raw<0 || raw>=228 || source>=8u ||
                sc64_race_building_from_raw(raw)<0) return 0;
            expected=race_z7_save_source_owner(map,source);
            if (expected!=saved.owner || !sc64_race_building_rule(saved.type) ||
                sc64_race_building_rule(saved.type)->race!=map->owner_races[source]) return 0;
            if (!race_z7_save_building_type_matches_source(source,raw,
                                                            saved.owner,saved.type)) return 0;
        }
        for (int q=0;q<saved.queue_count;++q) {
            int queued_type=saved.queue[(saved.queue_head+q)%5];
            const Sc64RaceUnitRule *ur=sc64_race_unit_rule(queued_type);
            const Sc64RaceBuildingRule *br=sc64_race_building_rule(saved.type);
            if (!ur || !br || ur->producer>=SC_RB_COUNT || !ur->build_frames ||
                ur->race!=br->race ||
                !sc64_race_producer_compatible(saved.type,ur->producer) ||
#ifdef SC64_Z7_EGG_PRODUCTION
                (ur->race==R_ZERG && (ur->flags&SC_RACE_F_LARVA)) ||
#endif
                (saved.owner==ENEMY && (saved.raw_owner!=4 || queued_type!=U_ZERGLING)) ||
                (saved.owner!=SIDE && saved.owner!=ENEMY)) return 0;
            queued += queued_type==U_ZERGLING ? 2u : 1u;
        }
    }

    /* Original enemy and neutral buildings may be destroyed or their slots
     * reused. Any retained provenance row must still resolve to a canonical
     * source placement; human Zerg buildings may be paid products/morphs. */
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building saved;
        if (!race_save_decode_building(candidate,i,&saved)) return 0;
        if (race_z7_save_unused_building(&saved)) continue;
        if (saved.raw_owner<0 && saved.raw_type<0) continue;
        if (saved.raw_owner<0 || saved.raw_owner>=8 || saved.raw_type<0 ||
            saved.raw_type>=228 || sc64_race_building_from_raw(saved.raw_type)<0 ||
            saved.owner!=race_z7_save_source_owner(map,(unsigned)saved.raw_owner) ||
            !sc64_race_building_rule(saved.type) ||
            sc64_race_building_rule(saved.type)->race!=map->owner_races[saved.raw_owner] ||
            !race_z7_save_building_type_matches_source((unsigned)saved.raw_owner,
                    saved.raw_type,saved.owner,saved.type)) return 0;
        if (saved.raw_owner==3 && saved.owner==SIDE &&
            race_z7_save_human_building_type(saved.type)) {
            int match=-1;
            for (unsigned row=0;row<map->unit_count;++row)
                if (!(seen_building_rows[row>>3]&(1u<<(row&7u))) &&
                    race_z7_save_source_building_match(map,&saved,row)) {
                    match=(int)row; break;
                }
            if (match>=0) {
                seen_building_rows[(unsigned)match>>3]|=(u8)(1u<<((unsigned)match&7u));
                continue;
            }
            continue; /* Paid/morphed human Zerg building. */
        }
        if (saved.x<0 || saved.y<0) return 0;
        {
            int matched=0;
            for (unsigned row=0;row<map->unit_count;++row)
                if (!(seen_building_rows[row>>3]&(1u<<(row&7u))) &&
                    race_z7_save_source_building_match(map,&saved,row)) {
                    seen_building_rows[row>>3]|=(u8)(1u<<(row&7u));
                    matched=1; break;
                }
            if (!matched) return 0;
        }
    }

    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        unsigned raw=src->source_type;
        if (raw>=176u && raw<=178u) {
            unsigned initial=src->resources?src->resources:1u;
            int found=0;
            for (unsigned i=0;i<MAX_MINERALS;++i) {
                Mineral saved;
                if (!race_save_decode_mineral(candidate,i,width*16u,height*16u,&saved)) return 0;
                if (!(seen_minerals[i>>3]&(1u<<(i&7u))) && saved.style==(int)raw &&
                    saved.x==src->x_px && saved.y==src->y_px &&
                    saved.amount>=0 && (unsigned)saved.amount<=initial) {
                    seen_minerals[i>>3]|=(u8)(1u<<(i&7u)); found=1; break;
                }
            }
            if (!found) return 0;
        } else if (raw==188u) {
            continue;
        }
    }
    if (source_geysers!=1u || source_extractors!=5u ||
        candidate->state[16]<(int)source_geysers ||
        candidate->state[16]>(int)(source_geysers+source_extractors)) return 0;
    /* The saved node list is exactly the immutable neutral starting geyser
     * plus raw-149 source Extractor positions whose original building row is
     * no longer alive.  This is independent of current runtime map binding,
     * and rejects duplicate, invented, consumed-while-occupied, and stale
     * off-map nodes before live state is mutated. */
    for (unsigned i=0;i<(unsigned)candidate->state[16];++i) {
        int x=candidate->geysers[i].x,y=candidate->geysers[i].y;
        if (x<0 || y<0 || x>=(int)width*16 || y>=(int)height*16 ||
            !race_z7_save_gas_source_row(map,x,y,0)) return 0;
        for (unsigned j=0;j<i;++j)
            if (candidate->geysers[j].x==x && candidate->geysers[j].y==y) return 0;
    }
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        unsigned raw=src->source_type;
        int expected;
        if (raw==188u && src->owner==SC64_OWNER_NEUTRAL) expected=1;
        else if (raw==149u && src->owner==4u)
            expected=!race_z7_save_candidate_has_live_extractor(
                candidate,src->x_px,src->y_px);
        else continue;
        if (race_z7_save_candidate_has_gas(candidate,src->x_px,src->y_px)!=expected)
            return 0;
    }
    for (unsigned i=0;i<MAX_MINERALS;++i) if (!(seen_minerals[i>>3]&(1u<<(i&7u)))) {
        Mineral saved;
        if (!race_save_decode_mineral(candidate,i,width*16u,height*16u,&saved)) return 0;
        if (saved.amount>0) return 0;
    }
    if (!race_z7_save_ai_started(state) &&
        (state->runtime.ai_plan_ticks || state->runtime.ai_plan_phase)) return 0;
    if (state->runtime.model.outcome==SC64_Z7_WON &&
        (!live_human_source_actors || live_enemy_source_buildings)) return 0;
    if (state->runtime.model.outcome==SC64_Z7_LOST &&
        live_human_source_actors) return 0;
    if (live+queued+race_z7_save_remaining_script_creates(state)>MAX_UNITS) return 0;
    return 1;
}

#endif /* SC64_Z7_RUNTIME */
#endif /* SC64_Z7_SAVE_H */
