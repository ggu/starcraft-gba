#ifndef SC64_RACE_CAMPAIGN_ADAPTER_H
#define SC64_RACE_CAMPAIGN_ADAPTER_H

/* Mission-specific port rules for the first two Zerg and Protoss maps,
 * plus the existing Terran rules when SC64_UNIFIED is enabled. Source map/trigger evidence is the user-supplied verified
 * ROM, extracted with references/sc64-maps/extract_sc64_maps.py @
 * 09b57f787a247107d1de82b8c5eab2734667ea1d. Unit names use the pinned BWAPI
 * UnitType.h @ d727fed68558c506163048ea889131d8cbb33915.
 *
 * This is an authored subset of those maps' triggers, not a CHK interpreter:
 * it keeps the controller-facing mission goals (ordered Z1 tech, Z2 Chrysalis
 * escort, P1 Fenix rescue/survival plus Zerg-base objective, P2 timed Fenix
 * arrival/survival and Cerebrate target) and omits tutorial dialogue/AI scripts.
 */

enum {
    SC64_RACE_MISSION_Z1,
    SC64_RACE_MISSION_Z2,
    SC64_RACE_MISSION_P1,
    SC64_RACE_MISSION_P2
#ifdef SC64_UNIFIED
    ,SC64_RACE_MISSION_T1, SC64_RACE_MISSION_T2
#ifdef SC64_T3_ENABLED
    ,SC64_RACE_MISSION_T3
#endif
#endif
};

static int
race_campaign_owner(const Sc64RaceMap *map, int raw_owner)
{
    if (raw_owner == 11 || raw_owner >= 8) return NEUTRAL;
    if (raw_owner < 0) return NEUTRAL;
    return map->owner_modes[raw_owner];
}

static int
race_campaign_count_buildings(int owner)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && !b[i].under_construction) ++n;
    return n;
}

static int
race_campaign_has_building(int owner, int type)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type &&
            !b[i].under_construction) return 1;
    return 0;
}

static int
race_campaign_find_unit(int type)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == type) return i;
    return -1;
}

static int
race_campaign_unit_alive(int slot)
{
    return slot >= 0 && slot < MAX_UNITS && u[slot].alive;
}

static void
race_campaign_set_message(int id)
{
    if (race_campaign.message == id) return;
    race_campaign.message = (unsigned char)id;
    race_campaign.message_ticks = 180;
    toast(0);
}

#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
#include "sc64_t3_adapter.h"
#endif

static void
race_campaign_assign_mining(void)
{
    int i, j;
    for (i = 0; i < MAX_UNITS; ++i) {
        int best = -1, best_d = 0x7fffffff;
        if (!u[i].alive || u[i].owner != ENEMY || !sc64_race_is_worker(u[i].type) ||
            u[i].order != ORDER_IDLE) continue;
        for (j = 0; j < MAX_MINERALS; ++j) if (m[j].amount > 0) {
            int d = dist2(u[i].x, u[i].y, m[j].x, m[j].y);
            if (d < best_d) { best = j; best_d = d; }
        }
        if (best >= 0) {
            u[i].order = ORDER_MINE;
            u[i].target = best;
        }
    }
}

static int
race_campaign_build_site(int owner, int type, int base_x, int base_y, int *x, int *y)
{
    static const int dx[8] = { 48, -48, 0, 0, 48, -48, 48, -48 };
    static const int dy[8] = { 0, 0, 48, -48, 48, 48, -48, -48 };
    int i;
    for (i = 0; i < 8; ++i) {
        int px = limit(base_x + dx[i], 24, WORLD_W - 24);
        int py = limit(base_y + dy[i], 24, WORLD_H - 24);
        if (race_can_site(owner, type, px, py) && can_place(type, px, py)) {
            *x = px; *y = py; return 1;
        }
    }
    return 0;
}

static void
race_campaign_ai_tick(void)
{
    int i, workers = 0, army = 0, producer = -1;
    int enemy_race = race_player[ENEMY], worker_type, producer_type;
    int next_build = -1, train_type = -1, x = 0, y = 0, base_x = -1, base_y = -1;
    if (race_campaign.elapsed_frames % 60u) return;

    race_campaign_assign_mining();
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        if (sc64_race_is_worker(u[i].type)) {
            ++workers;
            if (base_x < 0) { base_x = u[i].x; base_y = u[i].y; }
        } else ++army;
    }
    for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) && b[i].owner == ENEMY) {
        if (base_x < 0) { base_x = b[i].x; base_y = b[i].y; }
        if (enemy_race == R_ZERG && b[i].type == B_HATCHERY) producer = i;
        else if (enemy_race == R_PROTOSS && b[i].type == B_NEXUS) producer = i;
        else if (enemy_race == R_TER && b[i].type == B_COMMAND) producer = i;
    }
    if (base_x < 0) { base_x = race_cols * 8; base_y = race_rows * 8; }
    worker_type = enemy_race == R_ZERG ? U_DRONE :
                  enemy_race == R_PROTOSS ? U_PROBE : U_SCV;
    train_type = enemy_race == R_ZERG ? U_ZERGLING :
                 enemy_race == R_PROTOSS ? U_ZEALOT : U_MARINE;
    producer_type = enemy_race == R_ZERG ? B_HATCHERY :
                     enemy_race == R_PROTOSS ? B_NEXUS : B_COMMAND;
    if (producer >= 0 && workers < 7)
        (void)race_queue_unit(ENEMY, producer, worker_type);

    if (enemy_race == R_ZERG) {
        if (!race_owner_has_type(ENEMY, B_SPAWNING_POOL)) next_build = B_SPAWNING_POOL;
        else if (!race_owner_has_type(ENEMY, B_HYDRALISK_DEN)) next_build = B_HYDRALISK_DEN;
    } else if (enemy_race == R_PROTOSS) {
        if (!race_owner_has_type(ENEMY, B_PYLON)) next_build = B_PYLON;
        else if (!race_owner_has_type(ENEMY, B_GATEWAY)) next_build = B_GATEWAY;
    } else if (!race_owner_has_type(ENEMY, B_BARRACKS)) {
        next_build = B_BARRACKS;
    }
    if (next_build >= 0 && race_campaign_build_site(ENEMY, next_build, base_x, base_y, &x, &y)) {
        int builder = race_owner_worker(ENEMY, worker_type, x, y);
        if (builder >= 0 && race_start_build(ENEMY, builder, next_build, x, y)) return;
    }

    if (producer >= 0 && b[producer].queue_count < 2)
        (void)race_queue_unit(ENEMY, producer, train_type);
    /* Imported enemy squads start as map guards. Release only a small group
     * on a long cadence so the original bases do not turn into an immediate
     * map-wide rush. This is a deliberately small authored AI, not SC's full
     * trigger/AI-script interpreter. */
    if (army >= 4 && race_campaign.elapsed_frames % (180u * 30u) == 0) {
        int target_x = cx, target_y = cy, sent = 0;
        for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
            target_x = u[i].x; target_y = u[i].y; break;
        }
        for (i = 0; i < MAX_UNITS && sent < 2; ++i) if (u[i].alive &&
            u[i].owner == ENEMY && u[i].order == ORDER_IDLE &&
            !sc64_race_is_worker(u[i].type) && u[i].type != U_FENIX &&
            u[i].type != U_CHRYSALIS) {
            u[i].order = ORDER_ATTACK_MOVE;
            u[i].tx = target_x; u[i].ty = target_y;
            ++sent;
            ++race_campaign.enemy_units_sent;
        }
        if (sent) ++race_campaign.enemy_wave;
    }
    (void)producer_type;
}

static void
race_campaign_init(void)
{
    const Sc64RaceMap *map;
    int i, player_x = 0, player_y = 0, player_n = 0;
    int human_owner;

#ifdef SC64_UNIFIED
    memset(&backwater,0,sizeof(backwater));
    memset(invincible_units,0,sizeof(invincible_units));memset(invincible_buildings,0,sizeof(invincible_buildings));
    backwater_message_trigger=backwater_message_action=backwater_message_ticks=0;
#endif
    if (race_mode) {
        race_skirmish_init();
        return;
    }
    race_selected_mission = limit(race_selected_mission, 0, SC64_RACE_MAP_COUNT - 1);
    map = &sc64_race_maps[race_selected_mission];
    race_cols = map->width;
    race_rows = map->height;
    race_human = map->owner_races[map->human_slot];
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(&race_campaign, 0, sizeof(race_campaign));
    memset(race_player, 0, sizeof(race_player));
    memset(race_minerals, 0, sizeof(race_minerals));
    memset(race_gas, 0, sizeof(race_gas));
    memset(explored, 0, sizeof(explored));
    memset(visible, 0, sizeof(visible));
    memset(tracers, 0, sizeof(tracers));
    reset_selection_gestures();
    geyser_count = 0;
    supply_used = 0;
    supply_cap = 0;
    frame = 0;
    mode = MODE_NORMAL;
    menu = 0; menu_open = 0; menu_cursor = 0; help = 0;
    won = lost = 0;
    result_timer = 0;
    toast_timer = toast_id = 0;
    enemy_timer = 0;
    wave = 0;
    weapons_level = armor_level = 0;
    weapons_research = armor_research = 0;
    selected_building = -1;
    train_type = race_human == R_ZERG ? U_DRONE :
                 race_human == R_PROTOSS ? U_PROBE : U_SCV;
    seed = 0x51a7c0deu;
    screen = SCREEN_PLAY;
    race_campaign.mission = (unsigned char)race_selected_mission;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_campaign.rescue_unit = 0xffffu;
    race_campaign.chrysalis_unit = 0xffffu;
    race_campaign.carrier_unit = 0xffffu;
    race_campaign.message = 0;
    minerals = gas = 0;
    invalidate_path_grid();
    sc64_visibility_reset();

    for (i = 0; i < 8; ++i) {
        int owner = map->owner_modes[i];
        if (owner >= 0 && owner < 5 && map->owner_races[i] < 3)
            race_player[owner] = map->owner_races[i];
    }
    human_owner = map->human_slot;

    for (i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *src = &map->units[i];
        int raw = src->source_type, owner, type, slot;
        int x = src->x_px, y = src->y_px;
        if (x >= map->width * 16 || y >= map->height * 16) continue;
        if (raw == 176 || raw == 177 || raw == 178) {
            int j;
            for (j = 0; j < MAX_MINERALS; ++j) if (m[j].amount <= 0) break;
            if (j < MAX_MINERALS) {
                m[j].x = x; m[j].y = y;
                m[j].amount = src->resources ? src->resources : 1500;
                m[j].style = raw;
            }
            continue;
        }
        if (raw == 188) {
            if (geyser_count < 4) {
                geysers[geyser_count].x = x;
                geysers[geyser_count++].y = y;
            }
            continue;
        }
        owner = race_campaign_owner(map, src->owner);
        if (src->owner == human_owner) owner = SIDE;
#ifdef SC64_UNIFIED
        /* Terran Raynor starts neutral and joins on contact, as in the existing adapter. */
        if(race_selected_mission>=SC64_RACE_MISSION_T1&&raw==19)owner=NEUTRAL;
#endif
        type = sc64_race_unit_from_raw(raw);
        if (type >= 0) {
            slot = spawn_unit(owner, type, x, y);
            if (slot < 0) continue;
            u[slot].raw_type = raw;
            u[slot].raw_owner = src->owner;
            u[slot].flags = src->flags;
            if (src->hp_percent && src->hp_percent <= 100)
                u[slot].hp = u[slot].maxhp * src->hp_percent / 100;
            if (type == U_FENIX) race_campaign.rescue_unit = (unsigned short)slot;
#ifdef SC64_UNIFIED
            if(type==U_RAYNOR&&race_selected_mission>=SC64_RACE_MISSION_T1)race_campaign.rescue_unit=(unsigned short)slot;
#endif
            if (type == U_CHRYSALIS) race_campaign.chrysalis_unit = (unsigned short)slot;
            if (owner == SIDE) {
                player_x += x; player_y += y; ++player_n;
            }
            continue;
        }
        type = sc64_race_building_from_raw(raw);
        if (type >= 0) {
            slot = spawn_build(owner, type, x, y, 0);
            if (slot < 0) continue;
            b[slot].raw_type = raw;
            b[slot].raw_owner = src->owner;
            b[slot].flags = src->flags;
            if (src->hp_percent && src->hp_percent <= 100)
                b[slot].hp = b[slot].maxhp * src->hp_percent / 100;
            if (type == B_PYLON || type == B_NEXUS || type == B_COMMAND)
                supply_cap += type == B_NEXUS ? 18 : (type == B_COMMAND ? 20 : 16);
        }
#ifdef SC64_UNIFIED
        else if(race_selected_mission>=SC64_RACE_MISSION_T1)++mission_unsupported_count;
#endif
    }

    switch (race_selected_mission) {
    case SC64_RACE_MISSION_Z1: minerals = 200; break;
    case SC64_RACE_MISSION_Z2: minerals = 300; break;
    case SC64_RACE_MISSION_P1: minerals = 400; gas = 100; break;
#ifdef SC64_UNIFIED
    case SC64_RACE_MISSION_T1: minerals=0;break;
    case SC64_RACE_MISSION_T2: backwater_init();break;
#ifdef SC64_T3_ENABLED
    case SC64_RACE_MISSION_T3: minerals=100;gas=0;break;
#endif
#endif
    default: minerals = 170; break;
    }
    race_minerals[SIDE] = minerals;
    race_gas[SIDE] = gas;
    if (player_n) { cx = player_x / player_n; cy = player_y / player_n; }
    else { cx = map->width * 8; cy = map->height * 8; }
    cx = limit(cx, 120, map->width * 16 - 120);
    cy = limit(cy, 54, map->height * 16 - 54);
    camx = camy = 0;
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    if (race_selected_mission == SC64_RACE_MISSION_T3) {
        /* T3 TRIG 0 centers on MRGN 14 (the damaged Vulture area), at half
         * the CHK pixel coordinates used by imported map records. */
        cx = 544;
        cy = 1384;
        race_t3_init();
    }
#endif
    race_campaign_assign_mining();
    if (race_selected_mission == SC64_RACE_MISSION_Z1)
        race_campaign_set_message(1);
    else if (race_selected_mission == SC64_RACE_MISSION_Z2)
        race_campaign_set_message(3);
    else if (race_selected_mission == SC64_RACE_MISSION_P1)
        race_campaign_set_message(4);
    else if(race_selected_mission==SC64_RACE_MISSION_P2)
        race_campaign_set_message(5);
#ifdef SC64_UNIFIED
    else if (race_selected_mission==SC64_RACE_MISSION_T1) race_campaign_set_message(10);
    else if (race_selected_mission==SC64_RACE_MISSION_T2) race_campaign_set_message(11);
#ifdef SC64_T3_ENABLED
    else race_campaign_set_message(14);
#endif
#endif
    update_exploration();
}

static void
race_campaign_spawn_p2_reinforcements(void)
{
    int i, type, slot;
    /* P2 TRIG record 5: CountdownTimer(900), MRGN 1/3/4/5. */
    for (i = 0; i < 5; ++i) {
        type = sc64_race_unit_from_raw(66);
        slot = spawn_unit(SIDE, type, 1400 + (i % 2) * 8, 64 + (i / 2) * 8);
        if (slot >= 0) { u[slot].raw_type = 66; u[slot].raw_owner = 13; }
    }
    for (i = 0; i < 5; ++i) {
        type = sc64_race_unit_from_raw(65);
        slot = spawn_unit(SIDE, type, 1344 + (i % 2) * 8, 96 + (i / 2) * 8);
        if (slot >= 0) { u[slot].raw_type = 65; u[slot].raw_owner = 13; }
    }
    for (i = 0; i < 3; ++i) {
        type = sc64_race_unit_from_raw(83);
        slot = spawn_unit(SIDE, type, 1400 + (i % 2) * 8, 128 + (i / 2) * 8);
        if (slot >= 0) { u[slot].raw_type = 83; u[slot].raw_owner = 13; }
    }
    type = sc64_race_unit_from_raw(77);
    slot = spawn_unit(RESCUABLE, type, 1312, 96);
    if (slot >= 0) {
        u[slot].raw_type = 77; u[slot].raw_owner = 2;
        race_campaign.rescue_unit = (unsigned short)slot;
    }
    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_READY;
    race_campaign.phase = SC64_RACE_PHASE_REINFORCEMENTS;
    race_campaign_set_message(6);
}

static int
race_campaign_enemy_buildings_remain(void)
{
    return race_campaign_count_buildings(ENEMY) != 0;
}

#ifdef SC64_UNIFIED
/* Reuse the already-ported Terran scenario rules rather than substituting
 * the Zerg/Protoss mission AI. T2's callbacks retain its CHK region/timer rules. */
static void race_terran_tick(void)
{
    if(race_selected_mission==SC64_RACE_MISSION_T2){backwater_update();}
    else {
        int i,j,raynor_alive=0,raynor_found=0,marines=0,barracks=0,command=0,men=0;
        if(!mission_objective_stage&&frame>=30){minerals=40;race_minerals[SIDE]=40;mission_objective_stage=1;race_campaign_set_message(12);}
        for(i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].type==U_RAYNOR&&u[i].owner==NEUTRAL)
            for(j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&dist2(u[i].x,u[i].y,u[j].x,u[j].y)<=20*20){
                u[i].owner=SIDE;u[i].selected=0;race_campaign_set_message(13);break;}
        for(i=0;i<MAX_UNITS;i++)if(u[i].alive){
            if(u[i].owner==SIDE)++men;
            if(u[i].type==U_RAYNOR){raynor_alive=1;raynor_found=u[i].owner==SIDE;}
            else if(u[i].owner==SIDE&&u[i].type==U_MARINE)++marines;}
        for(i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE){
            if(b[i].type==B_BARRACKS)++barracks;else if(b[i].type==B_COMMAND)++command;}
        if(!raynor_alive||!men||(!command&&!barracks&&marines<=9&&minerals<=399))lost=1;
        if(raynor_found&&marines>=10&&barracks>=1)won=1;
    }
    if(lost){won=0;race_campaign.phase=SC64_RACE_PHASE_FAILED;}
    else if(won){race_campaign.phase=SC64_RACE_PHASE_COMPLETE;race_campaign.objective_flags|=SC64_RACE_OBJECTIVE_PRIMARY;race_completed|=1<<race_selected_mission;}
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
}
#endif

static void
race_campaign_tick(void)
{
    int i, j;
    const int mission = race_campaign.mission;
    if (race_campaign.phase == SC64_RACE_PHASE_COMPLETE ||
        race_campaign.phase == SC64_RACE_PHASE_FAILED) return;
    if (race_campaign.message_ticks) --race_campaign.message_ticks;
    ++race_campaign.elapsed_frames;
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    if (mission == SC64_RACE_MISSION_T3) {
        race_t3_campaign_tick();
        return;
    }
#endif
#ifdef SC64_UNIFIED
    if(mission>=SC64_RACE_MISSION_T1){race_terran_tick();return;}
#endif
    race_campaign_ai_tick();

    if (mission == SC64_RACE_MISSION_Z1) {
        int chrysalis = race_campaign_find_unit(U_CHRYSALIS);
        int pool = race_campaign_has_building(SIDE, B_SPAWNING_POOL);
        int den = race_campaign_has_building(SIDE, B_HYDRALISK_DEN);
        if (chrysalis < 0) {
            race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_CHRYSALIS_LOST;
            race_campaign.phase = SC64_RACE_PHASE_FAILED;
        } else if (pool && den) {
            race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
            if (race_campaign.objective_stage == 0) {
                race_campaign.objective_stage = 1;
                race_campaign_set_message(2);
            }
            if (!race_campaign_enemy_buildings_remain()) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_SECONDARY;
                race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
            }
        }
    } else if (mission == SC64_RACE_MISSION_Z2) {
        int c = race_campaign.chrysalis_unit;
        /* The source OWNR marks six Hunter Killers as Rescue Passive
         * (raw owner 0). Once a human unit reaches that group, take control
         * of the surviving special units for the original escort objective. */
        {
            int contact = 0, joined = 0;
            for (i = 0; i < MAX_UNITS && !contact; ++i) if (u[i].alive &&
                u[i].owner == RESCUABLE && u[i].raw_type == 53)
                for (j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
                    dist2(u[j].x,u[j].y,u[i].x,u[i].y) <= 28*28) { contact = 1; break; }
            if (contact) for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
                u[i].owner == RESCUABLE && u[i].raw_type == 53) {
                u[i].owner = SIDE; joined = 1;
            }
            if (joined) race_campaign_set_message(9);
        }
        if (!race_campaign_unit_alive(c)) {
            race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_CHRYSALIS_LOST;
            race_campaign.phase = SC64_RACE_PHASE_FAILED;
        } else {
            Unit *q = &u[c];
            if (race_campaign.carrier_unit < MAX_UNITS) {
                int carrier = race_campaign.carrier_unit;
                if (!u[carrier].alive || u[carrier].owner != SIDE || u[carrier].type != U_DRONE) {
                    race_campaign.carrier_unit = 0xffffu;
                    q->owner = NEUTRAL;
                    race_campaign.objective_flags &= ~SC64_RACE_OBJECTIVE_CHRYSALIS_RECOVERED;
                } else {
                    q->x = u[carrier].x;
                    q->y = u[carrier].y;
                }
            }
            if (q->owner != SIDE) {
                /* Z2's powerup is retrieved by a drone. It remains a mission
                 * object while carried and follows that controller-movable
                 * unit to the Warp Point. */
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                    u[i].type == U_DRONE && dist2(u[i].x, u[i].y, q->x, q->y) <= 16 * 16) {
                    q->owner = SIDE;
                    race_campaign.carrier_unit = (unsigned short)i;
                    q->selected = 0;
                    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_CHRYSALIS_RECOVERED;
                    race_campaign_set_message(7);
                    break;
                }
            }
            /* Source MRGN 1 in 008/015 is "Warp Point": 1184..1232 by
             * 1200..1232 source px, converted to the GBA's 16px scale. */
            if (q->owner == SIDE && q->x >= 1184 && q->x <= 1232 &&
                q->y >= 1200 && q->y <= 1232) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
                race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
            }
        }
    } else if (mission == SC64_RACE_MISSION_P1) {
        int fenix = race_campaign.rescue_unit;
        if (!race_campaign_unit_alive(fenix)) {
            race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_LOST;
            race_campaign.phase = SC64_RACE_PHASE_FAILED;
        } else {
            Unit *f = &u[fenix];
            /* P1 source OWNR marks the Antioch force as Rescue Passive.
             * MRGN 6 "Protoss Base" is [448,1696,1568,2400] source pixels;
             * coordinates here use the SC64 half-scale. Entering the region
             * brings its surviving rescuable units and buildings under the
             * human side, matching the original passive-rescue behavior. */
            for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                u[i].x >= 224 && u[i].x <= 784 && u[i].y >= 848 && u[i].y <= 1200) break;
            if (i < MAX_UNITS) {
                int joined = 0;
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE &&
                    u[i].x >= 224 && u[i].x <= 784 && u[i].y >= 848 && u[i].y <= 1200) {
                    if ((int)i == fenix) race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_MET;
                    u[i].owner = SIDE; joined = 1;
                }
                for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == RESCUABLE &&
                    b[i].x >= 224 && b[i].x <= 784 && b[i].y >= 848 && b[i].y <= 1200) {
                    b[i].owner = SIDE; joined = 1;
                }
                if (joined) race_campaign_set_message(8);
            }
            if (f->owner == RESCUABLE) {
                /* Source 008/01E MRGN 1 is "rescue Fenix". */
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                    dist2(u[i].x, u[i].y, f->x, f->y) <= 28 * 28) {
                    f->owner = SIDE;
                    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_MET;
                    race_campaign_set_message(4);
                    break;
                }
            }
            if (f->owner == SIDE && !race_campaign_enemy_buildings_remain()) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY |
                                                  SC64_RACE_OBJECTIVE_SECONDARY;
                race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
            }
        }
    } else if (mission == SC64_RACE_MISSION_P2) {
        int fenix;
        if (race_campaign.elapsed_frames >= 900u * 30u &&
            !(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY))
            race_campaign_spawn_p2_reinforcements();
        fenix = race_campaign.rescue_unit;
        if (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY) {
            if (!race_campaign_unit_alive(fenix)) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_LOST;
                race_campaign.phase = SC64_RACE_PHASE_FAILED;
            } else if (u[fenix].owner == RESCUABLE) {
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                    dist2(u[i].x, u[i].y, u[fenix].x, u[fenix].y) <= 28 * 28) {
                    u[fenix].owner = SIDE;
                    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_MET;
                    break;
                }
            }
            for (i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == ENEMY && u[i].raw_type == 151) break;
            for (j = 0; j < MAX_BUILDINGS; ++j)
                if (b[j].alive && b[j].owner == ENEMY && b[j].raw_type == 151) break;
            if (i == MAX_UNITS && j == MAX_BUILDINGS) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_CEREBRATE_DEAD;
                race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
            }
        }
    }
    if (race_campaign.phase == SC64_RACE_PHASE_COMPLETE) {
        race_completed |= 1 << race_campaign.mission;
        won = 1;
    } else if (race_campaign.phase == SC64_RACE_PHASE_FAILED) {
        lost = 1;
    }
}

static int race_campaign_won(void)
{ return race_campaign.phase == SC64_RACE_PHASE_COMPLETE; }

static int race_campaign_lost(void)
{ return race_campaign.phase == SC64_RACE_PHASE_FAILED; }

static const char *
race_campaign_objective_text(void)
{
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    if (race_campaign.mission == SC64_RACE_MISSION_T3)
        return race_t3_objective_text();
#endif
    static const char *const objectives[] = {
        "Build a Spawning Pool and Hydralisk Den; protect the Chrysalis, then clear Terran buildings.",
        "Recover the Chrysalis and move it to the Warp Point.",
        "Meet Fenix and destroy the Zerg base. Fenix must survive.",
        "Hold out until Fenix arrives, protect him, and destroy the Cerebrate."
#ifdef SC64_UNIFIED
        ,"Find Raynor; build a Barracks and train ten Marines.",
        "Destroy the Infested Command Center. Raynor must survive."
#ifdef SC64_T3_ENABLED
        ,"Survive the countdown, or unlock and complete the Zerg Hatchery objective."
#endif
#endif
    };
    int mission = limit(race_campaign.mission, 0, SC64_RACE_MAP_COUNT - 1);
    return objectives[mission];
}

static const char *
race_campaign_victory_text(void)
{
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    if (race_campaign.mission == SC64_RACE_MISSION_T3)
        return race_t3_victory_text();
#endif
    static const char *const titles[] = {
        "AMONG THE RUINS CLEARED", "CHRYSALIS DELIVERED",
        "FIRST STRIKE COMPLETE", "CEREBRATE DESTROYED"
#ifdef SC64_UNIFIED
        ,"WASTELAND SECURED", "BACKWATER SECURED"
#ifdef SC64_T3_ENABLED
        ,"THE DESPERATE ALLIANCE SURVIVED"
#endif
#endif
    };
    return titles[limit(race_campaign.mission, 0, SC64_RACE_MAP_COUNT - 1)];
}

static const char *
race_campaign_message_text(void)
{
    static const char *const messages[] = {
        "", "Protect the Chrysalis. Build a Spawning Pool and Hydralisk Den.",
        "The required Zerg structures are ready. Destroy the Terran encampment.",
        "Chrysalis recovered. Bring it to the Warp Point.",
        "Fenix is with you. Destroy the Zerg base.",
        "Fenix will move into position in 15 minutes.",
        "Fenix is in position. Destroy the Cerebrate and keep him alive.",
        "Move a Drone next to the Chrysalis, then guide it to the Warp Point.",
        "You have reached the Protoss Base. The allied forces join you.",
        "The Hunter Killers join you. Escort the Drone and Chrysalis to the Warp Point."
#ifdef SC64_UNIFIED
        ,"Build your outpost and find Jim Raynor.",
        "Rescue the outpost and destroy the Infested Command Center.",
        "Opening supplies received. Find Jim Raynor.",
        "Raynor has joined. Build a Barracks and train ten Marines.",
        "Hold the Terran base. Destroy a Zerg building before the final two minutes to reveal another objective.",
        "The enemy has begun its assault. Hold your buildings until the countdown ends.",
        "Ten minutes remain. Keep the Terran buildings alive.",
        "Five minutes remain. Keep the Terran buildings alive.",
        "One minute remains. Hold the line.",
        "The Hatchery objective is open. Destroy the Zerg Hatchery or survive the countdown."
#endif
    };
    return race_campaign.message < sizeof(messages) / sizeof(messages[0]) ?
        messages[race_campaign.message] : "";
}

#endif
