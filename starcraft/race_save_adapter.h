#ifndef SC64_RACE_SAVE_ADAPTER_H
#define SC64_RACE_SAVE_ADAPTER_H
/* Only the persisted units/buildings need 32-bit engine coordinates while
   running. Their bounded integer fields fit 16 bits on cartridge SRAM. */
#define RACE_UNIT_WORDS (sizeof(Unit)/sizeof(int))
#define RACE_BUILD_WORDS (sizeof(Building)/sizeof(int))
typedef struct {
    u32 version;
    s16 units[MAX_UNITS][RACE_UNIT_WORDS];
    s16 buildings[MAX_BUILDINGS][RACE_BUILD_WORDS];
    Mineral minerals_map[MAX_MINERALS];
    u8 fog[(MAP_CELLS+7)/8];
    int state[32],players[5],ore[5],vespene[5];
    Geyser geysers[4];
    Sc64RaceCampaignState campaign;
#ifdef SC64_UNIFIED
    Sc64BackwaterState backwater;
    u8 invincible_units[MAX_UNITS],invincible_buildings[MAX_BUILDINGS];
#endif
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    Sc64T3State t3;
    u16 t3_buildings_killed;
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    Sc64Z3State z3;
    u16 z3_action_ticks;
#endif
} RaceSave;
_Static_assert(sizeof(RaceSave)<=16368,"Race save exceeds one SRAM record");
static RaceSave race_data;
static int race_save_game(void) {
    race_data.version=SAVE_VERSION;
    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {
        int n;memcpy(&n,(u8*)&u[i]+j*sizeof(int),sizeof(n));race_data.units[i][j]=(s16)n;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)for(unsigned j=0;j<RACE_BUILD_WORDS;j++) {
        int n;memcpy(&n,(u8*)&b[i]+j*sizeof(int),sizeof(n));race_data.buildings[i][j]=(s16)n;
    }
    memset(race_data.fog,0,sizeof(race_data.fog));
    for(int i=0;i<MAP_CELLS;i++)if(explored[i])race_data.fog[i>>3]|=1u<<(i&7);
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
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);toast(7);return 1;
}
static int race_load_game(void) {
    if(!save_read(&race_data,sizeof(race_data),SAVE_VERSION)||race_data.version!=SAVE_VERSION)return 0;
    const int *v=race_data.state;
    if(v[0]<0||v[0]>1||v[1]<0||v[1]>2||v[2]<0||v[2]>=SC64_RACE_MAP_COUNT||v[4]<1||v[4]>3||
       v[3]<0||v[3]>=(1<<SC64_RACE_MAP_COUNT)||v[5]<0||v[6]<0||v[7]<0||v[10]<0||v[10]>=SC_RU_COUNT||v[16]<0||v[16]>4)return 0;
#ifdef SC64_CAMPAIGN_RACE
    if(!v[0]&&!sc64_catalog_mission_allowed(v[2],SC64_CAMPAIGN_RACE))return 0;
#endif
#ifdef SC64_UNIFIED
    if(!sc64_catalog_mission_allowed(v[2],
#ifdef SC64_CAMPAIGN_RACE
                                   SC64_CAMPAIGN_RACE
#else
                                   -1
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
    if(width<1||height<1||width>96||height>96)return 0;
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
        if(!RACE_B(i,alive))continue;
        if(RACE_B(i,type)<0||RACE_B(i,type)>=SC_RB_COUNT||RACE_B(i,owner)<0||RACE_B(i,owner)>4||
            RACE_B(i,hp)<=0||RACE_B(i,maxhp)<=0||RACE_B(i,hp)>RACE_B(i,maxhp)||
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
    memcpy(m,race_data.minerals_map,sizeof(m));
    for(int i=0;i<MAP_CELLS;i++)explored[i]=(race_data.fog[i>>3]>>(i&7))&1;
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
    memset(tracers,0,sizeof(tracers));mode=MODE_NORMAL;menu_open=help=0;selected_building=-1;
    invalidate_path_grid();sc64_visibility_reset();update_exploration();reset_selection_gestures();
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
    screen=won||lost?SCREEN_RESULT:SCREEN_PLAY;toast(7);return 1;
}
#endif
