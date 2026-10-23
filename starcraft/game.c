#include "../common/gba.h"
#include "sc64_bitgrid.h"
#include "sc64_map_math.h"
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int value, size_t n);
int memcmp(const void *a, const void *b, size_t n);

#ifdef SC64_P3_RUNTIME
#if !defined(SC64_P3_ENABLED) || !defined(SC64_UNIFIED) || \
    !defined(SC64_RACES) || !defined(SC64_LARGE_MAPS)
#error "SC64_P3_RUNTIME requires P3 data, unified races and large-map storage"
#endif
#if defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE != 2
#error "Higher Ground belongs to the Protoss campaign"
#endif
#endif

#ifdef SC64_T4_RUNTIME
#if !defined(SC64_T4_ENABLED) || !defined(SC64_UNIFIED) || !defined(SC64_RACES) || \
    !defined(SC64_LARGE_MAPS) || \
    !defined(SC64_ROUTE_PACK3) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0
#error "SC64_T4_RUNTIME requires the unified Terran 128x128 packed-map profile"
#endif
#endif
#if defined(SC64_T4_SCENARIO_RUNTIME) && !defined(SC64_T4_RUNTIME)
#error "SC64_T4_SCENARIO_RUNTIME requires the T4 actor/world profile"
#endif
#if defined(SC64_T5_RUNTIME) && \
    (!defined(SC64_T5_ENABLED) || !defined(SC64_T4_SCENARIO_RUNTIME) || \
     !defined(SC64_T3_ENABLED))
#error "SC64_T5_RUNTIME requires T5 data and the combined T3/T4 storage profile"
#endif
#if defined(SC64_T5_PROBE_ENTRY) && !defined(SC64_T5_RUNTIME)
#error "The internal T5 probe entry requires its runtime"
#endif
#if defined(SC64_T6_STORAGE) && \
    (!defined(SC64_T6_ENABLED) || !defined(SC64_T5_RUNTIME))
#error "T6 storage requires T6 data and the combined T1-T5 runtime"
#endif
#if defined(SC64_T6_RUNTIME) && !defined(SC64_T6_STORAGE)
#error "T6 mission runtime requires the expanded T6 storage profile"
#endif
#if defined(SC64_T6_PROBE_ENTRY) && !defined(SC64_T6_RUNTIME)
#error "T6 internal probe entry requires the T6 runtime"
#endif
#if defined(SC64_LATE_TERRAN_STORAGE) && !defined(SC64_T6_RUNTIME)
#error "Late Terran storage extends the combined T1-T6 runtime"
#endif
#if defined(SC64_TERRAN_LATE_RUNTIME) && \
    (!defined(SC64_LATE_TERRAN_STORAGE) || !defined(SC64_TERRAN_LATE_ENABLED))
#error "Late Terran runtime requires the complete imported maps and storage"
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME) && \
    (!defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0)
#error "Combined T3/T4 runtime is an experimental unified Terran profile"
#endif

#if defined(SC64_RACES)
#if defined(SC64_LATE_TERRAN_STORAGE)
#define MAX_UNITS 192
#elif defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
#define MAX_UNITS 160
#else
#define MAX_UNITS 128
#endif
#if defined(SC64_TERRAN_LATE_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 18u
#elif defined(SC64_LATE_TERRAN_STORAGE)
#define SC64_RACE_SAVE_GENERATION 17u
#elif defined(SC64_T6_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 16u
#elif defined(SC64_T6_STORAGE)
#define SC64_RACE_SAVE_GENERATION 15u
#elif defined(SC64_T5_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 14u
#elif defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 11u
#elif defined(SC64_T4_SCENARIO_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 10u
#elif defined(SC64_T4_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 9u
#elif defined(SC64_P3_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 7u
#elif defined(SC64_LARGE_MAPS)
#define SC64_RACE_SAVE_GENERATION 6u
#elif defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
#define SC64_RACE_SAVE_GENERATION 5u
#elif defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
#define SC64_RACE_SAVE_GENERATION 3u
#else
#define SC64_RACE_SAVE_GENERATION 2u
#endif
#if defined(SC64_LATE_TERRAN_STORAGE)
#define MAX_BUILDINGS 96
#elif defined(SC64_T6_STORAGE)
#define MAX_BUILDINGS 80
#else
#define MAX_BUILDINGS 64
#endif
#if defined(SC64_LATE_TERRAN_STORAGE)
#define MAX_MINERALS 80
#elif defined(SC64_LARGE_MAPS)
#define MAX_MINERALS 48
#else
#define MAX_MINERALS 32
#endif
#if defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 0
#define SAVE_VERSION (0x53435430u + SC64_RACE_SAVE_GENERATION)
#elif defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 1
#define SAVE_VERSION (0x53435a30u + SC64_RACE_SAVE_GENERATION)
#elif defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 2
#define SAVE_VERSION (0x53435030u + SC64_RACE_SAVE_GENERATION)
#elif defined(SC64_UNIFIED)
#if defined(SC64_P3_RUNTIME)
#define SAVE_VERSION 0x53435536u
#elif defined(SC64_LARGE_MAPS)
#define SAVE_VERSION 0x53435535u
#elif defined(SC64_Z3_ENABLED)
#define SAVE_VERSION 0x53435534u
#elif defined(SC64_T3_ENABLED)
#define SAVE_VERSION 0x53435532u
#else
#define SAVE_VERSION 0x53435531u
#endif
#else
#define SAVE_VERSION 0x53435231u
#endif
#elif defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES) || defined(SC64_TERRAN_CAMPAIGN)
#define MAX_UNITS 96
#define MAX_MINERALS 16
#define MAX_BUILDINGS 32
#ifdef SC64_TERRAN_CAMPAIGN
#define SAVE_VERSION 0x53435431u
#elif defined(SC64_BACKWATER)
#define SAVE_VERSION 0x53434232u
#else
#define SAVE_VERSION 0x53434B31u
#endif
#elif defined(SC64_IMPORTED_MISSION)
#define MAX_UNITS 48
#define MAX_MINERALS 16
#define SAVE_VERSION 0x53433631u
#else
#define MAX_UNITS 40
#define MAX_MINERALS 8
#define SAVE_VERSION 0x53434733u
#endif
#ifndef MAX_BUILDINGS
#define MAX_BUILDINGS 16
#endif
#define MAX_TRACERS 12
#ifdef SC64_IMPORTED_MISSION
#include "sc64_mission_generated.h"
#include "sc64_assets_generated.h"
#ifdef SC64_TERRAN_CAMPAIGN
#include "sc64_campaign_generated.h"
#endif
#ifdef SC64_RACES
#include "sc64_races_generated.h"
#include "sc64_frontend.h"
#ifdef SC64_IMPORTED_SKIRMISH
#include "sc64_skirmish_generated.h"
_Static_assert(SC64_FRONT_MAP_COUNT == SC64_SKIRMISH_IMPORTED_COUNT + 1,
               "Skirmish menu and imported map pack disagree");
#endif
#endif
#ifdef SC64_HAS_SPRITES
#include "sc64_sprites_generated.h"
#ifdef SC64_RACES
#include "sc64_race_sprites_generated.h"
#endif
#endif
#ifdef SC64_T6_STORAGE
#if SC64_RACE_MAP_COUNT < 10 || !defined(SC64_T6_SPRITE_SET_COMPLETE)
#error "Run make sc64-t6-prepare before building the T6 storage profile"
#endif
#endif
#define SC64_HAS_VERIFIED_COLLISION 1
#define SC64_HAS_TERRAIN_ART 1
#ifdef SC64_RACES
static int race_cols=64,race_rows=64;
#define MAP_COLS race_cols
#define MAP_ROWS race_rows
#else
#define MAP_COLS SC64_MAP_COLS
#define MAP_ROWS SC64_MAP_ROWS
#endif
#elif defined(SC64_SKIRMISH)
#define MAP_COLS 64
#define MAP_ROWS 64
#else
#define MAP_COLS 24
#define MAP_ROWS 16
#endif
#ifdef SC64_RACES
#ifdef SC64_T4_RUNTIME
#define MAP_CELLS (128*128)
#elif defined(SC64_LARGE_MAPS)
#define MAP_CELLS (128*96)
#else
#define MAP_CELLS (96*96)
#endif
#else
#define MAP_CELLS (MAP_COLS * MAP_ROWS)
#endif
#if defined(SC64_LARGE_MAPS) && !defined(SC64_RACES)
#error SC64_LARGE_MAPS requires SC64_RACES
#endif
#ifdef SC64_LARGE_MAPS
#define SC64_GRID_STORAGE_BYTES ((MAP_CELLS + 7) / 8)
/* Grid algorithms validate coordinates before indexing, as in the byte-grid
 * profile. Keep their inner accesses small enough for the IWRAM code guard;
 * the separately tested public bitgrid helpers also accept invalid indices. */
static inline int sc64_grid_get(const u8 *grid, unsigned cell) {
    return (grid[cell >> 3] >> (cell & 7u)) & 1u;
}
static inline void sc64_grid_set(u8 *grid, unsigned cell, int value) {
    u8 mask = (u8)(1u << (cell & 7u));
    if (value) grid[cell >> 3] |= mask;
    else grid[cell >> 3] &= (u8)~mask;
}
#define SC64_GRID_GET(grid, cell) sc64_grid_get((grid), (unsigned)(cell))
#define SC64_GRID_SET(grid, cell, value) sc64_grid_set((grid), (unsigned)(cell), (value))
#define SC64_GRID_CLEAR(grid) memset((grid), 0, sizeof(grid))
#else
#define SC64_GRID_STORAGE_BYTES MAP_CELLS
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#define SC64_GRID_CLEAR(grid) memset((grid), 0, sizeof(grid))
#endif
#define WORLD_W (MAP_COLS * 16)
#define WORLD_H (MAP_ROWS * 16)
#ifdef SC64_RACES
#define IS_WORKER(t) sc64_race_is_worker(t)
#else
#define IS_WORKER(t) ((t)==U_SCV)
#endif
#ifdef SC64_RACES
/* Keep the production 64/96 arithmetic unchanged; the opt-in large profile
 * adds the shift for 128-column maps. */
static inline int sc64_cell_row(unsigned cell) {
    if (race_cols == 64) return (int)(cell >> 6);
#ifdef SC64_LARGE_MAPS
    if (race_cols == 128) return (int)(cell >> 7);
#endif
    return (int)((cell * 43691u) >> 22);
}
static inline int sc64_cell_col(unsigned cell) {
    return (int)cell-sc64_cell_row(cell)*race_cols;
}
static inline int sc64_minimap_pixel(unsigned pixel,int tiles) {
    if (tiles == 64) return (int)((pixel * 3u) >> 7);
#ifdef SC64_LARGE_MAPS
    if (tiles == 128) return (int)((pixel * 3u) >> 8);
#endif
    return (int)(pixel >> 6);
}
#else
#define sc64_cell_row(cell) ((cell)/MAP_COLS)
#define sc64_cell_col(cell) ((cell)%MAP_COLS)
#endif
#define FAST_ABS(n) ((n) < 0 ? -(n) : (n))
#ifdef SC64_HAS_TERRAIN_ART
#include "sc64_display.h"
#endif
#define UNIT_DOUBLE_TAP_TICKS 12u
#define UNIT_DOUBLE_TAP_RADIUS 8
#define SIDE 0
#define ENEMY 1
#define NEUTRAL 2
#define RESCUABLE 3
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
#include "sc64_backwater.h"
#endif
#ifdef SC64_SKIRMISH
#include "skirmish_ai.h"
#endif

#ifdef SC64_RACES
#include "race_rules.h"
#ifdef SC64_T4_RUNTIME
#include "sc64_t4_world.h"
#ifdef SC64_T4_SCENARIO_RUNTIME
#include "sc64_t4_runtime.h"
#include "sc64_t4_scenario_generated.h"
#ifdef SC64_T5_RUNTIME
#include "sc64_t5_runtime.h"
#include "sc64_t5_actor_adapter.h"
#include "sc64_t5_scenario_generated.h"
#ifdef SC64_T6_RUNTIME
#include "sc64_t6_mission_state.h"
#include "sc64_t6_scenario_adapter.h"
#ifdef SC64_TERRAN_LATE_RUNTIME
#include "sc64_terran_late_mission_state.h"
#endif
#endif
#endif
#endif
#ifndef SC64_T4_GLOBAL_ID7
#define SC64_T4_GLOBAL_ID7 7
#endif
#ifndef SC64_T4_GLOBAL_ID
#define SC64_T4_GLOBAL_ID SC64_T4_GLOBAL_ID7
#endif
#endif
#include "race_missions.h"
#ifdef SC64_P3_RUNTIME
#include "sc64_p3_model.h"
_Static_assert(MAX_UNITS >= 96 && MAX_BUILDINGS >= 42 && MAX_MINERALS >= 48,
               "Higher Ground must retain its complete starting roster");
_Static_assert(MAX_UNITS + MAX_BUILDINGS <= 255,
               "Visibility source counts require wider storage");
#endif
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
#include "sc64_z3.h"
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
#include "sc64_t3.h"
_Static_assert(MAX_UNITS >= 138, "T3 must retain its complete starting roster");
#ifdef SC64_LATE_TERRAN_STORAGE
_Static_assert(MAX_UNITS + MAX_BUILDINGS <= 511, "Nine-bit visibility source count bound");
#else
_Static_assert(MAX_UNITS + MAX_BUILDINGS <= 255, "Visibility source counts require wider storage");
#endif
#endif
#else
enum {
    U_SCV, U_MARINE, U_FIREBAT, U_MEDIC, U_ZERGLING, U_HYDRALISK, U_RAYNOR
};
enum {
    B_COMMAND, B_BARRACKS, B_DEPOT, B_REFINERY, B_ACADEMY, B_ENG_BAY, B_HATCHERY,
    B_BUNKER, B_INFESTED_COMMAND, B_CREEP_COLONY
};
#endif
enum {
    ORDER_IDLE, ORDER_MOVE, ORDER_ATTACK_MOVE, ORDER_ATTACK_TARGET,
    ORDER_MINE, ORDER_BUILD, ORDER_GAS
#ifdef SC64_T5_RUNTIME
    , ORDER_BOARD, ORDER_TRANSPORTED, ORDER_UNLOAD
#endif
#ifdef SC64_T6_RUNTIME
    , ORDER_REPAIR
#endif
};
#ifdef SC64_T5_RUNTIME
#define SC64_TRANSPORT_LOADED(q) ((q)->order == ORDER_TRANSPORTED)
#else
#define SC64_TRANSPORT_LOADED(q) 0
#endif
enum {
    SCREEN_TITLE, SCREEN_PLAY, SCREEN_PAUSE, SCREEN_RESULT
};
enum {
    MENU_MOVE, MENU_ATTACK, MENU_BARRACKS, MENU_DEPOT, MENU_REFINERY,
    MENU_ACADEMY, MENU_ENG_BAY, MENU_WEAPONS, MENU_ARMOR, MENU_STOP,
#ifdef SC64_T5_RUNTIME
    MENU_FACTORY, MENU_STARPORT, MENU_CONTROL_TOWER, MENU_UNLOAD,
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    MENU_LIFT_LAND, MENU_COMMAND_CENTER,
#endif
    MENU_COUNT
};
enum {
    MODE_NORMAL, MODE_MOVE, MODE_ATTACK, MODE_BUILD
#ifdef SC64_T5_RUNTIME
    , MODE_UNLOAD
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    , MODE_LAND
#endif
};

typedef struct {
#ifdef SC64_LATE_TERRAN_STORAGE
    /* Named fields retain their engine meanings. The late campaign has a
     * distinct save codec, so runtime records need no int-word ABI. HP and
     * shields remain int because combat shares pointers with buildings. */
    int hp, maxhp, carry, shield;
    s16 x, y, tx, ty, build, target, path_x, path_y, raw_type, raw_owner;
    u16 cd, work_timer, weapon, path_timer, path_retry, flags, shield_remainder;
    u8 owner, type, alive, selected, order, armor;
#else
    int x, y, hp, maxhp, owner, type, alive, selected, cd, order, tx, ty;
    int carry, build, work_timer, armor, weapon, target, path_timer, path_retry, path_x, path_y;
#ifdef SC64_RACES
    int shield,raw_type,raw_owner,flags,shield_remainder;
#endif
#endif
} Unit;
#ifdef SC64_LATE_TERRAN_STORAGE
_Static_assert(sizeof(Unit) == 56, "Late Terran runtime unit budget");
#endif
typedef struct {
    int x, y, hp, maxhp, owner, type, alive;
    int queue[5], queue_head, queue_count, queue_progress;
    int build_progress, under_construction, level;
#ifdef SC64_RACES
    int shield,raw_type,raw_owner,flags,larva,larva_timer,shield_remainder;
#endif
} Building;
typedef struct {
    int x, y, amount;
#ifdef SC64_IMPORTED_MISSION
    int style;
#endif
} Mineral;
typedef struct {
    int x, y;
} Geyser;
typedef struct {
    int x0, y0, x1, y1, life, color;
} Tracer;
#ifdef SC64_RACES
#include "sc64_race_save_types.h"
#endif
#ifndef SC64_RACES
typedef struct {
    u32 version;
    Unit units[MAX_UNITS];
    Building buildings[MAX_BUILDINGS];
    Mineral minerals_map[MAX_MINERALS];
    Tracer tracer_data[MAX_TRACERS];
    u8 explored_map[MAP_CELLS];
    int minerals, gas, supply_used, supply_cap, frame, cx, cy;
    int enemy_timer, wave, weapons_level, armor_level;
    int weapons_research, armor_research, train_type, selected_building;
    u32 seed;
#ifdef SC64_IMPORTED_MISSION
    int mission_objective_stage;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    int campaign_mission, campaign_completed;
#endif
#ifdef SC64_BACKWATER
    Sc64BackwaterState backwater;
    u8 invincible_units[MAX_UNITS], invincible_buildings[MAX_BUILDINGS];
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    int geyser_x[4], geyser_y[4], geyser_count;
#endif
#ifdef SC64_SKIRMISH
    Sc64AiState bots[3];
    int bot_count;
#endif
} SaveData;
_Static_assert(sizeof(SaveData) <= 16368, "Save exceeds an alternating SRAM record");
/* Mission maps enlarge save payloads; keeping this single scratch record in
 * EWRAM avoids placing the full 64x64 save copy on the GBA stack. */
static SaveData data;
#endif
static Unit u[MAX_UNITS];
static Building b[MAX_BUILDINGS];
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
#include "sc64_collision.h"
#endif
/* Temporary ascending-index owner lists used only during update_orders(). */
static int target_unit_head[5], target_building_head[5];
static int target_unit_next[MAX_UNITS], target_building_next[MAX_BUILDINGS];
static int target_lists_valid;
#ifdef SC64_RACES
static u8 target_hostile_mask[5];
static int target_hostile_masks_valid;
#endif
#ifdef SC64_RACES
#include "sc64_targets.h"
#endif
static Mineral m[MAX_MINERALS];
static Geyser geysers[4] = {{
	224, 160
}, {
    272, 192
}
};
static Tracer tracers[MAX_TRACERS];
static u8 explored[SC64_GRID_STORAGE_BYTES];
static u8 visible[SC64_GRID_STORAGE_BYTES];
static u8 path_walkable[SC64_GRID_STORAGE_BYTES];
#ifdef SC64_IMPORTED_MISSION
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
typedef union {
    u16 bfs_queue[MAP_CELLS];
    RaceSave save;
} Sc64CombinedRouteSaveScratch;
static Sc64CombinedRouteSaveScratch sc64_combined_route_save_scratch;
#define bfs_queue sc64_combined_route_save_scratch.bfs_queue
#else
static u16 bfs_queue[MAP_CELLS];
#endif
static void sc64_routes_reset(void);
#else
static int bfs_prev[MAP_CELLS], bfs_queue[MAP_CELLS];
static u16 bfs_seen[MAP_CELLS], bfs_generation;
#endif
static int path_grid_valid;
static int minerals, gas, supply_used, supply_cap, frame, mode, menu, train_type;
static int geyser_count;
#ifdef SC64_IMPORTED_MISSION
static int mission_unsupported_count;
static int mission_objective_stage;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
static int campaign_mission, campaign_completed;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
static Sc64BackwaterState backwater;
static u8 invincible_units[MAX_UNITS], invincible_buildings[MAX_BUILDINGS];
static int backwater_message_trigger, backwater_message_action, backwater_message_ticks;
static void backwater_init(void);
static void backwater_update(void);
#endif
#ifdef SC64_SKIRMISH
static Sc64AiState skirmish_ai[3];
static int skirmish_bots = 2;
static int skirmish_ground_tile;
static void skirmish_init(void);
static void skirmish_update(void);
static void skirmish_outcome(void);
static int skirmish_hostile(int a, int b);
static int skirmish_bot_index(int owner);
#endif
#ifdef SC64_RACES
static int race_mode,race_human=R_ZERG,race_selected_mission,race_completed,race_bots=2;
static int race_skirmish_map;
static int race_bot_races[3]={R_ZERG,R_PROTOSS,R_TER};
static Sc64Frontend race_front;
static unsigned race_front_transition;
static int race_front_load_error;
static void race_frontend_reset(void)
{
    int completed=-1;
    for(int i=0;i<SC64_RACE_MAP_COUNT;i++)if(race_completed&(1<<i))completed=i;
    sc64_frontend_reset(&race_front,race_selected_mission,race_human,race_bots,completed);
    race_front.map=race_skirmish_map;
    for(int i=0;i<3;i++)race_front.bot_races[i]=race_bot_races[i];
    race_front_transition=0;race_front_load_error=0;
}
static int race_player[5],race_minerals[5],race_gas[5];
static Sc64RaceCampaignState race_campaign;
#ifdef SC64_T4_SCENARIO_RUNTIME
static Sc64T4RuntimeState race_t4;
static int race_t4_active(void)
{ return !race_mode && race_selected_mission == SC64_T4_GLOBAL_ID; }
static int race_t4_init(void), race_t4_tick(void);
static void race_t4_traps_tick(void);
static int race_t4_create_action(const Sc64T4Action *action, unsigned owner);
static int race_t4_doodad_action(const Sc64T4Action *action, unsigned owner);
#endif
#ifdef SC64_T5_RUNTIME
static Sc64T5RuntimeState race_t5;
static int race_t5_active(void)
{ return !race_mode && race_selected_mission == SC64_T5_GLOBAL_ID; }
static int race_t5_init(void), race_t5_tick(void);
static void race_t5_world_tick(void);
static void race_t5_mines_tick(void);
static int race_t5_start_control_tower(int producer);
static void race_t5_tech_tick(void);
static int sc64_race_t5_building_art_raw(int type);
static int race_t5_unit_product_available(int owner, int raw_owner, int type);
#endif
#ifdef SC64_T6_RUNTIME
static Sc64T6MissionState race_t6;
#ifdef SC64_TERRAN_LATE_RUNTIME
static int race_late_airborne(int slot);
static int race_late_lift(int slot);
static int race_late_flight_order(int slot, int x, int y, int land);
static void race_late_flight_tick(void);
static int race_late_request_repair(void);
static int race_late_repair_tick_unit(int slot);
#endif
static int race_t6_active(void)
{ return !race_mode && race_selected_mission == SC64_T6_GLOBAL_ID; }
static int race_t6_init(void), race_t6_tick(void);
static int sc64_t6_runtime_owner(unsigned source);
static int sc64_t6_owners_hostile(int owner, int other);
static int race_t6_unit_burrowed(int slot);
static void race_t6_unit_created(int slot);
static void race_t6_building_died(int owner, int raw_owner);
static int race_t6_unit_product_available(int owner, int raw_owner, int type);
static int race_t6_start_control_tower(int producer);
static int race_t6_can_train_dropship(int producer);
static void race_t6_tech_tick(void);
static inline int race_t6_tech_active(void);
static int race_t6_request_repair(void);
static int race_t6_repair_tick_unit(int slot);
#ifdef SC64_TERRAN_LATE_RUNTIME
static Sc64TerranLateMissionState race_late;
static int race_late_active(void)
{ return !race_mode && race_selected_mission >= 10 && race_selected_mission <= 13; }
static int race_late_init(void), race_late_tick(void);
static int sc64_terran_late_runtime_owner(unsigned source_slot);
static int sc64_terran_late_runtime_race(unsigned source_slot);
static int sc64_terran_late_current_source_owner(int live_owner, int raw_owner);
static int sc64_terran_late_product_source_owner(int live_owner);
static int race_late_unit_product_available(int owner, int raw_owner, int type);
static int race_late_building_product_available(int owner, int raw_owner, int type);
static int __attribute__((noinline)) sc64_terran_late_owners_hostile(int owner, int other);
static void race_late_record_actor_death(int killer_owner, int killer_raw_owner,
                                        int victim_owner, int victim_raw_owner,
                                        int victim_raw_type, int is_building);
#endif
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
static Sc64T3State race_t3;
static u16 race_t3_buildings_killed;
#endif
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
static Sc64Z3State race_z3;
static u16 race_z3_action_ticks;
static u16 race_z3_action_duration(const Sc64Z3State *s);
#endif
#ifdef SC64_P3_RUNTIME
static Sc64P3State race_p3;
static u16 race_p3_action_ticks;
static u16 race_p3_action_duration(const Sc64P3State *s);
#endif
static int race_queue_unit(int owner,int building,int type);
static int race_start_build(int owner,int worker,int type,int x,int y);
static int race_can_site(int owner,int type,int x,int y);
static int race_site_powered(int owner,int x,int y);
static int race_builder_arrived(int worker);
static void race_tick_buildings(void),race_tick_state(void),race_tick_ai(void),race_tick_defenses(void);
static void race_skirmish_init(void),race_skirmish_outcome(void);
static void race_skirmish_geometry(void);
static int race_owner_supply(int owner),race_owner_supply_cap(int owner),race_owner_reserved_supply(int owner);
static int race_owner_queued_supply(int owner);
static void race_add_owner_resources(int owner,int ore,int vespene);
static int race_save_game(void),race_load_game(void);
static void race_campaign_init(void),race_campaign_tick(void);
#endif
static int screen, menu_open, menu_cursor, help, won, lost, result_timer;
static int cx, cy, camx, camy, toast_timer, toast_id, enemy_timer, wave;
/* Transient controller gesture, deliberately outside the persistent save. */
static int box_state, box_anchor_x, box_anchor_y, box_end_x, box_end_y;
static int box_suppress_a;
static int unit_tap = -1, unit_tap_x, unit_tap_y;
static u32 unit_tap_frame;
static int weapons_level, armor_level, selected_building;
static int weapons_research, armor_research;
static u32 seed;
#ifdef SC64_IMPORTED_MISSION
volatile u32 sc64_orders_cycles, sc64_sight_cycles, sc64_other_cycles;
volatile u32 sc64_map_cycles, sc64_hud_cycles;
volatile u32 sc64_background_cycles, sc64_building_cycles, sc64_unit_draw_cycles;
#ifdef SC64_DIAGNOSTIC_TERRAIN
volatile u32 sc64_hud_minimap_cycles;
#endif
#if defined(SC64_DIAGNOSTIC_UPDATE)
volatile u32 sc64_update_mission_cycles, sc64_update_production_cycles;
volatile u32 sc64_update_race_state_cycles, sc64_update_defenses_cycles;
volatile u32 sc64_update_research_cycles, sc64_update_remainder_cycles;
#ifdef SC64_T4_SCENARIO_RUNTIME
volatile u32 sc64_t4_rescue_cycles, sc64_t4_script_cycles, sc64_t4_trap_cycles;
#endif
#ifdef SC64_T5_RUNTIME
volatile u32 sc64_t5_rescue_cycles, sc64_t5_mine_cycles, sc64_t5_trigger_ai_cycles;
#endif
#endif
#include "sc64_visibility.h"
#endif
#ifdef SC64_DIAGNOSTIC_ORDERS
volatile u32 sc64_order_setup_cycles, sc64_order_query_cycles, sc64_order_move_cycles;
#define SC64_ORDER_DIAG_ACCUM(counter, start) \
    do { (counter) += gba_clock() - (start); } while (0)
#endif
static int nearest_refinery(int x, int y);
static void update_exploration(void);
static int owner_command_center(int owner);
static void add_owner_resources(int owner, int ore, int vespene);
#ifdef SC64_P3_RUNTIME
static int GBA_HOT __attribute__((noinline))
#else
static int GBA_HOT
#endif
owners_hostile(int a, int other)
{
#ifdef SC64_RACES
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) return sc64_terran_late_owners_hostile(a, other);
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active()) return sc64_t6_owners_hostile(a, other);
#endif
#ifdef SC64_T4_RUNTIME
    if (!race_mode && race_selected_mission == SC64_T4_GLOBAL_ID7)
        return sc64_t4_owners_hostile(a, other);
#endif
#ifdef SC64_P3_RUNTIME
    if (!race_mode && race_selected_mission == SC64_P3_GLOBAL_ID)
        return (a == SIDE && (other == ENEMY || other == 4)) ||
               (other == SIDE && (a == ENEMY || a == 4));
#endif
    return a!=other && a!=NEUTRAL && other!=NEUTRAL &&
        (race_mode || a==ENEMY || other==ENEMY);
#elif defined(SC64_SKIRMISH)
    return skirmish_hostile(a, other);
#elif defined(SC64_BACKWATER)
    return (a == ENEMY && other != ENEMY) || (other == ENEMY && a != ENEMY);
#else
    return other == (a == SIDE ? ENEMY : SIDE);
#endif
}

#ifdef SC64_T4_RUNTIME
static int
sc64_t4_unit_has(int type, unsigned flag)
{
    unsigned actor = sc64_race_t4_actor_flags(type);
    return actor != 0 && (actor & flag) != 0;
}

static int
sc64_t4_unit_selectable(int type)
{
    return !sc64_race_t4_actor_flags(type) ||
        sc64_t4_unit_has(type, SC64_T4_ACTOR_SELECTABLE);
}

static int
sc64_t4_unit_collidable(int type)
{
    return !sc64_race_t4_actor_flags(type) ||
        sc64_t4_unit_has(type, SC64_T4_ACTOR_COLLIDABLE);
}

static int
sc64_t4_building_collidable(int slot)
{
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_airborne(slot)) return 0;
#endif
    int type = b[slot].type;
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active() && sc64_race_t4_is_door(type)) {
        for (unsigned row = 0; row < 32; ++row)
            if (race_t4.doodad_slot[row] == slot)
                return race_t4.doodad_enabled[row] != 0;
        return 1; /* An unbound door never becomes passable silently. */
    }
#endif
    return !sc64_race_t4_building_flags(type) ||
        (sc64_race_t4_building_flags(type) & SC64_T4_ACTOR_COLLIDABLE);
}
#ifdef SC64_T4_SCENARIO_RUNTIME
/* Door changes are infrequent. Keep this bounded grid rebuild in ROM rather
 * than consuming IWRAM reserved for per-frame movement and the stack. */
static int limit(int n, int lo, int hi);
static __attribute__((noinline)) void
race_t4_block_door_grid(int slot)
{
    int left, up, right, down;
    const Building *door = &b[slot];
    if (!sc64_race_t4_building_bounds_source(door->type, &left, &up, &right, &down))
        return;
    int min_col = limit((door->x * 2 - left) / 32, 0, MAP_COLS - 1);
    int max_col = limit((door->x * 2 + right) / 32, 0, MAP_COLS - 1);
    int min_row = limit((door->y * 2 - up) / 32, 0, MAP_ROWS - 1);
    int max_row = limit((door->y * 2 + down) / 32, 0, MAP_ROWS - 1);
    for (int row = min_row; row <= max_row; ++row)
        for (int col = min_col; col <= max_col; ++col)
            SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
}
#endif
#endif
static int
owner_color(int owner)
{
    return owner == SIDE ? C_GREEN : owner == NEUTRAL ? C_CYAN :
        owner == RESCUABLE ? C_GOLD : owner == 4 ? C_PURPLE : C_RED;
}
static void
reset_unit_tap(void)
{
    unit_tap = -1;
}
static void
reset_selection_gestures(void)
{
    box_state = 0;
    box_suppress_a = 0;
    reset_unit_tap();
}
static const char *toast_text[] = {"Command issued", "Need more minerals", "Supply limit reached", "Building placed", "Training unit", "Select a worker", "Site blocked", "Game saved", "Tech prerequisite missing", "Building unfinished", "Research unavailable", "Queue full", "No gas", "Unit pool full", "Mission data skipped"};
static int
limit(int n, int lo, int hi)
{
    return n < lo ? lo : (n > hi ? hi : n);
}
static int
viewport_left(void)
{
    int left = limit(cx - 120, 0, WORLD_W - 240);
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    /* Word-aligned background copies keep the larger maps affordable. */
    left &= ~3;
#endif
    return left;
}
static int
viewport_top(void)
{
    return limit(cy - 54, 0, WORLD_H - 108);
}
static int GBA_HOT
dist2(int x, int y, int a, int c)
{
    int dx = x - a, dy = y - c;
    return dx * dx + dy * dy;
}
static void invalidate_path_grid(void);
static void
toast(int id)
{
    toast_id = id;
    toast_timer = 55;
}
static int
unit_cost(int type)
{
#ifdef SC64_RACES
    return sc64_race_unit_rules[type].minerals;
#endif
    if (type == U_SCV || type == U_MARINE)
	return 50;
    if (type == U_FIREBAT)
	return 50;
    if (type == U_MEDIC)
	return 50;
    return 0;
}

static int
unit_gas_cost(int type)
{
#ifdef SC64_RACES
    return sc64_race_unit_rules[type].gas;
#endif
    if (type == U_FIREBAT)
	return 25;
    if (type == U_MEDIC)
	return 25;
    return 0;
}

static int
unit_supply(int type)
{
#ifdef SC64_RACES
    return sc64_race_unit_rules[type].supply;
#endif
    return type == U_ZERGLING ? 1 : type == U_SCV ? 2 : type == U_MARINE ? 2 :
	type == U_FIREBAT ? 2 : type == U_MEDIC ? 2 : 2;
}

static int
unit_build_time(int type)
{
#ifdef SC64_RACES
    return (sc64_race_unit_rules[type].build_frames*5+3)/4;
#endif
    return type == U_SCV ? 375 : type == U_MARINE ? 450 : type == U_FIREBAT ? 450 : type == U_MEDIC ? 563 : 525;
}

static int
building_build_time(int type)
{
#ifdef SC64_RACES
    return (sc64_race_building_rules[type].build_frames*5+3)/4;
#endif
    if (type == B_DEPOT || type == B_REFINERY)
	return 750;
    if (type == B_BARRACKS || type == B_ACADEMY || type == B_ENG_BAY)
	return 1500;
    return 1800;
}

static int
building_cost(int type)
{
#ifdef SC64_RACES
    return sc64_race_building_rules[type].minerals;
#endif
    if (type == B_BARRACKS)
	return 150;
    if (type == B_DEPOT)
	return 100;
    if (type == B_REFINERY)
	return 100;
    if (type == B_ACADEMY)
	return 150;
    if (type == B_ENG_BAY)
	return 125;
    return 0;
}

static int
building_gas_cost(int type)
{
#ifdef SC64_RACES
    return sc64_race_building_rules[type].gas;
#endif
    return type == B_ACADEMY ? 0 : 0;
}

static int
find_unit_slot(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
	if (!u[i].alive)
	    return i;
    return -1;
}

static int
find_build_slot(void)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (!b[i].alive)
	    return i;
    return -1;
}

static int
spawn_unit(int owner, int type, int x, int y)
{
    int i = find_unit_slot();
    if (i < 0)
	return -1;
    /* A newly produced unit can reuse a dead unit's array slot. */
    if (unit_tap == i) reset_unit_tap();
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
#ifdef SC64_T4_RUNTIME
    invincible_units[i] = (u8)sc64_race_t4_unit_intrinsically_invincible(type);
#else
    invincible_units[i] = 0;
#endif
#endif
    u[i].x = x;
    u[i].y = y;
    u[i].owner = owner;
    u[i].type = type;
    u[i].maxhp = type == U_RAYNOR ? 200 : type == U_SCV ? 60 : type == U_MARINE ? 40 :
	type == U_FIREBAT ? 50 : type == U_MEDIC ? 60 :
	type == U_ZERGLING ? 35 : 80;
#ifdef SC64_RACES
    u[i].maxhp=sc64_race_unit_rules[type].hp;
    u[i].shield=sc64_race_unit_rules[type].shields;
    u[i].raw_type=u[i].raw_owner=-1;u[i].flags=u[i].shield_remainder=0;
#ifdef SC64_T5_RUNTIME
    if (race_t5_active()) {
        u[i].raw_type = sc64_race_raw_unit(type);
        u[i].raw_owner = owner == SIDE ? 0 : owner == ENEMY ? 6 :
                         owner == RESCUABLE ? 3 : -1;
    }
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active()) {
        u[i].raw_type = sc64_race_raw_unit(type);
        u[i].raw_owner = owner == SIDE ? 0 : owner == ENEMY ? 1 :
                         owner == 4 ? 4 : -1;
    }
    race_t6_unit_created(i);
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) {
        u[i].raw_type = sc64_race_raw_unit(type);
        u[i].raw_owner = sc64_terran_late_product_source_owner(owner);
    }
#endif
#endif
    u[i].hp = u[i].maxhp;
    u[i].alive = 1;
    u[i].selected = 0;
    u[i].cd = 0;
    u[i].order = ORDER_IDLE;
    u[i].tx = x;
    u[i].ty = y;
    u[i].carry = 0;
    u[i].build = -1;
    u[i].work_timer = 0;
    u[i].armor = owner == SIDE ? armor_level : 0;
    if (type == U_FIREBAT || type == U_MEDIC)
	u[i].armor++;
#ifdef SC64_RACES
    u[i].armor=sc64_race_unit_armor(type);
#endif
    u[i].weapon = owner == SIDE ? weapons_level : 0;
    u[i].target = -1;
    u[i].path_timer = 0;
    u[i].path_retry = 0;u[i].path_x=x;u[i].path_y=y;
    if (owner == SIDE)
	supply_used += unit_supply(type);
    return i;
}

static int
spawn_build(int owner, int type, int x, int y, int constructing)
{
    int i = find_build_slot();
    if (i < 0)
	return -1;
    b[i].x = x;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
#ifdef SC64_T4_RUNTIME
    invincible_buildings[i] = (u8)sc64_race_t4_building_intrinsically_invincible(type);
#else
    invincible_buildings[i] = 0;
#endif
#endif
    b[i].y = y;
    b[i].owner = owner;
    b[i].type = type;
    b[i].alive = 1;
    b[i].queue_head = 0;
    b[i].queue_count = 0;
    b[i].queue_progress = 0;
    b[i].build_progress = 0;
    b[i].under_construction = constructing;
    b[i].level = 0;
    b[i].maxhp = type == B_COMMAND ? 1500 : type == B_HATCHERY ? 1250 :
	type == B_BUNKER ? 350 : type == B_INFESTED_COMMAND ? 1500 : type == B_CREEP_COLONY ? 400 :
	type == B_BARRACKS ? 1000 : type == B_ACADEMY ? 600 : type == B_ENG_BAY ? 850 :
	type == B_REFINERY ? 750 : 500;
#ifdef SC64_RACES
#ifdef SC64_T4_RUNTIME
    b[i].maxhp=sc64_race_building_max_hp(type);
#else
    b[i].maxhp=sc64_race_building_rules[type].hp;
#endif
    b[i].shield=sc64_race_building_rules[type].race==R_PROTOSS?b[i].maxhp:0;
    b[i].raw_type=b[i].raw_owner=-1;b[i].flags=b[i].shield_remainder=0;
#ifdef SC64_T5_RUNTIME
    if (race_t5_active()) {
        b[i].raw_type = sc64_race_raw_building(type);
        b[i].raw_owner = owner == SIDE ? 0 : owner == ENEMY ? 6 :
                         owner == RESCUABLE ? 3 : -1;
    }
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active()) {
        b[i].raw_type = sc64_race_raw_building(type);
        b[i].raw_owner = owner == SIDE ? 0 : owner == ENEMY ? 1 :
                         owner == 4 ? 4 : -1;
    }
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) {
        b[i].raw_type = sc64_race_raw_building(type);
        b[i].raw_owner = sc64_terran_late_product_source_owner(owner);
    }
#endif
    b[i].larva=3;b[i].larva_timer=0;
#endif
    b[i].hp = constructing ? 1 : b[i].maxhp;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (owner == SIDE && type == B_COMMAND && !constructing) supply_cap += 20;
#endif
    invalidate_path_grid();
    return i;
}

static int
building_complete(int i)
{
    return i >= 0 && i < MAX_BUILDINGS && b[i].alive && !b[i].under_construction;
}

static int
has_building(int owner, int type)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (b[i].alive && b[i].owner == owner && b[i].type == type && !b[i].under_construction)
	    return 1;
    return 0;
}

static int
has_tech_for(int type)
{
#ifdef SC64_RACES
    (void)type;return 1; /* Owner prerequisites are enforced by the shared action adapter. */
#endif
    if (type == B_BARRACKS)
	return has_building(SIDE, B_DEPOT);
    if (type == B_ACADEMY)
	return has_building(SIDE, B_BARRACKS);
    if (type == B_ENG_BAY)
	return has_building(SIDE, B_COMMAND);
    return 1;
}

static int
geyser_at(int x, int y)
{
    int i;
    for (i = 0; i < geyser_count; i++)
	if (dist2(x, y, geysers[i].x, geysers[i].y) < 15 * 15)
	    return i;
    return -1;
}

static int
terrain_blocked(int col, int row)
{
#ifdef SC64_RACES
    if(col<0||row<0||col>=MAP_COLS||row>=MAP_ROWS)return 1;
    if(!race_mode)return sc64_race_maps[race_selected_mission].blocked[col+row*MAP_COLS]!=0;
#ifdef SC64_IMPORTED_SKIRMISH
    if(race_skirmish_map>0)return sc64_skirmish_maps[race_skirmish_map-1].blocked[col+row*MAP_COLS]!=0;
#endif
    return (col==31&&row>3&&row<60&&!(row>=14&&row<=18)&&!(row>=45&&row<=49))||
           (row==31&&col>3&&col<60&&!(col>=14&&col<=18)&&!(col>=45&&col<=49));
#else
#ifdef SC64_SKIRMISH
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) return 1;
    return (col == 31 && row > 3 && row < 60 && !(row >= 14 && row <= 18) &&
            !(row >= 45 && row <= 49)) ||
           (row == 31 && col > 3 && col < 60 && !(col >= 14 && col <= 18) &&
            !(col >= 45 && col <= 49));
#else
#ifdef SC64_IMPORTED_MISSION
#ifdef SC64_HAS_TERRAIN_ART
	if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS)
	    return 1;
	#ifdef SC64_TERRAN_CAMPAIGN
	return sc64_campaign_maps[campaign_mission].blocked[col + row * MAP_COLS] != 0;
	#else
	return sc64_terrain_blocked[col + row * MAP_COLS] != 0;
	#endif
#else
	/* The CHK MTXM grid is retained for provenance, but it does not encode
	 * walkability. Keep the fallback collision pattern explicit and honest. */
	return (row == 5 && col >= 8 && col <= 11) || (row == 6 && col >= 8 && col <= 10);
#endif
#else
    return (row == 5 && col >= 8 && col <= 11) || (row == 6 && col >= 8 && col <= 10);
#endif
#endif
#endif
}

static void
invalidate_path_grid(void)
{
    path_grid_valid = 0;
#ifdef SC64_IMPORTED_MISSION
    sc64_routes_reset();
#endif
}

#if defined(SC64_T4_RUNTIME) && defined(SC64_VISIBILITY_SPANS)
/* Rebuilds follow map/door changes. Keep the larger span visibility path in
 * IWRAM and this occasional full-grid rebuild in ROM within the same reserve. */
static void
#else
static void GBA_HOT
#endif
build_path_grid(void)
{
    int col, row, i;
    for (row = 0; row < MAP_ROWS; row++)
	for (col = 0; col < MAP_COLS; col++) {
	    int cell = col + row * MAP_COLS;
	    SC64_GRID_SET(path_walkable, cell, !terrain_blocked(col, row));
	}
    for (i = 0; i < MAX_BUILDINGS; i++) if (b[i].alive) {
#ifdef SC64_RACES
        if(b[i].type==B_ZERG_BEACON)continue;
#ifdef SC64_T4_RUNTIME
        if (!sc64_t4_building_collidable(i)) continue;
#ifdef SC64_T4_SCENARIO_RUNTIME
        if (race_t4_active() && sc64_race_t4_is_door(b[i].type)) {
            /* Conservative tile overlap at the GBA's half scale. Opening
             * restores the retained terrain, rather than carving new floor. */
            race_t4_block_door_grid(i);
            continue;
        }
#endif
#endif
#endif
        int c = b[i].x / 16, r = b[i].y / 16;
        for (row = limit(r - 2, 0, MAP_ROWS - 1); row <= limit(r + 2, 0, MAP_ROWS - 1); row++)
            for (col = limit(c - 2, 0, MAP_COLS - 1); col <= limit(c + 2, 0, MAP_COLS - 1); col++)
                if (dist2(col * 16 + 8, row * 16 + 8, b[i].x, b[i].y) < 18 * 18)
                    SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
    }
    path_grid_valid = 1;
}

static int
can_place(int type, int x, int y)
{

    int i, radius = (type == B_COMMAND || type == B_HATCHERY
#ifdef SC64_RACES
        || type==B_NEXUS || type==B_LAIR
#endif
        ) ? 17 : 10;
    if (type == B_REFINERY
#ifdef SC64_RACES
        ||type==B_EXTRACTOR||type==B_ASSIMILATOR
#endif
        ) {
	if (geyser_at(x, y) < 0)
	    return 0;
	for (i = 0; i < MAX_BUILDINGS; i++)
	    if (b[i].alive
#ifdef SC64_T4_RUNTIME
                && sc64_t4_building_collidable(i)
#endif
                && dist2(x,y,b[i].x,b[i].y) <
#ifdef SC64_RACES
            36*36
#else
            20*20
#endif
            )
		return 0;
	return 1;
    }
#ifdef SC64_RACES
    /* Reserve the geyser footprint before its Extractor/Assimilator exists.
       Adjacent 24px centers overlap the 18px navigation footprints and can
       seal a gas worker between its refinery and a new tech structure. */
    for(i=0;i<geyser_count;i++)
        if(dist2(x,y,geysers[i].x,geysers[i].y)<36*36)return 0;
    for(i=0;i<MAX_MINERALS;i++)
        if(m[i].amount>0&&dist2(x,y,m[i].x,m[i].y)<24*24)return 0;
#endif
    if (terrain_blocked(x / 16, y / 16))
	return 0;
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (b[i].alive
#ifdef SC64_T4_RUNTIME
            && sc64_t4_building_collidable(i)
#endif
            && dist2(x, y, b[i].x, b[i].y) < (radius + 14) * (radius + 14))
	    return 0;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i])
#ifdef SC64_RACES
        && !sc64_race_unit_flying(u[i].type)
#endif
#ifdef SC64_T4_RUNTIME
        && sc64_t4_unit_collidable(u[i].type)
#endif
        && dist2(x, y, u[i].x, u[i].y) < (radius + 5) * (radius + 5))
	    return 0;
    return x > 12 && y > 18 && x < WORLD_W - 12 && y < WORLD_H - 12;
}

static int GBA_HOT
is_traversable(int col, int row, int from_cell)
{
    int cell;
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS)
	return 0;
    if (!path_grid_valid)
	build_path_grid();
    cell = col + row * MAP_COLS;
    /* Original placements and production can begin inside an inflated
     * building tile. Let an actor leave that tile without allowing entry
     * into any other blocked tile. Route searches pass only their start. */
    return SC64_GRID_GET(path_walkable, cell) || cell == from_cell;
}

#ifdef SC64_IMPORTED_MISSION
#include "sc64_routes.h"
#endif

/* Breadth-first on the bounded walk grid; return the first tile toward goal. */
static int GBA_HOT
path_next_tile(int x, int y, int tx, int ty, int *next_x, int *next_y)
{
#ifdef SC64_IMPORTED_MISSION
    return sc64_route_next(x, y, tx, ty, next_x, next_y);
#else
    static const int dx[4] = {1, -1, 0, 0};
    static const int dy[4] = {0, 0, 1, -1};
    int start_col = limit(x / 16, 0, MAP_COLS - 1);
    int start_row = limit(y / 16, 0, MAP_ROWS - 1);
    int start = start_col + start_row * MAP_COLS;
    int goal_x = limit(tx / 16, 0, MAP_COLS - 1);
    int goal_y = limit(ty / 16, 0, MAP_ROWS - 1);
    int goal_col = goal_x, goal_row = goal_y;
    int head = 0, tail = 0, i, cell, col, row, next, best = -1, bestd = 0x7fffffff;
    int goal_open = is_traversable(goal_col, goal_row, -1);

	if (++bfs_generation == 0) {
	    memset(bfs_seen, 0, sizeof(bfs_seen));
	    bfs_generation = 1;
	}
    bfs_prev[start] = -1;
    bfs_seen[start] = bfs_generation;
    bfs_queue[tail++] = start;
    while (head < tail) {
	cell = bfs_queue[head++];
	col = cell % MAP_COLS;
	row = cell / MAP_COLS;
	if (FAST_ABS(col - goal_col) <= 1 && FAST_ABS(row - goal_row) <= 1) {
	    int d = (col - goal_col) * (col - goal_col) + (row - goal_row) * (row - goal_row);
	    if (d < bestd) {
		bestd = d;
		best = cell;
	    }
	    /* A blocked building/terrain target is approached from its nearest
	     * reachable neighbour. Do not flood the entire map seeking that same
	     * blocked target after BFS has already found a shortest approach. */
	    if (!d || !goal_open)
		break;
	}
	for (i = 0; i < 4; i++) {
	    int nc = col + dx[i], nr = row + dy[i];
	    if (nc < 0 || nr < 0 || nc >= MAP_COLS || nr >= MAP_ROWS)
		continue;
	    next = nc + nr * MAP_COLS;
	    if (bfs_seen[next] == bfs_generation || !is_traversable(nc, nr, start))
		continue;
	    bfs_prev[next] = cell;
	    bfs_seen[next] = bfs_generation;
	    bfs_queue[tail++] = next;
	}
    }
    if (best < 0)
	return 0;
    while (bfs_prev[best] != start && bfs_prev[best] >= 0)
	best = bfs_prev[best];
    if (best == start) {
	*next_x = tx;
	*next_y = ty;
	return 1;
    }
    *next_x = (best % MAP_COLS) * 16 + 8;
    *next_y = (best / MAP_COLS) * 16 + 8;
    return 1;
#endif
}

/* Cheap direct routing handles the open map. Run the full BFS only when the
 * straight corridor intersects terrain or a structure. */
static int GBA_HOT
direct_path_clear(int x, int y, int tx, int ty)
{
    /* Movement advances both axes, then the remaining axis. Testing a
     * sampled straight line accepts a different route and can repeatedly
     * steer a worker into a cliff corner without ever requesting BFS. */
    int previous_cell = x / 16 + (y / 16) * MAP_COLS;
    while (x != tx || y != ty) {
	int cell, dx = tx > x ? 1 : tx < x ? -1 : 0;
        int dy = ty > y ? 1 : ty < y ? -1 : 0;
        int step = 0x7fffffff, remaining;
        /* Jump exactly to the next crossed tile boundary or axis endpoint;
         * this visits the same cells as the pixel-by-pixel movement path. */
        if (dx) { step = dx > 0 ? 16 - (x & 15) : (x & 15) + 1;
            remaining = FAST_ABS(tx - x); if (remaining < step) step = remaining; }
        if (dy) { int crossing = dy > 0 ? 16 - (y & 15) : (y & 15) + 1;
            remaining = FAST_ABS(ty - y); if (remaining < crossing) crossing = remaining;
            if (crossing < step) step = crossing; }
        x += dx * step; y += dy * step;
	cell = x / 16 + (y / 16) * MAP_COLS;
	if (cell != previous_cell) {
	    if (!is_traversable(x / 16, y / 16, -1)) return 0;
	    previous_cell = cell;
	}
    }
    return 1;
}

#ifdef SC64_RACES
static int __attribute__((noinline))
unit_step_clear_race_fallback(const Unit *q, int nx, int ny)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (&u[i] != q && u[i].alive && !SC64_TRANSPORT_LOADED(&u[i])) {
            if (u[i].type == U_CHRYSALIS ||
                sc64_race_unit_flying(u[i].type) != sc64_race_unit_flying(q->type))
                continue;
#ifdef SC64_T4_RUNTIME
            if ((sc64_race_t4_actor_flags(u[i].type) &&
                 !(sc64_race_t4_actor_flags(u[i].type)&SC64_T4_ACTOR_COLLIDABLE)) ||
                (sc64_race_t4_actor_flags(q->type) &&
                 !(sc64_race_t4_actor_flags(q->type)&SC64_T4_ACTOR_COLLIDABLE)))
                continue;
#endif
            /* Exact broad-phase reject avoids a distance calculation for
             * distant actors when the per-frame bucket index is unavailable. */
            if (u[i].x - nx >= 6 || nx - u[i].x >= 6 ||
                u[i].y - ny >= 6 || ny - u[i].y >= 6) continue;
            int next_distance = dist2(nx, ny, u[i].x, u[i].y);
            if (next_distance < 36 &&
                next_distance <= dist2(q->x, q->y, u[i].x, u[i].y))
                return 0;
        }
    return 1;
}
#endif

static int GBA_HOT
unit_step_clear(const Unit *q, int nx, int ny)
{
    if (nx < 0 || ny < 0 || nx >= WORLD_W || ny >= WORLD_H) return 0;
#ifdef SC64_RACES
    /* OpenBW bwgame.h unit_can_collide_with(): gathering workers pass
       through ordinary units; static terrain/building checks still apply. */
    if(IS_WORKER(q->type)&&(q->order==ORDER_MINE||q->order==ORDER_GAS))return 1;
#ifdef SC64_T4_RUNTIME
    if (sc64_race_t4_actor_flags(q->type) &&
        !(sc64_race_t4_actor_flags(q->type)&SC64_T4_ACTOR_MOVABLE)) return 1;
#endif
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (sc64_unit_bucket_active) return sc64_unit_bucket_step_clear(q, nx, ny);
#endif
#ifdef SC64_RACES
    return unit_step_clear_race_fallback(q, nx, ny);
#else
    for (int i = 0; i < MAX_UNITS; i++)
	if (&u[i] != q && u[i].alive) {
	    /* Exact broad-phase reject avoids a distance calculation for every
	       distant actor when larger campaign armies are moving. */
	    if (u[i].x - nx >= 6 || nx - u[i].x >= 6 ||
	        u[i].y - ny >= 6 || ny - u[i].y >= 6) continue;
	    int next_distance = dist2(nx, ny, u[i].x, u[i].y);
	    /* Allow a stacked production/starting unit to separate. Otherwise
	     * reserve six pixels of personal space around moving actors. */
	    if (next_distance < 36 && next_distance <= dist2(q->x, q->y, u[i].x, u[i].y))
		return 0;
	}
    return 1;
#endif
}

static void GBA_HOT
#if defined(SC64_T6_RUNTIME) && !defined(GBA_HOST)
__attribute__((optimize("Os")))
#endif
move_toward(Unit * q, int tx, int ty)
{
	int nx, ny, speed = q->type == U_ZERGLING || q->type == U_FIREBAT || q->type == U_RAYNOR ? 2 : 1;
#ifdef SC64_DIAGNOSTIC_ORDERS
    u32 sc64_move_start = gba_clock();
#endif
#ifdef SC64_RACES
    if(q->type==U_VULTURE)speed=2;
#ifdef SC64_T5_RUNTIME
    if(q->type==U_DROPSHIP)speed=2;
#endif
#ifdef SC64_T4_RUNTIME
    if (sc64_race_t4_actor_flags(q->type) &&
        !(sc64_race_t4_actor_flags(q->type)&SC64_T4_ACTOR_MOVABLE)) return;
#endif
    if(sc64_race_unit_flying(q->type)) {
        q->x=limit(q->x+(tx>q->x?speed:tx<q->x?-speed:0),0,WORLD_W-1);
        q->y=limit(q->y+(ty>q->y?speed:ty<q->y?-speed:0),0,WORLD_H-1);
        sc64_unit_bucket_move(q);
        sc64_target_index_move(q);
#ifdef SC64_DIAGNOSTIC_ORDERS
        SC64_ORDER_DIAG_ACCUM(sc64_order_move_cycles, sc64_move_start);
#endif
        return;
    }
#endif
    if (q->path_retry > 0) {
	q->path_retry--;
	q->path_x = q->x;
	q->path_y = q->y;
	q->path_timer = 1;
#ifdef SC64_DIAGNOSTIC_ORDERS
        SC64_ORDER_DIAG_ACCUM(sc64_order_move_cycles, sc64_move_start);
#endif
	return;
    }
    if (q->path_timer <= 0) {
	if (direct_path_clear(q->x, q->y, tx, ty)) {
	    q->path_x = tx;
	    q->path_y = ty;
	} else {
	    if (!path_next_tile(q->x, q->y, tx, ty, &nx, &ny)) {
		q->path_x = q->x;
		q->path_y = q->y;
		q->path_retry = 12;
		q->path_timer = 0;
#ifdef SC64_DIAGNOSTIC_ORDERS
                SC64_ORDER_DIAG_ACCUM(sc64_order_move_cycles, sc64_move_start);
#endif
		return;
	    }
	    q->path_x = nx;
	    q->path_y = ny;
	}
	q->path_timer = 24;
    }
    q->path_timer--;
    nx = q->x < q->path_x ? q->x + speed : q->x > q->path_x ? q->x - speed : q->x;
    ny = q->y < q->path_y ? q->y + speed : q->y > q->path_y ? q->y - speed : q->y;
	if (!is_traversable(nx / 16, ny / 16, (q->x / 16) + (q->y / 16) * MAP_COLS)) {
	q->path_timer = 0;
	q->path_retry = 3;
#ifdef SC64_DIAGNOSTIC_ORDERS
        SC64_ORDER_DIAG_ACCUM(sc64_order_move_cycles, sc64_move_start);
#endif
	return;
    }
    if (!unit_step_clear(q, nx, ny)) {
	static const s8 dx[8] = {1,1,0,-1,-1,-1,0,1};
	static const s8 dy[8] = {0,1,1,1,0,-1,-1,-1};
	int i, best = -1, best_cost = 0x7fffffff;
	int from_cell = q->x / 16 + (q->y / 16) * MAP_COLS;
	/* Local steering avoids repeatedly retrying the same occupied waypoint.
	 * Static terrain still uses BFS; candidates may leave an occupied tile
	 * but may never enter a different blocked cell. */
	for (i = 0; i < 8; i++) {
	    int sx = q->x + dx[i] * speed, sy = q->y + dy[i] * speed;
	    int cost = dist2(sx, sy, q->path_x, q->path_y);
	    if (cost < best_cost && is_traversable(sx / 16, sy / 16, from_cell) &&
		unit_step_clear(q, sx, sy)) { best = i; best_cost = cost; }
	}
	if (best < 0) {
	    q->path_timer = 0; q->path_retry = 6;
#ifdef SC64_DIAGNOSTIC_ORDERS
            SC64_ORDER_DIAG_ACCUM(sc64_order_move_cycles, sc64_move_start);
#endif
            return;
	}
	nx = q->x + dx[best] * speed; ny = q->y + dy[best] * speed;
    }
    q->x = nx;
    q->y = ny;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    sc64_unit_bucket_move(q);
#endif
#ifdef SC64_RACES
    sc64_target_index_move(q);
#endif
    if (FAST_ABS(q->x - q->path_x) <= speed && FAST_ABS(q->y - q->path_y) <= speed)
	q->path_timer = 0;
#ifdef SC64_DIAGNOSTIC_ORDERS
    SC64_ORDER_DIAG_ACCUM(sc64_order_move_cycles, sc64_move_start);
#endif
}

#ifdef SC64_T5_RUNTIME
#include "sc64_transport_adapter.h"
#endif

#ifdef SC64_RACES
static int race_can_attack(int attacker,int target);
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
static int sc64_p3_can_attack_from(int attacker, int target, int owner)
{
    if (attacker == U_SCOURGE) {
        return target >= 0 && target < MAX_UNITS && u[target].alive &&
            sc64_race_unit_flying(u[target].type) && !(u[target].flags & 16) &&
            !invincible_units[target] && owners_hostile(owner, u[target].owner);
    }
    return race_can_attack(attacker, target);
}
#define SC64_CAN_ATTACK_FROM(attacker, target, owner) \
    sc64_p3_can_attack_from((attacker), (target), (owner))
#elif defined(SC64_T4_RUNTIME)
static int sc64_t4_can_attack_from(int attacker, int target, int owner)
{
    (void)owner;
#ifdef SC64_T6_RUNTIME
    if (target >= 0 && target < MAX_UNITS && race_t6_unit_burrowed(target)) return 0;
#endif
    /* race_can_attack already applies the T4 hidden and intrinsic-immunity
     * rules, so keep only the T6-specific burrow rejection here. */
    return race_can_attack(attacker, target);
}
#define SC64_CAN_ATTACK_FROM(attacker, target, owner) \
    sc64_t4_can_attack_from((attacker), (target), (owner))
#else
#define SC64_CAN_ATTACK_FROM(attacker, target, owner) \
    race_can_attack((attacker), (target))
#endif
static void
sc64_target_hostility_snapshot(void)
{
    int owner, other;
    target_hostile_masks_valid = 0;
    if (!target_lists_valid || !sc64_target_index_active) return;
    for (owner = 0; owner < 5; ++owner) {
        u8 mask = 0;
        for (other = 0; other < 5; ++other)
            if (owners_hostile(owner, other)) mask |= (u8)(1u << other);
        target_hostile_mask[owner] = mask;
    }
    target_hostile_masks_valid = 1;
}
static int GBA_HOT __attribute__((noinline))
#if defined(SC64_T6_RUNTIME) && !defined(GBA_HOST)
__attribute__((optimize("Os")))
#endif
sc64_target_nearest(int x, int y, int owner, int max_distance, int attacker)
{
    int best = -1, best_distance = max_distance * max_distance, d;
    int cols = (WORLD_W + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int rows = (WORLD_H + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int col0 = (x - max_distance) / SC64_TARGET_CELL_SIZE;
    int row0 = (y - max_distance) / SC64_TARGET_CELL_SIZE;
    int col1 = (x + max_distance) / SC64_TARGET_CELL_SIZE;
    int row1 = (y + max_distance) / SC64_TARGET_CELL_SIZE;
    u32 col_mask, row_mask;
    int p, col, row;
    u8 hostile_mask;
    if (col0 < 0) col0 = 0;
    if (row0 < 0) row0 = 0;
    if (col1 >= cols) col1 = cols - 1;
    if (row1 >= rows) row1 = rows - 1;
    col_mask = (0xffffffffu >> (31 - col1)) & (0xffffffffu << col0);
    row_mask = (0xffffffffu >> (31 - row1)) & (0xffffffffu << row0);
    if (target_hostile_masks_valid && owner >= 0 && owner < 5) {
        hostile_mask = target_hostile_mask[owner];
    } else {
        hostile_mask = 0;
        for (p = 0; p < 5; ++p)
            if (owners_hostile(owner, p)) hostile_mask |= (u8)(1u << p);
    }
    for (p = 0; hostile_mask; ++p, hostile_mask >>= 1) {
        if (!(hostile_mask & 1u)) continue;
        if (!(sc64_target_owner_columns[p] & col_mask) ||
            !(sc64_target_owner_rows[p] & row_mask)) continue;
        for (row = row0; row <= row1; ++row)
            for (col = col0; col <= col1; ++col) {
                int cell = col + row * SC64_TARGET_GRID_COLS;
                int entity;
                for (entity = sc64_target_head[p][cell]; entity != SC64_TARGET_NONE;
                     entity = sc64_target_next[entity]) {
                    int candidate_x, candidate_y;
                    if (entity < MAX_UNITS) {
                        if (!u[entity].alive) continue;
                        candidate_x = u[entity].x; candidate_y = u[entity].y;
                    } else {
                        int bi = entity - MAX_UNITS;
                        if (!b[bi].alive) continue;
                        candidate_x = b[bi].x; candidate_y = b[bi].y;
                    }
                    if (FAST_ABS(x - candidate_x) >= max_distance ||
                        FAST_ABS(y - candidate_y) >= max_distance) continue;
                    d = dist2(x, y, candidate_x, candidate_y);
                    if (d > best_distance ||
                        (d == best_distance && (best < 0 || entity >= best)) ||
                        !SC64_CAN_ATTACK_FROM(attacker, entity, owner)) continue;
                    best_distance = d;
                    best = entity;
                }
            }
    }
    return best;
}
static int __attribute__((noinline))
nearest_enemy_scan(int x, int y, int owner, int max_distance, int attacker)
{
    int i, best = -1, best_distance = max_distance * max_distance, d;
    if (target_lists_valid) {
        int p;
        for (p = 0; p < 5; ++p) {
            if (!owners_hostile(owner, p)) continue;
            for (i = target_unit_head[p]; i >= 0; i = target_unit_next[i]) {
                if (!u[i].alive || FAST_ABS(x-u[i].x)>=max_distance || FAST_ABS(y-u[i].y)>=max_distance)continue;
                if(!SC64_CAN_ATTACK_FROM(attacker,i,owner))continue;
                d = dist2(x, y, u[i].x, u[i].y);
                if (d < best_distance || (best >= 0 && d == best_distance && i < best)) {
                    best_distance = d;
                    best = i;
                }
            }
        }
        for (p = 0; p < 5; ++p) {
            if (!owners_hostile(owner, p)) continue;
            for (i = target_building_head[p]; i >= 0; i = target_building_next[i]) {
                int candidate = MAX_UNITS + i;
                if (!b[i].alive || FAST_ABS(x-b[i].x)>=max_distance || FAST_ABS(y-b[i].y)>=max_distance)continue;
                if(!SC64_CAN_ATTACK_FROM(attacker,MAX_UNITS+i,owner))continue;
                d = dist2(x, y, b[i].x, b[i].y);
                if (d < best_distance || (best >= 0 && d == best_distance && candidate < best)) {
                    best_distance = d;
                    best = candidate;
                }
            }
        }
        return best;
    }
    for (i = 0; i < MAX_UNITS; i++) {
	if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i])
	    || FAST_ABS(x - u[i].x) >= max_distance ||
	    FAST_ABS(y - u[i].y) >= max_distance || !owners_hostile(owner, u[i].owner)
                    || !SC64_CAN_ATTACK_FROM(attacker,i,owner)
                    )
	    continue;
	d = dist2(x, y, u[i].x, u[i].y);
	if (d < best_distance) {
	    best_distance = d;
	    best = i;
	}
    }
    for (i = 0; i < MAX_BUILDINGS; i++) {
	if (!b[i].alive
	    || FAST_ABS(x - b[i].x) >= max_distance ||
	    FAST_ABS(y - b[i].y) >= max_distance || !owners_hostile(owner, b[i].owner)
                    || !SC64_CAN_ATTACK_FROM(attacker,MAX_UNITS+i,owner)
                    )
	    continue;
	d = dist2(x, y, b[i].x, b[i].y);
	if (d < best_distance) {
	    best_distance = d;
	    best = MAX_UNITS + i;
	}
    }
    return best;
}
#endif

static int GBA_HOT
nearest_enemy_typed(int x, int y, int owner, int max_distance, int attacker)
{
    int result;
#ifdef SC64_DIAGNOSTIC_ORDERS
    u32 sc64_query_start = gba_clock();
#endif
#if defined(SC64_UNARMED_QUERY_SKIP) && !defined(SC64_RACES)
#error "SC64_UNARMED_QUERY_SKIP requires SC64_RACES"
#endif
#ifdef SC64_UNARMED_QUERY_SKIP
    /* Nothing in the ground or air weapon tables can damage a candidate.
     * Keep attacker == -1 as the historical wildcard and leave invalid IDs
     * to the ordinary checked query path. This deliberately uses damage_vs
     * for both classes so the P3 air-only Scourge is not skipped. */
    if (attacker >= 0 && attacker < SC_RU_COUNT &&
        sc64_race_unit_damage_vs(attacker, 0) <= 0 &&
        sc64_race_unit_damage_vs(attacker, 1) <= 0) {
        result = -1;
        goto sc64_query_done;
    }
#endif
#ifdef SC64_RACES
    if (target_lists_valid && sc64_target_index_active && max_distance > 0 &&
        x >= 0 && y >= 0 && x < WORLD_W && y < WORLD_H)
        result = sc64_target_nearest(x, y, owner, max_distance, attacker);
    else
        result = nearest_enemy_scan(x, y, owner, max_distance, attacker);
#else
    int i, best = -1, best_distance = max_distance * max_distance, d;
    (void)attacker;
    if (target_lists_valid) {
        int p;
        for (p = 0; p < 5; ++p) {
            if (!owners_hostile(owner, p)) continue;
            for (i = target_unit_head[p]; i >= 0; i = target_unit_next[i]) {
                if (!u[i].alive || FAST_ABS(x-u[i].x)>=max_distance || FAST_ABS(y-u[i].y)>=max_distance)continue;
                d = dist2(x, y, u[i].x, u[i].y);
                if (d < best_distance || (best >= 0 && d == best_distance && i < best)) {
                    best_distance = d; best = i;
                }
            }
        }
        for (p = 0; p < 5; ++p) {
            if (!owners_hostile(owner, p)) continue;
            for (i = target_building_head[p]; i >= 0; i = target_building_next[i]) {
                int candidate = MAX_UNITS + i;
                if (!b[i].alive || FAST_ABS(x-b[i].x)>=max_distance || FAST_ABS(y-b[i].y)>=max_distance)continue;
                d = dist2(x, y, b[i].x, b[i].y);
                if (d < best_distance || (best >= 0 && d == best_distance && candidate < best)) {
                    best_distance = d; best = candidate;
                }
            }
        }
    } else {
        for (i = 0; i < MAX_UNITS; i++) {
            if (!u[i].alive || FAST_ABS(x-u[i].x)>=max_distance ||
                FAST_ABS(y-u[i].y)>=max_distance || !owners_hostile(owner,u[i].owner)) continue;
            d = dist2(x,y,u[i].x,u[i].y);
            if (d < best_distance) { best_distance = d; best = i; }
        }
        for (i = 0; i < MAX_BUILDINGS; i++) {
            if (!b[i].alive || FAST_ABS(x-b[i].x)>=max_distance ||
                FAST_ABS(y-b[i].y)>=max_distance || !owners_hostile(owner,b[i].owner)) continue;
            d = dist2(x,y,b[i].x,b[i].y);
            if (d < best_distance) { best_distance = d; best = MAX_UNITS+i; }
        }
    }
    result = best;
#endif
#ifdef SC64_UNARMED_QUERY_SKIP
sc64_query_done:
#endif
#ifdef SC64_DIAGNOSTIC_ORDERS
    SC64_ORDER_DIAG_ACCUM(sc64_order_query_cycles, sc64_query_start);
#endif
    return result;
}

static int nearest_enemy(int x,int y,int owner,int distance) {
    return nearest_enemy_typed(x,y,owner,distance,-1);
}

static int
target_x(int target)
{
    return target < MAX_UNITS ? u[target].x : b[target - MAX_UNITS].x;
}
static int
target_y(int target)
{
    return target < MAX_UNITS ? u[target].y : b[target - MAX_UNITS].y;
}
static int
target_alive(int target)
{
#ifdef SC64_T6_RUNTIME
    if (target >= 0 && target < MAX_UNITS && race_t6_unit_burrowed(target)) return 0;
#endif
#ifdef SC64_T5_RUNTIME
    if (target >= 0 && target < MAX_UNITS && SC64_TRANSPORT_LOADED(&u[target]))
        return 0;
#endif
#ifdef SC64_T4_RUNTIME
    if (target >= 0 && target < MAX_UNITS && u[target].alive &&
        (sc64_race_t4_actor_flags(u[target].type)&SC64_T4_ACTOR_HIDDEN)) return 0;
#endif
    return target >= 0 && (target < MAX_UNITS ? u[target].alive : b[target - MAX_UNITS].alive);
}

static void
add_tracer(int x0, int y0, int x1, int y1, int color)
{
    int i;
    for (i = 0; i < MAX_TRACERS; i++)
	if (!tracers[i].life) {
	    tracers[i].x0 = x0;
	    tracers[i].y0 = y0;
	    tracers[i].x1 = x1;
	    tracers[i].y1 = y1;
	    tracers[i].life = 4;
	    tracers[i].color = color;
	    return;
	}
}

static void
hit_target(int target, int damage, int source_type, int source_owner)
{
    int i, tx, ty;
#ifdef SC64_T6_RUNTIME
    if (target >= 0 && target < MAX_UNITS && race_t6_unit_burrowed(target)) return;
#endif
#ifdef SC64_T5_RUNTIME
    if (target >= 0 && target < MAX_UNITS && SC64_TRANSPORT_LOADED(&u[target]))
        return;
#endif
#ifdef SC64_RACES
#ifdef SC64_T4_RUNTIME
    if (target >= 0 && target < MAX_UNITS &&
        sc64_race_t4_unit_intrinsically_invincible(u[target].type)) return;
    if (target >= MAX_UNITS && target < MAX_UNITS + MAX_BUILDINGS &&
        sc64_race_t4_building_intrinsically_invincible(b[target-MAX_UNITS].type)) return;
#endif
    int flags=target<MAX_UNITS?u[target].flags:b[target-MAX_UNITS].flags;
    if(flags&16)return;
    int *shield=target<MAX_UNITS?&u[target].shield:&b[target-MAX_UNITS].shield;
    int absorbed=*shield<damage?*shield:damage;*shield-=absorbed;damage-=absorbed;
    if(!damage)return;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    if (target < MAX_UNITS ? invincible_units[target] : invincible_buildings[target - MAX_UNITS]) return;
#endif
    if (target < MAX_UNITS) {
	tx = u[target].x;
	ty = u[target].y;
#ifdef SC64_T4_RUNTIME
        if (source_type == U_GHOST
#ifdef SC64_T5_RUNTIME
            || source_type == U_KERRIGAN || source_type == U_RAYNOR_VULTURE
#endif
            ) {
            int armor = u[target].armor +
                (u[target].owner == SIDE ? armor_level : 0);
            int raw = u[target].raw_type >= 0 ? u[target].raw_type :
                      sc64_race_raw_unit(u[target].type);
            damage = sc64_race_t4_ghost_hp_damage(raw, damage, armor);
        }
#ifdef SC64_T5_RUNTIME
        else if (source_type == U_SPIDER_MINE) {
            int raw = u[target].raw_type >= 0 ? u[target].raw_type :
                      sc64_race_raw_unit(u[target].type);
            damage = sc64_t5_explosive_hp_damage(raw, damage,
                     u[target].armor + (u[target].owner == SIDE ? armor_level : 0));
        }
#endif
        else
#endif
	damage -= u[target].armor + (u[target].owner == SIDE ? armor_level : 0);
#ifndef SC64_T4_RUNTIME
	if (damage < 1)
	    damage = 1;
#else
#ifdef SC64_T5_RUNTIME
        if (damage < 1) damage = 1;
#else
        if (source_type != U_GHOST && damage < 1) damage = 1;
#endif
#endif
	u[target].hp -= damage;
	if (u[target].hp <= 0) {
#ifdef SC64_TERRAN_LATE_RUNTIME
            if (u[target].alive && race_late_active()) {
                race_late_record_actor_death(source_owner, -1, u[target].owner,
                                             u[target].raw_owner, u[target].raw_type, 0);
                if (u[target].type == U_DROPSHIP)
                    for (int cargo = 0; cargo < MAX_UNITS; ++cargo)
                        if (u[cargo].alive && u[cargo].order == ORDER_TRANSPORTED &&
                            u[cargo].target == target)
                            race_late_record_actor_death(source_owner, -1, u[cargo].owner,
                                                         u[cargo].raw_owner,
                                                         u[cargo].raw_type, 0);
            }
#endif
	    if (u[target].owner == SIDE)
		supply_used -= unit_supply(u[target].type);
	    u[target].alive = 0;
#ifdef SC64_T5_RUNTIME
            if (u[target].type == U_DROPSHIP) race_transport_destroy_cargo(target);
#endif
	}
	if (source_type == U_FIREBAT) {
	    for (i = 0; i < MAX_UNITS; i++)
		if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner != source_owner && dist2(tx, ty, u[i].x, u[i].y) < 12 * 12) {
#ifdef SC64_T6_RUNTIME
                    if (race_t6_active() && (!owners_hostile(source_owner, u[i].owner) ||
                        race_t6_unit_burrowed(i))) continue;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
                    if (race_late_active() &&
                        (!owners_hostile(source_owner, u[i].owner) ||
                         invincible_units[i] || (u[i].flags & 16) ||
                         (sc64_terran_late_unit_actor_flags(u[i].type) &
                          SC64_LATE_ACTOR_INVINCIBLE))) continue;
#endif
		    u[i].hp -= 3;
		    if (u[i].hp <= 0) {
#ifdef SC64_TERRAN_LATE_RUNTIME
                        if (race_late_active())
                            race_late_record_actor_death(source_owner, -1, u[i].owner,
                                                         u[i].raw_owner, u[i].raw_type, 0);
#endif
			if (u[i].owner == SIDE)
			    supply_used -= unit_supply(u[i].type);
			u[i].alive = 0;
		    }
		}
	}
    } else {
	Building *q = &b[target - MAX_UNITS];
	tx = q->x;
	ty = q->y;
	#ifdef SC64_RACES
#ifdef SC64_T4_RUNTIME
    if (source_type == U_GHOST
#ifdef SC64_T5_RUNTIME
        || source_type == U_KERRIGAN || source_type == U_RAYNOR_VULTURE
#endif
        ) {
        int raw = q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
        damage = sc64_race_t4_ghost_hp_damage(raw, damage,
            sc64_race_building_armor(q->type));
    } else
#endif
    damage-=sc64_race_building_armor(q->type);
    q->hp-=damage>0?damage:1;
#else
    q->hp -= damage > 1 ? damage - 1 : 1;
#endif
	if (q->hp <= 0) {
#ifdef SC64_TERRAN_LATE_RUNTIME
            if (q->alive && race_late_active())
                race_late_record_actor_death(source_owner, -1, q->owner,
                                             q->raw_owner, q->raw_type, 1);
#endif
#ifdef SC64_T6_RUNTIME
            if (q->alive && race_t6_active())
                race_t6_building_died(q->owner, q->raw_owner);
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
            if(q->alive && !race_mode && race_selected_mission==6 &&
               q->owner==ENEMY && source_owner==SIDE && race_t3_buildings_killed<65535u)
                ++race_t3_buildings_killed;
#endif
	    if (q->type == B_COMMAND && q->owner == SIDE) {
#ifndef SC64_IMPORTED_MISSION
		lost = 1;
#endif
	    }
	    if (q->type == B_DEPOT && q->owner == SIDE && !q->under_construction)
		supply_cap -= 16;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
            if (q->type == B_COMMAND && q->owner == SIDE && !q->under_construction) supply_cap -= 20;
#endif
	    q->alive = 0;
	    invalidate_path_grid();
	}
    }
    (void)tx;
    (void)ty;
    (void)source_type;
    (void)source_owner;
}

static int
save_game(void)
{
#ifdef SC64_RACES
    return race_save_game();
#else
    data.version = SAVE_VERSION;
    memcpy(data.units, u, sizeof(u));
    memcpy(data.buildings, b, sizeof(b));
    memcpy(data.minerals_map, m, sizeof(m));
    memcpy(data.tracer_data, tracers, sizeof(tracers));
    memcpy(data.explored_map, explored, sizeof(explored));
    data.minerals = minerals;
    data.gas = gas;
    data.supply_used = supply_used;
    data.supply_cap = supply_cap;
    data.frame = frame;
    data.cx = cx;
    data.cy = cy;
    data.enemy_timer = enemy_timer;
    data.wave = wave;
    data.weapons_level = weapons_level;
    data.armor_level = armor_level;
    data.weapons_research = weapons_research;
    data.armor_research = armor_research;
    data.train_type = train_type;
    data.selected_building = selected_building;
    data.seed = seed;
#ifdef SC64_IMPORTED_MISSION
    data.mission_objective_stage = mission_objective_stage;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    data.campaign_mission = campaign_mission;
    data.campaign_completed = campaign_completed;
#endif
#ifdef SC64_BACKWATER
    data.backwater = backwater;
    memcpy(data.invincible_units, invincible_units, sizeof(invincible_units));
    memcpy(data.invincible_buildings, invincible_buildings, sizeof(invincible_buildings));
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    for (int i = 0; i < 4; i++) { data.geyser_x[i] = geysers[i].x; data.geyser_y[i] = geysers[i].y; }
    data.geyser_count = geyser_count;
#endif
#ifdef SC64_SKIRMISH
    memcpy(data.bots, skirmish_ai, sizeof(skirmish_ai));
    data.bot_count = skirmish_bots;
#endif
    save_write(&data, sizeof(data), SAVE_VERSION);
    toast(7);
    return 1;
#endif
}

static int
load_game(void)
{
#ifdef SC64_RACES
    return race_load_game();
#else
    int i, j, computed_supply = 0, computed_cap =
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
        0;
#else
        20;
#endif
    if (!save_read(&data, sizeof(data), SAVE_VERSION) || data.version != SAVE_VERSION)
	return 0;
#ifdef SC64_IMPORTED_MISSION
    if (data.mission_objective_stage < 0 || data.mission_objective_stage > 1)
	return 0;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    if (data.campaign_mission < 0 || data.campaign_mission >= SC64_CAMPAIGN_COUNT ||
	data.campaign_completed < 0 || data.campaign_completed > 3) return 0;
#endif
    if (data.minerals < 0 || data.minerals > 100000 || data.gas < 0 || data.gas > 100000 || data.supply_cap < 0 || data.supply_cap > 400 || data.frame < 0)
	return 0;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (data.geyser_count < 0 || data.geyser_count > 4) return 0;
#endif
#ifdef SC64_SKIRMISH
    if (data.bot_count < 1 || data.bot_count > 3) return 0;
#endif
    for (i = 0; i < MAX_UNITS; i++) {
	if (data.units[i].alive) {
	    if (data.units[i].type < U_SCV || data.units[i].type > U_RAYNOR || data.units[i].owner < SIDE || data.units[i].owner > 4 || data.units[i].hp <= 0 || data.units[i].x < 0 || data.units[i].x >= WORLD_W || data.units[i].y < 0 || data.units[i].y >= WORLD_H || data.units[i].order < ORDER_IDLE || data.units[i].order > ORDER_GAS || data.units[i].path_retry < 0 || data.units[i].path_retry > 100)
		return 0;
	    if (data.units[i].owner == SIDE)
		computed_supply += unit_supply(data.units[i].type);
	}
    }
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (data.buildings[i].alive) {
    if (data.buildings[i].type < B_COMMAND || data.buildings[i].type > B_CREEP_COLONY ||
		data.buildings[i].queue_count < 0 || data.buildings[i].queue_count > 5 || data.buildings[i].queue_head < 0 || data.buildings[i].queue_head > 4 || data.buildings[i].queue_progress < 0 || data.buildings[i].build_progress < 0 ||
		data.buildings[i].hp <= 0 || data.buildings[i].x < 0 || data.buildings[i].x >= WORLD_W ||
		data.buildings[i].y < 0 || data.buildings[i].y >= WORLD_H ||
		(data.buildings[i].owner < SIDE || data.buildings[i].owner >
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
                4
#else
                ENEMY
#endif
                ))
		return 0;
	    for (j = 0; j < data.buildings[i].queue_count; j++) {
		int queued = data.buildings[i].queue[(data.buildings[i].queue_head + j) % 5];
		if (queued < U_SCV || queued > U_MEDIC)
		    return 0;
	    }
	    if (data.buildings[i].owner == SIDE && data.buildings[i].type == B_DEPOT && !data.buildings[i].under_construction)
		computed_cap += 16;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
            if (data.buildings[i].owner == SIDE && data.buildings[i].type == B_COMMAND && !data.buildings[i].under_construction) computed_cap += 20;
#endif
	}
    if (computed_cap != data.supply_cap || computed_supply > data.supply_cap ||
	data.train_type < U_MARINE || data.train_type > U_MEDIC ||
	data.weapons_level < 0 || data.weapons_level > 2 || data.armor_level < 0 || data.armor_level > 2)
	return 0;
    memcpy(u, data.units, sizeof(u));
    memcpy(b, data.buildings, sizeof(b));
    invalidate_path_grid();
    memcpy(m, data.minerals_map, sizeof(m));
    memcpy(tracers, data.tracer_data, sizeof(tracers));
    memcpy(explored, data.explored_map, sizeof(explored));
#ifdef SC64_TERRAN_CAMPAIGN
    campaign_mission = data.campaign_mission;
    campaign_completed = data.campaign_completed;
#endif
#ifdef SC64_IMPORTED_MISSION
    sc64_visibility_reset();
#endif
    minerals = data.minerals;
    gas = data.gas;
    supply_used = computed_supply;
    supply_cap = data.supply_cap;
    frame = data.frame;
    cx = data.cx;
    cy = data.cy;
    enemy_timer = data.enemy_timer;
    wave = data.wave;
    weapons_level = data.weapons_level;
    armor_level = data.armor_level;
    weapons_research = data.weapons_research;
    armor_research = data.armor_research;
    train_type = data.train_type;
    selected_building = -1;
    seed = data.seed;
#ifdef SC64_IMPORTED_MISSION
    mission_objective_stage = data.mission_objective_stage;
#endif
#ifdef SC64_BACKWATER
    backwater = data.backwater;
    memcpy(invincible_units, data.invincible_units, sizeof(invincible_units));
    memcpy(invincible_buildings, data.invincible_buildings, sizeof(invincible_buildings));
    backwater_message_ticks = 0;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    geyser_count = data.geyser_count;
    for (int k = 0; k < 4; k++) { geysers[k].x = data.geyser_x[k]; geysers[k].y = data.geyser_y[k]; }
#endif
#ifdef SC64_SKIRMISH
    memcpy(skirmish_ai, data.bots, sizeof(skirmish_ai)); skirmish_bots = data.bot_count;
#endif
    mode = MODE_NORMAL;
    menu_open = 0;
    selected_building = data.selected_building >= 0 && data.selected_building < MAX_BUILDINGS && b[data.selected_building].alive ? data.selected_building : -1;
    screen = SCREEN_PLAY;
    won = lost = 0;
    reset_selection_gestures();
    update_exploration();
    toast(0);
    return 1;
#endif
}

#ifdef SC64_IMPORTED_MISSION
static int
imported_unit_type(int raw)
{
    if (raw == 0) return U_MARINE;
    if (raw == 7) return U_SCV;
    if (raw == 19) return U_RAYNOR;
#ifdef SC64_T4_RUNTIME
    if (raw == 20) return U_RAYNOR_MARINE;
#endif
    if (raw == 37) return U_ZERGLING;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (raw == 32) return U_FIREBAT;
    if (raw == 38) return U_HYDRALISK;
#endif
    return -1;
}
static int
imported_building_type(int raw)
{
    if (raw == 106) return B_COMMAND;
    if (raw == 109) return B_DEPOT;
    if (raw == 111) return B_BARRACKS;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (raw == 110) return B_REFINERY;
    if (raw == 112) return B_ACADEMY;
    if (raw == 125) return B_BUNKER;
    if (raw == 130) return B_INFESTED_COMMAND;
    if (raw == 143) return B_CREEP_COLONY;
#endif
    return -1;
}
static void
spawn_imported_mission(void)
{
    int i, player_x = 0, player_y = 0, player_n = 0;
#ifdef SC64_TERRAN_CAMPAIGN
    const Sc64ImportedUnit *units = sc64_campaign_maps[campaign_mission].units;
    int unit_count = sc64_campaign_maps[campaign_mission].unit_count;
#else
    const Sc64ImportedUnit *units = sc64_units;
    int unit_count = SC64_IMPORTED_UNIT_COUNT;
#endif
    for (i = 0; i < unit_count; i++) {
	const Sc64ImportedUnit *src = &units[i];
	int owner, type = -1, slot;
	int x = src->x_px, y = src->y_px;
        if (src->source_type == 214) continue;
#ifdef SC64_BACKWATER
        if (src->source_type == 188) {
            if (geyser_count < 4) { geysers[geyser_count].x = x; geysers[geyser_count++].y = y; }
            else mission_unsupported_count++;
            continue;
        }
#endif
	if (x >= WORLD_W || y >= WORLD_H) {
	    mission_unsupported_count++;
	    continue;
	}
	if (src->source_type == SC64_RAW_MINERAL_FIELD_1 ||
	    src->source_type == SC64_RAW_MINERAL_FIELD_2 ||
	    src->source_type == SC64_RAW_MINERAL_FIELD_3) {
	    slot = -1;
	    for (int j = 0; j < MAX_MINERALS; j++)
		if (m[j].amount <= 0) { slot = j; break; }
	    if (slot < 0) { mission_unsupported_count++; continue; }
	    m[slot].x = x; m[slot].y = y;
	    m[slot].amount = src->resources ? src->resources : 1500;
	    m[slot].style = src->source_type;
	    continue;
	}
	owner = src->source_type == SC64_RAW_RAYNOR_VULTURE ? NEUTRAL :
#ifdef SC64_BACKWATER
            (src->owner == 3 ? RESCUABLE :
            (src->owner == 1 ? SIDE : (src->owner == 4 ? ENEMY : -1)));
#else
	    (src->owner == 1 ? SIDE : (src->owner == 4 ? ENEMY : -1));
#endif
	if (owner < 0) { mission_unsupported_count++; continue; }
        type = imported_unit_type(src->source_type);
	if (type >= 0) {
	    slot = spawn_unit(owner, type, x, y);
	    if (slot < 0) { mission_unsupported_count++; continue; }
	    if (src->hp_percent && src->hp_percent <= 100)
		u[slot].hp = u[slot].maxhp * src->hp_percent / 100;
	    if (owner == ENEMY) {
		u[slot].order = ORDER_ATTACK_MOVE;
		u[slot].tx = x; u[slot].ty = y;
	    }
	    if (owner == SIDE) {
		player_x += x; player_y += y; player_n++;
	    }
	    continue;
	}
        type = imported_building_type(src->source_type);
        if (type >= 0) {
	    int btype = type;
	    slot = spawn_build(owner, btype, x, y, 0);
	    if (slot < 0) { mission_unsupported_count++; continue; }
	    if (src->hp_percent && src->hp_percent <= 100)
		b[slot].hp = b[slot].maxhp * src->hp_percent / 100;
	    if (owner == SIDE && btype == B_DEPOT) supply_cap += 16;
	    continue;
	}
	mission_unsupported_count++;
    }
    if (player_n) { cx = player_x / player_n; cy = player_y / player_n; }
}
#endif

#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
#include "sc64_backwater_adapter.h"
#endif

static void
init_state(void)
{
#ifdef SC64_RACES
    reset_selection_gestures();
    mode=MODE_NORMAL;menu=menu_open=menu_cursor=help=0;
    screen=SCREEN_PLAY;selected_building=-1;result_timer=0;
    toast_timer=toast_id=enemy_timer=wave=0;
    weapons_level=armor_level=weapons_research=armor_research=0;
    mission_objective_stage=mission_unsupported_count=0;
    train_type=race_mode?(race_human==R_TER?U_SCV:race_human==R_ZERG?U_DRONE:U_PROBE):
        (race_selected_mission<2?U_DRONE:U_PROBE);
    SC64_GRID_CLEAR(explored);SC64_GRID_CLEAR(visible);
    memset(tracers,0,sizeof(tracers));
    sc64_visibility_reset();
    race_campaign_init();
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
    return;
#endif
    int i;
    reset_selection_gestures();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    invalidate_path_grid();
    memset(tracers, 0, sizeof(tracers));
    SC64_GRID_CLEAR(explored);
    SC64_GRID_CLEAR(visible);
#ifdef SC64_IMPORTED_MISSION
    sc64_visibility_reset();
    geyser_count = 0;
    mission_unsupported_count = 0;
#else
    geyser_count = 2;
#endif
    for (i = 0; i < MAX_MINERALS; i++) {
	m[i].amount = 1500;
	m[i].x = 42 + (i % 4) * 18;
	m[i].y = 95 + (i / 4) * 18;
#ifdef SC64_IMPORTED_MISSION
	m[i].style = 0;
#endif
    }
#ifdef SC64_IMPORTED_MISSION
    for (i = 0; i < MAX_MINERALS; i++) m[i].amount = 0;
    minerals = 0;
    gas = 0;
    supply_used = 0;
    supply_cap = 20;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES) || defined(SC64_TERRAN_CAMPAIGN)
    supply_cap = 0;
#endif
    mission_objective_stage = 0;
    frame = 0;
    mode = MODE_NORMAL;
    menu = 0; menu_open = 0; menu_cursor = 0; help = 0;
    won = lost = 0; result_timer = 0;
    cx = 512; cy = 512; camx = camy = 0;
    toast_timer = 0; toast_id = 0;
    enemy_timer = 0;
    wave = 0;
    weapons_level = armor_level = 0;
    weapons_research = armor_research = 0;
    selected_building = -1;
    train_type = U_MARINE;
    seed = 0x51a7c0deu;
    screen = SCREEN_PLAY;
#ifdef SC64_SKIRMISH
    skirmish_init();
#else
    spawn_imported_mission();
#ifdef SC64_BACKWATER
#ifdef SC64_TERRAN_CAMPAIGN
    if (campaign_mission == 1) backwater_init();
    else {
        memset(&backwater, 0, sizeof(backwater));
        memset(invincible_units, 0, sizeof(invincible_units));
        memset(invincible_buildings, 0, sizeof(invincible_buildings));
        backwater_message_ticks = 0;
    }
#else
    backwater_init();
#endif
#endif
#endif
    if (mission_unsupported_count) toast(14);
    update_exploration();
    return;
#endif
    minerals = 500;
    gas = 0;
    supply_used = 0;
    supply_cap = 20;
    frame = 0;
    mode = MODE_NORMAL;
    menu = 0;
    menu_open = 0;
    menu_cursor = 0;
    help = 0;
    won = 0;
    lost = 0;
    result_timer = 0;
    cx = 70;
    cy = 90;
    camx = 0;
    camy = 0;
    toast_timer = 0;
    enemy_timer = 7200;
    wave = 0;
    weapons_level = 0;
    armor_level = 0;
    weapons_research = 0;
    armor_research = 0;
    selected_building = -1;
    train_type = U_MARINE;
    seed = 0x51a7c0deu;
    screen = SCREEN_PLAY;
    spawn_build(SIDE, B_COMMAND, 76, 84, 0);
    spawn_build(ENEMY, B_HATCHERY, 314, 48, 0);
    /* Mission-start outpost: the opening fight is already underway, so this
     * scenario includes the supply infrastructure and a small guard squad. */
    spawn_build(SIDE, B_DEPOT, 120, 136, 0);
    spawn_build(SIDE, B_BARRACKS, 120, 48, 0);
    supply_cap += 16;
    spawn_unit(SIDE, U_SCV, 98, 82);
    spawn_unit(SIDE, U_SCV, 99, 91);
    spawn_unit(SIDE, U_SCV, 96, 99);
    spawn_unit(SIDE, U_SCV, 85, 103);
    spawn_unit(SIDE, U_SCV, 68, 105);
    for (i = 0; i < 8; i++) {
	int guard = spawn_unit(SIDE, U_MARINE, 86 + i * 9, 67);
	if (guard >= 0) {
	    u[guard].order = ORDER_ATTACK_MOVE;
	    u[guard].tx = 76;
	    u[guard].ty = 84;
	}
    }
    spawn_unit(ENEMY, U_ZERGLING, 290, 63);
    spawn_unit(ENEMY, U_ZERGLING, 290, 75);
    spawn_unit(ENEMY, U_HYDRALISK, 300, 88);
    update_exploration();
}

#ifdef SC64_IMPORTED_MISSION
static void
update_imported_mission(void)
{
#ifdef SC64_RACES
    if(race_mode)race_tick_ai();else race_campaign_tick();return;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    if (campaign_mission == 1) { backwater_update(); return; }
#endif
#if defined(SC64_BACKWATER) && !defined(SC64_TERRAN_CAMPAIGN)
    backwater_update();
#elif defined(SC64_SKIRMISH)
    skirmish_update();
#else
    int i, j;
    if (!mission_objective_stage && frame >= 30) {
	/* Wasteland trigger 3 grants 40 minerals and switches to the find-Raynor
	 * objective after one game second (30 simulation updates). */
	minerals = 40;
	mission_objective_stage = 1;
    }
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].type == U_RAYNOR && u[i].owner == NEUTRAL) {
	    for (j = 0; j < MAX_UNITS; j++)
		if (u[j].alive && u[j].owner == SIDE && dist2(u[i].x, u[i].y, u[j].x, u[j].y) <= 20 * 20) {
		    u[i].owner = SIDE;
		    supply_used += unit_supply(U_RAYNOR);
		    u[i].selected = 0;
		    toast(0);
		    break;
		}
	}
#endif
}

static void
check_imported_mission_outcome(void)
{
#ifdef SC64_RACES
    if(race_mode)race_skirmish_outcome();
    return;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
    if (campaign_mission == 1) return;
#endif
#if defined(SC64_BACKWATER) && !defined(SC64_TERRAN_CAMPAIGN)
    /* Backwater's source trigger sequences own victory and Raynor defeat. */
#elif defined(SC64_SKIRMISH)
    skirmish_outcome();
#else
    int i, raynor_alive = 0, raynor_found = 0, marines = 0, barracks = 0;
    int command = 0, men = 0;
    for (i = 0; i < MAX_UNITS; i++) if (u[i].alive) {
	if (u[i].owner == SIDE) men++;
	if (u[i].type == U_RAYNOR) {
	    raynor_alive = 1;
	    raynor_found = u[i].owner == SIDE;
	} else if (u[i].owner == SIDE && u[i].type == U_MARINE)
	    marines++;
    }
    for (i = 0; i < MAX_BUILDINGS; i++) {
	if (b[i].alive && b[i].owner == SIDE) {
	    if (b[i].type == B_BARRACKS) barracks++;
	    if (b[i].type == B_COMMAND) command++;
	}
    }
	/* The source has two Defeat triggers: no remaining player men, or no
	 * Command Center/Barracks, fewer than ten Marines, and <=399 minerals. */
	if (!raynor_alive || !men || (!command && !barracks && marines <= 9 && minerals <= 399))
	    lost = 1;
    if (raynor_found && marines >= 10 && barracks >= 1) won = 1;
#endif
}
#endif

void
game_init(void)
{
#ifdef SC64_RACES
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
    memset(&race_z3,0,sizeof(race_z3));race_z3_action_ticks=0;
#endif
#ifdef SC64_P3_RUNTIME
    memset(&race_p3,0,sizeof(race_p3));race_p3_action_ticks=0;
#endif
    race_mode=0;race_completed=0;race_bots=2;race_skirmish_map=0;
#ifdef SC64_CAMPAIGN_RACE
    race_human=SC64_CAMPAIGN_RACE;
    race_selected_mission=sc64_frontend_first_mission(SC64_CAMPAIGN_RACE);
#elif defined(SC64_UNIFIED)
    race_human=R_TER;race_selected_mission=4;
#else
    race_human=R_ZERG;race_selected_mission=0;
#endif
    for(int i=0;i<3;i++)race_bot_races[i]=(race_human+i+1)%3;
    race_frontend_reset();
#endif
    reset_selection_gestures();
#ifdef SC64_HAS_TERRAIN_ART
    sc64_display_palette(sc64_palette + 32);
#endif
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    invalidate_path_grid();
    screen = SCREEN_TITLE;
    help = 0;
    menu_open = 0;
    minerals = 500;
    gas = 0;
    supply_used = 0;
    supply_cap = 20;
    cx = 70;
    cy = 90;
    seed = 0x51a7c0deu;
#ifdef SC64_TERRAN_CAMPAIGN
    campaign_mission = 0;
    campaign_completed = 0;
    if (load_game()) {
        screen = SCREEN_TITLE;
        memset(u, 0, sizeof(u));
        memset(b, 0, sizeof(b));
        SC64_GRID_CLEAR(explored);
        SC64_GRID_CLEAR(visible);
        selected_building = -1;
        frame = 0;
    }
#endif
#ifdef SC64_T5_PROBE_ENTRY
    /* Internal engine/performance probe only. Ordinary campaign selection
     * still uses the gated public catalog. */
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
#endif
#ifdef SC64_T6_PROBE_ENTRY
    race_selected_mission = SC64_T6_GLOBAL_ID;
    init_state();
#endif
#ifdef SC64_TERRAN_LATE_PROBE_MISSION
    race_selected_mission = SC64_TERRAN_LATE_PROBE_MISSION;
    init_state();
#endif
}

static int
selected_count(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].selected)
	    n++;
    return n;
}

static int
reserved_supply(void)
{
    int i, j, n = supply_used;
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (b[i].alive && b[i].owner == SIDE) {
	    for (j = 0; j < b[i].queue_count; j++)
		n += unit_supply(b[i].queue[(b[i].queue_head + j) % 5]);
	}
    return n;
}

static void
clear_selection(void)
{
    int i;
    reset_unit_tap();
    for (i = 0; i < MAX_UNITS; i++)
	u[i].selected = 0;
    selected_building = -1;
}

static int
selection_target(void)
{
    int i, best_unit = -1, best_building = -1;
    int unit_distance = 31 * 31, building_distance = 26 * 26, d;
    for (i = 0; i < MAX_UNITS; i++) {
	if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i]) || u[i].owner != SIDE
#ifdef SC64_RACES
            ||u[i].type==U_CHRYSALIS
#endif
#ifdef SC64_T4_RUNTIME
            ||!sc64_t4_unit_selectable(u[i].type)
#endif
            )
	    continue;
	d = dist2(cx, cy, u[i].x, u[i].y);
	if (d < unit_distance) {
	    unit_distance = d;
	    best_unit = i;
	}
    }
    for (i = 0; i < MAX_BUILDINGS; i++) {
	if (!b[i].alive || b[i].owner != SIDE
#ifdef SC64_T4_RUNTIME
            || (sc64_race_t4_building_flags(b[i].type) &&
                !(sc64_race_t4_building_flags(b[i].type)&SC64_T4_ACTOR_SELECTABLE))
#endif
            )
	    continue;
	d = dist2(cx, cy, b[i].x, b[i].y);
	if (d < building_distance) {
	    building_distance = d;
	    best_building = i;
	}
    }
    if (best_building >= 0 && (best_unit < 0 || building_distance <= unit_distance)) {
        return MAX_UNITS + best_building;
    }
    return best_unit;
}

static int
select_at(void)
{
    int target = selection_target();
    clear_selection();
    if (target >= MAX_UNITS) selected_building = target - MAX_UNITS;
    else if (target >= 0) {
        u[target].selected = 1;
        return target;
    }
    return -1;
}

static void
select_visible_type(int type)
{
    int i, left = viewport_left(), top = viewport_top();
    clear_selection();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE && u[i].type == type &&
#ifdef SC64_T4_RUNTIME
            sc64_t4_unit_selectable(u[i].type) &&
#endif
            u[i].x >= left && u[i].x < left + 240 &&
            u[i].y >= top && u[i].y < top + 108) u[i].selected = 1;
}

static void
remember_unit_tap(int picked)
{
    /* Further rapid taps retain the group instead of collapsing to one. */
    unit_tap = picked;
    unit_tap_frame = (u32)frame;
    unit_tap_x = cx; unit_tap_y = cy;
}

static void
issue_order(int order, int tx, int ty)
{
    int i, n = 0;
    selected_building = -1;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE && u[i].selected) {
#ifdef SC64_T4_RUNTIME
            if (sc64_race_t4_actor_flags(u[i].type) &&
                !(sc64_race_t4_actor_flags(u[i].type)&SC64_T4_ACTOR_MOVABLE)) continue;
#endif
#ifdef SC64_T6_RUNTIME
            if (u[i].order == ORDER_REPAIR) {
                u[i].carry = 0; u[i].work_timer = 0;
            }
#endif
	    u[i].order = order;
	    if (selected_count() > 1) {
		u[i].tx = limit(tx + ((n % 5) - 2) * 8, 8, WORLD_W - 8);
		u[i].ty = limit(ty + ((n / 5) - 1) * 8, 8, WORLD_H - 8);
	    } else {
		u[i].tx = tx;
		u[i].ty = ty;
	    }
	    u[i].path_timer = 0;
	    u[i].path_retry = 0;
	    u[i].target = -1;
	    n++;
	}
    if (n)
	toast(0);
    else
	toast(5);
}

static int
find_selected_worker(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE && u[i].selected && IS_WORKER(u[i].type))
	    return i;
    return -1;
}

static void
start_build(int type)
{
#ifdef SC64_RACES
    if(race_start_build(SIDE,find_selected_worker(),type,cx,cy)){mode=MODE_NORMAL;toast(3);}else toast(8);
    return;
#endif
    int cost = building_cost(type), i, slot;
    if (!has_tech_for(type)) {
	toast(type == B_ACADEMY ? 10 : 8);
	return;
    }
    if (minerals < cost) {
	toast(1);
	return;
    }
    if (gas < building_gas_cost(type)) {
	toast(12);
	return;
    }
    if (!can_place(type, cx, cy)) {
	toast(6);
	return;
    }
    i = find_selected_worker();
    if (i < 0) {
	toast(5);
	return;
    }
    slot = find_build_slot();
    if (slot < 0) {
	toast(11);
	return;
    }
    minerals -= cost;
    gas -= building_gas_cost(type);
    slot = spawn_build(SIDE, type, cx, cy, 1);
    if (slot < 0) {
	minerals += cost;
	return;
    }
    u[i].order = ORDER_BUILD;
    u[i].target = slot;
    u[i].tx = cx;
    u[i].ty = cy;
    u[i].path_timer = 0;
    u[i].path_retry = 0;
    toast(3);
    mode = MODE_NORMAL;
}

static void
stop_selected(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].selected) {
	    u[i].order = ORDER_IDLE;
	    u[i].carry = 0;
	    u[i].target = -1;
	}
    mode = MODE_NORMAL;
    menu_open = 0;
}

static void
start_research(int type)
{
    int *progress = type == 0 ? &weapons_research : &armor_research;
    int level = type == 0 ? weapons_level : armor_level;
    if (!has_building(SIDE,
#ifdef SC64_RACES
        race_human==R_ZERG?B_EVOLUTION_CHAMBER:race_human==R_PROTOSS?B_FORGE:
#endif
        B_ENG_BAY)) {
	toast(8);
	return;
    }
    if (level >= 2 || *progress) {
	toast(10);
	return;
    }
    if (minerals < 100 || gas < 100) {
	toast(gas < 100 ? 12 : 1);
	return;
    }
    minerals -= 100;
    gas -= 100;
    *progress = 1;
    toast(4);
}

static void
tick_research(void)
{
    if (weapons_research && ++weapons_research > 600) {
	weapons_research = 0;
	weapons_level++;
    }
    if (armor_research && ++armor_research > 600) {
	armor_research = 0;
	armor_level++;
    }
}

#ifdef SC64_TERRAN_LATE_RUNTIME
#include "sc64_terran_flight.h"
#endif

static void
choose_command(int choice)
{
#ifdef SC64_T5_RUNTIME
    if (choice >= MENU_FACTORY) {
        menu_open = 0;
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (choice == MENU_COMMAND_CENTER) {
            mode = MODE_BUILD; menu = B_COMMAND; return;
        }
        if (choice == MENU_LIFT_LAND) {
            if (race_late_airborne(selected_building)) mode = MODE_LAND;
            else toast(race_late_lift(selected_building) ? 0 : 8);
            return;
        }
#endif
        if (choice == MENU_UNLOAD) {
            mode = MODE_UNLOAD; return;
        }
        if (race_human != R_TER) { toast(8); return; }
        if (choice == MENU_CONTROL_TOWER) {
#ifdef SC64_T6_RUNTIME
            if (race_t6_tech_active()) {
                toast(race_t6_start_control_tower(selected_building) ? 3 : 8);
                return;
            }
#endif
            toast(race_t5_start_control_tower(selected_building) ? 3 : 8);
            return;
        }
        mode = MODE_BUILD;
        menu = choice == MENU_FACTORY ? B_FACTORY : B_STARPORT;
        return;
    }
#endif
#ifdef SC64_RACES
    static const int buildings[3][5]={
        {B_BARRACKS,B_DEPOT,B_REFINERY,B_ACADEMY,B_ENG_BAY},
        {B_SPAWNING_POOL,B_HATCHERY,B_EXTRACTOR,B_HYDRALISK_DEN,B_EVOLUTION_CHAMBER},
        {B_GATEWAY,B_PYLON,B_ASSIMILATOR,B_CYBERNETICS_CORE,B_FORGE}};
    if(choice>=MENU_BARRACKS&&choice<=MENU_ENG_BAY) {
        menu_open=0;mode=MODE_BUILD;menu=buildings[race_human][choice-MENU_BARRACKS];return;
    }
#endif
    menu_open = 0;
    switch (choice) {
    case MENU_MOVE:
	mode = MODE_MOVE;
	toast(0);
	break;
    case MENU_ATTACK:
	mode = MODE_ATTACK;
	toast(0);
	break;
    case MENU_BARRACKS:
	mode = MODE_BUILD;
	menu = B_BARRACKS;
	break;
    case MENU_DEPOT:
	mode = MODE_BUILD;
	menu = B_DEPOT;
	break;
    case MENU_REFINERY:
	mode = MODE_BUILD;
	menu = B_REFINERY;
	break;
    case MENU_ACADEMY:
	mode = MODE_BUILD;
	menu = B_ACADEMY;
	break;
    case MENU_ENG_BAY:
	mode = MODE_BUILD;
	menu = B_ENG_BAY;
	break;
    case MENU_WEAPONS:
	start_research(0);
	break;
    case MENU_ARMOR:
	start_research(1);
	break;
    case MENU_STOP:
	stop_selected();
	break;
    default:
	mode = MODE_NORMAL;
	break;
    }
}

/* Advanced Terran commands are available in the T5 development profile.
 * Other factions retain their existing ten-row command list. */
#ifdef SC64_T5_RUNTIME
static int field_menu_count(void)
{ return race_human == R_TER ? MENU_COUNT : MENU_STOP + 1; }
#else
#define field_menu_count() MENU_COUNT
#endif

static void
queue_unit(int building, int type)
{
#ifdef SC64_RACES
    toast(race_queue_unit(SIDE,building,type)?4:8);return;
#endif
    int cost = unit_cost(type), gas_cost = unit_gas_cost(type), tail;
    if (!building_complete(building)) {
	toast(9);
	return;
    }
    if (type == U_SCV && b[building].type != B_COMMAND)
	return;
    if (type != U_SCV && b[building].type != B_BARRACKS)
	return;
    if (b[building].queue_count >= 5) {
	toast(11);
	return;
    }
    if ((type == U_FIREBAT || type == U_MEDIC) && !has_building(SIDE, B_ACADEMY)) {
	toast(10);
	return;
    }
    if (minerals < cost) {
	toast(1);
	return;
    }
    if (gas < gas_cost) {
	toast(12);
	return;
    }
    if (reserved_supply() + unit_supply(type) > supply_cap) {
	toast(2);
	return;
    }
    minerals -= cost;
    gas -= gas_cost;
    tail = (b[building].queue_head + b[building].queue_count) % 5;
    b[building].queue[tail] = type;
    b[building].queue_count++;
    toast(4);
}

static void
interact(void)
{
    int double_unit = -1;
    if (!menu_open && mode == MODE_NORMAL && selected_building < 0 && unit_tap >= 0 &&
        (u32)frame - unit_tap_frame <= UNIT_DOUBLE_TAP_TICKS &&
        dist2(cx, cy, unit_tap_x, unit_tap_y) <= UNIT_DOUBLE_TAP_RADIUS * UNIT_DOUBLE_TAP_RADIUS &&
        selection_target() == unit_tap) double_unit = unit_tap;
    /* Only an A press that actually selects a unit can be part of a double
     * tap. Orders, construction, gas assignment and production stay immediate. */
    reset_unit_tap();
    if (menu_open) {
	choose_command(menu_cursor);
	return;
    }
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (mode == MODE_LAND) {
        toast(race_late_flight_order(selected_building, cx, cy, 1) ? 0 : 8);
        mode = MODE_NORMAL; return;
    }
#endif
    if (mode == MODE_MOVE) {
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (race_late_airborne(selected_building)) {
            toast(race_late_flight_order(selected_building, cx, cy, 0) ? 0 : 8);
            mode = MODE_NORMAL; return;
        }
#endif
	issue_order(ORDER_MOVE, cx, cy);
	mode = MODE_NORMAL;
	return;
    }
    if (mode == MODE_ATTACK) {
	issue_order(ORDER_ATTACK_MOVE, cx, cy);
	mode = MODE_NORMAL;
	return;
    }
    if (mode == MODE_BUILD) {
	start_build(menu);
	return;
    }
#ifdef SC64_T5_RUNTIME
    if (mode == MODE_UNLOAD) {
        int issued = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].selected &&
                race_transport_request_unload(i, cx, cy)) ++issued;
        mode = MODE_NORMAL; toast(issued ? 0 : 8); return;
    }
    /* Select passengers, then press A on a friendly Dropship to board.
     * Selecting an empty carrier without selected passengers still works. */
    for (int carrier = 0; carrier < MAX_UNITS; ++carrier)
        if (race_transport_carrier_valid(carrier) && u[carrier].owner == SIDE &&
            dist2(cx, cy, u[carrier].x, u[carrier].y) < 18 * 18) {
            int issued = 0;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == SIDE && u[i].selected &&
                    race_transport_request_board(i, carrier)) ++issued;
            if (issued) { toast(0); return; }
        }
#endif
    if (double_unit >= 0) {
        /* A second tap on the SCV itself groups workers even beside a
         * Refinery. A tap on the Refinery still assigns gas normally. */
        select_visible_type(u[double_unit].type);
        remember_unit_tap(double_unit);
        return;
    }
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active() && race_late_request_repair()) { toast(0); return; }
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_request_repair()) { toast(0); return; }
#endif
    {
	int worker = find_selected_worker();
	int refinery = nearest_refinery(cx, cy);
	if (worker >= 0 && refinery >= 0 && dist2(cx, cy, b[refinery].x, b[refinery].y) < 24 * 24) {
	    u[worker].order = ORDER_GAS;
	    u[worker].target = refinery;
	    u[worker].carry = 0;
	    toast(0);
	    return;
	}
    }
    if (selected_building >= 0
#ifdef SC64_TERRAN_LATE_RUNTIME
        && (!race_late_active() ||
            dist2(cx, cy, b[selected_building].x, b[selected_building].y) < 26 * 26)
#endif
        ) {
#ifdef SC64_RACES
        if(b[selected_building].type==B_COMMAND)train_type=U_SCV;
        if(b[selected_building].type==B_NEXUS)train_type=U_PROBE;
        if(b[selected_building].type==B_GATEWAY&&train_type!=U_ZEALOT&&train_type!=U_DRAGOON)train_type=U_ZEALOT;
        if(b[selected_building].type==B_BARRACKS&&train_type!=U_MARINE&&train_type!=U_FIREBAT&&train_type!=U_MEDIC)train_type=U_MARINE;
#ifdef SC64_T5_RUNTIME
        if (b[selected_building].type == B_FACTORY &&
            train_type != U_VULTURE && train_type != U_GOLIATH && train_type != U_TANK)
            train_type = U_VULTURE;
        if (b[selected_building].type == B_STARPORT &&
            train_type != U_WRAITH && train_type != U_DROPSHIP) train_type = U_WRAITH;
#endif
        queue_unit(selected_building,train_type);return;
#endif
	if (b[selected_building].type == B_COMMAND)
	    queue_unit(selected_building, U_SCV);
	else if (b[selected_building].type == B_BARRACKS)
	    queue_unit(selected_building, train_type);
	else if (b[selected_building].type == B_REFINERY) {
	    int worker = -1, j;
	    for (j = 0; j < MAX_UNITS; j++)
		if (u[j].alive && u[j].owner == SIDE && IS_WORKER(u[j].type) && u[j].order == ORDER_IDLE) {
		    worker = j;
		    break;
		}
	    if (worker >= 0) {
		u[worker].order = ORDER_GAS;
		u[worker].target = selected_building;
		toast(0);
	    } else
		toast(5);
	}
	return;
    }
    {
        int picked = select_at();
        if (picked >= 0) remember_unit_tap(picked);
    }
}

static int
owner_command_center(int owner)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == owner && (b[i].type == B_COMMAND
#ifdef SC64_RACES
        ||sc64_race_is_zerg_townhall(b[i].type)||b[i].type==B_NEXUS
#endif
        ) && !b[i].under_construction
#ifdef SC64_TERRAN_LATE_RUNTIME
        && !race_late_airborne(i)
#endif
        ) return i;
    return -1;
}
#ifdef SC64_TERRAN_LATE_RUNTIME
/* The legacy model has one home depot. Late campaign expansion bases need
 * the nearest landed depot; cache its slot in the otherwise unused worker
 * build field for a resource trip, avoiding a full building scan each tick. */
static int __attribute__((noinline)) race_late_worker_home(Unit *worker)
{
    int cached = worker->build;
    if (cached >= 0 && cached < MAX_BUILDINGS && b[cached].alive &&
        b[cached].owner == worker->owner && !b[cached].under_construction &&
        !race_late_airborne(cached) && (b[cached].type == B_COMMAND ||
        b[cached].type == B_NEXUS || sc64_race_is_zerg_townhall(b[cached].type)))
        return cached;
    int best = -1, distance = 0x7fffffff;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == worker->owner &&
            !b[i].under_construction && !race_late_airborne(i) &&
            (b[i].type == B_COMMAND || b[i].type == B_NEXUS ||
             sc64_race_is_zerg_townhall(b[i].type))) {
            int d = dist2(worker->x, worker->y, b[i].x, b[i].y);
            if (d < distance) { distance = d; best = i; }
        }
    worker->build = best;
    return best;
}
#endif
#ifndef SC64_SKIRMISH
static void
add_owner_resources(int owner, int ore, int vespene)
{
#ifdef SC64_RACES
    race_add_owner_resources(owner,ore,vespene);
#else
    if (owner == SIDE) { minerals += ore; gas += vespene; }
#endif
}
#endif

static int GBA_HOT
nearest_friendly_unit(int x, int y, int max_distance)
{
    int i, best = -1, best_d = max_distance * max_distance, d;
    if (target_lists_valid) {
        int p;
        for (p = 0; p < 5; ++p) {
            if (p != SIDE
#ifdef SC64_IMPORTED_MISSION
                && p != NEUTRAL
#endif
                ) continue;
            for (i = target_unit_head[p]; i >= 0; i = target_unit_next[i]) {
                if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i]) || FAST_ABS(x - u[i].x) >= max_distance ||
                    FAST_ABS(y - u[i].y) >= max_distance
#ifdef SC64_IMPORTED_MISSION
                    || (p == NEUTRAL && u[i].type != U_RAYNOR)
#endif
                    ) continue;
                d = dist2(x, y, u[i].x, u[i].y);
                if (d < best_d || (best >= 0 && d == best_d && i < best)) {
                    best_d = d;
                    best = i;
                }
            }
        }
        return best;
    }
    for (i = 0; i < MAX_UNITS; i++) {
	if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i]) || FAST_ABS(x - u[i].x) >= max_distance ||
	    FAST_ABS(y - u[i].y) >= max_distance || (u[i].owner != SIDE
#ifdef SC64_IMPORTED_MISSION
	    && !(u[i].owner == NEUTRAL && u[i].type == U_RAYNOR)
#endif
	    ))
	    continue;
	d = dist2(x, y, u[i].x, u[i].y);
	if (d < best_d) {
	    best_d = d;
	    best = i;
	}
    }
    return best;
}

static int
unit_range(int type)
{
#ifdef SC64_RACES
    return (sc64_race_unit_range(type)+1)/2;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (type == U_MARINE || type == U_HYDRALISK) return 64;
    if (type == U_FIREBAT) return 16;
    if (type == U_SCV) return 5;
    if (type == U_ZERGLING) return 8;
#endif
    if (type == U_RAYNOR)
	return 80;
    if (type == U_MARINE)
	return 40;
    if (type == U_HYDRALISK)
	return 48;
    if (type == U_FIREBAT)
	return 14;
    if (type == U_ZERGLING || type == U_SCV)
	return 10;
    return 0;
}

static int
unit_damage(int type)
{
#ifdef SC64_RACES
    return sc64_race_unit_damage(type);
#endif
    if (type == U_RAYNOR)
	return 30;
    if (type == U_MARINE)
	return 6 + weapons_level;
    if (type == U_FIREBAT)
	return 8 + weapons_level;
    if (type == U_HYDRALISK)
	return 10;
    if (type == U_ZERGLING)
	return 5;
    if (type == U_SCV)
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
        return 5;
#else
	return 4;
#endif
    return 0;
}

static void
tick_combat(Unit * q, int target)
{
    int dx, dy, range, damage;
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    if (q->type == U_SCOURGE && !q->alive) return;
    /* Scourge target validation must precede target_alive(), which indexes
     * the unit or building arrays from this combined target namespace. */
    if (q->type == U_SCOURGE &&
        (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS)) return;
#endif
    if (target < 0 || !target_alive(target))
	return;
#ifdef SC64_RACES
    if(!SC64_CAN_ATTACK_FROM(q->type,target,q->owner))return;
#endif
    dx = target_x(target) - q->x;
    dy = target_y(target) - q->y;
    range = unit_range(q->type);
    damage = unit_damage(q->type);
#ifdef SC64_RACES
    int flying=target<MAX_UNITS&&sc64_race_unit_flying(u[target].type);
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (target >= MAX_UNITS) flying = race_late_airborne(target - MAX_UNITS);
#endif
    range=(sc64_race_unit_range_vs(q->type,flying)+1)/2;
    /* Weapon reach is measured from a structure's footprint edge. The
       navigation grid keeps actors outside the same 18px collision radius. */
    if(target>=MAX_UNITS)range+=18;
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    /* BWAPI lists zero bonus damage for Suicide_Scourge. Keep its fixed
     * 110 damage independent of the generic SIDE weapon upgrade. */
    if (q->type == U_SCOURGE)
        damage=sc64_race_unit_damage_vs(q->type,flying);
    else
#endif
        damage=sc64_race_unit_damage_vs(q->type,flying)+(q->owner==SIDE?weapons_level:0);
#ifdef SC64_T5_RUNTIME
    if (q->type == U_KERRIGAN || q->type == U_RAYNOR_VULTURE)
        damage = sc64_t5_actor_weapon_damage(q->type,
                     q->owner == SIDE ? weapons_level : 0, flying);
#endif
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    #ifndef SC64_RACES
    if (q->owner != SIDE && (q->type == U_MARINE || q->type == U_FIREBAT)) damage -= weapons_level;
#endif
#endif
    if (!damage)
	return;
    if (dx * dx + dy * dy <= range * range) {
	if (!q->cd) {
	    add_tracer(q->x, q->y, target_x(target), target_y(target), q->owner == SIDE ? C_GOLD : C_RED);
	#ifdef SC64_RACES
        int hits=sc64_race_unit_hits_vs(q->type,flying);
        for(int hit=0;hit<hits&&target_alive(target);hit++)hit_target(target,damage,q->type,q->owner);
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
        /* Suicide Scourge damage is applied by hit_target first, preserving
         * the regular shield/armor/death path, then the attacker is consumed. */
        if (q->type == U_SCOURGE) {
            if (q->owner == SIDE) supply_used -= unit_supply(q->type);
            q->alive = 0;
            q->selected = 0;
        }
#endif
#else
        hit_target(target, damage, q->type, q->owner);
#endif
	    #ifdef SC64_RACES
        q->cd=(sc64_race_unit_cooldown_vs(q->type,flying)*5+3)/4;
#else
        q->cd = q->type == U_ZERGLING ? 10 : q->type == U_FIREBAT ? 28 : q->type == U_RAYNOR ? 30 : 19;
#endif
	}
    } else {
	move_toward(q, target_x(target), target_y(target));
    }
}

static int GBA_HOT
nearest_command_target(int x, int y)
{
    int i, best = -1, best_d = 0x7fffffff, d;
    if (target_lists_valid) {
        for (i = target_building_head[SIDE]; i >= 0; i = target_building_next[i])
            if (b[i].alive && b[i].type == B_COMMAND) {
                d = dist2(x, y, b[i].x, b[i].y);
                if (d < best_d) { best_d = d; best = i; }
            }
        return best < 0 ? -1 : MAX_UNITS + best;
    }
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (b[i].alive && b[i].owner == SIDE && b[i].type == B_COMMAND) {
	    d = dist2(x, y, b[i].x, b[i].y);
	    if (d < best_d) {
		best_d = d;
		best = i;
	    }
	}
    return best < 0 ? -1 : MAX_UNITS + best;
}

static void GBA_HOT
update_orders(void)
{
    int i, target;
#ifdef SC64_T5_RUNTIME
    race_transport_activity = 0;
#endif
#ifdef SC64_DIAGNOSTIC_ORDERS
    u32 sc64_setup_start = gba_clock();
    sc64_order_setup_cycles = 0;
    sc64_order_query_cycles = 0;
    sc64_order_move_cycles = 0;
#endif
#if !defined(SC64_SKIRMISH) && !defined(SC64_RACES)
    int cc;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    sc64_unit_buckets_begin();
#endif
#ifdef SC64_RACES
    sc64_target_index_end();
#endif
    target_lists_valid = 0;
    for (i = 0; i < 5; ++i) target_unit_head[i] = target_building_head[i] = -1;
    for (i = 0; i < MAX_UNITS; ++i) target_unit_next[i] = -1;
    for (i = 0; i < MAX_BUILDINGS; ++i) target_building_next[i] = -1;
    for (i = MAX_UNITS - 1; i >= 0; --i) if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i])) {
        int owner = u[i].owner;
        if (owner < 0 || owner >= 5) goto target_lists_unavailable;
        target_unit_next[i] = target_unit_head[owner];
        target_unit_head[owner] = i;
    }
    for (i = MAX_BUILDINGS - 1; i >= 0; --i) if (b[i].alive) {
        int owner = b[i].owner;
        if (owner < 0 || owner >= 5) goto target_lists_unavailable;
        target_building_next[i] = target_building_head[owner];
        target_building_head[owner] = i;
    }
    target_lists_valid = 1;
target_lists_unavailable:
#ifdef SC64_RACES
    if (target_lists_valid) sc64_target_index_begin();
    sc64_target_hostility_snapshot();
#endif
#ifdef SC64_DIAGNOSTIC_ORDERS
    sc64_order_setup_cycles = gba_clock() - sc64_setup_start;
#endif
    for (i = 0; i < MAX_UNITS; i++) {
	Unit *q = &u[i];
	if (!q->alive)
	    continue;
#ifdef SC64_T6_RUNTIME
        /* Keep burrowed actors out of every ordinary movement/weapon path.
         * Transport boarding also rejects the corresponding source flag. */
        if (race_t6_unit_burrowed(i)) continue;
        if (q->order == ORDER_REPAIR) {
#ifdef SC64_TERRAN_LATE_RUNTIME
            if (race_late_active() && race_late_repair_tick_unit(i)) continue;
#endif
            if (race_t6_repair_tick_unit(i)) continue;
        }
#endif
	#ifdef SC64_T5_RUNTIME
        if (q->order >= ORDER_BOARD) {
            race_transport_activity = 1;
            if (race_transport_tick_unit(i)) continue;
        }
        /* Preplaced mines own their arm/acquire/chase state. The ordinary
         * actor loop must not clear their target or consume their timer. */
        if (q->type == U_SPIDER_MINE) continue;
	#endif
	#ifdef SC64_T4_RUNTIME
        if (sc64_race_t4_actor_flags(q->type) &&
            !(sc64_race_t4_actor_flags(q->type)&SC64_T4_ACTOR_MOVABLE)) {
            q->order = ORDER_IDLE;
            q->target = -1;
            continue;
        }
	#endif
	if (q->cd > 0)
	    q->cd--;

	if (q->owner == ENEMY && q->order != ORDER_ATTACK_MOVE
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
            && 0
#endif
            ) {
	    q->order = ORDER_ATTACK_MOVE;
	    q->target = -1;
#ifdef SC64_IMPORTED_MISSION
	    q->tx = q->x; q->ty = q->y;
#else
	    q->tx = 76;
	    q->ty = 84;
#endif
	}

#ifdef SC64_IMPORTED_MISSION
        /* An escort that reaches its waypoint must still return fire.
         * Holding combat units defend within weapon range without chasing
         * away from their post or interrupting worker mining/build orders. */
        if (q->order == ORDER_IDLE && !IS_WORKER(q->type)
#ifdef SC64_TERRAN_CAMPAIGN
            && (q->owner == SIDE || campaign_mission == 1)
#elif !defined(SC64_BACKWATER) && !defined(SC64_SKIRMISH) && !defined(SC64_RACES)
            && q->owner == SIDE
#endif
            ) {
            target = nearest_enemy_typed(q->x,q->y,q->owner,unit_range(q->type)+1
#ifdef SC64_RACES
                +18
#endif
                ,q->type);
            if (target >= 0) tick_combat(q, target);
            continue;
        }
#endif

	if (q->order == ORDER_BUILD) {
#ifdef SC64_RACES
            if(race_builder_arrived(i))continue;
#endif
	    if (q->target < 0 || q->target >= MAX_BUILDINGS || !b[q->target].alive) {
		q->order = ORDER_IDLE;
		q->target = -1;
		continue;
	    }
	    /* SCVs work from the footprint edge; center distance includes the
	     * building collision radius, so a 36px work radius avoids path wedges. */
	    if (dist2(q->x, q->y, b[q->target].x, b[q->target].y) > 36 * 36) {
		move_toward(q, b[q->target].x, b[q->target].y);
	    } else if (++b[q->target].build_progress >= building_build_time(b[q->target].type)) {
		b[q->target].under_construction = 0;
		b[q->target].hp = b[q->target].maxhp;
		b[q->target].build_progress = 0;
		if (b[q->target].type == B_DEPOT && q->owner == SIDE)
		    supply_cap += 16;
		q->order = ORDER_IDLE;
		q->target = -1;
	    }
	    continue;
	}

	if (q->order == ORDER_MINE || q->order == ORDER_GAS) {
            int home =
#ifdef SC64_TERRAN_LATE_RUNTIME
                race_late_active() ? race_late_worker_home(q) :
#endif
                owner_command_center(q->owner);
	    if (q->order == ORDER_GAS && !building_complete(q->target)) {
		q->order = ORDER_IDLE;
		continue;
	    }
	    if (q->order == ORDER_MINE && (q->target < 0 || q->target >= MAX_MINERALS)) {
		q->order = ORDER_IDLE;
		continue;
	    }
	    int tx, ty;
	    if (home < 0) {
		q->order = ORDER_IDLE;
		continue;
	    }
	    tx = q->order == ORDER_MINE ? m[q->target].x : b[q->target].x;
	    ty = q->order == ORDER_MINE ? m[q->target].y : b[q->target].y;
	    if (q->carry) {
		/* Stop at the approach ring outside the structure's blocked cells. */
		if (dist2(q->x, q->y, b[home].x, b[home].y) > 32 * 32) {
		    move_toward(q, b[home].x, b[home].y);
		} else {
                    add_owner_resources(q->owner, q->order == ORDER_MINE ? q->carry : 0,
                                        q->order == ORDER_GAS ? q->carry : 0);
		    q->carry = 0;
#ifdef SC64_TERRAN_LATE_RUNTIME
                    if (race_late_active()) q->build = -1;
#endif
		    q->work_timer = 20;
		}
	    } else if (dist2(q->x, q->y, tx, ty) >
#ifdef SC64_RACES
            (q->order==ORDER_GAS?24*24:7*7)
#else
            7*7
#endif
            ) {
		move_toward(q, tx, ty);
	    } else if (q->work_timer > 0) {
		q->work_timer--;
	    } else if (q->order == ORDER_MINE) {
		q->carry = m[q->target].amount < 8 ? m[q->target].amount : 8;
		m[q->target].amount -= q->carry;
#ifdef SC64_TERRAN_LATE_RUNTIME
                if (race_late_active()) q->build = -1;
#endif
		q->work_timer = 45;
		if (q->carry <= 0)
		    q->order = ORDER_IDLE;
	    } else {
		/* OpenBW bwgame.h gather_resource: a normal gas trip carries 8,
		 * like minerals; depleted geysers (not yet modelled here) yield 2. */
		#ifdef SC64_RACES
		q->carry = 8;
#ifdef SC64_TERRAN_LATE_RUNTIME
                if (race_late_active()) q->build = -1;
#endif
		#else
		q->carry = 4;
		#endif
		q->work_timer = 60;
	    }
	    continue;
	}

	if (q->order == ORDER_MOVE) {
	    if (dist2(q->x, q->y, q->tx, q->ty) <= 5 * 5)
		q->order = ORDER_IDLE;
	    else
		move_toward(q, q->tx, q->ty);
	    continue;
	}

	if (q->order == ORDER_ATTACK_MOVE) {
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
            target = nearest_enemy_typed(q->x,q->y,q->owner,
#ifdef SC64_RACES
                unit_range(q->type)>64?unit_range(q->type)+18:
#endif
                64,q->type);
            if (target >= 0) tick_combat(q, target);
            else if (dist2(q->x, q->y, q->tx, q->ty) > 36) move_toward(q, q->tx, q->ty);
            else q->order = ORDER_IDLE;
#else
	    if (q->owner == ENEMY) {
		target = nearest_friendly_unit(q->x, q->y,
#ifdef SC64_IMPORTED_MISSION
		    48
#else
		    112
#endif
		    );
		if (target >= 0) {
		    tick_combat(q, target);
		    continue;
		}
		cc = nearest_command_target(q->x, q->y);
		if (cc >= 0 && dist2(q->x, q->y, target_x(cc), target_y(cc)) < 24 * 24) {
		    tick_combat(q, cc);
		    continue;
		}
		#ifndef SC64_IMPORTED_MISSION
		if (cc >= 0)
		    move_toward(q, target_x(cc), target_y(cc));
		#else
		else if (dist2(q->x, q->y, q->tx, q->ty) > 6 * 6)
		    move_toward(q, q->tx, q->ty);
		else
		    q->order = ORDER_IDLE;
		#endif
	    } else {
		target = nearest_enemy_typed(q->x, q->y, SIDE, 52,q->type);
		if (target >= 0)
		    tick_combat(q, target);
		else if (dist2(q->x, q->y, q->tx, q->ty) > 6 * 6)
		    move_toward(q, q->tx, q->ty);
		else
		    q->order = ORDER_IDLE;
	    }
#endif
	}
    }
    target_lists_valid = 0;
#ifdef SC64_RACES
    target_hostile_masks_valid = 0;
    sc64_target_index_end();
#endif
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    sc64_unit_buckets_end();
#endif
}

static int
nearest_mineral(int x, int y)
{
    int i, best = -1, best_d = 0x7fffffff, d;
    for (i = 0; i < MAX_MINERALS; i++)
	if (m[i].amount > 0) {
	    d = dist2(x, y, m[i].x, m[i].y);
	    if (d < best_d) {
		best_d = d;
		best = i;
	    }
	}
    return best;
}

static int
nearest_refinery(int x, int y)
{
    int i, best = -1, best_d = 0x7fffffff, d;
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (building_complete(i) && b[i].owner == SIDE && (b[i].type == B_REFINERY
#ifdef SC64_RACES
        ||b[i].type==B_EXTRACTOR||b[i].type==B_ASSIMILATOR
#endif
        )) {
	    d = dist2(x, y, b[i].x, b[i].y);
	    if (d < best_d) {
		best_d = d;
		best = i;
	    }
	}
    return best;
}

static void
economy(void)
{
    int i, patch;
    if (frame % 30)
	return;
    for (i = 0; i < MAX_UNITS; i++) {
#ifdef SC64_RACES
        /* An escort carrier keeps its last controller order at waypoints. */
        if(!race_mode&&race_selected_mission==1&&race_campaign.carrier_unit==i)continue;
#endif
	if (!u[i].alive ||
#ifdef SC64_SKIRMISH
            u[i].owner == NEUTRAL ||
#else
            u[i].owner != SIDE ||
#endif
            !IS_WORKER(u[i].type) || u[i].order != ORDER_IDLE)
	    continue;
	patch = nearest_mineral(u[i].x, u[i].y);
	if (patch >= 0) {
	    u[i].order = ORDER_MINE;
	    u[i].target = patch;
	    u[i].work_timer = 0;
	    u[i].carry = 0;
#ifndef SC64_SKIRMISH
	    break;
#endif
	}
    }
}

#ifndef SC64_IMPORTED_MISSION
static void
enemy_wave(void)
{
    int i, alive = 0;
    if (frame < enemy_timer || won || lost || !has_building(ENEMY, B_HATCHERY))
	return;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].owner == ENEMY)
	    alive++;
    if (alive < 5 + wave) {
	spawn_unit(ENEMY, U_ZERGLING, 290, 60 + (int)(rng_next(&seed) % 45));
	spawn_unit(ENEMY, wave % 2 ? U_HYDRALISK : U_ZERGLING,
		   292, 66 + (int)(rng_next(&seed) % 40));
	wave++;
    }
    enemy_timer = frame + (wave < 3 ? 1500 : 1200);
}
#endif

static void
update_production(void)
{
#ifdef SC64_RACES
    race_tick_buildings();return;
#endif
    int i, type, spawn_x, spawn_y, time;
    for (i = 0; i < MAX_BUILDINGS; i++) {
	if (!building_complete(i) || !b[i].queue_count)
	    continue;
	type = b[i].queue[b[i].queue_head];
	time = unit_build_time(type);
	if (++b[i].queue_progress < time)
	    continue;
	spawn_x = b[i].x + 20;
	spawn_y = b[i].y + 4;
	if (spawn_x >= WORLD_W - 8)
	    spawn_x = b[i].x - 20;
	if (spawn_unit(b[i].owner, type, spawn_x, spawn_y) < 0) {
	    toast(11);
	    b[i].queue_progress = time - 1;
	    continue;
	}
	b[i].queue_head = (b[i].queue_head + 1) % 5;
	b[i].queue_count--;
	b[i].queue_progress = 0;
    }
}

#ifdef SC64_SKIRMISH
#include "skirmish_adapter.h"
#endif

static void
update_tracers(void)
{
    int i;
    for (i = 0; i < MAX_TRACERS; i++)
	if (tracers[i].life)
	    tracers[i].life--;
}

static void GBA_HOT
update_exploration(void)
{
#ifdef SC64_IMPORTED_MISSION
    sc64_update_exploration();
#else
    int i, col, row;
    SC64_GRID_CLEAR(visible);
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].owner == SIDE) {
	    int c = u[i].x / 16, r = u[i].y / 16, radius = IS_WORKER(u[i].type) ? 4 : 5;
	    for (row = r - radius; row <= r + radius; row++)
		for (col = c - radius; col <= c + radius; col++)
		    if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS &&
			(col - c) * (col - c) + (row - r) * (row - r) <= radius * radius)
			SC64_GRID_SET(explored, col + row * MAP_COLS, 1);
	    radius = IS_WORKER(u[i].type) ? 56 : 72;
	    for (row = r - 5; row <= r + 5; row++)
		for (col = c - 5; col <= c + 5; col++)
		    if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS) {
			int dx = col * 16 + 8 - u[i].x;
			int dy = row * 16 + 8 - u[i].y;
			if (dx * dx + dy * dy <= radius * radius)
			    SC64_GRID_SET(visible, col + row * MAP_COLS, 1);
		    }
	}
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (b[i].alive && b[i].owner == SIDE) {
	    int c = b[i].x / 16, r = b[i].y / 16, radius = b[i].type == B_COMMAND ? 7 : 3;
	    for (row = r - radius; row <= r + radius; row++)
		for (col = c - radius; col <= c + radius; col++)
		    if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS &&
			(col - c) * (col - c) + (row - r) * (row - r) <= radius * radius)
			SC64_GRID_SET(explored, col + row * MAP_COLS, 1);
	    if (b[i].type == B_COMMAND)
		for (row = r - 8; row <= r + 8; row++)
		    for (col = c - 8; col <= c + 8; col++)
			if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS) {
			    int dx = col * 16 + 8 - b[i].x;
			    int dy = row * 16 + 8 - b[i].y;
			    if (dx * dx + dy * dy <= 112 * 112)
				SC64_GRID_SET(visible, col + row * MAP_COLS, 1);
			}
	}
#endif
}

static void
cancel_queue(int building)
{
    int type;
    if (building < 0 || building >= MAX_BUILDINGS || !b[building].queue_count)
	return;
    type = b[building].queue[b[building].queue_head];
    minerals += unit_cost(type) * 3 / 4;
    gas += unit_gas_cost(type) * 3 / 4;
    b[building].queue_head = (b[building].queue_head + 1) % 5;
    b[building].queue_count--;
    b[building].queue_progress = 0;
    toast(0);
}

static void
cycle_selection(void)
{
    int i, selected = -1;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].selected) {
	    selected = i;
	    break;
	}
    clear_selection();
    for (i = selected + 1; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE
#ifdef SC64_RACES
            &&u[i].type!=U_CHRYSALIS
#endif
#ifdef SC64_T4_RUNTIME
            &&sc64_t4_unit_selectable(u[i].type)
#endif
            ) {
	    u[i].selected = 1;
	    cx = u[i].x;
	    cy = u[i].y;
	    return;
	}
    for (i = 0; i <= selected && i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE
#ifdef SC64_RACES
            &&u[i].type!=U_CHRYSALIS
#endif
#ifdef SC64_T4_RUNTIME
            &&sc64_t4_unit_selectable(u[i].type)
#endif
            ) {
	    u[i].selected = 1;
	    cx = u[i].x;
	    cy = u[i].y;
	    return;
	}
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE
#ifdef SC64_RACES
            &&u[i].type!=U_CHRYSALIS
#endif
#ifdef SC64_T4_RUNTIME
            &&sc64_t4_unit_selectable(u[i].type)
#endif
            ) {
	    u[i].selected = 1;
	    cx = u[i].x;
	    cy = u[i].y;
	    return;
	}
}

static void
select_combat_group(void)
{
    int i;
    clear_selection();
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE &&
	    (u[i].type == U_MARINE || u[i].type == U_FIREBAT || u[i].type == U_MEDIC
#ifdef SC64_RACES
             ||(!IS_WORKER(u[i].type)&&u[i].type!=U_OVERLORD&&u[i].type!=U_CHRYSALIS)
#endif
#ifdef SC64_IMPORTED_MISSION
	     || u[i].type == U_RAYNOR
#endif
#ifdef SC64_T4_RUNTIME
             || sc64_t4_unit_has(u[i].type, SC64_T4_ACTOR_WEAPON)
#endif
	     )
#ifdef SC64_T4_RUNTIME
            && sc64_t4_unit_selectable(u[i].type)
            && (!sc64_race_t4_actor_flags(u[i].type) ||
                sc64_t4_unit_has(u[i].type, SC64_T4_ACTOR_WEAPON))
#endif
	    )
	    u[i].selected = 1;
}

static void
select_next_train_type(void)
{
#ifdef SC64_T5_RUNTIME
    if (selected_building >= 0 && b[selected_building].alive &&
        (b[selected_building].type == B_FACTORY || b[selected_building].type == B_STARPORT)) {
        static const int advanced[] = { U_VULTURE, U_GOLIATH, U_TANK, U_WRAITH, U_DROPSHIP };
        int current = -1;
        for (int i = 0; i < 5; ++i) if (advanced[i] == train_type) current = i;
        for (int step = 1; step <= 5; ++step) {
            int type = advanced[(current + step) % 5];
            if (sc64_race_unit_rules[type].producer == b[selected_building].type) {
#ifdef SC64_T5_RUNTIME
                if (race_t5_active() &&
                    !race_t5_unit_product_available(b[selected_building].owner,
                                                    b[selected_building].raw_owner,
                                                    type)) continue;
#endif
#ifdef SC64_T6_RUNTIME
                if (!race_t6_unit_product_available(b[selected_building].owner,
                        b[selected_building].raw_owner, type)) continue;
#endif
                train_type = type; return;
            }
        }
    }
#endif
#ifdef SC64_RACES
    static const int types[3][4]={{U_SCV,U_MARINE,U_FIREBAT,U_MEDIC},
        {U_DRONE,U_OVERLORD,U_ZERGLING,U_HYDRALISK},{U_PROBE,U_ZEALOT,U_DRAGOON,U_SCOUT}};
    int n;for(n=0;n<4;n++)if(types[race_human][n]==train_type)break;
    for(int step=1;step<=4;step++) {
        int type=types[race_human][(n+step)%4];
        if(selected_building<0||sc64_race_unit_rules[type].producer==b[selected_building].type) {
            train_type=type;return;
        }
    }
    return;
#endif
    if (train_type == U_MARINE)
	train_type = U_FIREBAT;
    else if (train_type == U_FIREBAT)
	train_type = U_MEDIC;
    else
	train_type = U_MARINE;
}

static void
select_box_units(void)
{
    int i, left = box_anchor_x < box_end_x ? box_anchor_x : box_end_x;
    int right = box_anchor_x > box_end_x ? box_anchor_x : box_end_x;
    int top = box_anchor_y < box_end_y ? box_anchor_y : box_end_y;
    int bottom = box_anchor_y > box_end_y ? box_anchor_y : box_end_y;
    clear_selection();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE
#ifdef SC64_RACES
            &&u[i].type!=U_CHRYSALIS
#endif
#ifdef SC64_T4_RUNTIME
            &&sc64_t4_unit_selectable(u[i].type)
#endif
            && u[i].x >= left && u[i].x <= right &&
            u[i].y >= top && u[i].y <= bottom) u[i].selected = 1;
    mode = MODE_NORMAL;
}

/* L is a tap until the cursor actually moves. Delay its old action until
 * release so the anchor never jumps to the next unit before dragging. */
static u16
update_selection_box(u16 held, u16 pressed, int old_x, int old_y)
{
    u16 consume = 0;
    if (!box_state && (pressed & KEY_L)) {
        reset_unit_tap();
        box_state = 1;
        box_anchor_x = box_end_x = old_x;
        box_anchor_y = box_end_y = old_y;
    }
    if (box_state) {
        consume = KEY_A | KEY_L | KEY_R;
        if (held & KEY_A) box_suppress_a = 1;
        if (pressed & KEY_B) {
            /* Keep the old selection and command mode; wait for L release
             * before another gesture can begin. */
            box_state = 3;
            consume |= KEY_B;
        }
        if (box_state == 3) {
            if (!(held & KEY_L)) box_state = 0;
        } else if (held & KEY_L) {
            box_end_x = cx; box_end_y = cy;
            if (cx != box_anchor_x || cy != box_anchor_y) box_state = 2;
        } else {
            if (box_state == 2) select_box_units();
            else if (selected_building >= 0 && b[selected_building].alive &&
                     b[selected_building].type == B_BARRACKS) cancel_queue(selected_building);
            else cycle_selection();
            box_state = 0;
        }
    }
    if (!(held & KEY_A)) box_suppress_a = 0;
    if (box_suppress_a) consume |= KEY_A;
    return pressed & (u16)~consume;
}

static void
pause_action(u16 pressed)
{
    if (pressed & KEY_A)
	screen = SCREEN_PLAY;
    else if (pressed & KEY_B)
	save_game();
    else if (pressed & KEY_L) {
	if (!load_game())
	    toast(1);
    } else if (pressed & KEY_R)
	init_state();
}

void
game_update(u16 held, u16 pressed)
{
#ifdef SC64_IMPORTED_MISSION
    u32 profile_start, profile_orders_end, profile_sight_start;
#ifdef SC64_DIAGNOSTIC_UPDATE
    u32 sc64_update_stage_start, sc64_update_remainder_start;
#endif
#endif
    int speed = (held & KEY_R) ? 4 : 2;
    if (screen != SCREEN_PLAY) reset_selection_gestures();
    if (screen == SCREEN_TITLE) {
#ifdef SC64_RACES
        int old_page=race_front.page,old_row=race_front.row;
        int action=sc64_frontend_update(&race_front,held);
        if(race_front_load_error)race_front_load_error--;
        if(pressed){race_front_load_error=0;audio_tone((pressed&KEY_A)?660:440,2,2);}
        if(old_page!=race_front.page)race_front_transition=race_front.ticks;
        if(old_row!=race_front.row)audio_tone(440,2,2);
        if(action==SC_FRONT_START_CAMPAIGN||action==SC_FRONT_START_SKIRMISH) {
            race_mode=action==SC_FRONT_START_SKIRMISH;
            race_selected_mission=race_front.mission;
            race_human=race_mode?race_front.race:sc64_race_maps[race_selected_mission].owner_races[sc64_race_maps[race_selected_mission].human_slot];
            race_bots=race_front.bots;
            race_skirmish_map=race_front.map;
            for(int i=0;i<3;i++)race_bot_races[i]=race_front.bot_races[i];
            sc64_display_palette(sc64_palette+32);
            audio_tone(880,5,3);
            init_state();
        } else if(action==SC_FRONT_LOAD) {
            sc64_display_palette(sc64_palette+32);
            if(!load_game()){race_front_load_error=90;audio_tone(180,6,2);}
        }
        return;
#endif
#ifdef SC64_TERRAN_CAMPAIGN
        if (pressed & (KEY_UP | KEY_DOWN)) campaign_mission = 1 - campaign_mission;
#endif
#ifdef SC64_SKIRMISH
        if (pressed & KEY_UP) skirmish_bots = skirmish_bots % 3 + 1;
        if (pressed & KEY_DOWN) skirmish_bots = (skirmish_bots + 1) % 3 + 1;
#endif
	if (pressed & KEY_A)
	    init_state();
	else if (pressed & (KEY_B | KEY_L)) {
	    if (!load_game())
		toast(1);
	}
	return;
    }
    if (screen == SCREEN_PAUSE) {
	pause_action(pressed);
	return;
    }
    if (screen == SCREEN_RESULT) {
	if (pressed & KEY_A) {
#ifdef SC64_RACES
            if(won&&!race_mode){
                int next=sc64_catalog_available_next(sc64_catalog_mission_faction(race_selected_mission),race_selected_mission);
                if(next>=0)race_selected_mission=next;
            }
#endif
#ifdef SC64_TERRAN_CAMPAIGN
            if (won && campaign_mission == 0) {
                campaign_completed |= 1;
                campaign_mission = 1;
                save_game();
            }
#endif
	    init_state();
	} else if (pressed & KEY_B) {
	    screen = SCREEN_TITLE;
	    won = lost = 0;
#ifdef SC64_RACES
            race_frontend_reset();race_front.prev_buttons=held;
#endif
	}
	return;
    }
    if (pressed & KEY_START) {
	reset_selection_gestures();
	screen = SCREEN_PAUSE;
	return;
    }
    if (pressed & KEY_SELECT) {
	reset_selection_gestures();
	screen = SCREEN_PAUSE;
	return;
    }
    /* Menus own their inputs. Selection gestures start only in the field. */
    if (menu_open) reset_selection_gestures();
    if ((pressed & KEY_B) && !box_state && !(pressed & KEY_L)) {
	if (menu_open)
	    menu_open = 0;
	else if (mode != MODE_NORMAL) {
	    mode = MODE_NORMAL;
	    menu_open = 1;
	} else {
	    menu_open = 1;
	    menu_cursor = 0;
	}
    }
    if (menu_open) {
	reset_selection_gestures();
	if (pressed & KEY_UP)
	    menu_cursor = (menu_cursor + field_menu_count() - 1) % field_menu_count();
	if (pressed & KEY_DOWN)
	    menu_cursor = (menu_cursor + 1) % field_menu_count();
	if (pressed & KEY_A)
	    choose_command(menu_cursor);
	return;
    }
    if (pressed & KEY_START)
	help = !help;
    if (help) {
	reset_selection_gestures();
	if (pressed & KEY_A || pressed & KEY_B)
	    help = 0;
	return;
    }
    int old_cx = cx, old_cy = cy;
    if (held & KEY_LEFT)
	cx -= speed;
    if (held & KEY_RIGHT)
	cx += speed;
    if (held & KEY_UP)
	cy -= speed;
    if (held & KEY_DOWN)
	cy += speed;
    cx = limit(cx, 0, WORLD_W - 1);
    cy = limit(cy, 0, WORLD_H - 1);
    if (unit_tap >= 0 && dist2(cx, cy, unit_tap_x, unit_tap_y) >
        UNIT_DOUBLE_TAP_RADIUS * UNIT_DOUBLE_TAP_RADIUS) reset_unit_tap();
    pressed = update_selection_box(held, pressed, old_cx, old_cy);
    if ((pressed & KEY_R)
#ifdef SC64_RACES
        &&!(held&(KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN))
#endif
        ) {
	if (selected_building >= 0 &&
#ifdef SC64_RACES
            (sc64_race_building_rules[b[selected_building].type].flags&SC_RACE_F_PRODUCER)
#else
            b[selected_building].type == B_BARRACKS
#endif
        )
	    select_next_train_type();
	else
	    select_combat_group();
    }
    if (pressed & KEY_A)
	interact();

    frame++;
#ifdef SC64_IMPORTED_MISSION
    profile_start = gba_clock();
#endif
    update_orders();
#ifdef SC64_T5_RUNTIME
    race_transport_tick_after_orders();
#endif
#ifdef SC64_IMPORTED_MISSION
    profile_orders_end = gba_clock();
    sc64_orders_cycles = profile_orders_end - profile_start;
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_update_mission_cycles = 0;
    sc64_update_production_cycles = 0;
    sc64_update_race_state_cycles = 0;
    sc64_update_defenses_cycles = 0;
    sc64_update_research_cycles = 0;
    sc64_update_remainder_cycles = 0;
#ifdef SC64_T5_RUNTIME
    sc64_t5_rescue_cycles = 0;
    sc64_t5_mine_cycles = 0;
    sc64_t5_trigger_ai_cycles = 0;
#endif
    sc64_update_stage_start = gba_clock();
#endif
#endif
#ifdef SC64_IMPORTED_MISSION
    update_imported_mission();
#endif
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_UPDATE)
    sc64_update_mission_cycles = gba_clock() - sc64_update_stage_start;
    sc64_update_stage_start = gba_clock();
#endif
    update_production();
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_UPDATE)
    sc64_update_production_cycles = gba_clock() - sc64_update_stage_start;
    sc64_update_stage_start = gba_clock();
#endif
#ifdef SC64_RACES
    race_tick_state();
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_UPDATE)
    sc64_update_race_state_cycles = gba_clock() - sc64_update_stage_start;
    sc64_update_stage_start = gba_clock();
#endif
    race_tick_defenses();
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_UPDATE)
    sc64_update_defenses_cycles = gba_clock() - sc64_update_stage_start;
    sc64_update_stage_start = gba_clock();
#endif
#endif
    tick_research();
#ifdef SC64_T5_RUNTIME
    race_t5_tech_tick();
    race_transport_tick_after_orders();
#endif
#ifdef SC64_T6_RUNTIME
    race_t6_tech_tick();
#endif
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_UPDATE)
    sc64_update_research_cycles = gba_clock() - sc64_update_stage_start;
    sc64_update_remainder_start = gba_clock();
#endif
    if (frame % 30 == 0)
	economy();
    if (frame % 90 == 0) {
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	    if (u[i].alive && u[i].owner == SIDE && IS_WORKER(u[i].type) && u[i].hp < u[i].maxhp)
		u[i].hp++;
    }
#ifndef SC64_IMPORTED_MISSION
    enemy_wave();
#endif
    update_tracers();
#ifdef SC64_IMPORTED_MISSION
    profile_sight_start = gba_clock();
    sc64_other_cycles = profile_sight_start - profile_orders_end;
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_update_remainder_cycles = profile_sight_start - sc64_update_remainder_start;
#endif
#endif
    update_exploration();
#ifdef SC64_IMPORTED_MISSION
    sc64_sight_cycles = gba_clock() - profile_sight_start;
#endif
    if (toast_timer)
	toast_timer--;
    {
#ifdef SC64_IMPORTED_MISSION
	check_imported_mission_outcome();
#endif
#ifndef SC64_IMPORTED_MISSION
	int enemies = 0, i;
	for (i = 0; i < MAX_UNITS; i++)
	    if (u[i].alive && u[i].owner == ENEMY)
		enemies++;
	for (i = 0; i < MAX_BUILDINGS; i++)
	    if (b[i].alive && b[i].owner == ENEMY)
		enemies++;
	if (!enemies)
	    won = 1;
#endif
    }
    if (won || lost) {
#ifdef SC64_RACES
        if(won&&!race_mode)race_completed|=1<<race_selected_mission;
        save_game();
#endif
#ifdef SC64_TERRAN_CAMPAIGN
        if (won) {
            campaign_completed |= 1 << campaign_mission;
            save_game();
        }
#endif
	reset_selection_gestures();
	screen = SCREEN_RESULT;
	result_timer = 0;
    }
}

static int
currently_visible(int x, int y)
{
	int col = x / 16, row = y / 16;
	if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS)
	    return 0;
	return SC64_GRID_GET(visible, col + row * MAP_COLS);
}

#ifndef SC64_IMPORTED_MISSION
static int
cell_explored(int col, int row)
{
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS)
	return 0;
    return SC64_GRID_GET(explored, col + row * MAP_COLS);
}
#endif

#ifdef SC64_HAS_SPRITES
static const Sc64SpriteFrame *
mission_unit_sprite(int type)
{
#ifdef SC64_RACES
    return sc64_race_unit_sprite(sc64_race_raw_unit(type));
#else
    if (type == U_SCV) return &sc64_sprite_scv;
    if (type == U_MARINE) return &sc64_sprite_marine;
    if (type == U_ZERGLING) return &sc64_sprite_zergling;
    if (type == U_RAYNOR) return &sc64_sprite_raynor_vulture;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (type == U_FIREBAT) return &sc64_sprite_firebat;
    if (type == U_HYDRALISK) return &sc64_sprite_hydralisk;
#endif
    return 0;
#endif
}

static const Sc64SpriteFrame *
mission_building_sprite(int type)
{
#ifdef SC64_RACES
    return sc64_race_unit_sprite(sc64_race_raw_building(type));
#else
    if (type == B_COMMAND) return &sc64_sprite_command_center;
    if (type == B_BARRACKS) return &sc64_sprite_barracks;
    if (type == B_DEPOT) return &sc64_sprite_supply_depot;
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
    if (type == B_ACADEMY) return &sc64_sprite_academy;
    if (type == B_BUNKER) return &sc64_sprite_bunker;
    if (type == B_INFESTED_COMMAND) return &sc64_sprite_infested_command_center;
    if (type == B_CREEP_COLONY) return &sc64_sprite_creep_colony;
#endif
    return 0;
#endif
}

static const Sc64SpriteFrame *
mission_mineral_sprite(int style)
{
    if (style == SC64_RAW_MINERAL_FIELD_1) return &sc64_sprite_mineral_field_1;
    if (style == SC64_RAW_MINERAL_FIELD_2) return &sc64_sprite_mineral_field_2;
    if (style == SC64_RAW_MINERAL_FIELD_3) return &sc64_sprite_mineral_field_3;
    return 0;
}

static void
draw_mission_sprite(const Sc64SpriteFrame *s, int cx, int cy)
{
    gfx_blit_sprite(cx - s->canvas_w / 2 + s->offset_x,
	cy - s->canvas_h / 2 + s->offset_y, s->w, s->h, s->pixels);
}
#endif

static void
draw_unit(Unit * q)
{
#ifdef SC64_T6_RUNTIME
    if (race_t6_active() && race_t6_unit_burrowed((int)(q - u))) return;
#endif
    if (SC64_TRANSPORT_LOADED(q)) return;
    int x = q->x - camx, y = q->y - camy + 12;
    int col = q->owner == SIDE ? C_TERRAN : C_ZERG;
#ifdef SC64_T4_RUNTIME
    unsigned actor_flags = sc64_race_t4_actor_flags(q->type);
    if (actor_flags & SC64_T4_ACTOR_HIDDEN) return;
    if (actor_flags && !(actor_flags & SC64_T4_ACTOR_DRAWABLE)) return;
#endif
    if (x < -8 || x > 247 || y < 10 || y > 125)
	return;
    if (q->owner != SIDE && !currently_visible(q->x, q->y))
	return;
#ifdef SC64_HAS_SPRITES
#ifdef SC64_T5_RUNTIME
    /* This probe reuses the verified Ghost/Vulture sprite frames for their
     * hero variants; unique hero art is not present in the current bank. */
    int sprite_raw = q->raw_type;
    if (q->type == U_KERRIGAN) sprite_raw = 1;
    if (q->type == U_RAYNOR_VULTURE
#ifdef SC64_T6_RUNTIME
        && !race_t6_active()
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
        && !race_late_active()
#endif
        ) sprite_raw = 2;
#endif
    const Sc64SpriteFrame *sprite =
#ifdef SC64_RACES
#ifdef SC64_T5_RUNTIME
        sprite_raw>=0?sc64_race_unit_sprite(sprite_raw):
#else
        q->raw_type>=0?sc64_race_unit_sprite(q->raw_type):
#endif
#endif
        mission_unit_sprite(q->type);
    int sprite_x = sprite ? x - sprite->canvas_w / 2 + sprite->offset_x : x - 4;
    int sprite_y = sprite ? y - sprite->canvas_h / 2 + sprite->offset_y : y - 4;
    int sprite_w = sprite ? sprite->w : 8;
    int sprite_h = sprite ? sprite->h : 8;
#else
    int sprite_x = x - 4, sprite_y = y - 4, sprite_w = 8, sprite_h = 8;
#endif
#ifdef SC64_HAS_SPRITES
    if (sprite) draw_mission_sprite(sprite, x, y);
    else
#endif
    {
    if (IS_WORKER(q->type)) {
	gfx_rect(x - 3, y - 2, 7, 5, col);
	gfx_pixel(x + 2, y - 3, C_SILVER);
	gfx_pixel(x - 3, y + 3, C_GOLD);
    } else if (q->type == U_MARINE) {
	gfx_rect(x - 2, y - 4, 5, 8, col);
	gfx_rect(x - 3, y - 5, 6, 2, C_SILVER);
	gfx_line(x + 1, y, x + 5, y - 2, C_SILVER);
    } else if (q->type == U_FIREBAT) {
	gfx_rect(x - 3, y - 4, 7, 8, col);
	gfx_rect(x - 4, y - 5, 8, 2, C_ORANGE);
	gfx_pixel(x + 5, y, C_ORANGE);
    } else if (q->type == U_MEDIC) {
	gfx_rect(x - 3, y - 4, 7, 8, C_WHITE);
	gfx_pixel(x, y - 1, C_RED);
	gfx_pixel(x, y, C_RED);
	gfx_pixel(x, y + 1, C_RED);
    } else if (q->type == U_ZERGLING) {
	gfx_diamond(x, y, 5, 3, col);
	gfx_line(x - 3, y - 1, x - 6, y - 4, C_BLOOD);
	gfx_line(x + 3, y - 1, x + 6, y - 4, C_BLOOD);
	} else if (q->type == U_RAYNOR) {
	gfx_diamond(x, y, 6, 5, C_GOLD);
	gfx_rect(x - 2, y - 5, 5, 3, C_SILVER);
	gfx_pixel(x, y, C_WHITE);
    } else {
	gfx_diamond(x, y, 5, 4, col);
	gfx_pixel(x - 4, y - 3, C_PURPLE);
	gfx_pixel(x + 4, y - 3, C_PURPLE);
    }
    }
#ifdef SC64_T4_RUNTIME
    if (!(actor_flags & SC64_T4_ACTOR_INVINCIBLE))
#endif
	    gfx_bar(x - 6, sprite_y - 4, 12, 2, q->hp, q->maxhp,
	    owner_color(q->owner), C_RED_DARK);
    if (q->selected)
	gfx_frame(sprite_x - 2, sprite_y - 2, sprite_w + 4, sprite_h + 4, C_HIGHLIGHT);
}

static void
draw_tracers(void)
{
    int i;
    for (i = 0; i < MAX_TRACERS; i++)
	if (tracers[i].life &&
	    (currently_visible(tracers[i].x0, tracers[i].y0) ||
	     currently_visible(tracers[i].x1, tracers[i].y1))) {
	    gfx_line(tracers[i].x0 - camx, tracers[i].y0 - camy + 12,
		     tracers[i].x1 - camx, tracers[i].y1 - camy + 12, tracers[i].color);
	}
}

static void
draw_minimap(void)
{
    int row, col, x, y, i;
#ifdef SC64_IMPORTED_MISSION
	const int map_x = 197, map_y = 129, map_size = 24;
#endif
    gfx_rect(181, 124, 56, 34, C_BLACK);
    gfx_frame(181, 124, 56, 34, C_EDGE);
#ifdef SC64_IMPORTED_MISSION
    for (row = 0; row < 24; row++)
        for (col = 0; col < 24; col++) {
            int cell = row * 24 + col;
            if (sc64_minimap_visible[cell] || sc64_minimap_explored[cell])
                gfx_pixel(map_x + col, map_y + row,
                    sc64_minimap_visible[cell] ? C_GRASS : C_FOG);
        }
#else
    for (row = 0; row < MAP_ROWS; row++)
	for (col = 0; col < MAP_COLS; col++) {
	    if (!cell_explored(col, row))
		continue;
#ifdef SC64_IMPORTED_MISSION
	    x = map_x + col * map_size / MAP_COLS;
	    y = map_y + row * map_size / MAP_ROWS;
	    gfx_pixel(x, y, SC64_GRID_GET(visible, col + row * MAP_COLS) ? C_GRASS : C_FOG);
#else
	    x = 185 + col * 2;
	    y = 126 + row * 2;
	    gfx_rect(x, y, 2, 2, SC64_GRID_GET(visible, col + row * MAP_COLS) ? C_GRASS : C_FOG);
#endif
	}
#endif
    for (i = 0; i < MAX_BUILDINGS; i++)
	if (b[i].alive &&
#ifdef SC64_T4_RUNTIME
            (!sc64_race_t4_building_flags(b[i].type) ||
             (sc64_race_t4_building_flags(b[i].type)&SC64_T4_ACTOR_DRAWABLE)) &&
#endif
	    (b[i].owner == SIDE || SC64_GRID_GET(visible, (b[i].x / 16) + (b[i].y / 16) * MAP_COLS))) {
#ifdef SC64_IMPORTED_MISSION
	    x = map_x +
#ifdef SC64_RACES
        sc64_minimap_pixel(b[i].x,MAP_COLS)
#else
        b[i].x * map_size / WORLD_W
#endif
        ;
	    y = map_y +
#ifdef SC64_RACES
        sc64_minimap_pixel(b[i].y,MAP_ROWS)
#else
        b[i].y * map_size / WORLD_H
#endif
        ;
#else
	    x = 185 + b[i].x / 8;
	    y = 126 + b[i].y / 8;
#endif
	    gfx_pixel(x, y,
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
                owner_color(b[i].owner)
#else
                b[i].owner == SIDE ? C_TERRAN : C_RED
#endif
                );
	}
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
#ifdef SC64_T6_RUNTIME
            !race_t6_unit_burrowed(i) &&
#endif
#ifdef SC64_T4_RUNTIME
            !(sc64_race_t4_actor_flags(u[i].type)&SC64_T4_ACTOR_HIDDEN) &&
#endif
	    (u[i].owner == SIDE || SC64_GRID_GET(visible, (u[i].x / 16) + (u[i].y / 16) * MAP_COLS))) {
#ifdef SC64_IMPORTED_MISSION
	    x = map_x +
#ifdef SC64_RACES
        sc64_minimap_pixel(u[i].x,MAP_COLS)
#else
        u[i].x * map_size / WORLD_W
#endif
        ;
	    y = map_y +
#ifdef SC64_RACES
        sc64_minimap_pixel(u[i].y,MAP_ROWS)
#else
        u[i].y * map_size / WORLD_H
#endif
        ;
#else
	    x = 185 + u[i].x / 8;
	    y = 126 + u[i].y / 8;
#endif
	    gfx_pixel(x, y,
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
                owner_color(u[i].owner)
#else
                u[i].owner == SIDE ? C_WHITE : C_RED
#endif
                );
	}
#ifdef SC64_IMPORTED_MISSION
	{
	    int vx = map_x +
#ifdef SC64_RACES
        sc64_minimap_pixel(camx,MAP_COLS)
#else
        camx * map_size / WORLD_W
#endif
        ;
	    int vy = map_y +
#ifdef SC64_RACES
        sc64_minimap_pixel(camy,MAP_ROWS)
#else
        camy * map_size / WORLD_H
#endif
        ;
	    int vw = (240 * map_size + WORLD_W - 1) / WORLD_W;
	    int vh = (108 * map_size + WORLD_H - 1) / WORLD_H;
	    gfx_frame(vx, vy, vw, vh, C_HIGHLIGHT);
	}
#else
	gfx_frame(185 + camx / 8, 126 + camy / 8, 30, 14, C_HIGHLIGHT);
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active() && screen == SCREEN_PLAY && !won && !lost &&
        race_t4.ping_ticks) {
        int px = sc64_minimap_pixel(race_t4.ping_x, MAP_COLS);
        int py = sc64_minimap_pixel(race_t4.ping_y, MAP_ROWS);
        unsigned radius = (race_t4.ping_ticks & 8u) ? 1u : 2u;
        if (px >= 0 && px < map_size && py >= 0 && py < map_size) {
            /* The cross pulses between two radii. Clip to the 24x24 map, not
             * merely the containing panel, so edge pings cannot paint its
             * frame or neighboring HUD. */
            for (int offset = -(int)radius; offset <= (int)radius; ++offset) {
                int ax = px + offset, ay = py + offset;
                if (ax >= 0 && ax < map_size)
                    gfx_pixel(map_x + ax, map_y + py, C_GOLD);
                if (ay >= 0 && ay < map_size)
                    gfx_pixel(map_x + px, map_y + ay, C_GOLD);
            }
            gfx_pixel(map_x + px, map_y + py, C_WHITE);
        }
    }
#endif
}

#if defined(SC64_T4_SCENARIO_RUNTIME) && defined(SC64_HAS_SPRITES)
#include "sc64_t4_prop_art.h"
#endif
static void
draw_building_art(Building *q)
{
#ifdef SC64_T4_RUNTIME
    if (sc64_race_t4_building_flags(q->type) &&
        !(sc64_race_t4_building_flags(q->type)&SC64_T4_ACTOR_DRAWABLE)) return;
#endif
    int x = q->x - camx, y = q->y - camy + 12;
    int col = q->owner == SIDE ? C_TERRAN : C_ZERG;
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite =
#ifdef SC64_RACES
#ifdef SC64_T5_RUNTIME
#ifdef SC64_T6_RUNTIME
        q->type == B_CONTROL_TOWER && race_t6_tech_active() ? sc64_race_unit_sprite(115) :
#endif
        q->type == B_CONTROL_TOWER ? sc64_race_unit_sprite(sc64_race_t5_building_art_raw(q->type)) :
#endif
        q->raw_type>=0?sc64_race_unit_sprite(q->raw_type):
#endif
        mission_building_sprite(q->type);
#endif
    if (x < -20 || x > 260 || y < 2 || y > 130)
	return;
    if (q->owner != SIDE && !currently_visible(q->x, q->y))
	return;
#if defined(SC64_T4_SCENARIO_RUNTIME) && defined(SC64_HAS_SPRITES)
    if (race_t4_draw_prop(q, x, y)) return;
#endif
#ifdef SC64_HAS_SPRITES
    if (sprite) draw_mission_sprite(sprite, x, y);
    else
#endif
    {
    if (q->type == B_COMMAND) {
	gfx_rect(x - 12, y - 8, 25, 18, C_STONE);
	gfx_rect(x - 9, y - 11, 18, 5, col);
	gfx_frame(x - 12, y - 8, 25, 18, C_SILVER);
	gfx_rect(x - 4, y - 13, 8, 3, C_GOLD);
    } else if (q->type == B_HATCHERY) {
	gfx_diamond(x, y, 16, 9, C_ZERG);
	gfx_diamond(x, y - 1, 10, 6, C_PURPLE);
	gfx_pixel(x, y + 4, C_BLOOD);
    } else {
	gfx_rect(x - 8, y - 7, 17, 15, C_STONE);
	gfx_rect(x - 6, y - 10, 13, 4, col);
	gfx_frame(x - 8, y - 7, 17, 15, C_SILVER);
	if (q->type == B_BARRACKS) {
	    gfx_rect(x - 2, y - 4, 5, 4, C_ORANGE);
	    gfx_line(x - 7, y + 7, x + 8, y + 7, C_GOLD);
	} else if (q->type == B_DEPOT)
	    gfx_rect(x - 5, y - 3, 11, 6, C_BLUE_DARK);
	else if (q->type == B_REFINERY)
	    gfx_rect(x - 3, y - 6, 7, 12, C_GREEN_DARK);
	else {
	    gfx_rect(x - 4, y - 4, 9, 7, C_PURPLE);
	    gfx_pixel(x, y, C_CYAN);
	}
    }
    }
}

static void
draw_building_overlay(Building *q)
{
    int x = q->x - camx, y = q->y - camy + 12;
#ifdef SC64_T4_RUNTIME
    if (sc64_race_t4_building_flags(q->type)&SC64_T4_ACTOR_INVINCIBLE) return;
#endif
    if (x < -20 || x > 260 || y < 2 || y > 130)
        return;
    if (q->owner != SIDE && !currently_visible(q->x, q->y))
        return;
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite =
#ifdef SC64_RACES
        q->raw_type>=0?sc64_race_unit_sprite(q->raw_type):
#endif
        mission_building_sprite(q->type);
    int sprite_x = sprite ? x - sprite->canvas_w / 2 + sprite->offset_x : x - 12;
    int sprite_y = sprite ? y - sprite->canvas_h / 2 + sprite->offset_y : y - 12;
    int sprite_w = sprite ? sprite->w : 24;
    int sprite_h = sprite ? sprite->h : 24;
#else
    int sprite_x = x - 12, sprite_y = y - 12, sprite_w = 24, sprite_h = 24;
#endif
    gfx_bar(x - 9, sprite_y - 4, 18, 2, q->hp, q->maxhp,
            owner_color(q->owner), C_RED_DARK);
    if (q->under_construction)
        gfx_text(x - 3, y, "C", C_GOLD);
    if (q->under_construction)
        gfx_frame(sprite_x - 2, sprite_y - 2, sprite_w + 4, sprite_h + 4, C_GOLD);
}

#ifdef SC64_IMPORTED_MISSION
#ifdef SC64_STATIC_KEY_CULL
#include "sc64_static_key.h"
#else
/* Resources have static art until their presence, placement or center-cell
 * visibility changes. Compare exact snapshots, then use a generation key. */
static unsigned
sc64_static_objects_key(void)
{
    enum { FIELDS = MAX_MINERALS * 5 + 4 * 3 + 1 + MAX_BUILDINGS *
#ifdef SC64_T4_SCENARIO_RUNTIME
                    7
#else
                    6
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
                    + 32 * 2
#endif
    };
    static int previous[FIELDS];
    static unsigned generation;
    int values[FIELDS], n = 0, changed = 0, i;
    for (i = 0; i < MAX_MINERALS; i++) {
        int x = m[i].x - camx;
        int y = m[i].y - camy + 12;
        int in_bounds = m[i].amount > 0 && x > -6 && x < 245 && y > 7 && y < 123;
        values[n++] = m[i].amount > 0;
        values[n++] = m[i].x;
        values[n++] = m[i].y;
        values[n++] = m[i].style;
        values[n++] = in_bounds && currently_visible(m[i].x, m[i].y);
    }
    values[n++] = geyser_count;
    for (i = 0; i < 4; i++) {
        values[n++] = geysers[i].x;
        values[n++] = geysers[i].y;
        {
            int x = geysers[i].x - camx;
            int y = geysers[i].y - camy + 12;
            int in_bounds = i < geyser_count && x > 0 && x < 240 && y > 12 && y < 120;
            values[n++] = in_bounds && currently_visible(geysers[i].x, geysers[i].y);
        }
    }
    for (i = 0; i < MAX_BUILDINGS; i++) {
        Building *q = &b[i];
        int x = q->x - camx;
        int y = q->y - camy + 12;
        int in_bounds = q->alive && x >= -20 && x <= 260 && y >= 2 && y <= 130;
        values[n++] = q->alive;
        values[n++] = q->x;
        values[n++] = q->y;
        values[n++] = q->type;
        values[n++] = q->owner;
#ifdef SC64_T4_SCENARIO_RUNTIME
        values[n++] = q->raw_type;
#endif
        values[n++] = in_bounds && (q->owner == SIDE || currently_visible(q->x, q->y));
    }
#ifdef SC64_T4_SCENARIO_RUNTIME
    /* Prop art is static terrain too. Door endpoints and trap visibility
     * depend on these mutable doodad bindings even when Building slots stay
     * unchanged. Reading them here keeps restart/load invalidation immediate. */
    for (i = 0; i < 32; i++) {
        values[n++] = race_t4.doodad_slot[i];
        values[n++] = race_t4.doodad_enabled[i];
    }
#endif
    for (i = 0; i < n; i++) {
        if (previous[i] != values[i]) changed = 1;
        previous[i] = values[i];
    }
    if (changed) generation++;
    return generation;
}
#endif
#define SC64_TERRAIN_STATIC_KEY sc64_static_objects_key()
#include "sc64_terrain_cache.h"
#endif

static void
draw_map(void)
{
    int i, x, y, col, row, col0 = 0, row0 = 0, col1 = MAP_COLS, row1 = MAP_ROWS;
#ifdef SC64_IMPORTED_MISSION
    u32 profile_stage_start;
#endif
#if defined(SC64_T5_RUNTIME) && defined(SC64_IMPORTED_MISSION)
    unsigned int terrain_static_key;
#endif
    camx = viewport_left();
    camy = viewport_top();
#ifdef SC64_IMPORTED_MISSION
    profile_stage_start = gba_clock();
#endif
#if defined(SC64_T5_RUNTIME) && defined(SC64_IMPORTED_MISSION)
    /* The key is unchanged between cache lookup and capture; compute the
     * roster/visibility snapshot once and pass it through both operations. */
    terrain_static_key = sc64_static_objects_key();
#endif
#ifdef SC64_IMPORTED_MISSION
    col0 = limit(camx / 16, 0, MAP_COLS - 1);
    col1 = limit((camx + 240) / 16 + 1, 0, MAP_COLS);
    row0 = limit(camy / 16, 0, MAP_ROWS - 1);
    row1 = limit((camy + 108) / 16 + 1, 0, MAP_ROWS);
#endif
    /* Each tile gets one terrain/fog pass. Unknown space stays black from
     * gfx_begin, so it needs no second opaque fill after sprites are drawn. */
#ifdef SC64_IMPORTED_MISSION
#ifdef SC64_T5_RUNTIME
    if (!sc64_terrain_cached_key(camx, camy, terrain_static_key)) {
#else
    if (!sc64_terrain_cached(camx, camy)) {
#endif
#endif
    for (row = row0; row < row1; row++)
	for (col = col0; col < col1; col++) {
	    int idx = col + row * MAP_COLS;
#ifdef SC64_IMPORTED_MISSION
            if (sc64_terrain_tile_clean(col, row)) continue;
            if (!SC64_GRID_GET(visible, idx) && !SC64_GRID_GET(explored, idx)) {
                /* Restored pixels must disappear after load/restart removes fog. */
                gfx_rect(col * 16 - camx, 12 + row * 16 - camy, 16, 16, C_BLACK);
                continue;
            }
#else
	    if (!SC64_GRID_GET(visible, idx) && !SC64_GRID_GET(explored, idx)) continue;
#endif
	    x = col * 16 - camx;
	    y = 12 + row * 16 - camy;
	    if (x >= 240 || x + 16 <= 0 || y >= 120 || y + 16 <= 12)
		continue;
	    if (!SC64_GRID_GET(visible, idx)) {
		gfx_rect(x, y, 16, 16, C_FOG);
		continue;
	    }
#ifdef SC64_HAS_TERRAIN_ART
	    gfx_blit16(x, y,
#ifdef SC64_RACES
                race_mode ?
#ifdef SC64_IMPORTED_SKIRMISH
                    (race_skirmish_map>0?sc64_skirmish_maps[race_skirmish_map-1].tile_pixels[sc64_skirmish_maps[race_skirmish_map-1].tile_lookup[idx]]:sc64_tile_pixels[0])
#else
                    sc64_tile_pixels[0]
#endif
                    :sc64_race_maps[race_selected_mission].tile_pixels[sc64_race_maps[race_selected_mission].tile_lookup[idx]]
#elif defined(SC64_TERRAN_CAMPAIGN)
                sc64_campaign_maps[campaign_mission].pixels[
                  sc64_campaign_maps[campaign_mission].lookup[idx]]
#else
                sc64_tile_pixels[
#ifdef SC64_SKIRMISH
                skirmish_ground_tile
#else
                sc64_tile_lookup[idx]
#endif
                ]
#endif
                );
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
            /* The compact arena has authored barriers; make collision visible. */
            if (
#ifdef SC64_RACES
                race_mode && race_skirmish_map==0 &&
#endif
                terrain_blocked(col, row)) {
                gfx_rect(x, y, 16, 16, C_STONE);
                gfx_frame(x + 2, y + 2, 12, 12, C_SILVER);
            }
#endif
#else
	    if (terrain_blocked(col, row)) {
		x = col * 16 - camx;
		y = 12 + row * 16 - camy;
		gfx_rect(x, y, 16, 16, C_STONE);
		gfx_line(x + 2, y + 11, x + 7, y + 5, C_SILVER);
		gfx_line(x + 7, y + 5, x + 13, y + 10, C_DIRT);
	    } else
		gfx_rect(x, y, 16, 16, C_GRASS);
#endif
	}
    for (i = 0; i < MAX_MINERALS; i++)
	if (m[i].amount > 0 && currently_visible(m[i].x, m[i].y)) {
	    x = m[i].x - camx;
	    y = m[i].y - camy + 12;
	    if (x > -6 && x < 245 && y > 7 && y < 123) {
#ifdef SC64_HAS_SPRITES
		const Sc64SpriteFrame *sprite = mission_mineral_sprite(m[i].style);
		if (sprite) draw_mission_sprite(sprite, x, y);
		else {
#endif
		gfx_diamond(x, y, 7, 4, C_CYAN);
		gfx_pixel(x, y, C_WHITE);
#ifdef SC64_HAS_SPRITES
		}
#endif
	    }
	}
    for (i = 0; i < geyser_count; i++)
	if (currently_visible(geysers[i].x, geysers[i].y)) {
	    x = geysers[i].x - camx;
	    y = geysers[i].y - camy + 12;
	    if (x > 0 && x < 240 && y > 12 && y < 120)
		gfx_diamond(x, y, 6, 4, C_GREEN);
	}
#ifdef SC64_IMPORTED_MISSION
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive)
            draw_building_art(&b[i]);
#ifdef SC64_T5_RUNTIME
    sc64_terrain_store_key(camx, camy, terrain_static_key);
#else
    sc64_terrain_store(camx, camy);
#endif
    }
    sc64_background_cycles = gba_clock() - profile_stage_start;
    profile_stage_start = gba_clock();
#endif
    for (i = 0; i < MAX_BUILDINGS; i++)
#ifdef SC64_IMPORTED_MISSION
        if (b[i].alive)
            draw_building_overlay(&b[i]);
    sc64_building_cycles = gba_clock() - profile_stage_start;
    profile_stage_start = gba_clock();
#else
        if (b[i].alive) {
            draw_building_art(&b[i]);
            draw_building_overlay(&b[i]);
        }
#endif
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive)
	    draw_unit(&u[i]);
    draw_tracers();
#ifdef SC64_IMPORTED_MISSION
    sc64_unit_draw_cycles = gba_clock() - profile_stage_start;
#endif
    x = cx - camx;
    y = cy - camy + 12;
    gfx_frame(x - 4, y - 4, 9, 9, mode == MODE_BUILD ? C_GOLD : C_WHITE);
    gfx_line(x - 7, y, x + 7, y, C_HIGHLIGHT);
    gfx_line(x, y - 7, x, y + 7, C_HIGHLIGHT);
    if (box_state == 2) {
        int left = box_anchor_x < box_end_x ? box_anchor_x : box_end_x;
        int top = box_anchor_y < box_end_y ? box_anchor_y : box_end_y;
        int width = FAST_ABS(box_anchor_x - box_end_x) + 1;
        int height = FAST_ABS(box_anchor_y - box_end_y) + 1;
        gfx_frame(left - camx, top - camy + 12, width, height, C_HIGHLIGHT);
    }
    if (menu_open) {
	#ifdef SC64_RACES
        static const char *race_labels[3][MENU_STOP + 1]={
            {"MOVE ORDER","ATTACK MOVE","BARRACKS","SUPPLY DEPOT","REFINERY","ACADEMY","ENGINEERING BAY","WEAPONS","ARMOR","STOP / HOLD"},
            {"MOVE ORDER","ATTACK MOVE","SPAWNING POOL","HATCHERY","EXTRACTOR","HYDRALISK DEN","EVOLUTION","WEAPONS","ARMOR","STOP / HOLD"},
            {"MOVE ORDER","ATTACK MOVE","GATEWAY","PYLON","ASSIMILATOR","CYBERNETICS","FORGE","WEAPONS","ARMOR","STOP / HOLD"}};
        const char **labels=race_labels[race_human];
#else
        static const char *labels[MENU_COUNT] = {
	    "MOVE ORDER", "ATTACK MOVE", "BUILD BARRACKS", "BUILD DEPOT",
	    "BUILD REFINERY", "BUILD ACADEMY", "BUILD ENG BAY",
	    "WEAPON UPGRADE", "ARMOR UPGRADE", "STOP / HOLD"
	};
#endif
	gfx_rect(137, 14, 100, 96, C_PANEL);
	gfx_frame(137, 14, 100, 96, C_EDGE);
#ifdef SC64_T5_RUNTIME
        int first = 0, count = field_menu_count();
        static const char *const advanced_labels[] = {
            "FACTORY", "STARPORT", "CONTROL TOWER", "UNLOAD CARGO"
#ifdef SC64_TERRAN_LATE_RUNTIME
            , "LIFT / LAND", "COMMAND CENTER"
#endif
        };
        if (menu_cursor >= 10) first = menu_cursor - 9;
	for (i = first; i < count && i < first + 10; i++) {
            const char *label = i >= MENU_FACTORY ?
                advanced_labels[i - MENU_FACTORY] : labels[i];
	    gfx_text(142, 17 + (i - first) * 9, i == menu_cursor ? ">" : " ", C_GOLD);
	    gfx_text(149, 17 + (i - first) * 9, label, i == menu_cursor ? C_WHITE : C_DIM);
	}
#else
	for (i = 0; i < MENU_COUNT; i++) {
	    gfx_text(142, 17 + i * 9, i == menu_cursor ? ">" : " ", C_GOLD);
	    gfx_text(149, 17 + i * 9, labels[i], i == menu_cursor ? C_WHITE : C_DIM);
	}
#endif
    }
    if (mode == MODE_BUILD && box_state != 2) {
	gfx_frame(x - 8, y - 6, 17, 13, (can_place(menu, cx, cy)
#ifdef SC64_RACES
            &&race_can_site(SIDE,menu,cx,cy)
#endif
            ) ? C_GREEN : C_RED);
    }
    gfx_frame(0, 12, 240, 108, C_EDGE);
}

#ifdef SC64_RACES
#include "sc64_frontend_draw.h"
#endif

static void
draw_title(void)
{
#ifdef SC64_RACES
    sc64_frontend_draw();return;
#endif
    gfx_rect(0, 0, 240, 160, C_BLACK);
    gfx_rect(15, 18, 210, 124, C_PANEL);
    gfx_frame(15, 18, 210, 124, C_CYAN);
    gfx_text(45, 32, "STARCRAFT", C_GOLD);
#ifdef SC64_RACES
    gfx_text(45,45,race_mode?"THREE RACE SKIRMISH":"ZERG / PROTOSS",C_WHITE);
#elif defined(SC64_TERRAN_CAMPAIGN)
    gfx_text(45, 45, "TERRAN CAMPAIGN", C_WHITE);
#elif defined(SC64_BACKWATER)
    gfx_text(30, 45, "BACKWATER STATION", C_WHITE);
#elif defined(SC64_SKIRMISH)
    gfx_text(48, 45, "TERRAN SKIRMISH", C_WHITE);
#elif defined(SC64_IMPORTED_MISSION)
	 gfx_text(39, 45, "THE WASTELAND", C_WHITE);
#else
    gfx_text(39, 45, "TERRAN OUTPOST", C_WHITE);
#endif
    gfx_rect(54, 59, 132, 28, C_DIRT);
    gfx_diamond(83, 72, 14, 8, C_ZERG);
    gfx_rect(135, 67, 16, 12, C_TERRAN);
    gfx_line(95, 73, 133, 73, C_GOLD);
#ifdef SC64_RACES
    if(race_mode) {
        gfx_text(28,94,sc64_race_names[race_human],C_GOLD);gfx_num(159,94,race_bots,C_WHITE);
        gfx_text(28,105,"LEFT/RIGHT RACE UP/DN AI",C_CYAN);
    } else {gfx_text(28,94,sc64_race_maps[race_selected_mission].title,C_WHITE);gfx_text(28,105,"UP/DOWN SELECT MISSION",C_CYAN);}
    gfx_text(28,116,"SELECT MODE  A START B LOAD",C_CYAN);
#elif defined(SC64_TERRAN_CAMPAIGN)
    gfx_text(28, 94, sc64_campaign_maps[campaign_mission].title, C_WHITE);
    gfx_text(28, 105, campaign_mission ? "DESTROY INFESTED BASE" : "FIND JIM RAYNOR", C_GOLD);
    gfx_text(28, 116, "UP/DN SELECT A START B LOAD", C_CYAN);
#elif defined(SC64_BACKWATER)
    gfx_text(28, 94, "DESTROY THE INFESTED BASE", C_WHITE);
    gfx_text(28, 105, "JIM RAYNOR MUST SURVIVE", C_GOLD);
    gfx_text(28, 116, "A START  B LOAD", C_CYAN);
#elif defined(SC64_SKIRMISH)
    gfx_text(28, 94, "COMPUTER OPPONENTS", C_WHITE);
    gfx_num(148, 94, skirmish_bots, C_GOLD);
    gfx_text(28, 105, "UP/DOWN CHOOSE 1 TO 3", C_CYAN);
    gfx_text(28, 116, "A START  B LOAD", C_CYAN);
#elif defined(SC64_IMPORTED_MISSION)
	 gfx_text(35, 101, "FIND JIM RAYNOR", C_WHITE);
	 gfx_text(35, 112, "BUILD UP THE OUTPOST", C_CYAN);
#else
	 gfx_text(35, 101, "A NEW MISSION", C_WHITE);
    gfx_text(35, 112, "B LOAD SAVE", C_CYAN);
#endif
    gfx_text(35, 126, "GBA RTS PROTOTYPE", C_DIM);
}

static void
draw_pause(void)
{
    gfx_rect(29, 18, 182, 124, C_PANEL);
    gfx_frame(29, 18, 182, 124, C_CYAN);
    gfx_text(47, 27, "FIELD COMMAND", C_GOLD);
    gfx_text(47, 41, "A RESUME", C_WHITE);
    gfx_text(47, 52, "B SAVE GAME", C_WHITE);
    gfx_text(47, 63, "L LOAD SAVE", C_WHITE);
    gfx_text(47, 74, "R RESTART MISSION", C_WHITE);
    gfx_text(47, 93, "D-PAD MOVE CURSOR", C_DIM);
    gfx_text(47, 103, "PLAY B COMMAND  SEL PAUSE", C_DIM);
    gfx_text(47, 113, "L TAP NEXT HOLD BOX R ARMY", C_DIM);
    gfx_text(47, 125, "WORKER: A MINE / A GAS", C_CYAN);
}

static void
draw_result(void)
{
    gfx_rect(28, 40, 184, 66, C_PANEL);
    gfx_frame(28, 40, 184, 66, won ? C_GOLD : C_RED);
    gfx_text(won ? 81 : 79, 53, won ? "MISSION CLEAR" : "OUTPOST LOST", won ? C_GOLD : C_RED);
#ifdef SC64_RACES
    gfx_text(32,68,won?(race_mode?"OPPONENTS ELIMINATED":race_campaign_victory_text()):"MISSION FAILED",C_WHITE);
#elif defined(SC64_TERRAN_CAMPAIGN)
    gfx_text(53, 68, won ? (campaign_mission ? "BACKWATER SECURED" : "WASTELAND SECURED") : "MISSION FAILED", C_WHITE);
#elif defined(SC64_BACKWATER)
    gfx_text(38, 68, won ? "BACKWATER SECURED" : "MISSION FAILED", C_WHITE);
#elif defined(SC64_SKIRMISH)
    gfx_text(53, 68, won ? "OPPONENTS ELIMINATED" : "FORCES ELIMINATED", C_WHITE);
#elif defined(SC64_IMPORTED_MISSION)
    gfx_text(53, 68, won ? "WASTELAND SECURED" : "MISSION FAILED", C_WHITE);
#else
    gfx_text(53, 68, won ? "ZERG BASE ELIMINATED" : "COMMAND CENTER DESTROYED", C_WHITE);
#endif
    #ifdef SC64_RACES
    gfx_text(39,86,won&&!race_mode&&sc64_catalog_available_next(sc64_catalog_mission_faction(race_selected_mission),race_selected_mission)>=0?"A NEXT MISSION B TITLE":"A RETRY B TITLE SCREEN",C_CYAN);
#else
    gfx_text(47, 86, "A RETRY    B TITLE SCREEN", C_CYAN);
#endif
}

static const char *
unit_name(int type)
{
#ifdef SC64_RACES
    return sc64_race_unit_name(type);
#endif
    if (type == U_SCV)
	return "SCV WORKER";
    if (type == U_MARINE)
	return "MARINE";
    if (type == U_FIREBAT)
	return "FIREBAT";
    if (type == U_MEDIC)
	return "MEDIC";
    if (type == U_ZERGLING)
	return "ZERGLING";
    if (type == U_RAYNOR)
	return "RAYNOR";
    return "HYDRALISK";
}

static void
draw_hud_resources(void)
{
    gfx_rect(0, 0, 240, 12, C_PANEL);
    gfx_text(3, 3, "MIN", C_GOLD);
    gfx_num(24, 3, minerals, C_WHITE);
    gfx_text(61, 3, "GAS", C_CYAN);
    gfx_num(82, 3, gas, C_WHITE);
    gfx_text(127, 3, "SUP", C_WHITE);
    gfx_num(151,3,
#ifdef SC64_RACES
        (supply_used+race_owner_queued_supply(SIDE))/2,
#else
        supply_used/2,
#endif
        C_WHITE);
    gfx_text(165, 3, "/", C_WHITE);
    gfx_num(173, 3, supply_cap / 2, C_WHITE);
}

static void
draw_hud_commands(void)
{
    int i, n = selected_count();
    gfx_rect(0, 121, 240, 39, C_PANEL);
    gfx_line(0, 120, 240, 120, C_EDGE);
    if (box_state == 2) {
        gfx_text(5, 126, "BOX SELECT", C_CYAN);
        gfx_text(5, 137, "RELEASE L SELECT B CANCEL", C_GOLD);
    } else if (selected_building >= 0 && b[selected_building].alive) {
	Building *q = &b[selected_building];
	#ifdef SC64_RACES
        gfx_text(5,126,sc64_race_building_name(q->type),C_WHITE);
#else
	gfx_text(5, 126, q->type == B_COMMAND ? "COMMAND CENTER" :
		 q->type == B_BARRACKS ? "BARRACKS" : q->type == B_DEPOT ? "SUPPLY DEPOT" :
		 q->type == B_REFINERY ? "REFINERY" : q->type == B_ACADEMY ? "ACADEMY" :
                 q->type == B_BUNKER ? "BUNKER" : q->type == B_INFESTED_COMMAND ? "INFESTED COMMAND" :
                 q->type == B_CREEP_COLONY ? "CREEP COLONY" : "ENGINEERING BAY", C_WHITE);
#endif
#ifdef SC64_RACES
        if(!q->under_construction) {
            gfx_text(5,137,unit_name(train_type),C_WHITE);
            gfx_text(65,137,"A TRAIN R TYPE L CANCEL",C_GOLD);
        } else gfx_text(5,137,"UNDER CONSTRUCTION",C_GOLD);
#else
	if (q->under_construction)
	    gfx_text(5, 137, "UNDER CONSTRUCTION", C_GOLD);
	else if (q->type == B_BARRACKS) {
            gfx_text(65, 126, unit_name(train_type), C_WHITE);
            gfx_text(125, 126, "Q", C_CYAN);
            gfx_num(137, 126, q->queue_count, C_GOLD);
            gfx_text(5, 137, "A TRAIN L CANCEL R TYPE", C_GOLD);
	} else if (q->type == B_COMMAND)
	    gfx_text(5, 137, "A TRAIN SCV  L CANCEL QUEUE", C_GOLD);
	else if (q->type == B_REFINERY)
	    gfx_text(5, 137, "A ASSIGN WORKER TO GAS", C_CYAN);
#endif
    } else if (n) {
	for (i = 0; i < MAX_UNITS; i++)
	    if (u[i].alive && u[i].selected) {
		gfx_text(5, 126, n > 1 ? "COMBAT GROUP" : unit_name(u[i].type), C_WHITE);
		gfx_text(5, 137, "HP", C_CYAN);
		gfx_num(24, 137, u[i].hp, C_GREEN);
		if (IS_WORKER(u[i].type))
		    gfx_text(52, 137, "AUTO MINE / A GAS", C_GOLD);
		else
#ifdef SC64_T5_RUNTIME
                if (u[i].type == U_DROPSHIP) {
                    gfx_text(52, 137, "CARGO", C_GOLD);
                    gfx_num(90, 137, race_transport_cargo_space(i), C_WHITE);
                    gfx_text(104, 137, "/8  B UNLOAD", C_GOLD);
                } else
#endif
		    gfx_text(52, 137, "A ORDER B COMMANDS", C_GOLD);
		break;
	    }
    } else {
	gfx_text(5, 126, mode == MODE_BUILD ? "BUILD PLACEMENT" :
#ifdef SC64_TERRAN_LATE_RUNTIME
                 mode == MODE_LAND ? "LANDING SITE" :
#endif
		 mode == MODE_ATTACK ? "ATTACK MOVE" : mode == MODE_MOVE ? "MOVE ORDER" : "FIELD COMMAND", C_WHITE);
	gfx_text(5, 137, "B COMMANDS  A SELECT / PLACE", C_GOLD);
    }
#ifdef SC64_RACES
    if(race_mode)gfx_text(5,149,"DESTROY ALL ENEMY BUILDINGS",C_GOLD);
    else {
        static const char *const objective[4]={"POOL + DEN / CLEAR TERRAN BASE","DRONE: CARRY CHRYSALIS TO WARP","MEET FENIX / CLEAR ZERG BASE","PROTECT FENIX / KILL CEREBRATE"};
        if(race_selected_mission<4)
            gfx_text(5,149,objective[race_selected_mission],C_GOLD);
#ifdef SC64_UNIFIED
        else if(race_selected_mission==4)
            gfx_text(5,149,"FIND RAYNOR / BUILD OUTPOST",C_GOLD);
        else if(race_selected_mission==5)
            gfx_text(5,149,"DESTROY INFESTED BASE",C_GOLD);
#ifdef SC64_T3_ENABLED
        else if(race_selected_mission==6) {
            unsigned seconds=(race_t3.remaining_ticks+29u)/30u;
            char timer[6]={(char)('0'+seconds/600u),(char)('0'+seconds/60u%10u),':',
                (char)('0'+seconds/10u%6u),(char)('0'+seconds%10u),0};
            gfx_text(5,149,race_t3.objective?"HOLD / KILL HATCHERY":"HOLD BASE",C_GOLD);
            gfx_text(137,149,timer,C_WHITE);
        }
#endif
#ifdef SC64_T6_RUNTIME
        else if(race_t6_active())
            gfx_text(5,149,"RAYNOR + 2 DROPSHIPS / NORAD II",C_GOLD);
#endif
#ifdef SC64_Z3_ENABLED
        else if(race_selected_mission==14)
            gfx_text(5,149,race_z3.sequence?"CHRYSALIS / MISSION REPORT":"PROTECT CHRYSALIS / CLEAR TERRAN",C_GOLD);
#endif
#ifdef SC64_P3_RUNTIME
        else if(race_selected_mission==SC64_P3_GLOBAL_ID)
            gfx_text(5,149,race_p3.sequence?"HIGHER GROUND / MISSION REPORT":"DESTROY BOTH ZERG COLONIES",C_GOLD);
#endif
#endif
        if(race_selected_mission==3&&race_campaign.elapsed_frames<27000u) {
            gfx_text(170,126,"ETA",C_CYAN);
            gfx_num(194,126,(27000u-race_campaign.elapsed_frames+29u)/30u,C_WHITE);
        }
    }
#elif defined(SC64_TERRAN_CAMPAIGN)
    gfx_text(5, 149, campaign_mission ? "DESTROY INFESTED BASE" : "FIND RAYNOR / BUILD OUTPOST", C_GOLD);
#elif defined(SC64_BACKWATER)
    gfx_text(5, 149, "DESTROY INFESTED BASE", C_GOLD);
#elif defined(SC64_SKIRMISH)
    { int alive = 0; for (i = 0; i < skirmish_bots; i++) alive += skirmish_owner_alive(skirmish_owner_ids[i]);
      gfx_text(5, 149, "ENEMY PLAYERS", C_GOLD); gfx_num(90, 149, alive, C_WHITE); }
#elif defined(SC64_IMPORTED_MISSION)
	{
	    int found = 0, have_barracks = 0, marines = 0;
	    for (i = 0; i < MAX_UNITS; i++) if (u[i].alive && u[i].owner == SIDE) {
		if (u[i].type == U_RAYNOR) found = 1;
		if (u[i].type == U_MARINE) marines++;
	    }
	    for (i = 0; i < MAX_BUILDINGS; i++)
		if (b[i].alive && b[i].owner == SIDE && b[i].type == B_BARRACKS) have_barracks = 1;
	    if (mission_unsupported_count) {
		gfx_text(5, 149, "DATA SKIPPED", C_RED);
		gfx_num(83, 149, mission_unsupported_count, C_RED);
	    } else if (!found)
		gfx_text(5, 149, "FIND RAYNOR / BUILD BARRACKS", C_GOLD);
	    else if (!have_barracks)
		gfx_text(5, 149, "OBJ BUILD A BARRACKS", C_GOLD);
	    else {
		gfx_text(5, 149, "OBJ TRAIN MARINES", C_GOLD);
		gfx_num(112, 149, marines, C_WHITE);
		gfx_text(127, 149, "/10", C_WHITE);
	    }
	}
#else
	gfx_text(5, 151, "L+DPAD BOX R ARMY SEL PAUSE", C_DIM);
#endif
}

#ifdef SC64_IMPORTED_MISSION
static _Alignas(4) u8 sc64_hud_top[240 * 12], sc64_hud_bottom[240 * 40];
static int sc64_hud_previous[24], sc64_hud_valid;
#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_NO_FRAME_CLEAR)
static u8 sc64_hud_buffer_dirty[2];
static u32 sc64_hud_buffer_clear_generation[2];
#endif

#ifdef SC64_LATE_TERRAN_STORAGE
/* Keep the expanded actor loop and target search in IWRAM. This once-per-
 * render HUD invalidation pass can execute from ROM without using its stack
 * reserve. Its timing impact is measured with the late-profile replay. */
static int __attribute__((noinline))
#else
static int GBA_HOT __attribute__((noinline))
#endif
sc64_hud_changed(void)
{
    int values[24] = {minerals, gas, supply_used, supply_cap, mode, selected_building,
        train_type, mission_unsupported_count};
    int i, first = -1, changed = sc64_hud_valid ? 0 : 3;
    for (i = 0; i < MAX_UNITS; i++) if (u[i].alive && u[i].owner == SIDE) {
        if (u[i].selected) { values[8]++; if (first < 0) first = i; }
        if (u[i].type == U_MARINE) values[9]++;
        if (u[i].type == U_RAYNOR) values[10] = 1;
    }
    if (first >= 0) { values[11] = u[first].type; values[12] = u[first].hp; }
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_BARRACKS) values[13] = 1;
    if (selected_building >= 0 && b[selected_building].alive) {
        Building *q = &b[selected_building];
        values[14] = 1; values[15] = q->type; values[16] = q->under_construction;
        values[17] = q->queue_count;
    }
    values[18] = box_state == 2;
#ifdef SC64_RACES
    values[19]=race_campaign.phase;values[20]=race_campaign.message;
#ifdef SC64_P3_RUNTIME
    if (!race_mode && race_selected_mission == SC64_P3_GLOBAL_ID)
        values[20] |= (int)race_p3.sequence << 8;
#endif
    /* race_tick_state has already counted live supply before rendering. */
    values[23]=supply_used+race_owner_queued_supply(SIDE);
    values[21]=race_selected_mission;
#ifdef SC64_T5_RUNTIME
    /* T5 has no HUD text driven by the elapsed frame counter. Keep the
     * once-per-second invalidation for timed missions such as T1 and T3. */
    values[22]=race_t5_active()?0:frame/30;
#else
    values[22]=frame/30;
#endif
#endif
#ifdef SC64_BACKWATER
#ifdef SC64_TERRAN_CAMPAIGN
    values[19] = campaign_mission == 1 ? backwater.objective : mission_objective_stage;
    values[20] = campaign_completed;
#else
    values[19] = backwater.objective;
#endif
#elif defined(SC64_SKIRMISH)
    for (i = 0; i < skirmish_bots; i++) values[19] += skirmish_owner_alive(skirmish_owner_ids[i]);
#endif
    for (i = 0; i < 24; i++) {
        /* Wallet and supply changes only affect the top band. Commands,
           selection and objectives can retain their independently cached art. */
        if (sc64_hud_previous[i] != values[i]) changed |= i < 4 || i == 23 ? 1 : 2;
        sc64_hud_previous[i] = values[i];
    }
    return changed;
}
#endif

static void
draw_hud(void)
{
#ifdef SC64_IMPORTED_MISSION
#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_NO_FRAME_CLEAR)
    int changed = sc64_hud_changed();
    u8 buffer = gfx_backbuffer_index();
    u8 dirty;
    u32 generation = gfx_clear_generation(buffer);
    if (changed) {
        sc64_hud_buffer_dirty[0] |= (u8)changed;
        sc64_hud_buffer_dirty[1] |= (u8)changed;
    }
    dirty = sc64_hud_buffer_dirty[buffer];
    if (sc64_hud_buffer_clear_generation[buffer] != generation) dirty |= 3;
    if (dirty & 1) {
        if (changed & 1) {
            draw_hud_resources();
            gfx_read_band(0, 12, sc64_hud_top);
        } else gfx_blit_band(0, 12, sc64_hud_top);
        dirty &= (u8)~1;
    }
    if (dirty & 2) {
        if (changed & 2) {
            draw_hud_commands();
            gfx_read_band(120, 40, sc64_hud_bottom);
        } else gfx_blit_band(120, 40, sc64_hud_bottom);
        dirty &= (u8)~2;
    }
    sc64_hud_buffer_dirty[buffer] = dirty;
    sc64_hud_buffer_clear_generation[buffer] = generation;
    sc64_hud_valid = 1;
#else
    int changed = sc64_hud_changed();
    if (changed & 1) {
        draw_hud_resources();
        gfx_read_band(0, 12, sc64_hud_top);
    } else gfx_blit_band(0, 12, sc64_hud_top);
    if (changed & 2) {
        draw_hud_commands();
        gfx_read_band(120, 40, sc64_hud_bottom);
    } else gfx_blit_band(120, 40, sc64_hud_bottom);
    sc64_hud_valid = 1;
#endif
#else
    draw_hud_resources();
    draw_hud_commands();
#endif
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_TERRAIN)
    u32 minimap_start = gba_clock();
#endif
    draw_minimap();
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_TERRAIN)
    sc64_hud_minimap_cycles = gba_clock() - minimap_start;
#endif
#ifdef SC64_RACES
    if(!race_mode&&race_campaign.message_ticks) {
        gfx_rect(2,94,236,24,C_PANEL);
        const char *message=race_campaign_message_text();
        char line[39];int n=0;
        while(message[n]&&n<38){line[n]=message[n];n++;}line[n]=0;
        if(message[n]){int last=n;while(last>0&&line[last-1]!=' ')last--;if(last>0)n=last;line[n]=0;}
        gfx_text(5,96,line,C_CYAN);
        message+=n;while(*message==' ')message++;
        for(n=0;message[n]&&n<38;n++)line[n]=message[n];
        line[n]=0;
        gfx_text(5,106,line,C_CYAN);
    }
#endif
    if (toast_timer && toast_id >= 0 && toast_id < 15) {
	gfx_rect(54, 105, 132, 12, C_PANEL);
	gfx_frame(54, 105, 132, 12, C_EDGE);
	gfx_text(59, 108, toast_text[toast_id], C_GOLD);
    }
}

void
game_render(void)
{
#ifdef SC64_IMPORTED_MISSION
    u32 profile_start;
#endif
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_NO_FRAME_CLEAR)
    /* Imported live scenes overwrite every pixel in the map and HUD bands.
     * Title pages retain the regular clear; coverage fixtures exercise live,
     * paused and result scenes over poisoned backbuffers. */
    if (screen == SCREEN_TITLE) gfx_begin(C_BLACK);
    else gfx_begin_preserve();
#else
    gfx_begin(C_BLACK);
#endif
    if (screen == SCREEN_TITLE)
	draw_title();
    else {
#ifdef SC64_IMPORTED_MISSION
        profile_start = gba_clock();
#endif
	draw_map();
#ifdef SC64_IMPORTED_MISSION
        sc64_map_cycles = gba_clock() - profile_start;
        profile_start = gba_clock();
#endif
	draw_hud();
#ifdef SC64_IMPORTED_MISSION
        sc64_hud_cycles = gba_clock() - profile_start;
#endif
	if (screen == SCREEN_PAUSE) {
	    draw_pause();
	#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_NO_FRAME_CLEAR)
	    /* The pause panel crosses the bottom cached HUD band. */
	    sc64_hud_buffer_dirty[gfx_backbuffer_index()] |= 2;
	#endif
	} else if (screen == SCREEN_RESULT)
	    draw_result();
    }
    gfx_end();
}

int
game_self_test(void)
{
    int i;
#ifndef SC64_IMPORTED_MISSION
    int t, enemy, before, last;
#endif
    init_state();
#ifdef SC64_RACES
    return MAP_COLS!=64 || MAP_ROWS!=64 || race_owner_supply(SIDE)<=0 ||
        race_owner_supply_cap(SIDE)<=0 || !owners_hostile(SIDE,ENEMY) ||
        owners_hostile(SIDE,RESCUABLE);
#elif defined(SC64_BACKWATER)
    return mission_unsupported_count || bw_count(0, 17, 19, 64) != 1 || bw_count(0, 19, 130, 64) != 1;
#elif defined(SC64_SKIRMISH)
    return skirmish_bots < 1 || skirmish_bots > 3 || owner_command_center(SIDE) < 0;
#endif
#ifdef SC64_IMPORTED_MISSION
    {
	int scv = 0, marine = 0, raynor = 0, zergling = 0;
	int command = 0, depot = 0, barracks = 0, mineral = 0, raynor_slot = -1, side_slot = -1;
	if (MAP_COLS != 64 || MAP_ROWS != 64 || SC64_IMPORTED_UNIT_COUNT != 46)
	    return 1;
	for (i = 0; i < MAX_UNITS; i++) if (u[i].alive) {
	    if (u[i].owner == SIDE && side_slot < 0) side_slot = i;
	    if (IS_WORKER(u[i].type) && u[i].owner == SIDE) scv++;
	    else if (u[i].type == U_MARINE && u[i].owner == SIDE) marine++;
	    else if (u[i].type == U_RAYNOR) { raynor++; raynor_slot = i; }
	    else if (u[i].type == U_ZERGLING && u[i].owner == ENEMY) zergling++;
	}
	for (i = 0; i < MAX_BUILDINGS; i++) if (b[i].alive) {
	    if (b[i].type == B_COMMAND && b[i].owner == SIDE) command++;
	    else if (b[i].type == B_DEPOT && b[i].owner == SIDE) depot++;
	    else if (b[i].type == B_BARRACKS && b[i].owner == SIDE) barracks++;
	}
	for (i = 0; i < MAX_MINERALS; i++) if (m[i].amount > 0) mineral++;
	if (scv != 5 || marine != 5 || raynor != 1 || zergling != 23 ||
	    u[raynor_slot].owner != NEUTRAL || u[raynor_slot].maxhp != 200 ||
	    command != 1 || depot != 1 || barracks != 0 || mineral != 10 ||
	    supply_cap != 36 || supply_used != 20 || mission_unsupported_count)
	    return 2;
	check_imported_mission_outcome();
	if (won || lost) return 3;
	/* Touching the neutral hero recruits him and reserves his two supply. */
	u[side_slot].x = u[raynor_slot].x; u[side_slot].y = u[raynor_slot].y;
	update_imported_mission();
	if (u[raynor_slot].owner != SIDE || supply_used != 22) return 4;
	frame = 29; minerals = 0; update_imported_mission();
	if (mission_objective_stage || minerals) return 5;
	frame = 30; update_imported_mission();
	if (!mission_objective_stage || minerals != 40) return 6;
	for (i = 0; i < 5; i++)
	    if (spawn_unit(SIDE, U_MARINE, 1100 + i * 8, 1100) < 0) return 7;
	if (spawn_build(SIDE, B_BARRACKS, 1200, 1200, 0) < 0) return 8;
	check_imported_mission_outcome();
	if (!won || lost) return 9;
	init_state();
	for (i = 0; i < MAX_UNITS; i++)
	    if (u[i].alive && u[i].type == U_RAYNOR) u[i].alive = 0;
	check_imported_mission_outcome();
	if (!lost || won) return 10;
	init_state();
	/* The other source defeat trigger needs no CC/Barracks, at most nine
	 * Marines, and at most 399 minerals; losing only the CC is not enough. */
	b[0].alive = 0;
	check_imported_mission_outcome();
	if (lost) return 11;
	for (i = 0; i < MAX_BUILDINGS; i++) b[i].alive = 0;
	check_imported_mission_outcome();
	if (!lost) return 12;
	/* A sampled straight-line shortcut used to accept a different route
	 * from the movement rule and stick at this real cliff corner. */
	init_state();
	if (direct_path_clear(175, 658, 240, 456)) return 13;
	/* Complete a real outbound/mining/return cycle from an original SCV
	 * placement using the imported terrain, resource field and base. Other
	 * actors are removed to isolate routing from combat/crowd interference. */
	{
	    int worker = -1, patch = -1, best = 0x7fffffff;
	    for (i = 0; i < MAX_UNITS; i++) if (u[i].alive && u[i].owner == SIDE && IS_WORKER(u[i].type)) {
		int candidate = nearest_mineral(u[i].x, u[i].y);
		int distance = candidate < 0 ? 0x7fffffff : dist2(u[i].x, u[i].y, m[candidate].x, m[candidate].y);
		if (distance < best) { worker = i; patch = candidate; best = distance; }
	    }
	    if (worker < 0) return 14;
	    for (i = 0; i < MAX_UNITS; i++) if (i != worker) u[i].alive = 0;
	    u[worker].order = ORDER_MINE; u[worker].target = patch; minerals = 0;
	    for (i = 0; i < 5000 && minerals < 8; i++) update_orders();
	    if (minerals < 8 || u[worker].carry) return 15;
	}
	/* Loading in a fresh game must not repeat the one-time mineral grant. */
	init_state();
	frame = 30; update_imported_mission(); minerals = 321;
	if (!save_game()) return 16;
	init_state();
	if (!load_game() || mission_objective_stage != 1 || minerals != 321) return 17;
	update_imported_mission();
	if (minerals != 321) return 18;
	/* A save before the trigger still receives exactly one grant later. */
	init_state(); frame = 20;
	if (!save_game()) return 19;
	init_state();
	if (!load_game() || mission_objective_stage) return 20;
	frame = 30; update_imported_mission();
	if (minerals != 40 || mission_objective_stage != 1) return 21;
	/* Retain the whole friendly starting formation to catch workers
	 * deadlocking behind one another. Remove enemies only to isolate the
	 * economy from the player's escort and defense decisions. */
	init_state();
	for (i = 0; i < MAX_UNITS; i++) if (u[i].owner == ENEMY) u[i].alive = 0;
	for (i = 0; i < 4000; i++) game_update(0, 0);
	if (minerals <= 40 || lost || won) return 22;
        /* Cell-boundary jumps must visit exactly the same collision cells
         * as the old pixel-by-pixel trajectory in every direction. */
        {
            u32 check_seed = 0x9182bfu;
            for (i = 0; i < 2000; i++) {
                int x = (int)(rng_next(&check_seed) % WORLD_W);
                int y = (int)(rng_next(&check_seed) % WORLD_H);
                int tx = (int)(rng_next(&check_seed) % WORLD_W);
                int ty = (int)(rng_next(&check_seed) % WORLD_H);
                int px = x, py = y, previous = x / 16 + y / 16 * MAP_COLS, clear = 1;
                while (px != tx || py != ty) {
                    int cell;
                    if (px < tx) px++; else if (px > tx) px--;
                    if (py < ty) py++; else if (py > ty) py--;
                    cell = px / 16 + py / 16 * MAP_COLS;
                    if (cell != previous && !is_traversable(px / 16, py / 16, -1)) { clear = 0; break; }
                    previous = cell;
                }
                if (direct_path_clear(x, y, tx, ty) != clear) return 23;
            }
        }
        /* Reaching a waypoint must not leave an armed escort defenseless. */
        init_state();
        memset(u, 0, sizeof(u)); supply_used = 0;
        {
            int guard = spawn_unit(SIDE, U_MARINE, 80, 80);
            int foe = spawn_unit(ENEMY, U_ZERGLING, 100, 80);
            update_orders();
            if (u[guard].order != ORDER_IDLE || u[foe].hp >= u[foe].maxhp) return 24;
        }
#ifdef GBA_HOST
        /* Reusing the two HUD bands must be pixel-identical to drawing the
         * text again, including a resource change that invalidates the cache. */
        init_state();
        for (int state = 0; state < 2; state++) {
            u32 expected = 2166136261u, actual = 2166136261u;
            minerals = state ? 1234 : 40;
            sc64_hud_valid = 0;
            game_render();
            for (int y = 0; y < 160; y++) for (int x = 0; x < 240; x++)
                expected = (expected ^ gfx_test_pixel(x, y)) * 16777619u;
            game_render();
            for (int y = 0; y < 160; y++) for (int x = 0; x < 240; x++)
                actual = (actual ^ gfx_test_pixel(x, y)) * 16777619u;
            if (actual != expected) return 25;
        }
#endif
    }
    return 0;
#else
    if (minerals != 500 || supply_used != 26 || supply_cap != 36)
        return 1;
    /* Starting Marines overlap an inflated Command Center tile. They must
     * escape it while movement into a different blocked tile stays illegal. */
    if (is_traversable(5, 4, -1) || !is_traversable(5, 4, 5 + 4 * MAP_COLS) ||
        is_traversable(4, 5, 5 + 4 * MAP_COLS))
        return 42;
    for (i = 0; i < MAX_UNITS; i++) if (i != 5) u[i].alive = 0;
    for (i = 0; i < 30; i++) move_toward(&u[5], 104, 72);
    if (u[5].x < 96) return 43;
    init_state();
    if (unit_cost(U_MARINE) != 50 || unit_supply(U_MARINE) != 2 ||
        unit_supply(U_ZERGLING) != 1 || unit_build_time(U_SCV) != 375)
        return 2;
    if (!has_tech_for(B_BARRACKS) || !can_place(B_REFINERY, geysers[0].x, geysers[0].y) ||
        can_place(B_REFINERY, 200, 180))
        return 3;
    b[2].under_construction = 1;
    if (has_tech_for(B_BARRACKS))
        return 4;
    b[2].under_construction = 0;

    t = spawn_unit(SIDE, U_MARINE, 110, 105);
    if (t < 0 || supply_used != 28)
        return 5;
    hit_target(t, 6, U_ZERGLING, ENEMY);
    if (u[t].hp != 34)
        return 6;
    hit_target(t, 40, U_ZERGLING, ENEMY);
    if (u[t].alive || supply_used != 26)
        return 7;
    if (can_place(B_DEPOT, 80, 88) || !can_place(B_DEPOT, 200, 180))
        return 8;

    /* Queue slots reserve supply, reject overflow, and refund cancellation. */
    minerals = 1000;
    supply_cap = 28;
    queue_unit(3, U_MARINE);
    if (b[3].queue_count != 1 || reserved_supply() != 28 || minerals != 950)
        return 9;
    queue_unit(3, U_MARINE);
    if (b[3].queue_count != 1 || minerals != 950)
        return 10;
    cancel_queue(3);
    if (b[3].queue_count || minerals != 987)
        return 11;
    supply_cap = 36;
    queue_unit(3, U_MARINE);
    for (i = 0; i < unit_build_time(U_MARINE); i++)
        update_production();
    if (b[3].queue_count || supply_used != 28)
        return 12;

    /* Attack-move keeps advancing to its destination once nearby foes are gone. */
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    invalidate_path_grid();
    supply_used = 0;
    t = spawn_unit(SIDE, U_MARINE, 80, 120);
    u[t].selected = 1;
    issue_order(ORDER_ATTACK_MOVE, 300, 200);
    before = dist2(u[t].x, u[t].y, 300, 200);
    for (i = 0; i < 100; i++)
        update_orders();
    if (dist2(u[t].x, u[t].y, 300, 200) >= before)
        return 13;

    /* BFS routing skirts static terrain and a structure blocking the corridor. */
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    invalidate_path_grid();
    supply_used = 0;
    t = spawn_unit(SIDE, U_MARINE, 112, 120);
    spawn_build(ENEMY, B_BARRACKS, 160, 120, 0);
    if (is_traversable(10, 7, -1))
	return 40;
    hit_target(MAX_UNITS, 2000, U_MARINE, SIDE);
    if (!is_traversable(10, 7, -1))
	return 41;
    spawn_build(ENEMY, B_BARRACKS, 160, 120, 0);
    for (i = 0; i < 1200; i++)
        move_toward(&u[t], 232, 120);
    if (u[t].x < 205 || dist2(u[t].x, u[t].y, 160, 120) < 18 * 18)
	return 14;

    /* Enemy units choose the Command Center and the Hatchery schedules waves. */
    init_state();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].owner == SIDE)
            u[i].alive = 0;
    enemy = 13;
    before = dist2(u[enemy].x, u[enemy].y, b[0].x, b[0].y);
    for (i = 0; i < 100; i++)
        update_orders();
    if (dist2(u[enemy].x, u[enemy].y, b[0].x, b[0].y) >= before)
        return 15;
    init_state();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].owner == ENEMY || u[i].owner == SIDE)
            u[i].alive = 0;
    supply_used = 0;
    frame = enemy_timer;
    enemy_wave();
    last = 0;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == ENEMY)
            last++;
    if (last != 2)
        return 16;

    /* Construction travels to the site, completes, and adds Depot supply. */
    init_state();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].owner == ENEMY || u[i].owner == SIDE)
            u[i].alive = 0;
    supply_used = 0;
    b[2].alive = 0;
    invalidate_path_grid();
    supply_cap = 20;
    minerals = 1000;
    t = spawn_unit(SIDE, U_SCV, 92, 98);
    u[t].selected = 1;
    cx = 216;
    cy = 176;
    start_build(B_DEPOT);
    if (!b[2].alive || b[2].type != B_DEPOT || supply_cap != 20)
        return 17;
    for (i = 0; i < 1200; i++)
        update_orders();
    if (!building_complete(2))
        return 10000 + u[t].x * 10000 + u[t].y * 10 + b[2].build_progress;
    if (supply_cap != 36)
        return 181;
    if (!has_tech_for(B_BARRACKS))
        return 182;
    hit_target(MAX_UNITS + 2, 999, U_ZERGLING, ENEMY);
    if (supply_cap != 20)
        return 19;

    /* Save/load restores resources, queue state, cursor, and explored map. */
    init_state();
    minerals = 321;
    gas = 17;
    m[0].amount = 1234;
    cx = 245;
    cy = 191;
    SC64_GRID_SET(explored, MAP_CELLS - 1, 1);
    if (!save_game())
        return 20;
    minerals = gas = m[0].amount = 0;
    cx = cy = 10;
    SC64_GRID_SET(explored, MAP_CELLS - 1, 0);
    if (!load_game() || minerals != 321 || gas != 17 || m[0].amount != 1234 ||
        cx != 245 || cy != 191 || !SC64_GRID_GET(explored, MAP_CELLS - 1))
        return 21;

    /* Victory requires clearing the Zerg base; defeat follows CC destruction. */
    init_state();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].owner == ENEMY)
            u[i].alive = 0;
    hit_target(MAX_UNITS + 1, 1400, U_MARINE, SIDE);
    game_update(0, 0);
    if (!won || screen != SCREEN_RESULT)
        return 22;
    init_state();
    hit_target(MAX_UNITS, 2000, U_ZERGLING, ENEMY);
    game_update(0, 0);
    if (!lost || screen != SCREEN_RESULT)
        return 23;

    game_init();
    if (screen != SCREEN_TITLE)
        return 24;
    game_update(KEY_A, KEY_A);
    if (screen != SCREEN_PLAY)
        return 25;
    game_update(KEY_START, KEY_START);
    if (screen != SCREEN_PAUSE)
        return 26;
    game_update(KEY_A, KEY_A);
    if (screen != SCREEN_PLAY)
        return 27;

    /* Play a short legal end-to-end run: spend the starter budget through the
     * Barracks queue, let SCVs mine, attack-move the produced force, and clear
     * the Hatchery with the actual combat/economy/update systems. */
    init_state();
    for (i = 0; i < 5; i++)
	queue_unit(3, U_MARINE);
    for (i = 0; i < 1900; i++)
	game_update(0, 0);
    for (i = 0; i < 5; i++)
	queue_unit(3, U_MARINE);
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].owner == SIDE && u[i].type != U_SCV)
	    u[i].selected = 1;
    issue_order(ORDER_ATTACK_MOVE, b[1].x, b[1].y);
    for (i = 0; i < 7000 && !won && !lost; i++)
	game_update(0, 0);
    if (!won)
	return 1000 + (u[5].order * 1000) + u[5].x / 10 + (u[5].y / 10) * 100;
    init_state();
    return 0;
#endif
}

#ifdef SC64_RACES
#include "race_combat.h"
#ifdef SC64_T5_RUNTIME
#include "sc64_t5_availability_adapter.h"
#endif
#ifdef SC64_T6_RUNTIME
#include "sc64_t6_availability_adapter.h"
#endif
#include "race_adapter.h"
#ifdef SC64_T5_RUNTIME
#include "sc64_t5_tech_adapter.h"
#endif
#ifdef SC64_T6_RUNTIME
#include "sc64_t6_tech_adapter.h"
#endif
#ifdef SC64_T4_RUNTIME
#include "sc64_t4_actor_adapter.h"
#ifdef SC64_T4_SCENARIO_RUNTIME
#include "sc64_t4_effects_adapter.h"
#include "sc64_t4_creation_adapter.h"
#include "sc64_t4_scenario_adapter.h"
#include "sc64_t4_trap_adapter.h"
#endif
#endif
#ifdef SC64_T5_RUNTIME
#include "sc64_t5_world_adapter.h"
#include "sc64_t5_mine_adapter.h"
#include "sc64_t5_scenario_adapter.h"
#endif
#ifdef SC64_T6_RUNTIME
#include "sc64_t6_world_adapter.h"
#include "sc64_t6_repair_world.h"
#ifdef SC64_TERRAN_LATE_RUNTIME
#include "sc64_terran_late_repair.h"
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
#include "sc64_terran_late_world.h"
#endif
#endif
#include "race_campaign_adapter.h"
#include "race_save_adapter.h"
#endif
