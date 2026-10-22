#ifndef SC64_RACE_SAVE_ADAPTER_H
#define SC64_RACE_SAVE_ADAPTER_H
#ifdef SC64_T3_ENABLED
#ifdef SC64_T4_SCENARIO_RUNTIME
#define race_data sc64_combined_route_save_scratch.save
#else
static RaceSave race_data;
#endif
#else
static RaceSave race_data;
#endif
#ifdef SC64_T3_ENABLED
#ifdef SC64_T4_SCENARIO_RUNTIME
static int
race_t4_save_pack(Sc64T4SaveState *dst, const Sc64T4RuntimeState *src)
{
    Sc64T4TriggerState inactive;
    if (!dst || !src ||
        !sc64_t4_trigger_init(&inactive, &sc64_t4_scenario, 5,
                              sc64_t4_source_force(5)) ||
        memcmp(&src->player[5], &inactive, sizeof(inactive)) != 0) return 0;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->player, src->player, 5 * sizeof(dst->player[0]));
    dst->player[5] = src->player[6];
    dst->player[6] = src->player[7];
    dst->switches = src->switches;
    memcpy(dst->location30, src->location30, sizeof(dst->location30));
    dst->message_ticks = src->message_ticks;
    dst->ping_x = src->ping_x;
    dst->ping_y = src->ping_y;
    dst->ping_ticks = src->ping_ticks;
    dst->location30_moved = src->location30_moved;
    dst->message = src->message;
    dst->active_mask = src->active_mask;
    memcpy(dst->doodad_slot, src->doodad_slot, sizeof(dst->doodad_slot));
    memcpy(dst->doodad_enabled, src->doodad_enabled, sizeof(dst->doodad_enabled));
    return 1;
}

static int
race_t4_save_expand(Sc64T4RuntimeState *dst, const Sc64T4SaveState *src)
{
    Sc64T4TriggerState inactive;
    if (!dst || !src ||
        !sc64_t4_trigger_init(&inactive, &sc64_t4_scenario, 5,
                              sc64_t4_source_force(5))) return 0;
    memset(dst, 0, sizeof(*dst));
    memcpy(dst->player, src->player, 5 * sizeof(src->player[0]));
    dst->player[5] = inactive;
    dst->player[6] = src->player[5];
    dst->player[7] = src->player[6];
    dst->switches = src->switches;
    memcpy(dst->location30, src->location30, sizeof(dst->location30));
    dst->message_ticks = src->message_ticks;
    dst->ping_x = src->ping_x;
    dst->ping_y = src->ping_y;
    dst->ping_ticks = src->ping_ticks;
    dst->location30_moved = src->location30_moved;
    dst->message = src->message;
    dst->active_mask = src->active_mask;
    memcpy(dst->doodad_slot, src->doodad_slot, sizeof(dst->doodad_slot));
    memcpy(dst->doodad_enabled, src->doodad_enabled, sizeof(dst->doodad_enabled));
    return 1;
}
#endif
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
static int
race_t4_save_candidate_valid(const RaceSave *candidate, int width, int height)
{
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    const Sc64T4SaveState *state;
#else
    const Sc64T4RuntimeState *state;
#endif
    unsigned next_building = 8;
    if (!candidate) return 0;
    state = &candidate->t4;
    if (candidate->state[0] != 0 || candidate->state[2] != SC64_T4_GLOBAL_ID ||
        candidate->state[7] < 0 ||
        (u32)candidate->state[7] != candidate->campaign.elapsed_frames ||
        candidate->campaign.mission != SC64_T4_GLOBAL_ID ||
        state->active_mask != 0xdfu || state->location30_moved > 1 ||
        state->message_ticks > 90 || state->ping_ticks > 90 ||
        state->ping_x >= width * 16 || state->ping_y >= height * 16) return 0;
    if (state->message != candidate->campaign.message ||
        state->message_ticks != candidate->campaign.message_ticks ||
        (state->ping_ticks == 0 && (state->ping_x || state->ping_y)) ||
        candidate->campaign.objective_stage !=
            ((state->switches.bits[0] & 0x10u) ? 1u : 0u)) return 0;
    if (candidate->campaign.phase == SC64_RACE_PHASE_ACTIVE) {
        if (candidate->state[18] || candidate->state[19] ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (candidate->campaign.phase == SC64_RACE_PHASE_COMPLETE) {
        if (!candidate->state[18] || candidate->state[19] ||
            !(candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else if (candidate->campaign.phase == SC64_RACE_PHASE_FAILED) {
        if (candidate->state[18] || !candidate->state[19] ||
            (candidate->campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY)) return 0;
    } else return 0;
    if (state->location30_moved) {
        const Sc64T4Location *original = &sc64_t4_scenario.locations[29];
        unsigned left = state->location30[0], top = state->location30[1];
        unsigned right = state->location30[2], bottom = state->location30[3];
        if (original->right <= original->left || original->bottom <= original->top ||
            left >= right || top >= bottom || right > (unsigned)width * 32u ||
            bottom > (unsigned)height * 32u ||
            right - left != original->right - original->left ||
            bottom - top != original->bottom - original->top) return 0;
    } else if (state->location30[0] || state->location30[1] ||
               state->location30[2] || state->location30[3]) return 0;
    for (unsigned player = 0; player < 8; ++player) {
        const Sc64T4TriggerState *p;
        unsigned force = sc64_t4_source_force((int)player);
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
        if (player == 5) {
            Sc64T4TriggerState initial;
            if (!sc64_t4_trigger_init(&initial, &sc64_t4_scenario, player, force) ||
                !sc64_t4_trigger_state_valid(&initial, &sc64_t4_scenario)) return 0;
            continue;
        }
        p = &state->player[player < 5 ? player : player - 1];
#else
        p = &state->player[player];
#endif
        if (!sc64_t4_trigger_state_valid(p, &sc64_t4_scenario) ||
            p->owner != player || p->force != force) return 0;
#if !defined(SC64_T3_ENABLED) || !defined(SC64_UNIFIED)
        if (player == 5) {
            Sc64T4TriggerState initial;
            if (!sc64_t4_trigger_init(&initial, &sc64_t4_scenario, player, force) ||
                memcmp(p, &initial, sizeof(initial)) != 0) return 0;
            continue;
        }
#endif
        if (p->elapsed_ticks != candidate->campaign.elapsed_frames) {
            return 0;
        }
    }
    for (unsigned d = 0; d < sc64_t4_scenario.doodad_count; ++d) {
        const Sc64T4Doodad *row = &sc64_t4_scenario.doodads[d];
        unsigned expected_slot = 255;
        int type;
        if (state->doodad_enabled[d] > 1) return 0;
        if (!(row->flags & SC64_T4_THG2_SPRITE_FLAG)) expected_slot = next_building++;
        if (state->doodad_slot[d] != expected_slot) return 0;
        if (expected_slot == 255) {
            if (state->doodad_enabled[d]) return 0;
            continue;
        }
        if (expected_slot >= MAX_BUILDINGS) return 0;
        type = sc64_race_building_from_raw(row->type);
        if (type < 0 ||
            candidate->buildings[expected_slot][offsetof(Building,type)/sizeof(int)] != type ||
            candidate->buildings[expected_slot][offsetof(Building,raw_type)/sizeof(int)] != row->type ||
            candidate->buildings[expected_slot][offsetof(Building,raw_owner)/sizeof(int)] != row->owner ||
            candidate->buildings[expected_slot][offsetof(Building,x)/sizeof(int)] != row->x / 2 ||
            candidate->buildings[expected_slot][offsetof(Building,y)/sizeof(int)] != row->y / 2 ||
            candidate->invincible_buildings[expected_slot] !=
                (unsigned char)sc64_race_t4_building_intrinsically_invincible(type)) return 0;
        {
            int owner = sc64_race_t4_is_door(type) ? SC64_T4_OWNER_NEUTRAL :
                        sc64_t4_runtime_owner(row->owner);
            if (owner < 0 || candidate->buildings[expected_slot]
                    [offsetof(Building,owner)/sizeof(int)] != owner) return 0;
        }
        if (sc64_race_t4_is_door(type) &&
            candidate->buildings[expected_slot]
                [offsetof(Building,queue_progress)/sizeof(int)] != 0) return 0;
        if (sc64_race_t4_is_trap(type) &&
            (candidate->buildings[expected_slot]
                [offsetof(Building,queue_progress)/sizeof(int)] < 0 ||
             candidate->buildings[expected_slot]
                [offsetof(Building,queue_progress)/sizeof(int)] > 28)) return 0;
    }
    if (next_building != 34) return 0;
    return 1;
}
#endif
static int sc64_race_storage_geometry_valid(int width, int height) {
#ifdef SC64_T4_RUNTIME
    return width >= 1 && width <= 128 && height >= 1 && height <= 128;
#elif defined(SC64_LARGE_MAPS)
    return width >= 1 && width <= 128 && height >= 1 && height <= 96;
#else
    return width >= 1 && width <= 96 && height >= 1 && height <= 96;
#endif
}
static int race_save_game(void) {
#ifdef SC64_T5_RUNTIME
    if (race_t5_active()) return 0;
#endif
#ifdef SC64_T4_RUNTIME
    /* T4 props with intrinsic invincibility have 100000 source HP. Their
     * health words use canonical zero encoding instead of truncating to s16. */
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(sc64_race_t4_building_intrinsically_invincible(b[i].type) &&
           (b[i].hp!=100000 || b[i].maxhp!=100000 ||
            b[i].raw_type!=sc64_race_raw_building(b[i].type) || b[i].under_construction ||
            b[i].queue_count || b[i].shield || !invincible_buildings[i]))return 0;
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    /* The save shares storage with the transient route queue. Clear all
     * padding and optional fields before serializing this record. */
    memset(&race_data, 0, sizeof(race_data));
#endif
    race_data.version=SAVE_VERSION;
    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {
        int n;memcpy(&n,(u8*)&u[i]+j*sizeof(int),sizeof(n));race_data.units[i][j]=(s16)n;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)for(unsigned j=0;j<RACE_BUILD_WORDS;j++) {
        int n;memcpy(&n,(u8*)&b[i]+j*sizeof(int),sizeof(n));
#ifdef SC64_T4_RUNTIME
        if(sc64_race_t4_building_intrinsically_invincible(b[i].type) &&
           (j==offsetof(Building,hp)/sizeof(int) ||
            j==offsetof(Building,maxhp)/sizeof(int)))n=0;
#endif
        race_data.buildings[i][j]=(s16)n;
    }
    memset(race_data.fog,0,sizeof(race_data.fog));
    for(int i=0;i<MAP_CELLS;i++)if(SC64_GRID_GET(explored,i))race_data.fog[i>>3]|=1u<<(i&7);
    memcpy(race_data.minerals_map,m,sizeof(m));
    int state[]={race_mode,race_human,race_selected_mission,race_completed,race_bots,minerals,gas,
        frame,cx,cy,train_type,weapons_level,armor_level,weapons_research,armor_research,
        (int)seed,geyser_count,mission_objective_stage,won,lost
#ifdef SC64_IMPORTED_SKIRMISH
        ,race_skirmish_map,race_bot_races[0],race_bot_races[1],race_bot_races[2]
#endif
        };
    memcpy(race_data.state,state,sizeof(state));
    memcpy(race_data.players,race_player,sizeof(race_player));
    memcpy(race_data.ore,race_minerals,sizeof(race_minerals));
    memcpy(race_data.vespene,race_gas,sizeof(race_gas));
    memcpy(race_data.geysers,geysers,sizeof(race_data.geysers));
    race_data.campaign=race_campaign;
#ifdef SC64_UNIFIED
    race_data.backwater=backwater;
    memcpy(race_data.invincible_units,invincible_units,sizeof(invincible_units));
    memcpy(race_data.invincible_buildings,invincible_buildings,sizeof(invincible_buildings));
#endif
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    if(!race_mode&&race_selected_mission==6) {
        race_data.t3=race_t3;
        race_data.t3_buildings_killed=race_t3_buildings_killed;
    } else {
        memset(&race_data.t3,0,sizeof(race_data.t3));
        race_data.t3_buildings_killed=0;
    }
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    memset(&race_data.z3,0,sizeof(race_data.z3));race_data.z3_action_ticks=0;
    if(!race_mode&&race_selected_mission==14) {
        race_data.z3=race_z3;
        race_data.z3_action_ticks=race_z3_action_ticks;
    }
#endif
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    memset(&race_data.p3,0,sizeof(race_data.p3));race_data.p3_action_ticks=0;
    if(!race_mode&&race_selected_mission==22) {
        race_data.p3=race_p3;
        race_data.p3_action_ticks=race_p3_action_ticks;
    }
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active()) {
#ifdef SC64_T3_ENABLED
        if (!race_t4_save_pack(&race_data.t4, &race_t4)) return 0;
#else
        race_data.t4 = race_t4;
#endif
    }
    else memset(&race_data.t4, 0, sizeof(race_data.t4));
    if (race_t4_active() &&
        !race_t4_save_candidate_valid(&race_data, race_cols, race_rows)) return 0;
#endif
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);toast(7);return 1;
}
static int race_load_game(void) {
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    Sc64T4RuntimeState expanded_t4;
    int expanded_t4_valid = 0;
#endif
    if(!save_read(&race_data,sizeof(race_data),SAVE_VERSION)||race_data.version!=SAVE_VERSION)return 0;
    const int *v=race_data.state;
#ifdef SC64_T5_RUNTIME
    /* The internal T5 stage has no persisted AI/trigger schema yet. Reject
     * it before mutating any live state. Its catalog entry remains gated. */
    if (!v[0] && v[2] == SC64_T5_GLOBAL_ID) return 0;
#endif
    if(v[0]<0||v[0]>1||v[1]<0||v[1]>2||v[2]<0||v[2]>=SC64_RACE_MAP_COUNT||v[4]<1||v[4]>3||
       v[3]<0||v[3]>=(1<<SC64_RACE_MAP_COUNT)||v[5]<0||v[6]<0||v[7]<0||v[10]<0||v[10]>=SC_RU_COUNT||v[16]<0||v[16]>4)return 0;
#ifdef SC64_CAMPAIGN_RACE
    if(!v[0]&&!sc64_catalog_mission_allowed(v[2],SC64_CAMPAIGN_RACE)
#ifdef SC64_T4_RUNTIME
       &&v[2]!=SC64_T4_GLOBAL_ID
#endif
       )return 0;
#endif
#ifdef SC64_UNIFIED
    if((!sc64_catalog_mission_allowed(v[2],
#ifdef SC64_CAMPAIGN_RACE
                                   SC64_CAMPAIGN_RACE
#else
                                   -1
#endif
                                   )
#ifdef SC64_T4_RUNTIME
        &&v[2]!=SC64_T4_GLOBAL_ID
#endif
        )||!sc64_race_maps[v[2]].width)return 0;
    if((unsigned)v[3]&~sc64_catalog_playable_mask(
#ifdef SC64_CAMPAIGN_RACE
                                               SC64_CAMPAIGN_RACE
#else
                                               -1
#endif
                                               ))return 0;
#endif
    int width=v[0]?64:sc64_race_maps[v[2]].width;
    int height=v[0]?64:sc64_race_maps[v[2]].height;
#ifdef SC64_IMPORTED_SKIRMISH
    if(v[20]<0||v[20]>SC64_SKIRMISH_IMPORTED_COUNT)return 0;
    for(int i=21;i<24;i++)if(v[i]<0||v[i]>2)return 0;
    if(v[0]&&v[20]>0){width=sc64_skirmish_maps[v[20]-1].width;height=sc64_skirmish_maps[v[20]-1].height;}
#endif
    if(!sc64_race_storage_geometry_valid(width,height))return 0;
    if(v[8]<0||v[8]>=width*16||v[9]<0||v[9]>=height*16||v[11]<0||v[11]>2||
       v[12]<0||v[12]>2||v[18]<0||v[18]>1||v[19]<0||v[19]>1)return 0;
    for(int i=0;i<5;i++)if(race_data.players[i]<-1||race_data.players[i]>2||
        race_data.ore[i]<0||race_data.ore[i]>65535||race_data.vespene[i]<0||race_data.vespene[i]>65535)return 0;
#define RACE_U(i,f) race_data.units[i][offsetof(Unit,f)/sizeof(int)]
#define RACE_B(i,f) race_data.buildings[i][offsetof(Building,f)/sizeof(int)]
    for(int i=0;i<MAX_UNITS;i++) {
        if(RACE_U(i,alive)<0||RACE_U(i,alive)>1)return 0;
        if(!RACE_U(i,alive))continue;
        if(RACE_U(i,type)<0||RACE_U(i,type)>=SC_RU_COUNT||RACE_U(i,owner)<0||RACE_U(i,owner)>4||
            RACE_U(i,hp)<=0||RACE_U(i,maxhp)<=0||RACE_U(i,hp)>RACE_U(i,maxhp)||
            RACE_U(i,x)<0||RACE_U(i,x)>=width*16||RACE_U(i,y)<0||RACE_U(i,y)>=height*16||
            RACE_U(i,order)<ORDER_IDLE||RACE_U(i,order)>ORDER_GAS||RACE_U(i,shield)<0||
            RACE_U(i,shield_remainder)<0||RACE_U(i,shield_remainder)>=1280)return 0;
        int target=RACE_U(i,target),order=RACE_U(i,order);
        if((order==ORDER_MINE&&(target<0||target>=MAX_MINERALS))||
           ((order==ORDER_GAS||order==ORDER_BUILD)&&(target<0||target>=MAX_BUILDINGS)))return 0;
    }
    for(int i=0;i<MAX_BUILDINGS;i++) {
        if(RACE_B(i,alive)<0||RACE_B(i,alive)>1)return 0;
#ifdef SC64_T4_RUNTIME
        if(RACE_B(i,type)>=0&&RACE_B(i,type)<SC_RB_COUNT&&
           sc64_race_t4_building_intrinsically_invincible(RACE_B(i,type))&&
           (RACE_B(i,hp)!=0||RACE_B(i,maxhp)!=0||
            RACE_B(i,raw_type)!=sc64_race_raw_building(RACE_B(i,type))||
            RACE_B(i,under_construction)!=0||RACE_B(i,queue_count)!=0||
            RACE_B(i,shield)!=0||race_data.invincible_buildings[i]!=1))return 0;
#endif
        if(!RACE_B(i,alive))continue;
#ifdef SC64_T4_RUNTIME
        int health_ok=RACE_B(i,hp)>0&&RACE_B(i,maxhp)>0&&RACE_B(i,hp)<=RACE_B(i,maxhp);
        if(sc64_race_t4_building_intrinsically_invincible(RACE_B(i,type)))
            health_ok=1; /* Canonical zero was validated above. */
#endif
        if(RACE_B(i,type)<0||RACE_B(i,type)>=SC_RB_COUNT||RACE_B(i,owner)<0||RACE_B(i,owner)>4||
#ifdef SC64_T4_RUNTIME
            !health_ok||
#else
            RACE_B(i,hp)<=0||RACE_B(i,maxhp)<=0||RACE_B(i,hp)>RACE_B(i,maxhp)||
#endif
            RACE_B(i,x)<0||RACE_B(i,x)>=width*16||RACE_B(i,y)<0||RACE_B(i,y)>=height*16||
            RACE_B(i,queue_count)<0||RACE_B(i,queue_count)>5||RACE_B(i,queue_head)<0||RACE_B(i,queue_head)>4||
            RACE_B(i,queue_progress)<0||RACE_B(i,build_progress)<-1||RACE_B(i,shield)<0||
            RACE_B(i,larva)<0||RACE_B(i,larva)>3||RACE_B(i,shield_remainder)<0||RACE_B(i,shield_remainder)>=1280)return 0;
        for(int j=0;j<RACE_B(i,queue_count);j++) {
            int type=race_data.buildings[i][offsetof(Building,queue)/sizeof(int)+(RACE_B(i,queue_head)+j)%5];
            if(type<0||type>=SC_RU_COUNT)return 0;
        }
    }
#undef RACE_U
#undef RACE_B
    for(int i=0;i<MAX_MINERALS;i++)if(race_data.minerals_map[i].amount<0||
        race_data.minerals_map[i].amount>65535)return 0;
#ifdef SC64_UNIFIED
    if(race_data.backwater.outcome>SC64_BACKWATER_LOST||race_data.backwater.objective>1||
       race_data.backwater.spawn_remaining>5||race_data.campaign.phase>SC64_RACE_PHASE_FAILED||
       (!v[0]&&race_data.campaign.mission!=v[2]))return 0;
    for(int i=0;i<MAX_UNITS;i++)if(race_data.invincible_units[i]>1)return 0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(race_data.invincible_buildings[i]>1)return 0;
#endif
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    if(!v[0]&&v[2]==6) {
        u8 phase=race_data.campaign.phase;
        u32 t3_elapsed=race_data.t3.elapsed_ticks;
        if(!sc64_t3_state_valid(&race_data.t3))return 0;
        if(race_data.campaign.elapsed_frames!=t3_elapsed||(u32)v[7]!=t3_elapsed||
           race_data.campaign.objective_stage!=race_data.t3.objective)return 0;
        /* The optional objective can only be set after at least one building
           kill. Kills after the 120-second trigger window remain valid even
           when the alternate objective was never revealed. */
        if(race_data.t3.objective&&race_data.t3_buildings_killed==0)return 0;
        if(race_data.t3.outcome==SC64_T3_RUNNING) {
            if(v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else if(race_data.t3.outcome==SC64_T3_WON) {
            if(!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
               !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else if(race_data.t3.outcome==SC64_T3_LOST) {
            if(v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else return 0;
    }
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    if(!v[0]&&v[2]==14) {
        u8 phase=race_data.campaign.phase;
        if(!sc64_z3_state_valid(&race_data.z3)||
           race_data.campaign.elapsed_frames!=(u32)v[7]||
           race_data.campaign.objective_stage!=race_data.z3.switch7)return 0;
        u16 duration=race_z3_action_duration(&race_data.z3);
        if(duration ? (!race_data.z3_action_ticks||race_data.z3_action_ticks>duration) :
                      race_data.z3_action_ticks!=0)return 0;
        if(race_data.z3.outcome==SC64_Z3_RUNNING) {
            if(v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else if(race_data.z3.outcome==SC64_Z3_WON) {
            if(!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
               !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else {
            if(v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        }
    } else {
        if(race_data.z3.fired||race_data.z3.switch7||race_data.z3.outcome||
           race_data.z3.sequence||race_data.z3.transmission||race_data.z3_action_ticks)return 0;
    }
#endif
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if(!v[0]&&v[2]==SC64_P3_GLOBAL_ID&&
       (v[1]!=R_PROTOSS||race_data.players[SIDE]!=R_PROTOSS||
        race_data.players[ENEMY]!=R_ZERG||race_data.players[4]!=R_ZERG))return 0;
    if(!v[0]&&v[2]==22) {
        u8 phase=race_data.campaign.phase;
        if(!sc64_p3_model_state_valid(&race_data.p3)||
           race_data.campaign.elapsed_frames!=(u32)v[7]||
           race_data.campaign.objective_stage!=race_data.p3.sequence)return 0;
        u16 duration=race_p3_action_duration(&race_data.p3);
        if(duration ? (!race_data.p3_action_ticks||race_data.p3_action_ticks>duration) :
                      race_data.p3_action_ticks!=0)return 0;
        if(race_data.p3.outcome==SC64_P3_RUNNING) {
            if(v[18]||v[19]||phase!=SC64_RACE_PHASE_ACTIVE||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        } else if(race_data.p3.outcome==SC64_P3_WON) {
            if(!v[18]||v[19]||phase!=SC64_RACE_PHASE_COMPLETE||
               !(race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY)||
               !(v[3]&(1<<22)))return 0;
        } else {
            if(v[18]||!v[19]||phase!=SC64_RACE_PHASE_FAILED||
               (race_data.campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY))return 0;
        }
    } else {
        const u8 *p=(const u8 *)&race_data.p3;
        for(unsigned i=0;i<sizeof(race_data.p3);i++)if(p[i])return 0;
        if(race_data.p3_action_ticks)return 0;
    }
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (!v[0] && v[2] == SC64_T4_GLOBAL_ID) {
        if (!race_t4_save_candidate_valid(&race_data, width, height)) return 0;
#ifdef SC64_T3_ENABLED
        if (!race_t4_save_expand(&expanded_t4, &race_data.t4)) return 0;
        expanded_t4_valid = 1;
#endif
    } else {
        const u8 *t4 = (const u8 *)&race_data.t4;
        for (unsigned i = 0; i < sizeof(race_data.t4); ++i)
            if (t4[i]) return 0;
    }
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
    if (!v[0] && v[2] == SC64_T4_GLOBAL_ID && !expanded_t4_valid) return 0;
#endif
    race_mode=v[0];race_human=v[1];race_selected_mission=v[2];race_completed=v[3];race_bots=v[4];
#ifdef SC64_IMPORTED_SKIRMISH
    race_skirmish_map=v[20];for(int i=0;i<3;i++)race_bot_races[i]=v[21+i];
#endif
    minerals=v[5];gas=v[6];frame=v[7];cx=v[8];cy=v[9];train_type=v[10];
    weapons_level=v[11];armor_level=v[12];weapons_research=v[13];armor_research=v[14];seed=(u32)v[15];
    geyser_count=v[16];mission_objective_stage=v[17];won=v[18];lost=v[19];
    race_cols=race_mode?64:sc64_race_maps[race_selected_mission].width;
    race_rows=race_mode?64:sc64_race_maps[race_selected_mission].height;
    if(race_mode)race_skirmish_geometry();
    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {
        int n=j==offsetof(Unit,flags)/sizeof(int)?(u16)race_data.units[i][j]:race_data.units[i][j];
        memcpy((u8*)&u[i]+j*sizeof(int),&n,sizeof(n));
    }
    for(int i=0;i<MAX_BUILDINGS;i++)for(unsigned j=0;j<RACE_BUILD_WORDS;j++) {
        int n=j==offsetof(Building,flags)/sizeof(int)?(u16)race_data.buildings[i][j]:race_data.buildings[i][j];
        memcpy((u8*)&b[i]+j*sizeof(int),&n,sizeof(n));
    }
#ifdef SC64_T4_RUNTIME
    for(int i=0;i<MAX_BUILDINGS;i++)
        if(sc64_race_t4_building_intrinsically_invincible(b[i].type))
            b[i].hp=b[i].maxhp=100000;
#endif
    memcpy(m,race_data.minerals_map,sizeof(m));
    SC64_GRID_CLEAR(explored);
    for(int i=0;i<MAP_CELLS;i++)
        if((race_data.fog[i>>3]>>(i&7))&1)SC64_GRID_SET(explored,i,1);
    memcpy(race_player,race_data.players,sizeof(race_player));
    memcpy(race_minerals,race_data.ore,sizeof(race_minerals));
    memcpy(race_gas,race_data.vespene,sizeof(race_gas));
    memcpy(geysers,race_data.geysers,sizeof(race_data.geysers));race_campaign=race_data.campaign;
#ifdef SC64_UNIFIED
    backwater=race_data.backwater;
    memcpy(invincible_units,race_data.invincible_units,sizeof(invincible_units));
    memcpy(invincible_buildings,race_data.invincible_buildings,sizeof(invincible_buildings));
    backwater_message_ticks=0;
#endif
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    if(!race_mode&&race_selected_mission==6) {
        race_t3=race_data.t3;
        race_t3_buildings_killed=race_data.t3_buildings_killed;
    } else {
        memset(&race_t3,0,sizeof(race_t3));
        race_t3_buildings_killed=0;
    }
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    if(!race_mode&&race_selected_mission==14) {
        race_z3=race_data.z3;race_z3_action_ticks=race_data.z3_action_ticks;
    } else {
        memset(&race_z3,0,sizeof(race_z3));race_z3_action_ticks=0;
    }
#endif
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if(!race_mode&&race_selected_mission==22) {
        race_p3=race_data.p3;race_p3_action_ticks=race_data.p3_action_ticks;
    } else {
        memset(&race_p3,0,sizeof(race_p3));race_p3_action_ticks=0;
    }
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (!race_mode && race_selected_mission == SC64_T4_GLOBAL_ID) {
#ifdef SC64_T3_ENABLED
        race_t4 = expanded_t4;
#else
        race_t4 = race_data.t4;
#endif
    } else
        memset(&race_t4, 0, sizeof(race_t4));
#endif
    memset(tracers,0,sizeof(tracers));mode=MODE_NORMAL;menu_open=help=0;selected_building=-1;
    invalidate_path_grid();sc64_visibility_reset();update_exploration();reset_selection_gestures();
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
    screen=won||lost?SCREEN_RESULT:SCREEN_PLAY;toast(7);return 1;
}
#endif
