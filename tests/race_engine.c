/* Integration contracts: imported source flags, air combat/collision, all
   four map dimensions and packed SRAM round trips across mode changes. */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"

static void init_mission(int n) {game_init();race_selected_mission=n;init_state();}
static void validate_source_placements(int n) {
    init_mission(n);
    const Sc64RaceMap *map=&sc64_race_maps[n];
    int units=0,structures=0,ore=0,vents=0,live_units=0,live_structures=0;
    for(int j=0;j<map->unit_count;j++) {
        int raw=map->units[j].source_type;
        if(raw==176||raw==177||raw==178){ore++;continue;}
        if(raw==188){vents++;continue;}
        if(sc64_race_unit_from_raw(raw)>=0)units++;
        else {assert(sc64_race_building_from_raw(raw)>=0);structures++;}
    }
    for(int j=0;j<MAX_UNITS;j++)if(u[j].alive)live_units++;
    for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive)live_structures++;
    assert(units==live_units&&structures==live_structures&&vents==geyser_count);
    int ore_loaded=0;for(int j=0;j<MAX_MINERALS;j++)if(m[j].amount)ore_loaded++;
    assert(ore==ore_loaded&&mission_unsupported_count==0);
}
static void save_roundtrip(int n) {
    init_mission(n);
    for(int j=0;j<190;j++)game_update(0,0);
    static Unit old_u[MAX_UNITS];static Building old_b[MAX_BUILDINGS];
    static Mineral old_m[MAX_MINERALS];static u8 old_fog[MAP_CELLS];
    memcpy(old_u,u,sizeof(u));memcpy(old_b,b,sizeof(b));memcpy(old_m,m,sizeof(m));memcpy(old_fog,explored,sizeof(explored));
    Sc64RaceCampaignState old_c=race_campaign;
    assert(save_game());
    int saved_frame=frame,old_min=minerals,old_gas=gas;
    race_mode=1;race_human=R_TER;init_state();
    assert(load_game());
    assert(race_selected_mission==n&&!race_mode&&frame==saved_frame&&minerals==old_min&&gas==old_gas);
    assert(!memcmp(u,old_u,sizeof(u)));assert(!memcmp(b,old_b,sizeof(b)));
    assert(!memcmp(m,old_m,sizeof(m)));assert(!memcmp(explored,old_fog,sizeof(explored)));
    assert(!memcmp(&race_campaign,&old_c,sizeof(old_c)));
    game_render();
    /* Even a checksummed but semantically invalid record is rejected before
       any live state changes or array-index use. */
    race_data.units[0][offsetof(Unit,type)/sizeof(int)]=30000;
    race_data.units[0][offsetof(Unit,alive)/sizeof(int)]=1;
    save_write(&race_data,sizeof(race_data),SAVE_VERSION);
    assert(!load_game());assert(frame==saved_frame);
}
int main(void) {
    gba_init();
    for(int n=0;n<4;n++){validate_source_placements(n);save_roundtrip(n);}
    init_mission(0);memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    int z=spawn_unit(SIDE,U_ZERGLING,400,400);
    int a=spawn_unit(ENEMY,U_OVERLORD,401,400);
    int g=spawn_unit(ENEMY,U_MARINE,420,400);
    assert(race_can_attack(U_ZERGLING,a)==0&&race_can_attack(U_HYDRALISK,a));
    assert(nearest_enemy_typed(400,400,SIDE,64,U_ZERGLING)==g);
    assert(unit_step_clear(&u[z],401,400));
    sc64_unit_buckets_begin();assert(unit_step_clear(&u[z],401,400));sc64_unit_buckets_end();
    /* Mineral/gas orders use StarCraft's gathering-worker collision exception;
       ordinary move orders retain collisions, in both query paths. */
    int drone=spawn_unit(SIDE,U_DRONE,405,400);
    u[drone].order=ORDER_MOVE;assert(!unit_step_clear(&u[drone],400,400));
    u[drone].order=ORDER_MINE;assert(unit_step_clear(&u[drone],400,400));
    sc64_unit_buckets_begin();
    u[drone].order=ORDER_MOVE;assert(!unit_step_clear(&u[drone],400,400));
    u[drone].order=ORDER_GAS;assert(unit_step_clear(&u[drone],400,400));
    sc64_unit_buckets_end();
    int p=spawn_unit(SIDE,U_ZEALOT,600,600);
    int hp=u[p].hp;u[p].shield=5;hit_target(p,8,U_MARINE,ENEMY);
    assert(u[p].shield==0&&u[p].hp<hp);
    u[p].flags=16;hp=u[p].hp;hit_target(p,100,U_MARINE,ENEMY);assert(u[p].hp==hp);
    for(int n=0;n<200;n++)race_tick_state();
    assert(u[p].flags==16&&u[p].shield>0);
    assert(!owners_hostile(SIDE,RESCUABLE)&&owners_hostile(SIDE,ENEMY));
    race_mode=1;assert(owners_hostile(SIDE,RESCUABLE));
    /* Normal geysers yield eight gas per delivered trip, as in the pinned
       OpenBW gather_resource rule, rather than the old demo's four. */
    init_mission(0);memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    int home=spawn_build(SIDE,B_HATCHERY,600,600,0);
    int refinery=spawn_build(SIDE,B_EXTRACTOR,650,600,0);
    int gas_worker=spawn_unit(SIDE,U_DRONE,635,600);
    u[gas_worker].order=ORDER_GAS;u[gas_worker].target=refinery;
    u[gas_worker].work_timer=0;int old_gas=gas;
    update_orders();assert(u[gas_worker].carry==8);
    u[gas_worker].x=b[home].x+24;u[gas_worker].y=b[home].y;
    update_orders();assert(gas==old_gas+8&&!u[gas_worker].carry);
    /* Campaign worker production must gather local ore, not one of the
       authored skirmish arena's fixed patch indices in enemy territory. */
    init_mission(0);
    int hatch=-1;for(int j=0;j<MAX_BUILDINGS;j++)if(b[j].alive&&b[j].owner==SIDE&&b[j].type==B_HATCHERY)hatch=j;
    assert(hatch>=0&&race_queue_unit(SIDE,hatch,U_DRONE));
    for(int j=0;j<375;j++)race_tick_buildings();
    int produced=-1;for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&u[j].type==U_DRONE&&u[j].raw_type<0)produced=j;
    assert(produced>=0&&u[produced].order==ORDER_MINE);
    int patch=u[produced].target;
    assert(patch>=0&&patch<MAX_MINERALS&&dist2(u[produced].x,u[produced].y,m[patch].x,m[patch].y)<200*200);
    init_mission(1);
    int carrier=-1;for(int j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&u[j].type==U_DRONE)carrier=j;
    assert(carrier>=0);race_campaign.carrier_unit=carrier;u[carrier].order=ORDER_IDLE;frame=30;
    economy();assert(u[carrier].order==ORDER_IDLE);
    /* A melee attacker reaches a building perimeter and applies armor to
       each Zealot blade hit, rather than walking into blocked center tiles. */
    init_mission(0);memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    int melee=spawn_unit(SIDE,U_ZEALOT,576,600);
    int structure=spawn_build(ENEMY,B_HATCHERY,600,600,0);
    int building_hp=b[structure].hp;
    tick_combat(&u[melee],MAX_UNITS+structure);
    assert(b[structure].hp==building_hp-14);
    /* Construction remains unpaid and has no foundation while its Drone is
       walking; the pending order stores a building type, not a building slot. */
    game_init();race_mode=1;race_human=R_ZERG;race_bots=2;init_state();
    int before_minerals=minerals,before_gas=gas,foundations=0;
    assert(race_start_build(SIDE,1,B_SPAWNING_POOL,208,160));
    assert(u[1].order==ORDER_BUILD_PENDING&&u[1].target==B_SPAWNING_POOL);
    assert(minerals==before_minerals&&gas==before_gas);
    for(int i=0;i<MAX_BUILDINGS;i++)foundations+=b[i].alive&&b[i].type==B_SPAWNING_POOL;
    assert(foundations==0);
    assert(save_game());assert(load_game());
    assert(u[1].order==ORDER_BUILD_PENDING&&u[1].target==B_SPAWNING_POOL);
    assert(minerals==before_minerals&&gas==before_gas);
    for(int cells=64;cells<=96;cells+=32) {
        race_cols=cells;
        for(unsigned cell=0;cell<96u*(unsigned)cells;cell++) {
            assert(sc64_cell_row(cell)==(int)(cell/cells));
            assert(sc64_cell_col(cell)==(int)(cell%cells));
        }
        for(unsigned pixel=0;pixel<(unsigned)cells*16u;pixel++)
            assert(sc64_minimap_pixel(pixel,cells)==(int)(pixel*24u/(cells*16u)));
    }
    assert(sizeof(RaceSave)<=16368);
    printf("Race engine: four map SRAM round trips, hostile relations, air collision/targeting and shield flags pass (%zu-byte payload).\n",sizeof(RaceSave));
}
