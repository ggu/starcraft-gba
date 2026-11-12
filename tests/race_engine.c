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
/* Same production gate as the field commands and AI. Stock limits are
 * tested with real provider and unit rows rather than a forced counter. */
static void stock_supply_boundary(int faction, int owner, int custom)
{
    static const int bases[] = {B_COMMAND, B_HATCHERY, B_NEXUS};
    static const int workers[] = {U_SCV, U_DRONE, U_PROBE};
    static const int army[] = {U_TANK, U_MUTALISK, U_ZEALOT};
    int base, providers[26], number;
    init_mission(0);
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    race_mode = custom;
    race_player[owner] = faction;
    if (owner == SIDE) race_human = faction;
    race_bots = owner == 4 ? 3 : owner == RESCUABLE ? 2 : 1;
    minerals = gas = 1000;
    race_minerals[owner] = race_gas[owner] = 1000;
    base = spawn_build(owner, bases[faction], 128, 128, 0);
    assert(base >= 0);
    int base_supply = sc64_race_building_rule(bases[faction])->supply_provided;
    number = (SC64_SUPPLY_LIMIT + 16 - base_supply + 15) / 16;
    for (int i = 0; i < number; ++i) {
        int x = 32 + (i % 8) * 48, y = 200 + (i / 8) * 48;
        if (faction == R_ZERG)
            providers[i] = spawn_unit(owner, U_OVERLORD, x, y);
        else
            providers[i] = spawn_build(owner, faction == R_TER ? B_DEPOT : B_PYLON, x, y, 0);
        assert(providers[i] >= 0);
    }
    for (int i = 0; i < 99; ++i)
        assert(spawn_unit(owner, army[faction], 32 + (i % 16) * 24, 420 + (i / 16) * 24) >= 0);
    assert(spawn_unit(owner, workers[faction], 64, 64) >= 0);
    if (faction == R_ZERG)
        assert(spawn_build(owner, B_SPAWNING_POOL, 176, 128, 0) >= 0);
    assert(race_owner_supply(owner) == 398 && race_owner_supply_capacity(owner) >= 416);
    assert(race_owner_supply_cap(owner) == SC64_SUPPLY_LIMIT);
    if (owner == SIDE) {
        static u8 bar[12 * 240];
        race_tick_state();
        int raw_capacity = supply_cap;
        assert(raw_capacity == race_owner_supply_capacity(owner));
        gfx_begin(C_BLACK);
        draw_hud_resources();
        for (int y = 0; y < 12; ++y)
            for (int x = 0; x < 240; ++x)
                bar[y * 240 + x] = gfx_test_pixel(x, y);
        supply_cap = SC64_SUPPLY_LIMIT;
        gfx_begin(C_BLACK);
        draw_hud_resources();
        for (int y = 0; y < 12; ++y)
            for (int x = 0; x < 240; ++x)
                assert(bar[y * 240 + x] == gfx_test_pixel(x, y));
        supply_cap = raw_capacity;
    }
    assert(race_queue_unit(owner, base, faction == R_ZERG ? U_ZERGLING : workers[faction]));
    assert(race_owner_reserved_supply(owner) == SC64_SUPPLY_LIMIT);
    int ore = race_owner_minerals(owner);
    assert(!race_queue_unit(owner, base, workers[faction]) && race_refusal == 2);
    assert(race_owner_minerals(owner) == ore && b[base].queue_count == 1);

    /* Surplus providers can absorb a loss; only a further loss lowers the
     * effective cap. A provider death does not cancel a paid queue. */
    if (faction == R_ZERG) u[providers[0]].alive = 0;
    else b[providers[0]].alive = 0;
    assert(race_owner_supply_cap(owner) == SC64_SUPPLY_LIMIT);
    if (faction == R_ZERG) u[providers[1]].alive = 0;
    else b[providers[1]].alive = 0;
    assert(race_owner_supply_cap(owner) < SC64_SUPPLY_LIMIT);
    assert(race_owner_reserved_supply(owner) == SC64_SUPPLY_LIMIT);
    assert(!race_queue_unit(owner, base, workers[faction]) && race_refusal == 2);
    if (faction == R_ZERG)
        assert(race_queue_unit(owner, base, U_OVERLORD));
    else
        assert(race_owner_minerals(owner) == ore);
    assert(race_owner_reserved_supply(owner) == SC64_SUPPLY_LIMIT);

    /* Scripted placement/rescue uses the actor primitive, not production
     * admission. Above-cap actors remain and block new paid supply. */
    int reinforcement = spawn_unit(owner, army[faction], 80, 64);
    assert(reinforcement >= 0 && u[reinforcement].alive);
    assert(race_owner_reserved_supply(owner) == SC64_SUPPLY_LIMIT + 4);
    assert(!race_queue_unit(owner, base, workers[faction]) && race_refusal == 2);
}

int main(int argc, char **argv) {
    gba_init();
    if (argc == 2 && !strcmp(argv[1], "--stock-supply")) {
        static const int owners[] = {SIDE, ENEMY, RESCUABLE, 4};
        for (int custom = 0; custom < 2; ++custom)
            for (int faction = R_TER; faction <= R_PROTOSS; ++faction)
                for (int i = 0; i < (custom ? 4 : 2); ++i)
                    stock_supply_boundary(faction, owners[i], custom);
        puts("Stock supply: Terran, Zerg and Protoss campaign/skirmish caps, HUD and reservations pass.");
        return 0;
    }
    for(int n=0;n<4;n++){validate_source_placements(n);save_roundtrip(n);}
    init_mission(0);memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    int z=spawn_unit(SIDE,U_ZERGLING,400,400);
    int a=spawn_unit(ENEMY,U_OVERLORD,401,400);
    int g=spawn_unit(ENEMY,U_MARINE,420,400);
    assert(race_can_attack(U_ZERGLING,a)==0&&race_can_attack(U_HYDRALISK,a));
    SC64_GRID_CLEAR(visible);
    assert(nearest_enemy_typed(400,400,SIDE,64,U_ZERGLING)==-1);
    update_exploration();
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
    assert(carrier>=0);race_campaign.carrier_unit=carrier;u[carrier].order=ORDER_IDLE;frame=29;
    /* Exercise the normal update across the former 30-frame auto-mining
       boundary: a stopped carrier must keep its controller order. */
    assert(screen==SCREEN_PLAY&&!menu_open&&mode==MODE_NORMAL);
    game_update(0,0);assert(frame==30&&u[carrier].order==ORDER_IDLE);
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
