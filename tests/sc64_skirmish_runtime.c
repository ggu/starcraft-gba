/* Runtime admission and construction checks for ordinary skirmish builds.
 * The fixtures use the engine's real admission/queue paths; raw IDs whose
 * native type mapping is absent from a profile are skipped. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static void start_race(int race)
{
    game_init();
    race_mode = 1;
    race_human = race;
    race_bots = 0;
    init_state();
    assert(screen == SCREEN_PLAY && race_mode && race_player[SIDE] == race);
    minerals = gas = 5000;
}

static const Sc64SkirmishRecord *build_record(int race, int raw)
{
    return sc64_skirmish_menu_record((unsigned)race, (unsigned)raw);
}

static int building_raw(int raw)
{
    return sc64_race_building_from_raw(raw);
}

static int unit_raw(int raw)
{
    return sc64_race_unit_from_raw(raw);
}

static int find_building_raw(int owner, int raw)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (building_complete(i) && b[i].owner == owner &&
            sc64_race_raw_building(b[i].type) == raw) return i;
    return -1;
}

static int find_worker_raw(int owner, int raw)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner &&
            sc64_race_raw_unit(u[i].type) == raw) return i;
    return -1;
}

static int fixture_building_raw(int owner, int raw, int x, int y)
{
    int type = building_raw(raw);
    if (type < 0) return -1;
    int slot = spawn_build(owner, type, x, y, 0);
    if (slot >= 0) b[slot].raw_type = raw;
    return slot;
}

static int menu_row_raw(int raw)
{
    for (int i = 0; i < field_menu_count(); ++i) {
        Sc64SkirmishMenuRow row;
        if (sc64_skirmish_menu_at((unsigned)race_human, (unsigned)i, &row) &&
            row.record && row.record->raw_id == raw) return i;
    }
    return -1;
}

/* Complete one ordinary worker-built structure through the engine's request
 * and arrival paths. Searching the map also respects real placement rules. */
static int build_with_worker(int raw, int worker)
{
    int type = building_raw(raw);
    if (type < 0 || worker < 0) return -1;
    for (int y = 96; y < WORLD_H - 64; y += 24) {
        for (int x = 96; x < WORLD_W - 64; x += 24) {
            int sx = x, sy = y;
            int old_minerals, old_gas;
            if (!race_build_request_valid(SIDE, worker, type, &sx, &sy, 0)) continue;
            u[worker].x = (short)(sx + 80); u[worker].y = (short)sy;
            old_minerals = minerals; old_gas = gas;
            assert(race_start_build(SIDE, worker, type, sx, sy));
            assert(minerals == old_minerals && gas == old_gas); /* travel order does not charge */
            u[worker].x = (short)sx; u[worker].y = (short)sy;
            race_pending_build_tick(worker);
            int slot = -1;
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (b[i].alive && b[i].owner == SIDE && b[i].type == type &&
                    b[i].under_construction) { slot = i; break; }
            if (slot < 0) return -1;
            {
                const Sc64RaceBuildingRule *rule = sc64_race_building_rule(type);
                assert(rule && minerals == old_minerals - rule->minerals &&
                       gas == old_gas - rule->gas);
            }
            for (int tick = 0; tick < building_build_time(type) + 8 &&
                 !building_complete(slot); ++tick) {
                update_orders();
                race_tick_buildings();
            }
            assert(building_complete(slot));
            return slot;
        }
    }
    return -1;
}

static void terran_construction_and_addon(void)
{
    int worker, barracks, factory, addon, second_factory, tank, minerals_before, gas_before;
    const Sc64SkirmishRecord *factory_row, *tank_row, *shop_row;
    start_race(R_TER);
    worker = find_worker_raw(SIDE, 7);
    assert(worker >= 0 && find_building_raw(SIDE, 106) >= 0);
    factory_row = build_record(R_TER, 113);
    tank_row = sc64_skirmish_product_at(113, 2);
    shop_row = build_record(R_TER, 120);
    if (building_raw(111) < 0 || building_raw(113) < 0 || building_raw(120) < 0 ||
        unit_raw(5) < 0 || !factory_row || !tank_row || !shop_row) return;

    assert(!race_skirmish_requirements(SIDE, factory_row));
    assert(menu_row_raw(113) >= 10);
    choose_command(menu_row_raw(113));
    assert(mode == MODE_NORMAL); /* Menu dispatch cannot bypass the Barracks. */
    assert(!race_skirmish_product_admitted(SIDE, find_building_raw(SIDE, 113), tank_row));
    barracks = build_with_worker(111, worker);
    assert(barracks >= 0 && sc64_race_raw_building(b[barracks].type) == 111);
    assert(race_skirmish_requirements(SIDE, factory_row));
    {
        int wrong_worker = spawn_unit(SIDE, U_PROBE, 700, 700);
        int x = 700, y = 700;
        assert(wrong_worker >= 0);
        assert(!race_build_request_valid(SIDE, wrong_worker, building_raw(113), &x, &y, 0));
        u[wrong_worker].alive = 0;
    }
    choose_command(menu_row_raw(113));
    assert(mode == MODE_BUILD && menu == building_raw(113));
    factory = build_with_worker(113, worker);
    assert(factory >= 0 && sc64_race_raw_building(b[factory].type) == 113);
    assert(!race_skirmish_product_admitted(SIDE, factory, tank_row));
    assert(!race_skirmish_attached(factory, 120, 1));

    selected_building = factory;
    minerals_before = minerals; gas_before = gas;
    assert(race_skirmish_choose_record(shop_row));
    addon = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE &&
            sc64_race_raw_building(b[i].type) == 120 &&
            b[i].x == b[factory].x + 48 && b[i].y == b[factory].y + 8) addon = i;
    assert(addon >= 0 && b[addon].under_construction);
    {
        const Sc64RaceBuildingRule *rule = sc64_race_building_rule(b[addon].type);
        assert(rule && minerals == minerals_before - rule->minerals && gas == gas_before - rule->gas);
    }
    for (int tick = 0; tick < building_build_time(b[addon].type) + 8 &&
         !building_complete(addon); ++tick) race_tick_buildings();
    assert(building_complete(addon));
    assert(race_skirmish_attached(factory, 120, 1));
    assert(race_skirmish_product_admitted(SIDE, factory, tank_row));

    second_factory = fixture_building_raw(SIDE, 113, b[factory].x + 160, b[factory].y + 80);
    assert(second_factory >= 0);
    assert(!race_skirmish_product_admitted(SIDE, second_factory, tank_row));
    minerals_before = minerals; gas_before = gas;
    assert(!race_queue_unit(SIDE, second_factory, unit_raw(5)));
    assert(minerals == minerals_before && gas == gas_before && !b[second_factory].queue_count);
    minerals = gas = 5000;
    minerals_before = minerals; gas_before = gas;
    assert(race_queue_unit(SIDE, factory, unit_raw(5)));
    assert(minerals == minerals_before - 150 && gas == gas_before - 100);
    assert(b[factory].queue_count == 1 && b[factory].queue[b[factory].queue_head] == unit_raw(5));
    assert(sc64_skirmish_menu_next_product(113, SC64_SK_NONE,
        race_skirmish_product_available, &factory) == 2);
    assert(sc64_skirmish_menu_next_product(113, 2,
        race_skirmish_product_available, &factory) == 5);
    for (int tick = 0; tick < unit_build_time(unit_raw(5)) + 8 && b[factory].queue_count;
         ++tick) race_tick_buildings();
    assert(!b[factory].queue_count);
    {
        int tank_found = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && sc64_race_raw_unit(u[i].type) == 5)
                tank_found = 1;
        assert(tank_found);
    }
}

static void protoss_admission(void)
{
    int pylon, gateway, core, stargate, probe, old_minerals, old_gas, scouts_before;
    const Sc64SkirmishRecord *row;
    start_race(R_PROTOSS);
    row = build_record(R_PROTOSS, 167);
    if (building_raw(167) < 0 || !row) return;
    assert(find_building_raw(SIDE, 154) >= 0); /* Nexus from ordinary start. */
    assert(!race_skirmish_requirements(SIDE, row));
    gateway = fixture_building_raw(SIDE, 160, 300, 300);
    core = fixture_building_raw(SIDE, 164, 360, 300);
    assert(gateway >= 0 && core >= 0);
    assert(race_skirmish_requirements(SIDE, row));
    /* The prerequisites alone are insufficient: the Stargate site needs psi. */
    assert(!race_site_powered(SIDE, 500, 500));
    pylon = fixture_building_raw(SIDE, 156, 500, 500);
    assert(pylon >= 0 && race_site_powered(SIDE, 500, 500));
    stargate = building_raw(167);
    assert(stargate >= 0 && race_can_site(SIDE, stargate, 500, 500));
    if (unit_raw(70) < 0) return;
    probe = find_worker_raw(SIDE, 64);
    assert(probe >= 0);
    /* Freeze mining during the long warp-in so the exact debit stays visible. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_race_raw_unit(u[i].type) == 64)
            u[i].order = ORDER_IDLE;
    old_minerals = minerals; old_gas = gas;
    stargate = build_with_worker(167, probe);
    assert(stargate >= 0 && building_complete(stargate));
    assert(minerals == old_minerals - 150 && gas == old_gas - 150);
    scouts_before = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && sc64_race_raw_unit(u[i].type) == 70)
            ++scouts_before;
    old_minerals = minerals; old_gas = gas;
    assert(race_queue_unit(SIDE, stargate, unit_raw(70)));
    assert(minerals == old_minerals - 275 && gas == old_gas - 125);
    for (int tick = 0; tick < unit_build_time(unit_raw(70)) + 8 && b[stargate].queue_count;
         ++tick) race_tick_buildings();
    assert(!b[stargate].queue_count);
    {
        int scouts_after = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && sc64_race_raw_unit(u[i].type) == 70)
                ++scouts_after;
        assert(scouts_after == scouts_before + 1);
    }
}

static void zerg_morph_chain(void)
{
    int hatch, pool, nest, lair, hive, spire, colony, drone, drones_before;
    const Sc64SkirmishRecord *lair_row, *hive_row, *greater_row, *sunken_row;
    start_race(R_ZERG);
    hatch = find_building_raw(SIDE, 131);
    if (hatch < 0 || building_raw(132) < 0 || building_raw(133) < 0 ||
        building_raw(137) < 0 || building_raw(146) < 0) return;
    pool = fixture_building_raw(SIDE, 142, 300, 300);
    nest = fixture_building_raw(SIDE, 138, 340, 300);
    assert(pool >= 0 && nest >= 0);
    lair_row = build_record(R_ZERG, 132);
    selected_building = hatch;
    assert(lair_row && race_skirmish_record_available(lair_row));
    {
        const Sc64RaceBuildingRule *rule = sc64_race_building_rule(building_raw(132));
        int old_minerals = minerals, old_gas = gas;
        assert(rule && race_skirmish_choose_record(lair_row));
        assert(minerals == old_minerals - rule->minerals && gas == old_gas - rule->gas);
    }
    assert(b[hatch].type == building_raw(132) && b[hatch].under_construction);
    assert(race_skirmish_has_raw(SIDE, 131) && !race_skirmish_has_raw(SIDE, 132));
    assert(race_owner_supply_cap(SIDE) >= 2);
    assert(race_site_creep(SIDE, b[hatch].x + 48, b[hatch].y));

    /* Morphing halls continue to regenerate and spend their own larva. The
     * queued Drone must finish before the longer Lair morph and completion
     * must preserve the remaining larva instead of restoring a full batch. */
    if (unit_raw(41) >= 0) {
        const Sc64RaceUnitRule *drone_rule = sc64_race_unit_rule(unit_raw(41));
        int old_minerals = minerals, old_gas = gas;
        drones_before = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && sc64_race_raw_unit(u[i].type) == 41)
                ++drones_before;
        b[hatch].larva = 1; b[hatch].larva_timer = 427;
        race_tick_state();
        assert(b[hatch].under_construction && b[hatch].larva == 2 &&
               b[hatch].larva_timer == 0);
        assert(drone_rule && race_queue_unit(SIDE, hatch, unit_raw(41)));
        assert(b[hatch].larva == 1 && b[hatch].queue_count == 1);
        assert(minerals == old_minerals - drone_rule->minerals &&
               gas == old_gas - drone_rule->gas);
        for (int tick = 0; tick < unit_build_time(unit_raw(41)) + 8; ++tick) {
            int count = 0;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == SIDE && sc64_race_raw_unit(u[i].type) == 41)
                    ++count;
            if (count > drones_before) break;
            race_tick_buildings();
        }
        {
            int count = 0;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == SIDE && sc64_race_raw_unit(u[i].type) == 41)
                    ++count;
            assert(count == drones_before + 1);
        }
        assert(b[hatch].under_construction && b[hatch].larva == 1);
    }
    for (int tick = 0; tick < building_build_time(b[hatch].type) + 8 &&
         !building_complete(hatch); ++tick) race_tick_buildings();
    assert(building_complete(hatch));
    assert(b[hatch].larva == 1);

    hive_row = build_record(R_ZERG, 133);
    assert(hive_row && race_skirmish_requirements(SIDE, hive_row));
    selected_building = hatch;
    assert(race_skirmish_record_available(hive_row));
    {
        const Sc64RaceBuildingRule *rule = sc64_race_building_rule(building_raw(133));
        int old_minerals = minerals, old_gas = gas;
        assert(rule && race_skirmish_choose_record(hive_row));
        assert(minerals == old_minerals - rule->minerals && gas == old_gas - rule->gas);
    }
    assert(b[hatch].type == building_raw(133) && b[hatch].under_construction);
    for (int tick = 0; tick < building_build_time(b[hatch].type) + 8 &&
         !building_complete(hatch); ++tick) race_tick_buildings();
    assert(building_complete(hatch));

    spire = fixture_building_raw(SIDE, 141, 400, 400);
    assert(spire >= 0);
    greater_row = build_record(R_ZERG, 137);
    selected_building = spire;
    assert(greater_row && race_skirmish_record_available(greater_row));
    assert(race_skirmish_choose_record(greater_row));
    assert(b[spire].type == building_raw(137) && b[spire].under_construction);
    for (int tick = 0; tick < building_build_time(b[spire].type) + 8 &&
         !building_complete(spire); ++tick) race_tick_buildings();
    assert(building_complete(spire));

    colony = fixture_building_raw(SIDE, 143, 460, 400);
    sunken_row = build_record(R_ZERG, 146);
    if (colony >= 0 && sunken_row) {
        selected_building = colony;
        assert(race_skirmish_record_available(sunken_row));
        assert(race_skirmish_choose_record(sunken_row));
        assert(b[colony].type == building_raw(146) && b[colony].under_construction);
        for (int tick = 0; tick < building_build_time(b[colony].type) + 8 &&
             !building_complete(colony); ++tick) race_tick_buildings();
        assert(building_complete(colony));
    }
    /* Lair/Hive/Greater Spire and colony morphs stay gated on profiles that
     * do not map those ordinary building IDs. */
    lair = building_raw(132); hive = building_raw(133);
    assert(lair >= 0 && hive >= 0);
}

static void busy_selected_worker(void)
{
    start_race(R_TER);
    int first = find_worker_raw(SIDE, 7);
    assert(first >= 0);
    for (int i=0; i<MAX_UNITS; ++i)
        if (i != first && u[i].alive && u[i].owner == SIDE && sc64_race_is_worker(u[i].type)) u[i].alive = 0;
    int second = spawn_unit(SIDE, U_SCV, u[first].x + 32, u[first].y);
    assert(second >= 0);
    u[first].selected = 1; u[second].selected = 0;
    int x=-1,y=-1;
    for (int yy=96; yy<WORLD_H-64 && x<0; yy+=24)
        for (int xx=96; xx<WORLD_W-64; xx+=24) {
            int sx=xx,sy=yy;
            if (race_build_request_valid(SIDE,first,building_raw(111),&sx,&sy,0)) { x=sx;y=sy;break; }
        }
    assert(x>=0 && race_start_build(SIDE,first,building_raw(111),x,y));
    assert(u[first].order==ORDER_BUILD_PENDING);
    assert(race_skirmish_worker(building_raw(109))==second);
    u[first].x=(short)x; u[first].y=(short)y; race_pending_build_tick(first);
    assert(u[first].order==ORDER_BUILD);
    int foundation=u[first].target;
    assert(foundation>=0 && foundation<MAX_BUILDINGS && b[foundation].under_construction);
    assert(race_skirmish_worker(building_raw(109))==second);
    int old_order=u[first].order,old_target=u[first].target;
    u[second].order=ORDER_BUILD_PENDING;
    assert(race_skirmish_worker(building_raw(109))==-1);
    assert(u[first].order==old_order && u[first].target==old_target);
    u[second].order=ORDER_IDLE;
    int sx=-1, sy=-1;
    for (int yy=96; yy<WORLD_H-64 && sx<0; yy+=24)
        for (int xx=96; xx<WORLD_W-64; xx+=24) {
            int px=xx,py=yy;
            if (race_build_request_valid(SIDE,second,building_raw(109),&px,&py,0)) { sx=px;sy=py;break; }
        }
    assert(sx>=0 && race_start_build(SIDE,second,building_raw(109),sx,sy));
    u[second].x=(short)sx; u[second].y=(short)sy; race_pending_build_tick(second);
    assert(u[second].order==ORDER_BUILD);
    int second_foundation=u[second].target;
    assert(second_foundation>=0 && second_foundation!=foundation);
    for (int t=0;t<16;t++) { update_orders(); race_tick_buildings(); }
    assert(b[foundation].build_progress>0 && b[second_foundation].build_progress>0);
}

static void reaver_service_unavailable(void)
{
    start_race(R_PROTOSS);
    const Sc64SkirmishRecord *reaver = NULL;
    for (unsigned i=0; i<sc64_skirmish_product_count(155); ++i) {
        const Sc64SkirmishRecord *r=sc64_skirmish_product_at(155,i);
        if (r->raw_id==83) reaver=r;
    }
    assert(reaver && unit_raw(83)>=0 && building_raw(155)>=0 && building_raw(171)>=0);
    int factory=fixture_building_raw(SIDE,155,500,500);
    assert(factory>=0 && fixture_building_raw(SIDE,171,560,500)>=0);
    assert(fixture_building_raw(SIDE,156,500,440)>=0);
    assert(race_skirmish_requirements(SIDE,reaver));
    int old_minerals=minerals,old_gas=gas;
    assert(!race_skirmish_product_admitted(SIDE,factory,reaver));
    assert(!race_queue_unit(SIDE,factory,unit_raw(83)));
    assert(minerals==old_minerals && gas==old_gas && !b[factory].queue_count);
    assert(sc64_skirmish_menu_next_product(155,SC64_SK_NONE,
        race_skirmish_product_available,&factory)!=83);
}

int main(void)
{
    reaver_service_unavailable();
    busy_selected_worker();
    terran_construction_and_addon();
    protoss_admission();
    zerg_morph_chain();
    puts("sc64 skirmish runtime: ok");
    return 0;
}
