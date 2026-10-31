#if defined(SC64_CONTACT_MEMO_ORACLE) && \
    (!defined(GBA_HOST) || \
     !defined(SC64_CONTACT_CACHE))
#error "contact memo oracle requires the host contact-memo build"
#endif
#if defined(SC64_CONTACT_CACHE)
#define SC64_CONTACT_MEMO_ENABLED 1
static void sc64_contact_memo_invalidate(void);
#define SC64_CONTACT_MEMO_INVALIDATE() sc64_contact_memo_invalidate()
#endif
#include "../common/gba.h"
#include "sc64_engine_capabilities.h"
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
#include "sc64_battlecruiser_rules.h"
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
#include "sc64_battlecruiser_yamato_motion_codec.h"
#include "sc64_bc_heading.h"
#endif
#endif
#include "sc64_bitgrid.h"
#include "sc64_map_math.h"
#include "sc64_construction_hp.h"
#include "sc64_building_damage.h"
#include "sc64_building_fire_vfx.h"
#include "sc64_order_marker_vfx.h"
#include "sc64_building_placement_vfx.h"
#ifdef SC64_IMPORTED_MISSION
#include "sc64_minimap_terrain.h"
#endif
#if defined(SC64_DIRECT_ATTACK) && !defined(SC64_RACES)
#error "SC64_DIRECT_ATTACK requires SC64_RACES"
#endif
#if defined(SC64_REACHABLE_FORMATION) && !defined(SC64_RACES)
#error "SC64_REACHABLE_FORMATION requires SC64_RACES"
#endif
#if defined(SC64_Z7_PRUNE_STARTED) && \
    (!defined(SC64_RACES) || !defined(SC64_Z7_RUNTIME) || \
     !defined(SC64_Z7_PREVALIDATED_TICK) || !defined(SC64_Z7_COUNT_SNAPSHOT))
#error "Z7 started-trigger pruning requires the race runtime, prevalidated ticks and count snapshots"
#endif
#ifdef SC64_DIRECT_ATTACK
#include "sc64_direct_attack.h"
#endif
#if defined(SC64_RENDER_KEY_ONCE) && !defined(SC64_IMPORTED_MISSION)
#error "SC64_RENDER_KEY_ONCE requires SC64_IMPORTED_MISSION"
#endif
#if defined(SC64_STATIC_DIRTY_ROWS) && \
    (!defined(GBA_SCENE_CLIP) || !defined(SC64_IMPORTED_MISSION))
#error "SC64_STATIC_DIRTY_ROWS requires GBA_SCENE_CLIP and SC64_IMPORTED_MISSION"
#endif
#if defined(SC64_TANK_SOURCE_VFX) && (!defined(SC64_RACES) || !defined(SC64_HAS_SPRITES))
#error "SC64_TANK_SOURCE_VFX requires imported race sprites"
#endif
#include "sc64_weapon_vfx.h"
#ifdef SC64_WEAPON_SOURCE_VFX
#if !defined(SC64_RACES) || !defined(SC64_HAS_SPRITES)
#error "Source weapon effects require imported race sprites"
#endif
#include "sc64_weapon_source_runtime.h"
#ifndef SC64_IMPACT_EVENTS_DISABLED
/* Ordinary moving projectiles deal damage at their source impact frame. */
#define SC64_IMPACT_EVENTS 1
#endif
#endif
#ifdef SC64_TANK_SOURCE_VFX
#include "sc64_tank_source_vfx.h"
#endif
#ifdef SC64_RACES
#include "sc64_construction_vfx.h"
#endif
#if defined(SC64_CONSTRUCTION_SOURCE_ART) && \
    (!defined(SC64_RACES) || !defined(SC64_HAS_SPRITES))
#error "Source construction art requires the imported race sprite profile"
#endif
#if defined(SC64_DEATH_SOURCE_ART) && \
    (!defined(SC64_RACES) || !defined(SC64_HAS_SPRITES) || defined(SC64_DEATH_VFX_DISABLED))
#error "Source death art requires the imported race sprite profile and death effects"
#endif
#ifndef SC64_DEATH_VFX_DISABLED
#include "sc64_death_vfx.h"
#endif
#if defined(SC64_T5_RUNTIME) || defined(SC64_P5_SHUTTLE_ENABLED) || defined(SC64_Z6_TRANSPORT_ENABLED) || defined(SC64_SHARED_BUNKER)
#define SC64_TRANSPORT_RUNTIME
#endif
#if defined(SC64_P5_SHUTTLE_ENABLED) && !defined(SC64_P5_RUNTIME)
#error "P5 Shuttle transport requires the P5 runtime"
#endif
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int value, size_t n);
int memcmp(const void *a, const void *b, size_t n);

#if defined(SC64_SHOWCASE_ENABLED) && \
    (!defined(SC64_RACES) || !defined(SC64_UNIFIED) || \
     !defined(SC64_P7_ACTORS) || !defined(SC64_ZP_CAMPAIGN_ACTORS))
#error "Showcase requires the unified P7/ZP actor profile"
#endif
#ifdef SC64_SHOWCASE_ENABLED
/* Transient filming state: never serialized into ordinary saves. */
static int sc64_showcase_scene = -1;
static unsigned sc64_showcase_ticks;
#endif
#ifdef SC64_UNIT_WORD_SPRITE_DIAGNOSTICS
static u32 sc64_unit_words_derived_raw_calls;
static u32 sc64_unit_words_explicit_raw_calls;
#endif

#ifdef SC64_Z6_RUNTIME
#include "sc64_z6_state.h"
#endif
#ifdef SC64_Z7_RUNTIME
#if !defined(SC64_Z6_RUNTIME) || !defined(SC64_Z5_RUNTIME)
#error "Z7 runtime extends the complete Zerg campaign storage chain"
#endif
#include "sc64_z7_state.h"
#endif

#if defined(SC64_P4_STORM_ENABLED) && !defined(SC64_P4_RUNTIME)
#error "SC64_P4_STORM_ENABLED requires SC64_P4_RUNTIME"
#endif
#ifdef SC64_P5_RUNTIME
#if !defined(SC64_P5_ENABLED) || !defined(SC64_P5_DATA_ENABLED) || \
    !defined(SC64_P5_CAMPAIGN_ENTRY) || !defined(SC64_P5_ACTORS) || \
    !defined(SC64_Z6_ACTORS) || !defined(SC64_P4_STORM_ENABLED) || \
    !defined(SC64_P4_RUNTIME) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_ZP_CAMPAIGN_STORAGE) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2
#error "P5 runtime requires P5 actors/data, P4 Storm, and full Protoss storage"
#endif
#include "sc64_p5.h"
#include "sc64_p5_state.h"
#endif

#ifdef SC64_P6_RUNTIME
#if !defined(SC64_P5_SHUTTLE_ENABLED) || !defined(SC64_P6_DATA_ENABLED) || \
    !defined(SC64_P6_ACTORS) || !defined(SC64_Z5_ACTORS) || !defined(SC64_Z5_DOODADS)
#error "P6 runtime requires its complete source data, actors, props and P1-P5 chain"
#endif
#include "sc64_p6.h"
#include "sc64_p6_state.h"
#endif

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
#if defined(SC64_ZP_CAMPAIGN_STORAGE) && \
    (!defined(SC64_RACES) || !defined(SC64_UNIFIED) || \
     !defined(SC64_CAMPAIGN_RACE) || \
     (SC64_CAMPAIGN_RACE != 1 && SC64_CAMPAIGN_RACE != 2) || \
     defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_T6_STORAGE))
#error "Zerg/Protoss campaign storage requires a unified Zerg or Protoss race profile"
#endif
#if defined(SC64_ZP_CAMPAIGN_STORAGE) && !defined(SC64_ROUTE_PACK3)
#error "Zerg/Protoss campaign storage requires packed route fields"
#endif
#if defined(SC64_ZP_FULL_STORAGE) && !defined(SC64_ZP_CAMPAIGN_STORAGE)
#error "Full Z/P storage requires the isolated campaign storage profile"
#endif
#ifdef SC64_Z5_RUNTIME
#if !defined(SC64_Z4_RUNTIME) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_Z5_ENABLED) || !defined(SC64_Z5_DATA_ENABLED) || \
    !defined(SC64_Z5_CAMPAIGN_ENTRY) || !defined(SC64_Z5_ACTORS) || \
    !defined(SC64_Z5_DOODADS) || !defined(SC64_ZP_CAMPAIGN_ACTORS) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 1
#error "Z5 runtime requires authenticated Z5 data, Z4 chain, and full Zerg storage"
#endif
#include "sc64_z5.h"
#include "sc64_z5_state.h"
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME) && \
    (!defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 0)
#error "Combined T3/T4 runtime is an experimental unified Terran profile"
#endif

#if defined(SC64_P7_TYPED_COMPACT_ENABLED) && \
    (!defined(SC64_P7_BATTERY_ENABLED) || \
     !defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED) || \
     !defined(SC64_P7_RUNTIME) || !defined(SC64_UNIFIED) || \
     !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_ZP_CAMPAIGN_STORAGE) || \
     !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2)
#error "P7 typed compact save requires unified P7 battery and Arbiter cache profile"
#endif

#if defined(SC64_P7_PROJECTILES_ENABLED) && \
    (!defined(SC64_P7_TYPED_COMPACT_ENABLED) || !defined(SC64_P7_ACTORS))
#error "Interceptor projectiles require the P7 typed save and shared source clock"
#endif
#if defined(SC64_P7_PROJECTILES_ENABLED) && defined(SC64_TILE_RENDERER)
#error "Interceptor arena and experimental tile sprites overlap OBJ VRAM"
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
#include "sc64_interceptor_motion.h"
_Static_assert(0x14000u+SC64_ARENA_WORDS*2u<=0x18000u,
               "Interceptor arena exceeds Mode 4 OBJ reserve");
#endif
#if defined(SC64_P7_CARRIER_TRAINING_ENABLED) && !defined(SC64_P7_PROJECTILES_ENABLED)
#error "Carrier training requires P7 projectile storage and typed actor state"
#endif
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) && !defined(SC64_P7_CARRIER_TRAINING_ENABLED)
#error "Interceptor flight requires the P7 Carrier training stage"
#endif

#if defined(SC64_P7_GEOMETRY_RECT_HOIST_ENABLED) && \
    !defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED)
#error "P7 geometry hoist requires the Arbiter cloak cache"
#endif

#if defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED) && \
    (!defined(SC64_P7_OBSERVER_ENABLED) || !defined(SC64_P7_BATTERY_ENABLED) || \
     !defined(SC64_RACES) || !defined(SC64_P7_RUNTIME) || \
     !defined(SC64_P7_ACTORS) || !defined(SC64_P7_DATA_ENABLED) || \
     !defined(SC64_ZP_CAMPAIGN_ACTORS))
#error "P7 Arbiter cloak requires Observer, Battery state, and authenticated P7 runtime"
#endif
#if defined(SC64_P7_ARBITER_CLOAK_ENABLED) && \
    !defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED)
#error "Use the unified P7 Arbiter cloak/cache feature gate"
#endif

#if defined(SC64_RACES)
#if defined(SC64_ZP_FULL_STORAGE)
#define MAX_UNITS 240
#elif defined(SC64_ZP_CAMPAIGN_STORAGE) && SC64_CAMPAIGN_RACE == 2
#define MAX_UNITS 240
#elif defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#define MAX_UNITS 192
#elif defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
#define MAX_UNITS 160
#else
#define MAX_UNITS 128
#endif
#if defined(SC64_Z7_COLONY_DEFENSE_ENABLED) && !defined(SC64_Z7_RUNTIME)
#error "Z7 colony defense requires the Z7 runtime"
#endif
#if defined(SC64_Z7_EGG_PRODUCTION) && \
    (!defined(SC64_RACES) || !defined(SC64_Z7_RUNTIME))
#error "Z7 Egg production requires the race and Z7 runtimes"
#endif
#if defined(SC64_P7_BATTERY_ENABLED) && \
    (!defined(SC64_RACES) || !defined(SC64_P7_RUNTIME) || \
     !defined(SC64_P7_ACTORS) || !defined(SC64_ZP_FULL_STORAGE) || \
     !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || \
     SC64_CAMPAIGN_RACE != 2)
#error "P7 Battery requires authenticated P7 data and full Protoss campaign storage"
#endif
#if defined(SC64_Z7_SCOURGE_COMBAT) && \
    (!defined(SC64_RACES) || !defined(SC64_Z7_RUNTIME) || \
     !defined(SC64_Z7_EGG_PRODUCTION))
#error "Z7 Scourge combat requires the Z7 Egg production runtime"
#endif
#ifdef SC64_Z7_DERIVED_SAVE
#if !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_ZP_CAMPAIGN_STORAGE) || \
    !defined(SC64_ZP_CAMPAIGN_ACTORS) || !defined(SC64_Z7_RUNTIME) || \
    !defined(SC64_Z7_EGG_PRODUCTION) || !defined(SC64_Z6_RUNTIME) || \
    !defined(SC64_Z5_RUNTIME) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 1
#error "derived save rows require the complete Zerg ZP/Z5/Z6/Z7 Egg profile"
#endif
#endif
#if defined(SC64_Z8_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 46u
#elif defined(SC64_P9_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 45u
#elif defined(SC64_P8_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 44u
#elif defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED)
#define SC64_RACE_SAVE_GENERATION 43u
#elif defined(SC64_P7_CARRIER_TRAINING_ENABLED)
#define SC64_RACE_SAVE_GENERATION 42u
#elif defined(SC64_P7_PROJECTILES_ENABLED)
#define SC64_RACE_SAVE_GENERATION 41u
#elif defined(SC64_P7_TYPED_COMPACT_ENABLED)
#define SC64_RACE_SAVE_GENERATION 40u
#elif defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED)
#define SC64_RACE_SAVE_GENERATION 39u
#elif defined(SC64_Z7_DERIVED_SAVE)
#define SC64_RACE_SAVE_GENERATION 38u
#elif defined(SC64_P7_BATTERY_ENABLED)
#define SC64_RACE_SAVE_GENERATION 37u
#elif defined(SC64_Z7_EGG_PRODUCTION)
#define SC64_RACE_SAVE_GENERATION 36u
#elif defined(SC64_Z7_COLONY_DEFENSE_ENABLED)
#define SC64_RACE_SAVE_GENERATION 35u
#elif defined(SC64_Z7_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 33u
#elif defined(SC64_Z6_TRANSPORT_ENABLED)
#define SC64_RACE_SAVE_GENERATION 32u
#elif defined(SC64_Z6_CLOAK_ENABLED)
#define SC64_RACE_SAVE_GENERATION 31u
#elif defined(SC64_Z6_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 29u
#elif defined(SC64_P6_STORM_ENABLED)
#define SC64_RACE_SAVE_GENERATION 30u
#elif defined(SC64_P6_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 28u
#elif defined(SC64_Z5_CONSUME_ENABLED)
#define SC64_RACE_SAVE_GENERATION 27u
#elif defined(SC64_P5_SHUTTLE_ENABLED)
#define SC64_RACE_SAVE_GENERATION 26u
#elif defined(SC64_Z5_CLOAK_ENABLED)
#define SC64_RACE_SAVE_GENERATION 25u
#elif defined(SC64_P5_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 24u
#elif defined(SC64_Z5_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 23u
#elif defined(SC64_ZP_FULL_STORAGE)
#define SC64_RACE_SAVE_GENERATION 22u
#elif defined(SC64_P4_STORM_ENABLED)
#define SC64_RACE_SAVE_GENERATION 21u
#elif defined(SC64_TERRAN_LATE_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 18u
#elif defined(SC64_LATE_TERRAN_STORAGE)
#define SC64_RACE_SAVE_GENERATION 17u
#elif defined(SC64_T6_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 16u
#elif defined(SC64_T6_STORAGE)
#define SC64_RACE_SAVE_GENERATION 15u
#elif defined(SC64_T5_RUNTIME)
#define SC64_RACE_SAVE_GENERATION 14u
#elif defined(SC64_ZP_CAMPAIGN_STORAGE)
#define SC64_RACE_SAVE_GENERATION 20u
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
/* Opt-in Z/P pools are staged chapter bounds, not full-campaign maxima.
 * Protoss reserves headroom for 87 scripted creations; production must keep
 * those slots available until the corresponding events have executed. */
#if defined(SC64_ZP_FULL_STORAGE)
#define MAX_BUILDINGS 160
#elif defined(SC64_ZP_CAMPAIGN_STORAGE)
#define MAX_BUILDINGS 96
#elif defined(SC64_LATE_TERRAN_STORAGE)
#define MAX_BUILDINGS 96
#elif defined(SC64_T6_STORAGE)
#define MAX_BUILDINGS 80
#else
#define MAX_BUILDINGS 64
#endif
#if defined(SC64_ZP_FULL_STORAGE)
#define MAX_MINERALS 144
#elif defined(SC64_ZP_CAMPAIGN_STORAGE)
#define MAX_MINERALS 48
#elif defined(SC64_LATE_TERRAN_STORAGE)
#define MAX_MINERALS 80
#elif defined(SC64_LARGE_MAPS)
#define MAX_MINERALS 48
#else
#define MAX_MINERALS 32
#endif
#if defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 0
#define SC64_BASE_SAVE_VERSION (0x53435430u + SC64_RACE_SAVE_GENERATION)
#elif defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 1
#define SC64_BASE_SAVE_VERSION (0x53435a30u + SC64_RACE_SAVE_GENERATION)
#elif defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 2
#define SC64_BASE_SAVE_VERSION (0x53435030u + SC64_RACE_SAVE_GENERATION)
#elif defined(SC64_UNIFIED)
#if defined(SC64_P3_RUNTIME)
#define SC64_BASE_SAVE_VERSION 0x53435536u
#elif defined(SC64_LARGE_MAPS)
#define SC64_BASE_SAVE_VERSION 0x53435535u
#elif defined(SC64_Z3_ENABLED)
#define SC64_BASE_SAVE_VERSION 0x53435534u
#elif defined(SC64_T3_ENABLED)
#define SC64_BASE_SAVE_VERSION 0x53435532u
#else
#define SC64_BASE_SAVE_VERSION 0x53435531u
#endif
#else
#define SC64_BASE_SAVE_VERSION 0x53435231u
#endif
#elif defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES) || defined(SC64_TERRAN_CAMPAIGN)
#define MAX_UNITS 96
#define MAX_MINERALS 16
#define MAX_BUILDINGS 32
#ifdef SC64_TERRAN_CAMPAIGN
#define SC64_BASE_SAVE_VERSION 0x53435431u
#elif defined(SC64_BACKWATER)
#define SC64_BASE_SAVE_VERSION 0x53434232u
#else
#define SC64_BASE_SAVE_VERSION 0x53434B31u
#endif
#elif defined(SC64_IMPORTED_MISSION)
#define MAX_UNITS 48
#define MAX_MINERALS 16
#define SC64_BASE_SAVE_VERSION 0x53433631u
#else
#define MAX_UNITS 40
#define MAX_MINERALS 8
#define SC64_BASE_SAVE_VERSION 0x53434733u
/* The original-art skirmish owns its menus; see field_ui.h. */
#define SC_DEFAULT_UI 1
#endif
/* Roster-enabled saves contain identities absent from ordinary profiles.
 * Preserve every existing tag while reserving a separate high-bit namespace.
 * Fail at compile time if a future base tag claims that namespace. */
_Static_assert((SC64_BASE_SAVE_VERSION & 0x80000000u) == 0,
               "base save tag overlaps roster save namespace");
#ifdef SC64_SKIRMISH_ROSTER
#define SC64_LEGACY_SAVE_VERSION (SC64_BASE_SAVE_VERSION | 0x80000000u)
#else
#define SC64_LEGACY_SAVE_VERSION SC64_BASE_SAVE_VERSION
#endif
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
_Static_assert((SC64_LEGACY_SAVE_VERSION & 0x20000000u) == 0,
               "BC save namespace overlaps legacy tag");
#define SAVE_VERSION (SC64_LEGACY_SAVE_VERSION | 0x20000000u)
#else
#define SAVE_VERSION SC64_LEGACY_SAVE_VERSION
#endif
#ifndef MAX_BUILDINGS
#define MAX_BUILDINGS 16
#endif
#if defined(SC_DEFAULT_UI) || defined(SC64_FIELD_CONTROLS)
/* Default controls: field_ui.h, or field_controls.h in the race engine. */
#define SC_FIELD_FEEDBACK 1
#endif
#ifdef SC64_ZP_FULL_STORAGE
#define MAX_RACE_GEYSERS 12
#else
#define MAX_RACE_GEYSERS 4
#endif
#ifdef SC64_RACES
/* Transient visuals are absent from RaceSave. Keep dense volleys readable
 * without growing actor/save rows; the ARM link still checks the RAM guard. */
#define MAX_TRACERS 32
#else
#define MAX_TRACERS 12
#endif
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
#include "sc64_p8_sprite_adapter.h"
#ifdef SC64_CONSTRUCTION_SOURCE_ART
#include "sc64_construction_source.h"
#include "sc64_construction_sprites_generated.h"
#if !defined(SC64_CONSTRUCTION_COLOR_MODIFIERS_VERSION) || SC64_CONSTRUCTION_COLOR_MODIFIERS_VERSION != 1
#error "Regenerate construction art with tools/sc64_construction_sprites.py for blue warp blending"
#endif
#endif
#ifdef SC64_DEATH_SOURCE_ART
#include "sc64_death_source.h"
#include "sc64_death_sprites_generated.h"
#if !defined(SC64_DEATH_SOURCE_BANK_VERSION) || SC64_DEATH_SOURCE_BANK_VERSION != 2 || \
    !defined(SC64_DEATH_SOURCE_METADATA_VERSION) || SC64_DEATH_SOURCE_METADATA_VERSION != 1 || \
    !defined(SC64_DEATH_SOURCE_PIXEL_BYTES) || SC64_DEATH_SOURCE_PIXEL_BYTES == 0
#error "Regenerate source death art with tools/sc64_death_sprites.py (bank v2)"
#endif
#ifdef SC64_UNIT_WORD_SPRITES
#if !defined(SC64_DEATH_SOURCE_WORD_METADATA) || SC64_DEATH_SOURCE_WORD_METADATA != 1 || \
    !defined(SC64_DEATH_SOURCE_WORD_BYTES) || SC64_DEATH_SOURCE_WORD_BYTES == 0
#error "Regenerate source death art with packed remnant frames"
#endif
#endif
#endif
#endif
#endif
#ifdef SC64_UNIT_ATTACK_POSES_ENABLED
#include "sc64_unit_attack_source.h"
#endif
#if defined(SC64_Z5_RUNTIME) && defined(SC64_HAS_SPRITES)
#if !defined(SC64_Z5_SPRITE_SET_COMPLETE) || \
    !defined(SC64_Z5_DOODAD_SPRITE_SET_COMPLETE)
#error "Z5 runtime requires the complete actor and unit-backed doodad sprite banks"
#endif
#endif
#if defined(SC64_P6_RUNTIME) && defined(SC64_HAS_SPRITES)
#if !defined(SC64_P6_SPRITE_SET_COMPLETE) || \
    !defined(SC64_Z5_DOODAD_SPRITE_SET_COMPLETE)
#error "P6 runtime requires the complete actor and unit-backed doodad sprite banks"
#endif
#endif
#if defined(SC64_Z6_RUNTIME) && defined(SC64_HAS_SPRITES)
#if !defined(SC64_Z6_SPRITE_SET_COMPLETE) || \
    !defined(SC64_Z6_BUILDING_SPRITE_SET_COMPLETE)
#error "Z6 runtime requires its complete Guardian and source building sprite banks"
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_STORAGE)
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
#if defined(SC64_LARGE_MAPS) || defined(SC64_ZP_CAMPAIGN_STORAGE)
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
#if defined(SC64_LARGE_MAPS) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    if (race_cols == 128) return (int)(cell >> 7);
#endif
    return (int)((cell * 43691u) >> 22);
}
static inline int sc64_cell_col(unsigned cell) {
    return (int)cell-sc64_cell_row(cell)*race_cols;
}
static inline int sc64_minimap_pixel(unsigned pixel,int tiles) {
    if (tiles == 64) return (int)((pixel * 3u) >> 7);
#if defined(SC64_LARGE_MAPS) || defined(SC64_ZP_CAMPAIGN_STORAGE)
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
#include "sc64_skirmish_menu.h"
#include "sc64_stock_motion.h"
#include "sc64_stock_combat.h"
#ifdef SC64_Z7_EGG_PRODUCTION
#include "sc64_z7_egg_production.h"
_Static_assert(SC_RU_COUNT <= 64,
               "Packed ZP actor type needs a six-bit enum");
#endif
#include "sc64_game_actor_facts.h"
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
#ifndef SC64_ZP_CAMPAIGN_STORAGE
_Static_assert(MAX_UNITS + MAX_BUILDINGS <= 255,
               "Visibility source counts require wider storage");
#endif
#endif
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
#include "sc64_z3.h"
#endif
#ifdef SC64_Z4_RUNTIME
#if !defined(SC64_Z4_ENABLED) || !defined(SC64_Z4_DATA_ENABLED) || !defined(SC64_ZP_CAMPAIGN_STORAGE) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "Z4 runtime requires its complete source map, actor rules and expanded storage"
#endif
#include "sc64_z4.h"
_Static_assert(SC64_RACE_MAP_COUNT > 15,
               "Z4 runtime requires the staged Z4 map descriptor");
#endif
#ifdef SC64_P4_RUNTIME
#if !defined(SC64_P4_ENABLED) || !defined(SC64_P4_DATA_ENABLED) || !defined(SC64_ZP_CAMPAIGN_STORAGE) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "P4 runtime requires its complete source map, actor rules and expanded storage"
#endif
#ifdef SC64_P4_STORM_ENABLED
#ifndef SC64_P4_RUNTIME
#error "P4 Storm requires the P4 runtime"
#endif
#endif
#include "sc64_p4.h"
_Static_assert(SC64_RACE_MAP_COUNT > SC64_P4_GLOBAL_ID,
               "P4 runtime requires the staged P4 map descriptor");
#endif
#ifdef SC64_Z5_RUNTIME
_Static_assert(SC64_RACE_MAP_COUNT > SC64_Z5_GLOBAL_ID,
               "Z5 runtime requires the staged Z5 map descriptor");
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
#include "sc64_t3.h"
_Static_assert(MAX_UNITS >= 138, "T3 must retain its complete starting roster");
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
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
#include "sc64_weapon_vfx_profiles.h"
enum {
    ORDER_IDLE, ORDER_MOVE, ORDER_ATTACK_MOVE, ORDER_ATTACK_TARGET,
    ORDER_MINE, ORDER_BUILD, ORDER_GAS
#ifdef SC64_Z5_CONSUME_ENABLED
    , ORDER_Z5_CONSUME
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    , ORDER_BOARD, ORDER_TRANSPORTED, ORDER_UNLOAD
#endif
#ifdef SC64_T6_RUNTIME
    , ORDER_REPAIR
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    , ORDER_P7_RECHARGE
#endif
#ifdef SC64_NYDUS_ENABLED
    /* Profiles without Nydus keep their existing order values. */
    , ORDER_NYDUS_ENTER
#endif
#ifdef SC64_RACES
    /* Previously unused in every race profile. Fits the existing four-bit
     * save fields without moving any established order value. */
    , ORDER_PROFILE_END
    , ORDER_BUILD_PENDING = 14
#ifdef SC64_SHARED_BATTERY
    , ORDER_RECHARGE = 15
#endif
#endif
};
#if defined(SC64_P7_BATTERY_ENABLED) && !defined(SC64_SHARED_BATTERY)
#define ORDER_RECHARGE ORDER_P7_RECHARGE
#endif
#ifdef SC64_RACES
_Static_assert(ORDER_PROFILE_END <= ORDER_BUILD_PENDING,
               "pending construction must not alias an established order");
_Static_assert(ORDER_BUILD_PENDING < 16, "four-bit persistent order field");
#define SC64_PENDING_BUILD_ORDER(o) ((o) == ORDER_BUILD_PENDING)
#else
#define SC64_PENDING_BUILD_ORDER(o) 0
#endif
#define SC64_BUILD_WORKER_ORDER(o) ((o) == ORDER_BUILD || SC64_PENDING_BUILD_ORDER(o))
#ifdef SC64_NYDUS_ENABLED
#define SC64_NYDUS_ORDER(o) ((o) == ORDER_NYDUS_ENTER)
#else
#define SC64_NYDUS_ORDER(o) 0
#endif
#ifdef SC64_TRANSPORT_RUNTIME
#define SC64_TRANSPORT_LOADED(q) ((q)->order == ORDER_TRANSPORTED)
#else
#define SC64_TRANSPORT_LOADED(q) 0
#endif
#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
/* Training (state 1) and inside (state 2) children stay off-map; launched
 * and detached Interceptors fly, but the flight adapter owns their orders. */
#define SC64_CARRIER_HANGAR_HIDDEN(q) ((q)->type==U_P7_INTERCEPTOR && \
    ((((unsigned)(q)->flags >> 8) & 7u) - 1u) < 2u)
#define SC64_CARRIER_CHILD_SCRIPTED(q) ((q)->type==U_P7_INTERCEPTOR)
/* Raw73 keeps no type-level hidden flag here, so per-row checks replace it. */
#define SC64_CARRIER_STATE_HIDDEN(q) SC64_CARRIER_HANGAR_HIDDEN(q)
#elif defined(SC64_P7_CARRIER_TRAINING_ENABLED)
/* This stage has only training/inside children; all raw73 rows stay off-map. */
#define SC64_CARRIER_HANGAR_HIDDEN(q) ((q)->type==U_P7_INTERCEPTOR)
#define SC64_CARRIER_CHILD_SCRIPTED(q) ((q)->type==U_P7_INTERCEPTOR)
#define SC64_CARRIER_STATE_HIDDEN(q) 0 /* the raw73 type flag hides them */
#else
#define SC64_CARRIER_HANGAR_HIDDEN(q) 0
#define SC64_CARRIER_CHILD_SCRIPTED(q) 0
#define SC64_CARRIER_STATE_HIDDEN(q) 0
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
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    MENU_ZP_LAIR, MENU_ZP_SPIRE,
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    MENU_Z7_CREEP,
#endif
    MENU_COUNT
};
enum {
    MODE_NORMAL, MODE_MOVE, MODE_ATTACK, MODE_BUILD
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    , MODE_YAMATO
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
    , MODE_Z5_CONSUME
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    , MODE_UNLOAD
#endif
#if defined(SC64_TERRAN_LATE_RUNTIME) || defined(SC64_BATTLECRUISER_FLIGHT_EXPERIMENT)
    , MODE_LAND
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    , MODE_P7_RECHARGE
#endif
#if defined(SC64_P4_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
    , MODE_P4_STORM
#endif
#if defined(SC64_SHARED_BATTERY) && !defined(SC64_P7_BATTERY_ENABLED)
    , MODE_RECHARGE
#endif
};
#ifdef SC64_P7_BATTERY_ENABLED
#define MODE_RECHARGE MODE_P7_RECHARGE
#endif

#ifdef SC64_P4_STORM_ENABLED
static int race_p4_active(void);
static int race_p4_storm_menu_available(void);
static int race_p4_storm_cast_selected(int x, int y);
static void race_p4_storm_initialize_source_units(void);
static void race_p4_storm_init_actor(int i);
static void race_p4_storm_draw(void);
#endif
#ifdef SC64_P6_STORM_ENABLED
static int race_p6_storm_menu_available(void);
static int race_p6_storm_cast_selected(int x, int y);
static void race_p6_storm_initialize_source_units(void);
static void race_p6_storm_draw(void);
#endif

typedef struct {
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
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
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
static int sc64_battlecruiser_source_tick_due(void);
#endif
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
_Static_assert(sizeof(Unit) == 56, "Late Terran runtime unit budget");
#endif
#ifdef SC64_ZP_FULL_STORAGE
#include "sc64_zp_building_types.h"
typedef Sc64ZpCompactBuilding Building;
#else
typedef struct {
    int x, y, hp, maxhp, owner, type, alive;
    int queue[5], queue_head, queue_count, queue_progress;
    int build_progress, under_construction, level;
#ifdef SC64_RACES
    int shield,raw_type,raw_owner,flags,larva,larva_timer,shield_remainder;
#endif
} Building;
#endif
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
#ifdef SC64_RACES
    /* RaceSave omits tracers; race maps are at most 128 cells per side. */
    s16 x0, y0, x1, y1;
    int life, color;
#else
    int x0, y0, x1, y1, life, color;
#endif
} Tracer;
#ifdef SC64_RACES
#include "sc64_race_save_types.h"
#endif
#ifdef SC64_IMPACT_EVENTS
#include "sc64_impact_save_types.h"
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
    int geyser_x[MAX_RACE_GEYSERS], geyser_y[MAX_RACE_GEYSERS], geyser_count;
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
#ifdef SC64_UNIT_FACING
static void sc64_facing_reset_all(void);
static void sc64_facing_reset_slot(int slot);
#endif
#ifdef SC64_P6_COMBAT_ENABLED
/* The collision headers below consult per-instance burrow state. */
static int race_p6_combat_unit_burrowed(int slot);
#endif
static Building b[MAX_BUILDINGS];
#if defined(SC64_BACKWATER) || defined(SC64_SKIRMISH) || defined(SC64_RACES)
#include "sc64_collision.h"
#endif
/* Temporary ascending-index owner lists used only during update_orders(). */
static int target_unit_head[5], target_building_head[5];
static int target_unit_next[MAX_UNITS], target_building_next[MAX_BUILDINGS];
static int target_lists_valid;
#if defined(SC64_RACES) && (defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY))
static u8 target_hostile_mask[5];
static int target_hostile_masks_valid;
#endif
#ifdef SC64_RACES
#include "sc64_targets.h"
#endif
static Mineral m[MAX_MINERALS];
static Geyser geysers[MAX_RACE_GEYSERS] = {{
	224, 160
}, {
    272, 192
}
};
static Tracer tracers[MAX_TRACERS];
#ifdef SC64_TRACE_DURATION_CACHE
#ifdef SC64_TRACE_PROFILE_INDEX_CACHE
#include "sc64_trace_cache_word.h"
#endif
/* A trace's packed identity and captured endpoints stay fixed until its slot
 * is replaced. Cache only their derived presentation duration, never age or
 * simulation state. Long durations deliberately remain uncached. */
static u16 sc64_trace_durations[MAX_TRACERS];
static inline void sc64_trace_duration_cache_reset(void)
{
    memset(sc64_trace_durations, 0, sizeof(sc64_trace_durations));
}
static inline void sc64_trace_duration_cache_store(int slot, int duration)
{
#ifdef SC64_TRACE_PROFILE_INDEX_CACHE
    sc64_trace_durations[slot] = (u16)sc64_trace_short_duration_word(duration);
#else
    sc64_trace_durations[slot] = duration > 0 && duration <= 65535 ?
        (u16)duration : 0;
#endif
}
#else
#define sc64_trace_duration_cache_reset() ((void)0)
#define sc64_trace_duration_cache_store(slot, duration) ((void)0)
#endif
#ifdef SC64_IMPACT_EVENTS
static void race_impact_reset(void);
#endif
/* Every world start and load clears in-flight effects; a load restores its
 * saved pending impacts afterwards. */
static inline void clear_tracers(void)
{
    memset(tracers, 0, sizeof(tracers));
    sc64_trace_duration_cache_reset();
#ifdef SC64_IMPACT_EVENTS
    race_impact_reset();
#endif
}
#ifndef SC64_DEATH_VFX_DISABLED
/* Presentation-only death effects. The pool and the alive shadow are never
 * read by the simulation and are absent from every save format. */
#ifdef SC64_RACES
#define SC64_DEATH_VFX_POOL 16
#else
#define SC64_DEATH_VFX_POOL 12
#endif
/* Result outro: the simulation stops on the outcome update, but final deaths
 * keep animating on a presentation clock before the result panel appears. */
#define SC64_RESULT_OUTRO_MIN 24u
#define SC64_RESULT_OUTRO_MAX 96u
#define SC64_RESULT_PANEL_OPEN 6u
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
#ifdef SC64_TILE_RENDERER
#error "death-VFX VRAM storage conflicts with the tile renderer"
#endif
enum {
    SC64_DEATH_VFX_VRAM_ROW_WORDS = 10,
    SC64_DEATH_VFX_VRAM_POOL_WORDS = SC64_DEATH_VFX_POOL * SC64_DEATH_VFX_VRAM_ROW_WORDS,
    SC64_DEATH_VFX_VRAM_SEEN_WORDS =
        ((((MAX_UNITS + 7) / 8) + ((MAX_BUILDINGS + 7) / 8) + 1) / 2),
    SC64_DEATH_VFX_VRAM_PRIMED_WORD = SC64_DEATH_VFX_VRAM_POOL_WORDS + SC64_DEATH_VFX_VRAM_SEEN_WORDS,
    SC64_DEATH_VFX_VRAM_OUTRO_WORD = SC64_DEATH_VFX_VRAM_PRIMED_WORD + 1,
    SC64_DEATH_VFX_VRAM_OUTRO_END_WORD = SC64_DEATH_VFX_VRAM_OUTRO_WORD + 1,
    SC64_DEATH_VFX_VRAM_TOTAL_WORDS = SC64_DEATH_VFX_VRAM_OUTRO_END_WORD + 1
};
/* The gap after the sampled-audio arena is not free: imported game audio
 * keeps a live actor snapshot there. Use only the 2.5 KiB padding after the
 * second Mode-4 framebuffer page, before the projectile OBJ arena. */
#define SC64_DEATH_VFX_VRAM_BASE 0x06013600u
#define SC64_DEATH_VFX_VRAM_LIMIT 0x06014000u
_Static_assert(SC64_DEATH_VFX_VRAM_BASE >=
               GBA_AUDIO_ARENA + GBA_AUDIO_MIX_BYTES + (MAX_UNITS + MAX_BUILDINGS) * 2u,
               "death-VFX storage overlaps the imported game-audio snapshot");
_Static_assert(SC64_DEATH_VFX_VRAM_BASE + SC64_DEATH_VFX_VRAM_TOTAL_WORDS * 2u <= SC64_DEATH_VFX_VRAM_LIMIT,
               "death-VFX state exceeds Mode-4 page padding or overlaps projectile VRAM");
_Static_assert(((MAX_UNITS + 7) / 8 + (MAX_BUILDINGS + 7) / 8) <=
               SC64_DEATH_VFX_VRAM_SEEN_WORDS * 2,
               "death-VFX alive shadow exceeds packed VRAM words");
#ifdef GBA_HOST
static u16 sc64_death_vfx_vram_host[SC64_DEATH_VFX_VRAM_TOTAL_WORDS + 2];
#define SC64_DEATH_VFX_VRAM_WORDS ((volatile u16 *)(sc64_death_vfx_vram_host + 1))
static void death_vfx_test_storage_guards_init(void)
{
    sc64_death_vfx_vram_host[0] = 0xa55au;
    sc64_death_vfx_vram_host[SC64_DEATH_VFX_VRAM_TOTAL_WORDS + 1] = 0x5aa5u;
}
static int death_vfx_test_storage_guards_ok(void)
{
    return sc64_death_vfx_vram_host[0] == 0xa55au &&
           sc64_death_vfx_vram_host[SC64_DEATH_VFX_VRAM_TOTAL_WORDS + 1] == 0x5aa5u;
}
#else
#define SC64_DEATH_VFX_VRAM_WORDS ((volatile u16 *)SC64_DEATH_VFX_VRAM_BASE)
#endif
static void
death_vfx_load_row(int slot, Sc64DeathVfx *e)
{
    volatile u16 *p = SC64_DEATH_VFX_VRAM_WORDS + slot * SC64_DEATH_VFX_VRAM_ROW_WORDS;
    e->x = (int16_t)p[0]; e->y = (int16_t)p[1];
    e->birth = (uint32_t)p[2] | ((uint32_t)p[3] << 16);
    e->raw = p[4]; e->burst_su = p[5]; e->remnant_start = p[6]; e->total_su = p[7];
    e->style = (uint8_t)p[8]; e->extent = (uint8_t)(p[8] >> 8);
    e->source = (uint8_t)p[9]; e->color = (uint8_t)(p[9] >> 8);
}
static void
death_vfx_store_row(int slot, const Sc64DeathVfx *e)
{
    volatile u16 *p = SC64_DEATH_VFX_VRAM_WORDS + slot * SC64_DEATH_VFX_VRAM_ROW_WORDS;
    p[0] = (u16)e->x; p[1] = (u16)e->y;
    p[2] = (u16)e->birth; p[3] = (u16)(e->birth >> 16);
    p[4] = e->raw; p[5] = e->burst_su; p[6] = e->remnant_start; p[7] = e->total_su;
    p[8] = (u16)e->style | ((u16)e->extent << 8);
    p[9] = (u16)e->source | ((u16)e->color << 8);
}
static u16
death_vfx_seen_word(unsigned word)
{
    return SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_POOL_WORDS + word];
}
static void
death_vfx_seen_set_word(unsigned word, u16 value)
{
    SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_POOL_WORDS + word] = value;
}
static int
death_vfx_seen_chunk(unsigned byte)
{
    u16 word = death_vfx_seen_word(byte >> 1);
    return (byte & 1u) ? word >> 8 : word & 0xffu;
}
static void
death_vfx_seen_set_chunk(unsigned byte, unsigned value)
{
    unsigned word_index = byte >> 1;
    u16 word = death_vfx_seen_word(word_index);
    if (byte & 1u) word = (u16)((word & 0x00ffu) | ((value & 0xffu) << 8));
    else word = (u16)((word & 0xff00u) | (value & 0xffu));
    death_vfx_seen_set_word(word_index, word);
}
static void death_vfx_seen_clear(void)
{
    for (unsigned i = 0; i < SC64_DEATH_VFX_VRAM_SEEN_WORDS; ++i)
        death_vfx_seen_set_word(i, 0);
}
static int death_vfx_primed_get(void) { return SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_PRIMED_WORD]; }
static void death_vfx_primed_set(int v) { SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_PRIMED_WORD] = (u16)(v != 0); }
static u16 result_outro_get(void) { return SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_OUTRO_WORD]; }
static void result_outro_set(u16 v) { SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_OUTRO_WORD] = v; }
static u16 result_outro_end_get(void) { return SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_OUTRO_END_WORD]; }
static void result_outro_end_set(u16 v) { SC64_DEATH_VFX_VRAM_WORDS[SC64_DEATH_VFX_VRAM_OUTRO_END_WORD] = v; }
static void death_vfx_pool_clear(void)
{
    for (int i = 0; i < SC64_DEATH_VFX_POOL * SC64_DEATH_VFX_VRAM_ROW_WORDS; ++i)
        SC64_DEATH_VFX_VRAM_WORDS[i] = 0;
}
static void death_vfx_test_load_pool(Sc64DeathVfx *out)
{
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i) death_vfx_load_row(i, &out[i]);
}
static void death_vfx_test_store_pool(const Sc64DeathVfx *in)
{
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i) death_vfx_store_row(i, &in[i]);
}
static void death_vfx_test_load_row(int slot, Sc64DeathVfx *out) { death_vfx_load_row(slot, out); }
static void death_vfx_test_store_row(int slot, const Sc64DeathVfx *in) { death_vfx_store_row(slot, in); }
static int death_vfx_seen_get_byte(unsigned byte) { return death_vfx_seen_chunk(byte); }
static void death_vfx_seen_set_byte(unsigned byte, unsigned value) { death_vfx_seen_set_chunk(byte, value); }
#else
static Sc64DeathVfx death_vfx[SC64_DEATH_VFX_POOL];
static u8 death_vfx_seen[(MAX_UNITS + 7) / 8 + (MAX_BUILDINGS + 7) / 8];
static u8 death_vfx_primed;
static u16 result_outro, result_outro_end;
#define death_vfx_primed_get() (death_vfx_primed)
#define death_vfx_primed_set(v) (death_vfx_primed = (u8)((v) != 0))
#define result_outro_get() (result_outro)
#define result_outro_set(v) (result_outro = (u16)(v))
#define result_outro_end_get() (result_outro_end)
#define result_outro_end_set(v) (result_outro_end = (u16)(v))
#define death_vfx_pool_clear() memset(death_vfx, 0, sizeof(death_vfx))
static void death_vfx_test_load_pool(Sc64DeathVfx *out) { memcpy(out, death_vfx, sizeof(death_vfx)); }
static void death_vfx_test_store_pool(const Sc64DeathVfx *in) { memcpy(death_vfx, in, sizeof(death_vfx)); }
static void death_vfx_test_load_row(int slot, Sc64DeathVfx *out) { *out = death_vfx[slot]; }
static void death_vfx_test_store_row(int slot, const Sc64DeathVfx *in) { death_vfx[slot] = *in; }
static int death_vfx_seen_get_byte(unsigned byte) { return death_vfx_seen[byte]; }
static void death_vfx_seen_set_byte(unsigned byte, unsigned value) { death_vfx_seen[byte] = (u8)value; }
#endif
/* New worlds and loads re-prime the shadow, so stale dead rows never play.
 * A world loaded on its result screen shows the panel at once. */
static void
death_vfx_reset(void)
{
    death_vfx_pool_clear();
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
    death_vfx_seen_clear();
#endif
    death_vfx_primed_set(0);
    result_outro_end_set(SC64_RESULT_OUTRO_MAX);
    result_outro_set(SC64_RESULT_OUTRO_MAX + SC64_RESULT_PANEL_OPEN);
}
#else
#define death_vfx_reset() ((void)0)
#endif
static u8 explored[SC64_GRID_STORAGE_BYTES];
static u8 visible[SC64_GRID_STORAGE_BYTES];
static u8 path_walkable[SC64_GRID_STORAGE_BYTES];
#ifdef SC64_ZP_CAMPAIGN_STORAGE
#ifdef SC64_Z8_RUNTIME
#include "sc64_z8_triggers.h"
#endif
typedef union {
    u16 bfs_queue[MAP_CELLS];
    RaceSave save;
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    struct { RaceSave save; Unit units[MAX_UNITS]; } carrier_candidate;
#endif
#ifdef SC64_Z8_RUNTIME
    /* A Z8 save candidate expands into this image before it is accepted. */
    struct { RaceSave save; Sc64Z8SchedulerState scheduler; } z8_candidate;
#endif
#ifdef SC64_IMPACT_EVENTS
    RaceImpactRecord impact_record;
#endif
} Sc64ZpRouteSaveScratch;
#ifdef SC64_Z8_RUNTIME
_Static_assert(sizeof(RaceSave)+sizeof(Sc64Z8SchedulerState)<=sizeof(u16)*MAP_CELLS,
               "Z8 candidate expansion must fit existing route scratch");
#endif
#ifdef SC64_IMPACT_EVENTS
_Static_assert(sizeof(RaceImpactRecord) <= sizeof(u16) * MAP_CELLS,
               "impact save record must fit existing route scratch");
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
_Static_assert(sizeof(RaceSave)+sizeof(Unit)*MAX_UNITS<=sizeof(u16)*MAP_CELLS,
               "Carrier candidate decode must fit existing route scratch");
#endif
static Sc64ZpRouteSaveScratch sc64_zp_route_save_scratch;
#endif
#ifdef SC64_IMPORTED_MISSION
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
typedef union {
    u16 bfs_queue[MAP_CELLS];
    RaceSave save;
#ifdef SC64_IMPACT_EVENTS
    RaceImpactRecord impact_record;
#endif
} Sc64CombinedRouteSaveScratch;
#ifdef SC64_IMPACT_EVENTS
_Static_assert(sizeof(RaceImpactRecord) <= sizeof(u16) * MAP_CELLS,
               "impact save record must fit existing route scratch");
#endif
static Sc64CombinedRouteSaveScratch sc64_combined_route_save_scratch;
#define bfs_queue sc64_combined_route_save_scratch.bfs_queue
#elif defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_IMPORTED_MISSION
#define bfs_queue sc64_zp_route_save_scratch.bfs_queue
#else
static u16 bfs_queue[MAP_CELLS];
#endif
#else
static u16 bfs_queue[MAP_CELLS];
#endif
static void sc64_routes_reset(void);
#else
static int bfs_prev[MAP_CELLS], bfs_queue[MAP_CELLS];
static u16 bfs_seen[MAP_CELLS], bfs_generation;
#endif
static int path_grid_valid;
#if defined(SC64_DIRECT_FIRING_POINTS)
#if !defined(SC64_RACES) || !defined(SC64_IMPORTED_MISSION)
#error "SC64_DIRECT_FIRING_POINTS requires the imported race runtime"
#endif
/* One shared, transient direct firing point. The six u16 fields hold the
 * exact target/range key and waypoint; valid is cleared with map topology. */
static struct {
    u16 tx, ty, minimum, maximum, x, y;
    u8 valid;
} sc64_direct_firing_cache;
static void
sc64_direct_firing_cache_invalidate(void)
{
    sc64_direct_firing_cache.valid = 0;
}
#endif
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
#include "sc64_font.h"
#include "sc64_icons.h"
#include "sc64_chrome.h"
#include "sc64_briefing_generated.h"
#if SC64_BRIEFING_LAYOUT_VERSION != 2
#error "Briefings need source-font pixel wrapping; run make sc64-interface"
#endif
static int race_mode,race_human=R_ZERG,race_selected_mission,race_completed,race_bots=2;
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
static u16 sc64_battlecruiser_tech_bits;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
static Sc64BcYamatoShot sc64_bc_shots[SC64_BC_SHOT_LIMIT];
/* Number of frame-counter increments during scripted world pauses, mod 5. */
static u8 sc64_bc_pause_offset;
/* Private spell-only LCG: deterministic across saves, but not the complete
 * OpenBW shared RNG stream used by unrelated events. */
static u32 sc64_bc_spell_rng=42u;
static unsigned sc64_bc_spell_random(void)
{
    sc64_bc_spell_rng=sc64_bc_spell_rng*22695477u+1u;
    return (sc64_bc_spell_rng>>16)&0x7fffu;
}
static void race_bc_yamato_target_removed(unsigned target)
{
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i) {
        Sc64BcYamatoShot *shot=&sc64_bc_shots[i];
        if (shot->state==SC64_BC_SHOT_CHARGING && shot->target==target &&
            shot->caster<MAX_UNITS && u[shot->caster].alive &&
            u[shot->caster].order==ORDER_ATTACK_TARGET && u[shot->caster].target==(int)target)
            u[shot->caster].target=-1;
    }
    sc64_bc_shot_target_gone(sc64_bc_shots,target);
}
/* Some scenario scripts remove actors without the ordinary damage callback.
 * Reconcile those removals before simulation and before a save snapshot. */
static void race_bc_yamato_sync_references(void)
{
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i) {
        Sc64BcYamatoShot *shot=&sc64_bc_shots[i];
        if (!shot->state) continue;
        if (shot->caster<MAX_UNITS && !u[shot->caster].alive)
            sc64_bc_shot_caster_gone(sc64_bc_shots,shot->caster);
        if (!shot->state || shot->target==SC64_BC_SHOT_NO_TARGET) continue;
        if (shot->target<MAX_UNITS) {
            if (!u[shot->target].alive) race_bc_yamato_target_removed(shot->target);
        } else if (shot->target<MAX_UNITS+MAX_BUILDINGS &&
                   !b[shot->target-MAX_UNITS].alive)
            race_bc_yamato_target_removed(shot->target);
    }
}
#endif
static int sc64_battlecruiser_owner_key(int custom, int mission, int owner, int raw_owner);
static int race_bc_before_unit_transfer(Unit *q,int new_owner,int new_raw_owner);
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
static void race_bc_building_destroyed(int slot);
static void race_bc_parent_detach(int slot);
static int race_bc_parent_addon_constructing(int slot);
static void race_bc_parent_relink(int slot);
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
static int race_bc_facility_airborne(int slot);
static int race_bc_facility_landing_site(int slot,int x,int y);
static int race_bc_facility_lift(int slot);
static int race_bc_facility_land(int slot,int x,int y);
static void race_bc_facility_flight_tick(void);
#endif
static int race_bc_lab_start(int slot, unsigned kind);
static int race_bc_lab_cancel(int slot, int destroyed);
static int race_bc_before_building_transfer(int slot,int new_owner);
#define SC64_BC_ALLOW_BUILDING_TRANSFER(slot,owner) race_bc_before_building_transfer((slot),(owner))
#else
#define SC64_BC_ALLOW_BUILDING_TRANSFER(slot,owner) 1
#endif
#endif
#ifndef SC64_BC_ALLOW_BUILDING_TRANSFER
#define SC64_BC_ALLOW_BUILDING_TRANSFER(slot,owner) 1
#endif
static int race_skirmish_map;
static int race_bot_races[3]={R_ZERG,R_PROTOSS,R_TER};
static Sc64Frontend race_front;
static unsigned race_front_transition;
static int race_front_load_error;
static const Sc64BriefingPage *race_front_briefing_page(void)
{
    const Sc64Briefing *briefing=sc64_briefing_get(race_front.mission);
    return briefing && race_front.briefing_page < briefing->page_count ?
        &briefing->pages[race_front.briefing_page] : NULL;
}

/* Generated text stays in ROM. Only a page cursor and reveal clock live in
 * RAM, and playback is configured in update rather than as a render effect. */
static void race_front_briefing_configure(void)
{
    if(race_front.page!=SC_FRONT_BRIEFING)return;
    const Sc64Briefing *briefing=sc64_briefing_get(race_front.mission);
    const Sc64BriefingPage *page=race_front_briefing_page();
    unsigned length=0;
    if(page)while(length<SC64_BRIEFING_MAX_TEXT_BYTES && page->text[length])length++;
    sc64_frontend_briefing_configure(&race_front,
        briefing?briefing->page_count:1,length);
}
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
#ifdef SC64_Z5_RUNTIME
static Sc64Z5PersistentState race_z5;
#endif
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
/* Set by race_queue_unit() when it refuses for funds or supply: the toast id
 * of the shortfall (1 minerals, 12 gas, 2 supply). Callers clear it first. */
static u8 race_refusal;
#ifdef SC64_ZP_CAMPAIGN_STORAGE
static int race_start_lair_morph(int building);
#endif
#ifdef SC64_Z4_RUNTIME
static unsigned race_z4_hud_seconds(void);
#endif
#ifdef SC64_P4_RUNTIME
static unsigned race_p4_reserved_units(void);
static int race_p4_world_paused(void);
#endif
#ifdef SC64_P5_RUNTIME
static int race_p5_active(void);
static int race_p5_source_owner(unsigned source_slot);
#ifdef SC64_P5_SHUTTLE_ENABLED
static int race_p5_product_source_owner(int owner);
#endif
static int race_p5_owners_hostile(int owner, int other);
static unsigned race_p5_reserved_units(void);
static void race_p5_note_unit_death(const Unit *q);
static void race_p5_note_building_death(const Building *q);
#endif
#ifdef SC64_P6_RUNTIME
static int race_p6_active(void);
static int race_p6_source_owner(unsigned source_slot);
static int race_p6_owners_hostile(int owner, int other);
static int race_p6_building_collidable(int slot);
static unsigned race_p6_prop_cache_field(unsigned row, int enabled);
#ifdef SC64_HAS_SPRITES
static int race_p6_draw_prop(Building *q, int x, int y);
#endif
static void race_p6_note_unit_death(const Unit *q);
static void race_p6_note_building_death(const Building *q);
#endif
#ifdef SC64_Z6_RUNTIME
static Sc64Z6PersistentState race_z6;
static int race_z6_active(void);
static int race_z6_source_owner(unsigned source_slot);
static int race_z6_product_source_owner(int owner);
static void race_z6_note_unit_death(const Unit *q);
static void race_z6_note_building_death(const Building *q);
static int race_z6_hallucination_incoming_damage(int target, int damage);
static int race_z6_hallucination_can_deal_damage(int attacker);
static int race_z6_hallucination_cleanup_dead(Unit *q);
#ifdef SC64_Z6_TRANSPORT_ENABLED
static int race_z6_transport_research_selected(void);
static int race_z6_transport_start_research_selected(void);
#endif
#endif
#ifdef SC64_Z7_RUNTIME
static Sc64Z7PersistentState race_z7;
static int race_z7_active(void);
static int race_z7_source_owner(unsigned source_slot);
static int race_z7_product_source_owner(int owner);
static void race_z7_note_building_death(const Building *q);
#ifdef SC64_Z7_EGG_PRODUCTION
static int __attribute__((noinline)) race_z7_egg_tick_unit(int slot);
static int race_z7_egg_cancel_available(void);
static int race_z7_egg_cancel_selected(void);
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
static int race_z7_colony_start_sunken(int building);
static int race_z7_colony_start_spore(int building);
static int race_z7_colony_morph_available_for(int building,int target_type);
static int race_z7_spore_morph_command_available(void);
static int race_z7_colony_pre_building_tick(void);
static int race_z7_colony_post_building_tick(void);
#endif
#endif
#ifdef SC64_P6_COMBAT_ENABLED
static int race_p6_combat_unit_burrowed(int slot);
static int race_p6_combat_target_detected(int target, int viewer_owner);
static int race_p6_combat_infested_attack(Unit *q, int target);
static int race_p6_combat_burrow_release_tick(void);
static int race_p6_combat_tick_traps(void);
#endif
#ifdef SC64_P7_ARCHON_SPLASH
static int race_p7_archon_splash_attack(Unit *q, int target);
#endif
#ifdef SC64_Z8_RUNTIME
#define SC64_Z8_GLOBAL_ID 19
static int race_z8_active(void);
static int race_z8_map_shape_valid(const Sc64RaceMap *map);
static int race_z8_source_owner(unsigned slot);
static int race_z8_runtime_init(const Sc64RaceMap *map);
static int race_z8_runtime_tick(void);
static int race_z8_outcome(void);
static void race_z8_note_unit_spawn(int slot, int owner, int type);
static void race_z8_note_building_spawn(int slot, int owner, int type);
static void race_z8_note_product_building(int building);
static int race_z8_unit_hidden_from_side(int unit);
#endif
#ifdef SC64_P8_RUNTIME
#define SC64_P8_GLOBAL_ID 27
static int race_p8_active(void);
static int race_p8_source_owner(unsigned slot);
static int race_p8_unit_product_available(int owner,int raw_owner,int type);
static int race_p8_building_product_available(int owner,int raw_owner,int type);
static int race_p8_world_paused(void);
static int race_p8_map_shape_valid(const Sc64RaceMap *map);
static int race_p8_runtime_init(const Sc64RaceMap *map);
static int race_p8_runtime_tick(u32 elapsed_updates);
static int race_p8_outcome(void);
static void race_p8_note_unit_spawn(int slot, int owner, int type);
static void race_p8_note_building_spawn(int slot, int owner, int type);
static void race_p8_note_product_building(int building);
/* P8 binds the shared Protoss unit services (sc64_protoss_services.h). */
#define SC64_PROTOSS_LATER_BINDINGS
#endif
#ifdef SC64_P9_RUNTIME
#define SC64_P9_GLOBAL_ID 28
static int race_p9_active(void);
static int race_p9_map_shape_valid(const Sc64RaceMap *map);
static int race_p9_runtime_init(const Sc64RaceMap *map);
static int race_p9_runtime_tick(u32 elapsed_updates);
static int race_p9_outcome(void);
static void race_p9_note_unit_spawn(int slot, int owner, int type);
static void race_p9_note_building_spawn(int slot, int owner, int type);
static void race_p9_note_product_building(int building);
#endif
enum { SC64_HIT_PRECOMPUTED_HP=1, SC64_HIT_ALLOW_SPLASH_BURROW=2,
       SC64_HIT_SKIP_HALLUCINATION_SCALE=4 };
#ifdef SC64_SIEGE_SPLASH
static void hit_target_impl_ex(int,int,int,int,int,int);
static void race_siege_splash_attack(Unit *,int,int);
#endif
#if defined(SC64_P7_OBSERVER_ENABLED) || defined(SC64_P7_BATTERY_ENABLED)
static int race_p7_active(void);
#include "sc64_protoss_services.h"
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
static int race_p7_carrier_training_stock(int carrier, unsigned count);
#endif
#endif
#ifdef SC64_NYDUS_ENABLED
static void race_nydus_note_building_lost(int slot);
static void race_nydus_tick_world(void);
static int race_nydus_tick_unit(int slot);
static int race_nydus_issue_selected(int x, int y);
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
static int race_battery_active(void);
static int race_battery_recharge_order(void);
static int race_battery_order_is_recharge(int order);
static int race_battery_site_powered(int owner,int x,int y);
static int race_battery_powered_live(int battery);
static int race_battery_command_available(void);
static int race_battery_build_available(void);
static int race_battery_issue_command(int building, int x, int y);
static int __attribute__((noinline)) race_battery_tick_unit(int slot);
static void race_battery_clear_building_references(int slot);
#endif
#ifdef SC64_P7_BATTERY_ENABLED
static int race_p7_battery_initialize_source(const Sc64RaceMap *map);
static int race_p7_battery_live_world_valid(void);
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
static int race_p7_source_owner(unsigned source_slot);
static void race_p7_projectile_reset(void);
static void race_p7_projectile_clear_references(int ref);
static void race_p7_projectile_clear_dead_references(void);
static void race_p7_projectile_source_tick(void);
static void race_p7_projectile_tick_if_due(void);
static void race_p7_projectile_draw(void);
static int race_p7_projectile_encode(u8 *packet);
static int race_p7_projectile_restore(const u8 *packet);
#endif
#ifdef SC64_IMPACT_EVENTS
static void race_impact_clear_target(int ref);
static unsigned race_impact_unit_steps(const Unit *q, int target, int air);
static unsigned race_impact_building_steps(const Building *q, int target, int air);
static int race_impact_full(void);
static void race_impact_launch(int target, int damage, int hits, int owner,
                               int damage_class, int source_type, unsigned steps);
static void race_impact_tick(void);
static int race_impact_save_write(void);
static int race_impact_save_read(Sc64ImpactEvent *candidate, int *count);
static void race_impact_restore(const Sc64ImpactEvent *candidate, int count);
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
static int race_p7_carrier_training_menu_available(void);
static int race_p7_carrier_training_queue_selected(void);
static void race_p7_carrier_training_source_tick(void);
static void race_p7_carrier_training_frame_tick(void);
static void race_p7_carrier_training_clear_slot(int slot);
static void race_p7_carrier_training_end_frame(void);
static int race_p7_carrier_training_slots_available(unsigned wanted);
static unsigned race_p7_carrier_training_child_count(int carrier);
#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
static int race_p7_interceptor_flight_carrier_holds(int carrier);
static int race_p7_interceptor_flight_carrier_order(int carrier);
#endif
#endif
#ifdef SC64_P7_OBSERVER_ENABLED
static int sc64_p7_observer_can_attack_from(int attacker, int target,
                                            int live_owner, int source_owner);
static int sc64_p7_observer_detector_sees(int target, int source_owner,
                                          int live_owner);
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
static int sc64_p7_observer_target_revealed_to(int target,int source_owner,
                                                int live_owner);
static int sc64_p7_observer_target_is_cloaked(int target);
static int sc64_p7_arbiter_cache_due;
static int sc64_p7_arbiter_cache_initialized;
static void sc64_p7_arbiter_cache_frame_tick(void);
static void sc64_p7_arbiter_cache_process_due(void);
#endif
#endif
#ifdef SC64_Z5_RUNTIME
static int race_z5_active(void);
static int race_z5_world_paused(void);
static int race_z5_is_door_type(int type);
static int race_z5_building_collidable(int slot);
static int race_z5_draw_prop(Building *q, int x, int y);
static unsigned race_z5_prop_cache_field(unsigned row, int enabled);
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
static int race_z5_cloak_actor(const Unit *q);
static int race_z5_cloak_initialize_source_units(void);
static int race_z5_cloak_menu_available(void);
static int race_z5_cloak_toggle_selected(void);
static int race_z5_cloak_tick(void);
static int race_z5_cloak_world_state_valid(void);
static int race_z5_cloaked_target_attackable(int target);
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
static int race_z6_cloak_actor(const Unit *q);
static int race_z6_cloak_menu_available(void);
static int race_z6_cloak_toggle_selected(void);
static int race_z6_cloaked_target_attackable(int target);
#endif
#ifdef SC64_Z6_STORM_ENABLED
static int race_z6_storm_menu_available(void);
static int race_z6_storm_cast_selected(int x,int y);
static void race_z6_storm_draw(void);
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
static int race_z5_consume_issue_selected(int x, int y);
static int race_z5_consume_tick_unit(int slot);
#endif
static int race_start_build(int owner,int worker,int type,int x,int y);
static int race_skirmish_choose_record(const Sc64SkirmishRecord *record);
static int race_skirmish_record_available(const Sc64SkirmishRecord *record);
static int race_skirmish_worker(int type);
static int race_skirmish_product_available(const Sc64SkirmishRecord *record, void *context);
static int race_can_site(int owner,int type,int x,int y);
static int race_site_powered(int owner,int x,int y);
static int race_builder_arrived(int worker);
static void race_pending_build_tick(int worker);
#ifdef SC64_CONSTRUCTION_HP
static void race_construction_hp_progress(int building, int progress, int total);
static void race_complete_building(int building);
static void race_construction_worker_tick(int worker);
#endif
static int race_building_can_burn(const Building *building);
static void race_tick_buildings(void),race_tick_state(void),race_tick_ai(void),race_tick_defenses(void);
static void race_skirmish_init(void),race_skirmish_outcome(void);
static void race_skirmish_geometry(void);
#if defined(SC64_RACES) && defined(SC64_HAS_SPRITES)
static const Sc64SpriteFrame *mission_geyser_sprite(void);
static const Sc64SpriteFrame *mission_building_sprite(int type);
#endif
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
#ifdef SC_DEFAULT_UI
/* Default-profile menu and feedback state (field_ui.h); never saved. */
static u8 menu_last, title_cursor, title_notice, pause_cursor;
static u8 pause_notice, pause_notice_ticks, r_tap_state;
static u8 ping_life, ping_color;
static s16 ping_x, ping_y;
#elif defined(SC64_FIELD_CONTROLS)
/* The same controls and feedback state for field_controls.h; never saved.
 * Bytes: the Showcase build has little working RAM left. ui_tick and
 * ui_tick_mode key field_ui_refresh()'s cached ui_action. pause_notice
 * holds the pause message and its countdown (field_controls.h). */
static u8 menu_last, r_tap_state, ping_life, ping_color, ui_action, ui_tick, ui_tick_mode;
static u8 pause_cursor, pause_notice;
static s16 ping_x, ping_y;
#endif
static int box_suppress_a;
static int unit_tap = -1, unit_tap_x, unit_tap_y;
static u32 unit_tap_frame;
static int weapons_level, armor_level, selected_building;
static int weapons_research, armor_research;
static u32 seed;
#ifdef SC64_IMPORTED_MISSION
volatile u32 sc64_orders_cycles, sc64_sight_cycles, sc64_other_cycles;
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
volatile u32 diag_tracer_cycles, diag_key_cycles, diag_lookup, diag_cull, diag_raster, diag_motion, diag_actor, diag_projectile, diag_children;
#endif
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
static int currently_visible(int x, int y);
#ifdef SC64_SHARED_BUNKER
static void race_bunker_clear_slot(int slot);
#endif
#ifdef SC64_DIRECT_ATTACK
static int target_alive(int target);
static int target_x(int target), target_y(int target);
static void sc64_direct_attack_clear_references(int entity);
static int sc64_direct_attack_find_target(int x, int y);
static int sc64_direct_attack_issue_target(int target);
static void sc64_direct_attack_tick_unit(int slot);
static void tick_combat(Unit *q, int target);
#endif
#ifdef SC64_NEAREST_RESOURCE_DEPOT
#ifndef SC64_RACES
#error "SC64_NEAREST_RESOURCE_DEPOT requires the race campaign runtime"
#endif
static int race_nearest_resource_depot(Unit *worker);
static void race_depot_cache_invalidate_topology(void);
static void race_depot_cache_clear_no_home(void);
#endif
static void add_owner_resources(int owner, int ore, int vespene);
#ifdef SC64_P3_RUNTIME
static int GBA_HOT __attribute__((noinline))
#elif defined(SC64_Z6_TRANSPORT_ENABLED) && !defined(GBA_HOST)
static int GBA_HOT __attribute__((optimize("Os")))
#else
static int GBA_HOT
#endif
owners_hostile(int a, int other)
{
#ifdef SC64_RACES
#ifdef SC64_Z4_RUNTIME
    if (!race_mode && race_selected_mission == 15)
        return (a == SIDE && (other == ENEMY || other == 4)) ||
               (other == SIDE && (a == ENEMY || a == 4));
#endif
#ifdef SC64_P4_RUNTIME
    if (!race_mode && race_selected_mission == SC64_P4_GLOBAL_ID)
        return ((a == SIDE || a == RESCUABLE) && (other == ENEMY || other == 4)) ||
               ((other == SIDE || other == RESCUABLE) && (a == ENEMY || a == 4));
#endif
#ifdef SC64_P5_RUNTIME
    if (race_p5_active()) return race_p5_owners_hostile(a, other);
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) return race_p6_owners_hostile(a, other);
#endif
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

#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
static int sc64_game_unit_collidable(int type)
{ unsigned flags=sc64_game_unit_actor_flags(type); return !flags || (flags & SC64_GAME_ACTOR_COLLIDABLE); }
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
static int
sc64_t4_building_collidable(int slot)
{
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_airborne(slot)) return 0;
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_airborne(slot)) return 0;
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
#ifdef SC64_Z5_RUNTIME
    if (race_z5_active() && race_z5_is_door_type(type))
        return race_z5_building_collidable(slot);
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) return race_p6_building_collidable(slot);
#endif
    unsigned flags = sc64_game_building_actor_flags(type);
    return !flags || (flags & SC64_GAME_ACTOR_COLLIDABLE);
}
#endif
#ifdef SC64_T4_RUNTIME
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
#ifdef SC_FIELD_FEEDBACK
    r_tap_state = 0;
#endif
}
static const char *toast_text[] = {"Command issued", "Need more minerals", "Supply limit reached", "Building placed", "Training unit", "Select a worker", "Site blocked", "Game saved", "Tech prerequisite missing", "Building unfinished", "Research unavailable", "Queue full", "No gas", "Unit pool full", "Mission data skipped", "Deliver cargo first"};
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
/* The player's own mineral, gas and supply refusals (toasts 1, 12 and 2) also
 * cue the race advisor: game_audio_observe() takes the cue at render time, so
 * the command paths gain only this store. Presentation only, never saved. */
static u8 toast_advisor;
static void
toast_refusal(int id)
{
    toast(id);
    toast_advisor = (u8)id;
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

#ifdef SC64_Z7_EGG_PRODUCTION
static inline int
unit_slot_supply(const Unit *q)
{
    if (q && q->type == U_Z7_EGG)
        return sc64_z7_egg_reserved_supply(q->target) > 0 ?
               sc64_z7_egg_reserved_supply(q->target) : 0;
    return q ? unit_supply(q->type) : 0;
}
#else
#define unit_slot_supply(q) unit_supply((q)->type)
#endif
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

#include "sc64_unit_walk.h"

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
#ifdef SC64_CONTACT_MEMO_ENABLED
    SC64_CONTACT_MEMO_INVALIDATE();
#endif

    int i = find_unit_slot();
    if (i < 0)
	return -1;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    /* A recycled slot must never inherit a cast or an incoming missile. */
    sc64_bc_shot_caster_gone(sc64_bc_shots,(unsigned)i);
    race_bc_yamato_target_removed((unsigned)i);
#endif
    sc64_walk_reset_slot(i);
#ifdef SC64_UNIT_FACING
    sc64_facing_reset_slot(i);
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    race_p7_projectile_clear_references(i);
#endif
#ifdef SC64_IMPACT_EVENTS
    race_impact_clear_target(i);
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    race_p7_carrier_training_clear_slot(i);
#endif
#ifdef SC64_DIRECT_ATTACK
    sc64_direct_attack_clear_references(i);
#endif
    /* A newly produced unit can reuse a dead unit's array slot. */
    if (unit_tap == i) reset_unit_tap();
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    invincible_units[i] = (u8)(
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        (sc64_zp_actor_flags(type) & SC64_ZP_ACTOR_INVINCIBLE) != 0 ||
#endif
#ifdef SC64_T4_RUNTIME
        sc64_race_t4_unit_intrinsically_invincible(type)
#else
        0
#endif
        );
#else
    invincible_units[i] = 0;
#endif
#endif
    u[i].x = x;
    u[i].y = y;
    u[i].owner = owner;
    u[i].type = type;
#ifdef SC64_COLLISION_CLASS_CACHE
    sc64_unit_collision_class_invalidate(i);
#endif
    u[i].maxhp = type == U_RAYNOR ? 200 : type == U_SCV ? 60 : type == U_MARINE ? 40 :
	type == U_FIREBAT ? 50 : type == U_MEDIC ? 60 :
	type == U_ZERGLING ? 35 : 80;
#ifdef SC64_RACES
    u[i].maxhp=sc64_race_unit_rules[type].hp;
    u[i].shield=sc64_race_unit_rules[type].shields;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    { int hp = sc64_zp_actor_default_hp(type); if (hp) u[i].maxhp = hp; }
#endif
    u[i].raw_type=u[i].raw_owner=-1;u[i].flags=u[i].shield_remainder=0;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    if (sc64_zp_actor_fact(type)) u[i].raw_type = sc64_race_raw_unit(type);
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (race_p5_active()) {
        u[i].raw_type = sc64_race_raw_unit(type);
        u[i].raw_owner = race_p5_product_source_owner(owner);
    }
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) {
        u[i].raw_type = sc64_race_raw_unit(type);
        u[i].raw_owner = race_z6_product_source_owner(owner);
    }
#endif
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) {
        u[i].raw_type = sc64_race_raw_unit(type);
        u[i].raw_owner = race_z7_product_source_owner(owner);
    }
#endif
#ifdef SC64_Z8_RUNTIME
    race_z8_note_unit_spawn(i, owner, type);
#endif
#if defined(SC64_P7_OBSERVER_ENABLED) || defined(SC64_P7_BATTERY_ENABLED)
    if (race_p7_active() && owner == SIDE) {
        u[i].raw_type = sc64_race_raw_unit(type);
        u[i].raw_owner = 7;
    }
#endif
#ifdef SC64_P8_RUNTIME
    race_p8_note_unit_spawn(i, owner, type);
#endif
#ifdef SC64_P9_RUNTIME
    race_p9_note_unit_spawn(i, owner, type);
#endif
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
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    if (sc64_race_raw_unit(type) == 12) {
        int key = sc64_battlecruiser_owner_key(race_mode, race_selected_mission, owner, u[i].raw_owner);
        int reactor = key >= 0 && sc64_battlecruiser_tech_has(
            sc64_battlecruiser_tech_bits, (unsigned)key, SC64_BC_TECH_REACTOR);
        u[i].carry = sc64_battlecruiser_energy_initial(reactor);
    }
#endif
    u[i].build = -1;
    u[i].work_timer = 0;
    /* Stored armor is the unit's base; combat applies the live upgrade. */
    u[i].armor = 0;
    if (type == U_FIREBAT || type == U_MEDIC)
	u[i].armor++;
#ifdef SC64_RACES
    u[i].armor=sc64_race_unit_armor(type);
#endif
    u[i].weapon = owner == SIDE ? weapons_level : 0;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (race_protoss_services_active()) u[i].weapon = (u16)(1u << 13);
#endif
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
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    race_bc_yamato_target_removed((unsigned)(MAX_UNITS+i));
#endif
    /* Destruction leaves the selection index on the dead slot; a new
       structure there must not inherit it (and its selected vital bar). */
    if (selected_building == i)
        selected_building = -1;
#ifdef SC64_P7_PROJECTILES_ENABLED
    race_p7_projectile_clear_references(MAX_UNITS+i);
#endif
#ifdef SC64_IMPACT_EVENTS
    race_impact_clear_target(MAX_UNITS + i);
#endif
#ifdef SC64_DIRECT_ATTACK
    sc64_direct_attack_clear_references(MAX_UNITS + i);
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
    race_battery_clear_building_references(i);
#endif
#ifdef SC64_SHARED_BUNKER
    race_bunker_clear_slot(i);
#endif
#ifdef SC64_NYDUS_ENABLED
    race_nydus_note_building_lost(i);
#endif
    b[i].x = x;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    invincible_buildings[i] = (u8)(
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        (sc64_zp_building_flags(type) & SC64_ZP_ACTOR_INVINCIBLE) != 0 ||
#endif
#ifdef SC64_T4_RUNTIME
        sc64_race_t4_building_intrinsically_invincible(type)
#else
        0
#endif
        );
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
    b[i].maxhp=sc64_race_building_max_hp(type);
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    { int hp = sc64_zp_building_default_hp(type); if (hp) b[i].maxhp = hp; }
#endif
    b[i].shield=sc64_race_building_shields(type);
    b[i].raw_type=b[i].raw_owner=-1;b[i].flags=b[i].shield_remainder=0;
    if (race_building_can_burn(&b[i])) b[i].shield_remainder=NEUTRAL*256;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    { const Sc64ZpBuildingFact *zp = sc64_zp_building_fact(type);
      if (zp) { b[i].shield = zp->shields;
        b[i].raw_type = sc64_race_raw_building(type); } }
#endif
#ifdef SC64_CONSTRUCTION_HP
    if (constructing)
        b[i].shield = (unsigned short)sc64_construction_shield_start(b[i].shield);
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (race_p5_active()) {
        b[i].raw_type = sc64_race_raw_building(type);
        b[i].raw_owner = race_p5_product_source_owner(owner);
    }
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) {
        b[i].raw_type = sc64_race_raw_building(type);
        b[i].raw_owner = race_z6_product_source_owner(owner);
    }
#endif
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) {
        b[i].raw_type = sc64_race_raw_building(type);
        b[i].raw_owner = race_z7_product_source_owner(owner);
    }
#endif
#ifdef SC64_Z8_RUNTIME
    race_z8_note_building_spawn(i, owner, type);
#endif
#if defined(SC64_P7_OBSERVER_ENABLED) || defined(SC64_P7_BATTERY_ENABLED)
    if (race_p7_active() && owner == SIDE) {
        b[i].raw_type = sc64_race_raw_building(type);
        b[i].raw_owner = 7;
    }
#endif
#ifdef SC64_P8_RUNTIME
    race_p8_note_building_spawn(i, owner, type);
#endif
#ifdef SC64_P9_RUNTIME
    race_p9_note_building_spawn(i, owner, type);
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (race_battery_active() && sc64_race_raw_building(type) == 172) {
        b[i].queue_progress = 100 * 256;
        b[i].level = 0;
        for (int q=0;q<5;++q) b[i].queue[q]=0;
    }
#endif
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
#ifdef SC64_CONSTRUCTION_HP
    b[i].hp = constructing ? sc64_construction_hp_start(b[i].maxhp) : b[i].maxhp;
#else
    b[i].hp = constructing ? 1 : b[i].maxhp;
#endif
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
    for (i = 0; i < geyser_count; i++) {
        if (dist2(x, y, geysers[i].x, geysers[i].y) < 15 * 15)
            return i;
#if defined(SC64_RACES) && defined(SC64_HAS_SPRITES)
        /* Match the actual opaque geyser art. The crop is wider than the
         * 15-pixel center radius, and its canvas offset is asymmetric. */
        const Sc64SpriteFrame *sprite = mission_geyser_sprite();
        if (sprite && sprite->pixels && sprite->w && sprite->h) {
            int left = geysers[i].x - sprite->canvas_w / 2 + sprite->offset_x;
            int top = geysers[i].y - sprite->canvas_h / 2 + sprite->offset_y;
            if (x >= left && x < left + sprite->w &&
                y >= top && y < top + sprite->h) {
                int px = x - left, py = y - top;
                if (sprite->pixels[py * sprite->w + px]) return i;
            }
            continue;
        }
#endif
    }
    return -1;
}

#if defined(SC64_CV5_COVER) && defined(SC64_RACES) && !defined(SC64_RACES_COVER_DATA)
#error "CV5 cover requires a cover-encoded race map header"
#endif
#if defined(SC64_CV5_COVER) && defined(SC64_IMPORTED_SKIRMISH) && !defined(SC64_SKIRMISH_COVER_DATA)
#error "CV5 cover requires a cover-encoded skirmish map header"
#endif
#if defined(SC64_CV5_COVER) && defined(SC64_HAS_TERRAIN_ART) && !defined(SC64_ASSET_COVER_DATA)
#error "CV5 cover requires a cover-encoded terrain asset header"
#endif
#if defined(SC64_CV5_COVER) && defined(SC64_TERRAN_CAMPAIGN) && !defined(SC64_CAMPAIGN_COVER_DATA)
#error "CV5 cover requires a cover-encoded campaign map header"
#endif
/* Experimental CV5 metadata occupies bit 1 of the existing ROM map byte. */
#ifdef SC64_CV5_COVER
static int __attribute__((unused))
terrain_provides_cover(int x, int y)
{
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return 0;
    int cell = x / 16 + (y / 16) * MAP_COLS;
#ifdef SC64_RACES
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) return 0;
#endif
    if (!race_mode) return (sc64_race_maps[race_selected_mission].blocked[cell] & 2) != 0;
#ifdef SC64_IMPORTED_SKIRMISH
    if (race_skirmish_map > 0) return (sc64_skirmish_maps[race_skirmish_map-1].blocked[cell] & 2) != 0;
#endif
    return 0; /* authored arena has no authenticated CV5 terrain */
#elif defined(SC64_IMPORTED_MISSION) && defined(SC64_HAS_TERRAIN_ART) && !defined(SC64_SKIRMISH)
#ifdef SC64_TERRAN_CAMPAIGN
    return (sc64_campaign_maps[campaign_mission].blocked[cell] & 2) != 0;
#else
    return (sc64_terrain_blocked[cell] & 2) != 0;
#endif
#else
    (void)cell; return 0;
#endif
}
#endif

static int
terrain_blocked(int col, int row)
{
#ifdef SC64_RACES
    if(col<0||row<0||col>=MAP_COLS||row>=MAP_ROWS)return 1;
#ifdef SC64_SHOWCASE_ENABLED
    /* The filming set is a flat arena; normal skirmish barriers stay intact. */
    if(sc64_showcase_scene>=0)return 0;
#endif
    if(!race_mode)return (sc64_race_maps[race_selected_mission].blocked[col+row*MAP_COLS]&1) != 0;
#ifdef SC64_IMPORTED_SKIRMISH
    if(race_skirmish_map>0)return (sc64_skirmish_maps[race_skirmish_map-1].blocked[col+row*MAP_COLS]&1) != 0;
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
	return (sc64_campaign_maps[campaign_mission].blocked[col + row * MAP_COLS] & 1) != 0;
	#else
	return (sc64_terrain_blocked[col + row * MAP_COLS] & 1) != 0;
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
#ifdef SC64_DIRECT_FIRING_POINTS
    sc64_direct_firing_cache_invalidate();
#endif
#ifdef SC64_NEAREST_RESOURCE_DEPOT
    race_depot_cache_invalidate_topology();
#endif
#ifdef SC64_IMPORTED_MISSION
    sc64_routes_reset();
#endif
}

#if (defined(SC64_T4_RUNTIME) && defined(SC64_VISIBILITY_SPANS)) || defined(SC64_ZP_CAMPAIGN_STORAGE)
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
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        if (race_bc_facility_airborne(i)) continue;
#endif
#ifdef SC64_RACES
        if(b[i].type==B_ZERG_BEACON)continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (!sc64_t4_building_collidable(i)) continue;
#ifdef SC64_T4_RUNTIME
#ifdef SC64_T4_SCENARIO_RUNTIME
        if (race_t4_active() && sc64_race_t4_is_door(b[i].type)) {
            /* Conservative tile overlap at the GBA's half scale. Opening
             * restores the retained terrain, rather than carving new floor. */
            race_t4_block_door_grid(i);
            continue;
        }
#endif
#endif
#ifdef SC64_Z5_RUNTIME
        if (race_z5_active() && race_z5_is_door_type(b[i].type)) {
            if (race_z5_building_collidable(i)) {
                int left, up, right, down;
                if (sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down)) {
                    int c0=limit((b[i].x*2-left)/32,0,MAP_COLS-1);
                    int c1=limit((b[i].x*2+right)/32,0,MAP_COLS-1);
                    int r0=limit((b[i].y*2-up)/32,0,MAP_ROWS-1);
                    int r1=limit((b[i].y*2+down)/32,0,MAP_ROWS-1);
                    for(row=r0;row<=r1;row++) for(col=c0;col<=c1;col++)
                        SC64_GRID_SET(path_walkable,col+row*MAP_COLS,0);
                }
            }
            continue;
        }
#endif
#endif
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        {
            int left, up, right, down;
            if (sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down)) {
                int x0 = b[i].x * 2 - left, x1 = b[i].x * 2 + right;
                int y0 = b[i].y * 2 - up, y1 = b[i].y * 2 + down;
                int c0 = limit(x0 / 32, 0, MAP_COLS - 1);
                int c1 = limit(x1 / 32, 0, MAP_COLS - 1);
                int r0 = limit(y0 / 32, 0, MAP_ROWS - 1);
                int r1 = limit(y1 / 32, 0, MAP_ROWS - 1);
                for (row = r0; row <= r1; ++row)
                    for (col = c0; col <= c1; ++col) {
                        int cell_x0 = col * 32, cell_y0 = row * 32;
                        int cell_x1 = cell_x0 + 32, cell_y1 = cell_y0 + 32;
                        if (cell_x0 <= x1 && cell_x1 >= x0 &&
                            cell_y0 <= y1 && cell_y1 >= y0)
                            SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
                    }
                continue;
            }
        }
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
can_place_except_worker(int type, int x, int y, int worker)
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
	int geyser = geyser_at(x, y);
	if (geyser < 0)
	    return 0;
#ifdef SC64_RACES
	/* The cursor may be anywhere on the visible source geyser crop; keep the
	 * placed race gas structure centered on the resource it will harvest. */
	x = geysers[geyser].x;
	y = geysers[geyser].y;
#endif
	for (i = 0; i < MAX_BUILDINGS; i++)
	    if (b[i].alive
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
                && !race_bc_facility_airborne(i)
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
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
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
            && !race_bc_facility_airborne(i)
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            && sc64_t4_building_collidable(i)
#endif
            && dist2(x, y, b[i].x, b[i].y) < (radius + 14) * (radius + 14))
	    return 0;
    for (i = 0; i < MAX_UNITS; i++)
	if (i != worker && u[i].alive && !SC64_TRANSPORT_LOADED(&u[i])
#ifdef SC64_RACES
        && !sc64_race_unit_flying(u[i].type)
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        && sc64_game_unit_collidable(u[i].type)
#endif
#ifdef SC64_P6_COMBAT_ENABLED
        && !race_p6_combat_unit_burrowed(i)
#endif
        && dist2(x, y, u[i].x, u[i].y) < (radius + 5) * (radius + 5))
	    return 0;
    return x > 12 && y > 18 && x < WORLD_W - 12 && y < WORLD_H - 12;
}

static int
can_place(int type, int x, int y)
{
    return can_place_except_worker(type, x, y, -1);
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
#include "sc64_depot_goal.h"
_Static_assert(MAX_BUILDINGS < 0x8000, "depot slot keys must fit the tagged route namespace");

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

/* Depot routing has a circular goal rather than a blocked center tile. The
 * non-imported profile uses its existing BFS storage and stops at the first
 * reachable walk cell intersecting the deposit circle. */
static int __attribute__((noinline))
worker_depot_next(int x, int y, int home, int *next_x, int *next_y)
{
#ifdef SC64_IMPORTED_MISSION
    return sc64_route_depot_next(x, y, home, b[home].x, b[home].y, next_x, next_y);
#else
    static const int dx[4] = { 1, -1, 0, 0 };
    static const int dy[4] = { 0, 0, 1, -1 };
    int col = limit(x / 16, 0, MAP_COLS - 1), row = limit(y / 16, 0, MAP_ROWS - 1);
    int start = col + row * MAP_COLS, head = 0, tail = 0, goal = -1, gx = 0, gy = 0;
    if (++bfs_generation == 0) { memset(bfs_seen, 0, sizeof(bfs_seen)); bfs_generation = 1; }
    bfs_prev[start] = -1; bfs_seen[start] = bfs_generation; bfs_queue[tail++] = start;
    while (head < tail) {
        int cell = bfs_queue[head++];
        col = cell % MAP_COLS; row = cell / MAP_COLS;
        if (is_traversable(col, row, -1) &&
            sc64_depot_cell_point(b[home].x, b[home].y, col, row, &gx, &gy)) {
            goal = cell; break;
        }
        for (int i = 0; i < 4; ++i) {
            int nc = col + dx[i], nr = row + dy[i], next;
            if (nc < 0 || nr < 0 || nc >= MAP_COLS || nr >= MAP_ROWS) continue;
            next = nc + nr * MAP_COLS;
            if (bfs_seen[next] == bfs_generation || !is_traversable(nc, nr, start)) continue;
            bfs_seen[next] = bfs_generation; bfs_prev[next] = cell; bfs_queue[tail++] = next;
        }
    }
    if (goal < 0) return 0;
    if (goal == start) { *next_x = gx; *next_y = gy; return 1; }
    while (bfs_prev[goal] != start && bfs_prev[goal] >= 0) goal = bfs_prev[goal];
    *next_x = (goal % MAP_COLS) * 16 + 8;
    *next_y = (goal / MAP_COLS) * 16 + 8;
    return 1;
#endif
}

#ifdef SC64_REACHABLE_FORMATION
static int __attribute__((noinline))
formation_route_pair(int sx, int sy, int slot_x, int slot_y, int tx, int ty,
                     int *hint_slots, int *hint_roots, int *hint_next)
{
#ifdef SC64_IMPORTED_MISSION
    int reachable = sc64_route_formation_fallback_with_hint(
        sx, sy, slot_x, slot_y, tx, ty, hint_slots, hint_roots, hint_next);
    if (reachable >= 0) return reachable;
#else
    (void)hint_slots;
    (void)hint_roots;
    (void)hint_next;
#endif
    {
        int next_x, next_y;
        if (path_next_tile(sx, sy, slot_x, slot_y, &next_x, &next_y)) return 0;
        return path_next_tile(sx, sy, tx, ty, &next_x, &next_y) ? 1 : 0;
    }
}
#endif

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
#ifdef SC64_P6_COMBAT_ENABLED
            if (race_p6_combat_unit_burrowed(i)) continue;
#endif
            int radius = 6;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
            int mover_radius = sc64_game_unit_collision_radius(q->type);
            int candidate_radius = sc64_game_unit_collision_radius(u[i].type);
            radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
#endif
            if (u[i].type == U_CHRYSALIS ||
                sc64_race_unit_flying(u[i].type) != sc64_race_unit_flying(q->type))
                continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            if ((sc64_game_unit_actor_flags(u[i].type) &&
                 !(sc64_game_unit_actor_flags(u[i].type)&SC64_GAME_ACTOR_COLLIDABLE)) ||
                (sc64_game_unit_actor_flags(q->type) &&
                 !(sc64_game_unit_actor_flags(q->type)&SC64_GAME_ACTOR_COLLIDABLE)))
                continue;
#endif
            /* Exact broad-phase reject avoids a distance calculation for
             * distant actors when the per-frame bucket index is unavailable. */
            if (u[i].x - nx >= radius || nx - u[i].x >= radius ||
                u[i].y - ny >= radius || ny - u[i].y >= radius) continue;
            int next_distance = dist2(nx, ny, u[i].x, u[i].y);
            if (next_distance < radius * radius &&
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_unit_actor_flags(q->type) &&
        !(sc64_game_unit_actor_flags(q->type)&SC64_GAME_ACTOR_MOVABLE)) return 1;
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

#ifdef SC64_RACES
/* Keep fact lookup and endpoint arithmetic in ROM: large campaign profiles
 * have very little spare IWRAM. The path/collision loops remain hot. */
#if defined(SC64_CONTACT_CACHE) && !defined(SC64_T5_RUNTIME) && !defined(SC64_CONTACT_STOCK_ROM)
/* Without the division the step is a short table read; keep it inline in the
 * hot mover where the IWRAM reserve allows (the late Terran profile does not,
 * and SC64_CONTACT_STOCK_ROM keeps it out of line for other tight profiles). */
static inline __attribute__((always_inline)) unsigned
#else
static unsigned __attribute__((noinline))
#endif
sc64_unit_stock_step(const Unit *q,int tx,int ty)
{
#ifdef SC64_CONTACT_CACHE
    return sc64_contact_stock_step(q->raw_type>=0?q->raw_type:
        sc64_race_raw_unit(q->type),(unsigned)frame,(unsigned)(q-u),
        tx!=q->x && ty!=q->y);
#else
    return sc64_stock_motion_step(q->raw_type>=0?q->raw_type:
        sc64_race_raw_unit(q->type),(unsigned)frame,(unsigned)(q-u),
        tx!=q->x && ty!=q->y);
#endif
}

static void __attribute__((noinline))
sc64_stock_flying_move(Unit *q,int tx,int ty)
{
    unsigned step=sc64_unit_stock_step(q,tx,ty);
    q->x=limit(sc64_stock_motion_axis(q->x,tx,step),0,WORLD_W-1);
    q->y=limit(sc64_stock_motion_axis(q->y,ty,step),0,WORLD_H-1);
    sc64_unit_bucket_move(q);
    sc64_target_index_move(q);
}
#endif

#if defined(SC64_COLLISION_STEER_LIST) && defined(SC64_Z7_RUNTIME)
/* The bounded neighbor candidate list is a short-lived local to blocked
 * steering. Keep its 388 bytes off BSS and off move_toward()'s common path. */
static int __attribute__((noinline))
sc64_steer_best_direction(Unit *q, int speed, int from_cell,
                          int tx, int ty)
{
    static const s8 dx[8] = {1,1,0,-1,-1,-1,0,1};
    static const s8 dy[8] = {0,1,1,1,0,-1,-1,-1};
    Sc64SteerList list;
    int best = -1, best_cost = 0x7fffffff;
    int built = sc64_steer_list_build(&list, q, speed);
    for (int i = 0; i < 8; ++i) {
        int sx = q->x + dx[i] * speed, sy = q->y + dy[i] * speed;
        int cost = dist2(sx, sy, tx, ty);
        if (cost >= best_cost ||
            !is_traversable(sx / 16, sy / 16, from_cell))
            continue;
        if (built ? sc64_steer_list_step_clear(&list, q, sx, sy)
                  : unit_step_clear(q, sx, sy)) {
            best = i;
            best_cost = cost;
        }
    }
    return best;
}
#endif

static void GBA_HOT
#if defined(SC64_T6_RUNTIME) && !defined(GBA_HOST)
__attribute__((optimize("Os")))
#endif
move_toward(Unit * q, int tx, int ty)
{
	int nx, ny, speed = q->type == U_ZERGLING || q->type == U_FIREBAT || q->type == U_RAYNOR ? 2 : 1;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    if (q->type == U_ZP_RAYNOR_VULTURE) speed = 2;
#endif
#ifdef SC64_DIAGNOSTIC_ORDERS
    u32 sc64_move_start = gba_clock();
#endif
#ifdef SC64_RACES
    if(q->type==U_VULTURE)speed=2;
#ifdef SC64_T5_RUNTIME
    if(q->type==U_DROPSHIP)speed=2;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_unit_actor_flags(q->type) &&
        !(sc64_game_unit_actor_flags(q->type)&SC64_GAME_ACTOR_MOVABLE)) return;
#endif
    if(sc64_race_unit_flying(q->type)) {
        sc64_stock_flying_move(q,tx,ty);
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
#ifdef SC64_RACES
    speed=sc64_unit_stock_step(q,q->path_x,q->path_y);
    nx=sc64_stock_motion_axis(q->x,q->path_x,(unsigned)speed);
    ny=sc64_stock_motion_axis(q->y,q->path_y,(unsigned)speed);
#else
    nx = q->x < q->path_x ? q->x + speed : q->x > q->path_x ? q->x - speed : q->x;
    ny = q->y < q->path_y ? q->y + speed : q->y > q->path_y ? q->y - speed : q->y;
#endif
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
	int best = -1;
#if !defined(SC64_Z7_RUNTIME) || !defined(SC64_COLLISION_STEER_LIST)
	int i, best_cost = 0x7fffffff;
#endif
	int from_cell = q->x / 16 + (q->y / 16) * MAP_COLS;
	/* Local steering avoids repeatedly retrying the same occupied waypoint.
	 * Static terrain still uses BFS; candidates may leave an occupied tile
	 * but may never enter a different blocked cell. */
#if defined(SC64_CONTACT_CACHE)
	/* One bucket pass answers all eight unit_step_clear() offsets; the
	 * selection order and tie-breaking below are unchanged. */
	unsigned blocked = sc64_contact_blocked_mask(q, speed);
	for (i = 0; i < 8; i++) {
	    int sx = q->x + dx[i] * speed, sy = q->y + dy[i] * speed;
	    int cost = dist2(sx, sy, q->path_x, q->path_y);
	    if (cost < best_cost && is_traversable(sx / 16, sy / 16, from_cell) &&
		!(blocked & (1u << i))) { best = i; best_cost = cost; }
	}
#elif defined(SC64_COLLISION_STEER_LIST)
	#if defined(SC64_Z7_RUNTIME)
	best = sc64_steer_best_direction(q, speed, from_cell,
	                                  q->path_x, q->path_y);
	#else
	/* Build the neighbor list lazily, where the first query would run. */
	int steer_list = -1;
	for (i = 0; i < 8; i++) {
	    int sx = q->x + dx[i] * speed, sy = q->y + dy[i] * speed;
	    int cost = dist2(sx, sy, q->path_x, q->path_y);
	    if (cost >= best_cost || !is_traversable(sx / 16, sy / 16, from_cell))
		continue;
	    if (steer_list < 0) steer_list = sc64_steer_list_build(q, speed);
	    if (steer_list ? sc64_steer_list_step_clear(q, sx, sy)
	                   : unit_step_clear(q, sx, sy)) { best = i; best_cost = cost; }
	}
	#endif
#else
	for (i = 0; i < 8; i++) {
	    int sx = q->x + dx[i] * speed, sy = q->y + dy[i] * speed;
	    int cost = dist2(sx, sy, q->path_x, q->path_y);
	    if (cost < best_cost && is_traversable(sx / 16, sy / 16, from_cell) &&
		unit_step_clear(q, sx, sy)) { best = i; best_cost = cost; }
	}
#endif
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

#ifdef SC64_TRANSPORT_RUNTIME
#include "sc64_transport_adapter.h"
#endif

/* These expand in query declarations/calls outside the race-specific block
 * too, so keep the source-owner plumbing available in every profile. */
#ifdef SC64_P7_OBSERVER_ENABLED
#define SC64_P7_SOURCE_OWNER_PARAM , int source_owner
#define SC64_P7_SOURCE_OWNER_ARG(owner) , (owner)
#define SC64_P7_SOURCE_OWNER_VALUE source_owner
#else
#define SC64_P7_SOURCE_OWNER_PARAM
#define SC64_P7_SOURCE_OWNER_ARG(owner)
#define SC64_P7_SOURCE_OWNER_VALUE (-1)
#endif

/* New automatic target selection observes the player's current fog. Keep this
 * out of the hot candidate loops; explicit orders and projectiles do not use
 * this helper, so remembered targets keep their existing behavior. */
static int __attribute__((noinline))
sc64_auto_target_visible(int target, int owner)
{
    int tx, ty;
    if (owner != SIDE) return 1;
    if (target < 0) return 0;
    if (target < MAX_UNITS) {
        tx = u[target].x; ty = u[target].y;
    } else {
        int bi = target - MAX_UNITS;
        if (bi < 0 || bi >= MAX_BUILDINGS) return 0;
        tx = b[bi].x; ty = b[bi].y;
    }
    return tx >= 0 && ty >= 0 && tx < WORLD_W && ty < WORLD_H &&
           currently_visible(tx, ty);
}

#ifdef SC64_RACES
static int race_can_attack(int attacker,int target);
#if (defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)) || \
    defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
/* Keep the shared target protections and campaign-specific T6 burrow check
 * even when multiple scenario capabilities are compiled together. */
static int __attribute__((noinline))
sc64_p3_can_attack_from(int attacker, int target, int owner)
{
    if (attacker == U_SCOURGE &&
        (target < 0 || target >= MAX_UNITS || !u[target].alive ||
         !sc64_race_unit_flying(u[target].type) || (u[target].flags & 16) ||
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
         invincible_units[target] ||
#endif
         !owners_hostile(owner, u[target].owner))) return 0;
#ifdef SC64_T6_RUNTIME
    if (target >= 0 && target < MAX_UNITS && race_t6_unit_burrowed(target)) return 0;
#endif
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
#ifdef SC64_P6_COMBAT_ENABLED
static int sc64_p6_can_attack_from(int attacker, int target, int owner)
{
    if (race_p6_active() &&
        (!race_p6_combat_target_detected(target, owner) ||
         (target >= 0 && target < MAX_UNITS && race_p6_combat_unit_burrowed(target))))
        return 0;
    return sc64_p3_can_attack_from(attacker, target, owner);
}
#undef SC64_CAN_ATTACK_FROM
#define SC64_CAN_ATTACK_FROM(attacker, target, owner) \
    sc64_p6_can_attack_from((attacker), (target), (owner))
#endif
#ifdef SC64_Z8_RUNTIME
/* Z8 hero Dark Templars stay cloaked to the human until a human detector
 * reveals them (sc64_z8_runtime_adapter.h). */
static int sc64_z8_base_can_attack_from(int attacker, int target, int owner)
{
    return SC64_CAN_ATTACK_FROM(attacker, target, owner);
}
static int sc64_z8_can_attack_from(int attacker, int target, int owner)
{
    if (owner == SIDE && race_z8_unit_hidden_from_side(target)) return 0;
    return sc64_z8_base_can_attack_from(attacker, target, owner);
}
#undef SC64_CAN_ATTACK_FROM
#define SC64_CAN_ATTACK_FROM(attacker, target, owner) \
    sc64_z8_can_attack_from((attacker), (target), (owner))
#endif
#ifdef SC64_P7_OBSERVER_ENABLED
#define SC64_CAN_ATTACK_FROM_SOURCE(attacker, target, owner, source_owner) \
    sc64_p7_observer_can_attack_from((attacker), (target), (owner), \
                                     (source_owner))
#else
#define SC64_CAN_ATTACK_FROM_SOURCE(attacker, target, owner, source_owner) \
    SC64_CAN_ATTACK_FROM((attacker), (target), (owner))
#endif
static int __attribute__((noinline))
sc64_auto_target_candidate_allowed(int attacker, int target, int owner,
                                   int source_owner)
{
    int tx, ty;
    (void)source_owner;
    if (owner == SIDE) {
        if (target < 0) return 0;
        if (target < MAX_UNITS) {
            tx = u[target].x; ty = u[target].y;
        } else {
            int bi = target - MAX_UNITS;
            if (bi < 0 || bi >= MAX_BUILDINGS) return 0;
            tx = b[bi].x; ty = b[bi].y;
        }
        if (tx < 0 || ty < 0 || tx >= WORLD_W || ty >= WORLD_H ||
            !currently_visible(tx, ty)) return 0;
    }
    return SC64_CAN_ATTACK_FROM_SOURCE(attacker, target, owner, source_owner);
}
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
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
#endif
static int GBA_HOT __attribute__((noinline))
#if defined(SC64_T6_RUNTIME) && !defined(GBA_HOST)
__attribute__((optimize("Os")))
#endif
sc64_target_nearest(int x, int y, int owner, int max_distance, int attacker
                    SC64_P7_SOURCE_OWNER_PARAM)
{
#ifdef SC64_Z7_EGG_PRODUCTION
    int egg_best = -1, egg_distance = max_distance * max_distance;
#endif
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
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
        for (row = row0; row <= row1; ++row) {
            if (!(sc64_target_owner_rows[p] & ((u32)1u << row))) continue;
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
#ifdef SC64_Z7_EGG_PRODUCTION
                    if (entity < MAX_UNITS && u[entity].type == U_Z7_EGG) {
                        if (d < egg_distance &&
                            sc64_auto_target_candidate_allowed(
                                attacker, entity, owner,
                                SC64_P7_SOURCE_OWNER_VALUE)) {
                            egg_distance = d;
                            egg_best = entity;
                        }
                        continue;
                    }
#endif
                    if (d > best_distance ||
                        (d == best_distance && (best < 0 || entity >= best)) ||
                        !sc64_auto_target_candidate_allowed(
                            attacker, entity, owner,
                            SC64_P7_SOURCE_OWNER_VALUE)) continue;
                    best_distance = d;
                    best = entity;
                }
            }
        }
    }
#ifdef SC64_Z7_EGG_PRODUCTION
    return best >= 0 ? best : egg_best;
#else
    return best;
#endif
#else
    int best = -1, best_distance = max_distance * max_distance, d;
    int cols = (WORLD_W + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int rows = (WORLD_H + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int col0 = (x - max_distance) / SC64_TARGET_CELL_SIZE;
    int row0 = (y - max_distance) / SC64_TARGET_CELL_SIZE;
    int col1 = (x + max_distance) / SC64_TARGET_CELL_SIZE;
    int row1 = (y + max_distance) / SC64_TARGET_CELL_SIZE;
    int p, col, row;
    if (col0 < 0) col0 = 0;
    if (row0 < 0) row0 = 0;
    if (col1 >= cols) col1 = cols - 1;
    if (row1 >= rows) row1 = rows - 1;
    for (p = 0; p < 5; ++p) {
        if (!owners_hostile(owner, p)) continue;
        for (row = row0; row <= row1; ++row)
            for (col = col0; col <= col1; ++col) {
                int cell = col + row * SC64_TARGET_GRID_COLS, entity;
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
                        FAST_ABS(y - candidate_y) >= max_distance ||
                        !sc64_auto_target_candidate_allowed(
                            attacker, entity, owner,
                            SC64_P7_SOURCE_OWNER_VALUE)) continue;
                    d = dist2(x, y, candidate_x, candidate_y);
#ifdef SC64_Z7_EGG_PRODUCTION
                    if (entity < MAX_UNITS && u[entity].type == U_Z7_EGG) {
                        if (d < egg_distance) {
                            egg_distance = d;
                            egg_best = entity;
                        }
                        continue;
                    }
#endif
                    if (d < best_distance ||
                        (best >= 0 && d == best_distance && entity < best)) {
                        best_distance = d;
                        best = entity;
                    }
                }
            }
    }
#ifdef SC64_Z7_EGG_PRODUCTION
    return best >= 0 ? best : egg_best;
#else
    return best;
#endif
#endif
}
static int __attribute__((noinline))
nearest_enemy_scan(int x, int y, int owner, int max_distance, int attacker
                   SC64_P7_SOURCE_OWNER_PARAM)
{
    int i, best = -1, best_distance = max_distance * max_distance, d;
#ifdef SC64_Z7_EGG_PRODUCTION
    int egg_best = -1, egg_distance = max_distance * max_distance;
#endif
    if (target_lists_valid) {
        int p;
        for (p = 0; p < 5; ++p) {
            if (!owners_hostile(owner, p)) continue;
            for (i = target_unit_head[p]; i >= 0; i = target_unit_next[i]) {
                if (!u[i].alive || FAST_ABS(x-u[i].x)>=max_distance || FAST_ABS(y-u[i].y)>=max_distance)continue;
                if(!sc64_auto_target_candidate_allowed(attacker,i,owner,SC64_P7_SOURCE_OWNER_VALUE))continue;
                d = dist2(x, y, u[i].x, u[i].y);
#ifdef SC64_Z7_EGG_PRODUCTION
                if (u[i].type == U_Z7_EGG) {
                    if (d < egg_distance) { egg_distance = d; egg_best = i; }
                    continue;
                }
#endif
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
                if(!sc64_auto_target_candidate_allowed(attacker,MAX_UNITS+i,owner,SC64_P7_SOURCE_OWNER_VALUE))continue;
                d = dist2(x, y, b[i].x, b[i].y);
                if (d < best_distance || (best >= 0 && d == best_distance && candidate < best)) {
                    best_distance = d;
                    best = candidate;
                }
            }
        }
#ifdef SC64_Z7_EGG_PRODUCTION
        return best >= 0 ? best : egg_best;
#else
        return best;
#endif
    }
    for (i = 0; i < MAX_UNITS; i++) {
	if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i])
	    || FAST_ABS(x - u[i].x) >= max_distance ||
	    FAST_ABS(y - u[i].y) >= max_distance || !owners_hostile(owner, u[i].owner)
                    || !sc64_auto_target_candidate_allowed(attacker,i,owner,SC64_P7_SOURCE_OWNER_VALUE)
                    )
	    continue;
	d = dist2(x, y, u[i].x, u[i].y);
#ifdef SC64_Z7_EGG_PRODUCTION
        if (u[i].type == U_Z7_EGG) {
            if (d < egg_distance) { egg_distance = d; egg_best = i; }
            continue;
        }
#endif
	if (d < best_distance) {
	    best_distance = d;
	    best = i;
	}
    }
    for (i = 0; i < MAX_BUILDINGS; i++) {
	if (!b[i].alive
	    || FAST_ABS(x - b[i].x) >= max_distance ||
	    FAST_ABS(y - b[i].y) >= max_distance || !owners_hostile(owner, b[i].owner)
                    || !sc64_auto_target_candidate_allowed(attacker,MAX_UNITS+i,owner,SC64_P7_SOURCE_OWNER_VALUE)
                    )
	    continue;
	d = dist2(x, y, b[i].x, b[i].y);
	if (d < best_distance) {
	    best_distance = d;
	    best = MAX_UNITS + i;
	}
    }
#ifdef SC64_Z7_EGG_PRODUCTION
    return best >= 0 ? best : egg_best;
#else
    return best;
#endif
}
#endif

#if defined(SC64_Z7_SCOURGE_COMBAT) && defined(SC64_UNARMED_QUERY_SKIP)
/* Share the expanded ground/air lookup from ROM in the larger Z7 build. */
static int __attribute__((noinline))
sc64_z7_unit_has_weapon(int type)
{
    return sc64_race_unit_damage_vs(type, 0) > 0 ||
           sc64_race_unit_damage_vs(type, 1) > 0;
}
#endif

static int GBA_HOT
nearest_enemy_typed(int x, int y, int owner, int max_distance, int attacker
                    SC64_P7_SOURCE_OWNER_PARAM)
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
#ifdef SC64_Z7_SCOURGE_COMBAT
        !sc64_z7_unit_has_weapon(attacker)) {
#else
        sc64_race_unit_damage_vs(attacker, 0) <= 0 &&
        sc64_race_unit_damage_vs(attacker, 1) <= 0) {
#endif
        result = -1;
        goto sc64_query_done;
    }
#endif
#ifdef SC64_RACES
    if (target_lists_valid && sc64_target_index_active && max_distance > 0 &&
        x >= 0 && y >= 0 && x < WORLD_W && y < WORLD_H)
        result = sc64_target_nearest(x, y, owner, max_distance, attacker
                                     SC64_P7_SOURCE_OWNER_ARG(SC64_P7_SOURCE_OWNER_VALUE));
    else
        result = nearest_enemy_scan(x, y, owner, max_distance, attacker
                                    SC64_P7_SOURCE_OWNER_ARG(SC64_P7_SOURCE_OWNER_VALUE));
#else
    int i, best = -1, best_distance = max_distance * max_distance, d;
    (void)attacker;
    if (target_lists_valid) {
        int p;
        for (p = 0; p < 5; ++p) {
            if (!owners_hostile(owner, p)) continue;
            for (i = target_unit_head[p]; i >= 0; i = target_unit_next[i]) {
                if (!u[i].alive || FAST_ABS(x-u[i].x)>=max_distance || FAST_ABS(y-u[i].y)>=max_distance || !sc64_auto_target_visible(i,owner))continue;
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
                if (!b[i].alive || FAST_ABS(x-b[i].x)>=max_distance || FAST_ABS(y-b[i].y)>=max_distance || !sc64_auto_target_visible(candidate,owner))continue;
                d = dist2(x, y, b[i].x, b[i].y);
                if (d < best_distance || (best >= 0 && d == best_distance && candidate < best)) {
                    best_distance = d; best = candidate;
                }
            }
        }
    } else {
        for (i = 0; i < MAX_UNITS; i++) {
            if (!u[i].alive || FAST_ABS(x-u[i].x)>=max_distance ||
                FAST_ABS(y-u[i].y)>=max_distance || !owners_hostile(owner,u[i].owner) ||
                !sc64_auto_target_visible(i,owner)) continue;
            d = dist2(x,y,u[i].x,u[i].y);
            if (d < best_distance) { best_distance = d; best = i; }
        }
        for (i = 0; i < MAX_BUILDINGS; i++) {
            if (!b[i].alive || FAST_ABS(x-b[i].x)>=max_distance ||
                FAST_ABS(y-b[i].y)>=max_distance || !owners_hostile(owner,b[i].owner) ||
                !sc64_auto_target_visible(MAX_UNITS+i,owner)) continue;
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
    return nearest_enemy_typed(x,y,owner,distance,-1 SC64_P7_SOURCE_OWNER_ARG(-1));
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
#ifdef SC64_P6_COMBAT_ENABLED
    if (target >= 0 && target < MAX_UNITS && race_p6_combat_unit_burrowed(target)) return 0;
#endif
#ifdef SC64_T6_RUNTIME
    if (target >= 0 && target < MAX_UNITS && race_t6_unit_burrowed(target)) return 0;
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    if (target >= 0 && target < MAX_UNITS && SC64_TRANSPORT_LOADED(&u[target]))
        return 0;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (target >= 0 && target < MAX_UNITS && u[target].alive &&
        (sc64_game_unit_has_flag(u[target].type, SC64_GAME_ACTOR_HIDDEN) ||
         SC64_CARRIER_STATE_HIDDEN(&u[target]))) return 0;
#endif
    return target >= 0 && (target < MAX_UNITS ? u[target].alive : b[target - MAX_UNITS].alive);
}

#ifdef SC64_DIRECT_ATTACK
/* Direct orders use the packed order/target fields already present in each
 * unit.  Slot reuse must invalidate references before the dead entity's
 * coordinates/type/owner are overwritten. */
static void __attribute__((noinline))
sc64_direct_attack_clear_references(int entity)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (q->alive && q->order == ORDER_ATTACK_TARGET && q->target == entity) {
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
            if (sc64_race_raw_unit(q->type)==12 && (q->work_timer&7)>0 && (q->work_timer&7)<=6) {
                q->target=-1; continue; /* cast animation outlives its target */
            }
#endif
            q->order = ORDER_IDLE;
            q->target = -1;
            q->tx = q->x;
            q->ty = q->y;
            q->path_timer = q->path_retry = 0;
        }
    }
}

static int __attribute__((noinline))
sc64_direct_target_state(int entity, int source_live_owner,
                         Sc64DirectTargetState *state)
{
    int owner, x, y, loaded = 0, immune;
    if (!state || entity < 0 || entity >= MAX_UNITS + MAX_BUILDINGS ||
        !target_alive(entity)) return 0;
    if (entity < MAX_UNITS) {
        owner = u[entity].owner;
        x = u[entity].x; y = u[entity].y;
        loaded = SC64_TRANSPORT_LOADED(&u[entity]);
        immune = (u[entity].flags & 16) != 0;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
        immune |= invincible_units[entity] != 0;
#endif
#ifdef SC64_T4_RUNTIME
        immune |= sc64_race_t4_unit_intrinsically_invincible(u[entity].type);
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        immune |= (sc64_game_unit_actor_flags(u[entity].type) &
                   (SC64_GAME_ACTOR_HIDDEN | SC64_GAME_ACTOR_INVINCIBLE)) != 0 ||
                   SC64_CARRIER_STATE_HIDDEN(&u[entity]);
#endif
    } else {
        int bi = entity - MAX_UNITS;
        owner = b[bi].owner;
        x = b[bi].x; y = b[bi].y;
        immune = (b[bi].flags & 16) != 0;
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
        immune |= invincible_buildings[bi] != 0;
#endif
#ifdef SC64_T4_RUNTIME
        immune |= sc64_race_t4_building_intrinsically_invincible(b[bi].type);
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        immune |= (sc64_game_building_actor_flags(b[bi].type) &
                   SC64_GAME_ACTOR_INVINCIBLE) != 0;
#endif
#ifdef SC64_RACES
        immune |= b[bi].type == B_ZERG_BEACON;
#endif
    }
    state->alive = 1;
    state->hostile = owners_hostile(source_live_owner, owner);
    state->visible = source_live_owner != SIDE || currently_visible(x, y);
    state->loaded = loaded;
    state->invincible = immune;
    /* Actual attack/detection eligibility is evaluated per selected shooter
     * below; this field carries only generic target policy here. */
    state->detectable = 1;
    return sc64_direct_target_valid(*state);
}

static int __attribute__((noinline))
sc64_direct_attack_find_target(int x, int y)
{
    int entity, best = -1, best_d = 0x7fffffff;
    for (entity = 0; entity < MAX_UNITS + MAX_BUILDINGS; ++entity) {
        int owner, tx, ty, radius, d;
        Sc64DirectTargetState s;
        if (entity < MAX_UNITS) {
            if (!u[entity].alive || SC64_TRANSPORT_LOADED(&u[entity])) continue;
            owner = u[entity].owner; tx = u[entity].x; ty = u[entity].y;
            radius = 31;
        } else {
            int bi = entity - MAX_UNITS;
            if (!b[bi].alive) continue;
            owner = b[bi].owner; tx = b[bi].x; ty = b[bi].y;
            radius = 26;
        }
        if (!owners_hostile(SIDE, owner) || !currently_visible(tx, ty) ||
            !sc64_direct_target_state(entity, SIDE, &s)) continue;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
        if (race_protoss_services_active() && sc64_p7_observer_target_is_cloaked(entity)) {
            int eligible=0;
            for (int a=0;a<MAX_UNITS;++a) if (u[a].alive && u[a].selected &&
                u[a].owner==SIDE && !SC64_TRANSPORT_LOADED(&u[a]) &&
                SC64_CAN_ATTACK_FROM_SOURCE(u[a].type,entity,u[a].owner,
                                            u[a].raw_owner)) {
                eligible=1;
                break;
            }
            if (!eligible) continue;
        }
#endif
        d = dist2(x, y, tx, ty);
        if (d < radius * radius && d < best_d) {
            best = entity;
            best_d = d;
        }
    }
    return best;
}

static int __attribute__((noinline))
sc64_direct_attack_issue_target(int target)
{
    int i, issued = 0;
    Sc64DirectTargetState state;
    if (!sc64_direct_target_state(target, SIDE, &state) ||
        !sc64_direct_target_can_damage(state))
        return 0;
    selected_building = -1;
    for (i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (!q->alive || !q->selected || q->owner != SIDE ||
            SC64_TRANSPORT_LOADED(q)) continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (sc64_game_unit_actor_flags(q->type) &&
            !(sc64_game_unit_actor_flags(q->type) & SC64_GAME_ACTOR_MOVABLE))
            continue;
#endif
        if (!SC64_CAN_ATTACK_FROM_SOURCE(q->type, target, q->owner,
                                         q->raw_owner)) continue;
#ifdef SC64_T6_RUNTIME
        if (q->order == ORDER_REPAIR) {
            q->carry = 0;
            q->work_timer = 0;
        }
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
        if (sc64_race_raw_unit(q->type)==12) q->work_timer&=~7;
#endif
        q->order = ORDER_ATTACK_TARGET;
        q->target = target;
        q->tx = target_x(target);
        q->ty = target_y(target);
        q->path_timer = q->path_retry = 0;
        ++issued;
    }
    toast(issued ? 0 : 5);
    return issued;
}

static void __attribute__((noinline))
sc64_direct_attack_tick_unit(int slot)
{
    Unit *q = &u[slot];
    int target = q->target, tx, ty;
    Sc64DirectTargetState state;
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS ||
        !sc64_direct_target_state(target, q->owner, &state)) {
        q->order = ORDER_IDLE;
        q->target = -1;
        return;
    }
    if (state.visible) {
        if (SC64_CAN_ATTACK_FROM_SOURCE(q->type, target, q->owner,
                                        q->raw_owner)) {
            q->tx = target_x(target);
            q->ty = target_y(target);
            tick_combat(q, target);
        } else {
            /* Optical visibility without weapon/detection eligibility is not
             * a fogged last-known target. Drop it rather than chasing. */
            q->order = ORDER_IDLE;
            q->target = -1;
        }
        return;
    }
    /* A fogged or undetected target never contributes its current coordinates
     * to movement or damage. The order approaches only its saved last-seen
     * position and expires there; no native order queue/reacquisition exists. */
    tx = q->tx; ty = q->ty;
    if (!sc64_direct_order_keep_last_seen(0, dist2(q->x, q->y, tx, ty))) {
        q->order = ORDER_IDLE;
        q->target = -1;
    } else {
        move_toward(q, tx, ty);
    }
}
#endif

#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
#ifndef SC64_WEAPON_SOURCE_VFX
#error "Yamato art requires the shared source-VFX renderer"
#endif
#define SC64_YAMATO_TRAIL_STYLE 29
#define SC64_YAMATO_IMPACT_STYLE 30
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
static unsigned sc64_bc_p7_visual_phase(void);
#endif
static unsigned sc64_bc_yamato_visual_phase(void)
{
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (race_p7_active()) return sc64_bc_p7_visual_phase();
#endif
    return (((unsigned)frame%5u)+5u-sc64_bc_pause_offset)%5u;
}
static int sc64_bc_yamato_visual_tag(unsigned style,unsigned heading)
{ return (int)((style<<8)|((heading&31u)<<3)|sc64_bc_yamato_visual_phase()); }
#endif

/* Reconstruct presentation age from existing endpoints; no new saved state. */
#if defined(SC64_TRACE_PROFILE_INDEX_CACHE) && defined(SC64_TRACE_DURATION_CACHE) && defined(SC64_WEAPON_SOURCE_VFX)
#include "sc64_trace_profile_cache.h"
#endif
static int __attribute__((noinline))
sc64_trace_duration(int packed, int x0, int y0, int x1, int y1)
{
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
    if (sc64_weapon_vfx_style(packed)==SC64_YAMATO_TRAIL_STYLE) return 9;
    if (sc64_weapon_vfx_style(packed)==SC64_YAMATO_IMPACT_STYLE) return 36;
#endif
#ifdef SC64_WEAPON_SOURCE_VFX
    if (sc64_weapon_source_vfx_is_tagged((uint32_t)packed)) {
        Sc64WeaponSourceVfxKey key;
        const Sc64WeaponSourceAttackProfile *profile =
            sc64_weapon_source_trace_profile((uint32_t)packed);
        if (!profile || !sc64_weapon_source_vfx_unpack((uint32_t)packed, &key)) return 0;
        unsigned duration = sc64_weapon_source_trace_duration_heading(profile,key.direction,x0,y0,x1,y1);
#ifdef SC64_UNIT_ATTACK_POSES_ENABLED
        if (key.actor_slot_plus1) {
            unsigned pose = sc64_weapon_source_vfx_gba_duration(
                sc64_unit_attack_source_full_duration(key.actor_raw,key.target_air));
            if (pose > duration) duration = pose;
        }
#endif
        return (int)duration;
    }
#else
    (void)x0; (void)y0; (void)x1; (void)y1;
#endif
    return sc64_weapon_vfx_lifetime(sc64_weapon_vfx_style(packed));
}

static inline int
sc64_tracer_duration(int slot)
{
#ifdef SC64_TRACE_DURATION_CACHE
#ifdef SC64_TRACE_PROFILE_INDEX_CACHE
    if (sc64_trace_durations[slot] & 255u) return sc64_trace_durations[slot] & 255u;
#else
    if (sc64_trace_durations[slot]) return sc64_trace_durations[slot];
#endif
#endif
    const Tracer *t = &tracers[slot];
    int duration = sc64_trace_duration(t->color,t->x0,t->y0,t->x1,t->y1);
    sc64_trace_duration_cache_store(slot, duration);
    return duration;
}

static void __attribute__((noinline))
add_tracer(int x0, int y0, int x1, int y1, int color)
{
    int slot=0,oldest=-1;
    if (!color) return; /* Non-combat actor: no invented weapon. */
    if (!sc64_weapon_vfx_style(color))
        color=sc64_weapon_vfx_pack(SC64_WEAPON_VFX_STYLE_MUZZLE,color);
    int duration=sc64_trace_duration(color,x0,y0,x1,y1);
    if (duration<=0) return;
    for (int i=0;i<MAX_TRACERS;++i) {
        if (!tracers[i].life) { slot=i; break; }
        int age=sc64_tracer_duration(i)+1-tracers[i].life;
        if (age>oldest) { oldest=age; slot=i; }
    }
    /* Cosmetic overflow replaces the oldest effect, never a damage event.
     * Fresh shots cannot be starved indefinitely by an earlier volley. */
    tracers[slot].x0=x0; tracers[slot].y0=y0;
    tracers[slot].x1=x1; tracers[slot].y1=y1;
    tracers[slot].color=color;
    sc64_trace_duration_cache_store(slot, duration);
#if defined(SC64_TRACE_PROFILE_INDEX_CACHE) && defined(SC64_TRACE_DURATION_CACHE) && defined(SC64_WEAPON_SOURCE_VFX)
    if (duration <= 255 && sc64_weapon_source_vfx_is_tagged((uint32_t)color)) {
        Sc64WeaponSourceVfxKey key;
        if (sc64_weapon_source_vfx_unpack((uint32_t)color, &key)) {
            const Sc64WeaponSourceAttackProfile *p=sc64_weapon_source_profile_lookup(key.actor_raw,key.weapon_id,key.target_air,key.variant);
            sc64_trace_durations[slot] |= (u16)(sc64_trace_profile_slot_index(p,key)<<8);
        }
    }
#endif
    /* Combat emits before end-of-update aging, including the first draw. */
    tracers[slot].life=duration+1;
}

#ifdef SC64_WEAPON_SOURCE_VFX
static int
sc64_source_effect_direction(int dx,int dy)
{
    static const short vector[32][2] = {
        {0,-256},{50,-251},{98,-237},{142,-213},{181,-181},{213,-142},{237,-98},{251,-50},
        {256,0},{251,50},{237,98},{213,142},{181,181},{142,213},{98,237},{50,251},
        {0,256},{-50,251},{-98,237},{-142,213},{-181,181},{-213,142},{-237,98},{-251,50},
        {-256,0},{-251,-50},{-237,-98},{-213,-142},{-181,-181},{-142,-213},{-98,-237},{-50,-251}
    };
    int best=0; long score=-0x7fffffffL;
    for(int i=0;i<32;++i) {
        long dot=(long)dx*vector[i][0]+(long)dy*vector[i][1];
        if(dot>score) {score=dot;best=i;}
    }
    return best;
}

static int __attribute__((noinline))
sc64_source_emit(int raw,int slot_plus1,int weapon,int air,
                 int x0,int y0,int x1,int y1)
{
    if(raw<0 || raw>=228 || weapon<0 || weapon>=130) return 0;
    unsigned variant=raw>=106 ? 2u : (raw==83 && weapon==82) ? 1u : 0u;
    const Sc64WeaponSourceAttackProfile *profile =
        sc64_weapon_source_profile_lookup((uint8_t)raw,(uint8_t)weapon,(uint8_t)air,(uint8_t)variant);
    if(!profile) return 0;
    unsigned count=profile->weapon_count==2 ? 2u : 1u;
    for(unsigned shot=0;shot<count;++shot) {
        Sc64WeaponSourceVfxKey key = {
            .actor_raw=(uint8_t)raw,.actor_slot_plus1=(uint8_t)slot_plus1,
            .weapon_id=(uint8_t)weapon,
            .direction=(uint8_t)sc64_source_effect_direction(x1-x0,y1-y0),
            .target_air=(uint8_t)!!air,.variant=(uint8_t)(variant?variant:shot)
        };
        uint32_t packed;
        if(sc64_weapon_source_vfx_pack(key,&packed))
            add_tracer(x0,y0,x1,y1,(int)packed);
    }
    return 1;
}

static int
sc64_source_unit_vfx(Unit *q,int x1,int y1,int air)
{
    int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    Sc64StockAttack attack;
    int weapon;
    if(sc64_stock_attack_select(raw,air,&attack)) weapon=attack.weapon_id;
    else if(raw==83 && !air) weapon=82; /* Existing Reaver/Scarab adapter. */
    else return 0;
    return sc64_source_emit(raw,(int)(q-u)+1,weapon,air,q->x,q->y,x1,y1);
}

static int
sc64_source_building_vfx(const Building *q,int x1,int y1,int air)
{
    int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_building(q->type);
    Sc64StockAttack attack;
    return sc64_stock_attack_select(raw,air,&attack) &&
        sc64_source_emit(raw,0,attack.weapon_id,air,q->x,q->y,x1,y1);
}
#endif

/* Weapon damage and environmental burn share one destruction lifecycle. */
static void __attribute__((noinline))
destroy_building(int slot, int source_owner)
{
    Building *q = &b[slot];
    if (!q->alive) return;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    race_bc_yamato_target_removed((unsigned)(MAX_UNITS+slot));
#endif
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
    race_bc_building_destroyed(slot);
#endif
#ifdef SC64_SHARED_BUNKER
    if (q->type == B_BUNKER) race_bunker_destroy_cargo(slot, source_owner);
#endif

#ifdef SC64_TERRAN_LATE_RUNTIME
            if (q->alive && race_late_active())
                race_late_record_actor_death(source_owner, -1, q->owner,
                                             q->raw_owner, q->raw_type, 1);
#endif
#ifdef SC64_T6_RUNTIME
            if (q->alive && race_t6_active())
                race_t6_building_died(q->owner, q->raw_owner);
#endif
#ifdef SC64_P5_RUNTIME
            if (q->alive && race_p5_active()) race_p5_note_building_death(q);
#endif
#ifdef SC64_P6_RUNTIME
            if (q->alive && race_p6_active()) race_p6_note_building_death(q);
#endif
#ifdef SC64_Z6_RUNTIME
            if (q->alive && race_z6_active()) race_z6_note_building_death(q);
#endif
#ifdef SC64_Z7_RUNTIME
            if (q->alive && race_z7_active()) race_z7_note_building_death(q);
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
#ifdef SC64_BATTERY_SERVICE_ENABLED
            race_battery_clear_building_references(slot);
#endif
#ifdef SC64_NYDUS_ENABLED
            race_nydus_note_building_lost(slot);
#endif
	    invalidate_path_grid();
    (void)source_owner;
}

static void
hit_target_impl_ex(int target, int damage, int source_type, int source_owner,
                   int stock_damage_class, int hit_flags)
{
#ifdef SC64_CONTACT_MEMO_ENABLED
    SC64_CONTACT_MEMO_INVALIDATE();
#endif

    int tx, ty;
#ifndef SC64_RACES
    (void)stock_damage_class;
#endif
#ifndef SC64_SIEGE_SPLASH
    (void)hit_flags;
#endif
#ifdef SC64_TEST_HIT_OBSERVER
    SC64_TEST_HIT_OBSERVER(target, damage, source_type, source_owner);
#endif
#ifdef SC64_Z6_RUNTIME
    if (!(hit_flags & SC64_HIT_SKIP_HALLUCINATION_SCALE)) damage = race_z6_hallucination_incoming_damage(target, damage);
#endif
#ifdef SC64_P6_COMBAT_ENABLED
    if (target >= 0 && target < MAX_UNITS && race_p6_combat_unit_burrowed(target) &&
        source_type != U_P6_INFESTED_TERRAN && !(hit_flags & SC64_HIT_ALLOW_SPLASH_BURROW)) return;
#endif
#ifdef SC64_T6_RUNTIME
    if (target >= 0 && target < MAX_UNITS && race_t6_unit_burrowed(target) &&
        !(hit_flags & SC64_HIT_ALLOW_SPLASH_BURROW)) return;
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    if (target >= 0 && target < MAX_UNITS && SC64_TRANSPORT_LOADED(&u[target]))
        return;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    if (target >= 0 && target < MAX_UNITS &&
        (sc64_game_unit_has_flag(u[target].type, SC64_GAME_ACTOR_HIDDEN) ||
         SC64_CARRIER_STATE_HIDDEN(&u[target]) ||
         sc64_game_unit_has_flag(u[target].type, SC64_GAME_ACTOR_INVINCIBLE))) return;
    if (target >= MAX_UNITS && target < MAX_UNITS + MAX_BUILDINGS &&
        sc64_game_building_has_flag(b[target-MAX_UNITS].type,
                                    SC64_GAME_ACTOR_INVINCIBLE)) return;
#endif
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    if (target < MAX_UNITS ? invincible_units[target] : invincible_buildings[target - MAX_UNITS]) return;
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
    if (!(hit_flags & SC64_HIT_PRECOMPUTED_HP)) {
        int absorbed=*shield<damage?*shield:damage;*shield-=absorbed;damage-=absorbed;
    }
    if(!damage)return;
#endif
    if (target < MAX_UNITS) {
	tx = u[target].x;
	ty = u[target].y;
#ifdef SC64_RACES
        if (hit_flags & SC64_HIT_PRECOMPUTED_HP) { /* Q8 splash damage already resolved. */ }
        else if (stock_damage_class>=0) {
            int raw=u[target].raw_type>=0?u[target].raw_type:
                    sc64_race_raw_unit(u[target].type);
            const Sc64StockUnitProfile *stock=sc64_stock_unit_profile(raw);
            damage=sc64_stock_hp_damage_after_armor(damage,
                u[target].armor+(u[target].owner==SIDE?armor_level:0),
                stock_damage_class,stock?stock->size:SC64_STOCK_SIZE_LARGE);
        } else
#endif
        {
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
	}
	u[target].hp -= damage;
	if (u[target].hp <= 0) {
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
            race_bc_yamato_target_removed((unsigned)target);
            sc64_bc_shot_caster_gone(sc64_bc_shots,(unsigned)target);
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
            race_p7_carrier_training_clear_slot(target);
#endif
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
#ifdef SC64_P5_RUNTIME
            if (u[target].alive && race_p5_active())
                race_p5_note_unit_death(&u[target]);
#endif
#ifdef SC64_P6_RUNTIME
            if (u[target].alive && race_p6_active()) race_p6_note_unit_death(&u[target]);
#endif
#ifdef SC64_Z6_RUNTIME
            if (u[target].alive && race_z6_active()) {
                race_z6_note_unit_death(&u[target]);
            }
#endif
#ifdef SC64_TRANSPORT_RUNTIME
            /* Carrier recognition stays shared across campaign and skirmish;
             * inspect the live carrier before clearing its slot. */
            if (race_transport_carrier_type(&u[target])) race_transport_destroy_cargo(target);
#endif
	    if (u[target].owner == SIDE)
		supply_used -= unit_slot_supply(&u[target]);
	    u[target].alive = 0;
#ifdef SC64_Z6_RUNTIME
            if (race_z6_active()) (void)race_z6_hallucination_cleanup_dead(&u[target]);
#endif
	}
        /* Flame collateral needs source geometry and event research. Do not
         * invent area hits here: this path cannot resolve their eligibility,
         * shields, armor and death callbacks correctly. */
    } else {
	Building *q = &b[target - MAX_UNITS];
	tx = q->x;
	ty = q->y;
	#ifdef SC64_RACES
    if (hit_flags & SC64_HIT_PRECOMPUTED_HP) { /* Q8 splash damage already resolved. */ }
    else if (stock_damage_class>=0) {
        int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_building(q->type);
        const Sc64StockUnitProfile *stock=sc64_stock_unit_profile(raw);
        damage=sc64_stock_hp_damage_after_armor(damage,
            sc64_race_building_armor(q->type),stock_damage_class,
            stock?stock->size:SC64_STOCK_SIZE_LARGE);
    } else {
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
    }
    if (race_building_can_burn(q) && source_owner>=0 && source_owner<5)
        q->shield_remainder=(int)sc64_building_burn_with_owner(
            (unsigned)q->shield_remainder,(unsigned)source_owner);
    q->hp-=(hit_flags & SC64_HIT_PRECOMPUTED_HP)?damage:(damage>0?damage:1);
#else
    q->hp -= damage > 1 ? damage - 1 : 1;
#endif
	if (q->hp <= 0) destroy_building(target - MAX_UNITS, source_owner);
    }
    (void)tx;
    (void)ty;
    (void)source_type;
    (void)source_owner;
}

/* Special adapters pass already-resolved packets through the legacy path;
 * ordinary stock weapons supply an explicit damage class below. */
static void hit_target_impl(int target,int damage,int source_type,int source_owner,
                            int stock_damage_class)
{ hit_target_impl_ex(target,damage,source_type,source_owner,stock_damage_class,0); }
static void hit_target(int target,int damage,int source_type,int source_owner)
{ hit_target_impl(target,damage,source_type,source_owner,-1); }

#ifdef SC64_RACES
/* Returns zero only when a full impact queue defers this shot; the caller
 * then emits no effect and keeps its weapon ready. */
static int __attribute__((noinline))
hit_stock_building_target(const Building *source,int target,int damage)
{
    Sc64StockAttack attack;
    int raw=source->raw_type>=0?source->raw_type:
            sc64_race_raw_building(source->type);
    int flying=target<MAX_UNITS&&sc64_race_unit_flying(u[target].type);
    int stock=sc64_stock_attack_select(raw,flying,&attack);
#ifdef SC64_IMPACT_EVENTS
    unsigned steps=race_impact_building_steps(source,target,flying);
    if (steps) {
        if (race_impact_full()) return 0;
        race_impact_launch(target,damage,1,source->owner,
                           stock?attack.weapon->damage_class:-1,U_MARINE,steps);
        return 1;
    }
#endif
    if (stock)
        hit_target_impl(target,damage,U_MARINE,source->owner,attack.weapon->damage_class);
    else hit_target(target,damage,U_MARINE,source->owner);
    return 1;
}
#endif

static int
save_game(void)
{
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) return 0;
#endif
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
    for (int i = 0; i < MAX_RACE_GEYSERS; i++) { data.geyser_x[i] = geysers[i].x; data.geyser_y[i] = geysers[i].y; }
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
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) return 0;
#endif
#ifdef SC64_RACES
    int loaded = race_load_game();
    if (loaded) sc64_walk_reset_all();
#ifdef SC64_UNIT_FACING
    if (loaded) sc64_facing_reset_all();
#endif
#ifdef SC64_FIELD_CONTROLS
    if (loaded) { ping_life=0; ui_action = ui_tick_mode = 0; } /* frame jumped: resolve A afresh */
#endif
    return loaded;
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
    if (data.geyser_count < 0 || data.geyser_count > MAX_RACE_GEYSERS) return 0;
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
    /* Older non-race saves recorded the current upgrade in newly spawned
     * units. Armor is derived from type and the live upgrade is applied on
     * hit, so normalize the validated living units without changing the save
     * format or their other state. */
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive)
            u[i].armor = u[i].type == U_FIREBAT || u[i].type == U_MEDIC;
    memcpy(b, data.buildings, sizeof(b));
    invalidate_path_grid();
    memcpy(m, data.minerals_map, sizeof(m));
    memcpy(tracers, data.tracer_data, sizeof(tracers));
    sc64_trace_duration_cache_reset();
    death_vfx_reset();
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
    for (int k = 0; k < MAX_RACE_GEYSERS; k++) { geysers[k].x = data.geyser_x[k]; geysers[k].y = data.geyser_y[k]; }
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
#ifdef SC_FIELD_FEEDBACK
    ping_life=0;
#endif
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
            if (geyser_count < MAX_RACE_GEYSERS) { geysers[geyser_count].x = x; geysers[geyser_count++].y = y; }
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
    sc64_walk_reset_all();
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    sc64_battlecruiser_tech_bits = 0;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));
    sc64_bc_pause_offset=0; sc64_bc_spell_rng=42u;
#endif
#endif
#ifdef SC64_RACES
#ifdef SC64_UNIT_FACING
    sc64_facing_reset_all();
#endif
    reset_selection_gestures();
    mode=MODE_NORMAL;menu=menu_open=menu_cursor=help=0;
    screen=SCREEN_PLAY;selected_building=-1;result_timer=0;
    toast_timer=toast_id=enemy_timer=wave=0;
    weapons_level=armor_level=weapons_research=armor_research=0;
    mission_objective_stage=mission_unsupported_count=0;
    train_type=race_mode?(race_human==R_TER?U_SCV:race_human==R_ZERG?U_DRONE:U_PROBE):
        (race_selected_mission<2?U_DRONE:U_PROBE);
    SC64_GRID_CLEAR(explored);SC64_GRID_CLEAR(visible);
    clear_tracers();
    death_vfx_reset();
    sc64_visibility_reset();
    race_campaign_init();
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
#ifdef SC64_FIELD_CONTROLS
    /* A restart jumps frame back, so the cached A action must resolve afresh. */
    menu_last=ping_life=r_tap_state=0;ui_action=ui_tick_mode=0;
    pause_cursor=pause_notice=0;
#endif
    return;
#endif
    int i;
    reset_selection_gestures();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    invalidate_path_grid();
    clear_tracers();
    death_vfx_reset();
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
#ifdef SC_DEFAULT_UI
    menu_last = 0; ping_life = 0; pause_cursor = 0; pause_notice = pause_notice_ticks = 0;
#elif defined(SC64_FIELD_CONTROLS)
    menu_last = 0; ping_life = 0; ui_action = 0; ui_tick_mode = 0;
#endif
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
    /* This original-art outpost starts with its own authored mineral jobs.
     * Later idle workers require an explicit order. */
    static const int opening_workers[5][3] = {
        {98, 82, 3}, {99, 91, 3}, {96, 99, 3}, {85, 103, 3}, {68, 105, 1}
    };
    for (i = 0; i < 5; ++i) {
        int worker = spawn_unit(SIDE, U_SCV, opening_workers[i][0], opening_workers[i][1]);
        if (worker >= 0) {
            u[worker].order = ORDER_MINE;
            u[worker].target = opening_workers[i][2];
        }
    }
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
#ifdef SC64_SHOWCASE_ENABLED
    /* Staged armies use regular orders/combat without AI rebuilding them. */
    if (sc64_showcase_scene >= 0) return;
#endif
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
    sc64_walk_reset_all();
#ifdef SC64_UNIT_FACING
    sc64_facing_reset_all();
#endif
#ifdef SC64_SHOWCASE_ENABLED
    sc64_showcase_scene=-1;sc64_showcase_ticks=0;
#endif
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
    death_vfx_reset();
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            ||!sc64_game_unit_selectable(u[i].type)
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            ||!sc64_game_building_selectable(b[i].type)
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            sc64_game_unit_selectable(u[i].type) &&
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

static int
issue_order(int order, int tx, int ty)
{
    int i, n = 0;
    int group_order = selected_count() > 1;
#ifdef SC64_REACHABLE_FORMATION
    int route_hint_slots[4] = {-1, -1, -1, -1};
    int route_hint_roots[4] = {-1, -1, -1, -1};
    int route_hint_next = 0;
#endif
    selected_building = -1;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE && u[i].selected) {
#ifdef SC64_Z7_EGG_PRODUCTION
            /* The Egg's target field stores its paid product until birth. It
             * has no movement command; the selected-unit menu owns cancel. */
            if (u[i].type == U_Z7_EGG) continue;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            if (sc64_game_unit_actor_flags(u[i].type) &&
                !(sc64_game_unit_actor_flags(u[i].type)&SC64_GAME_ACTOR_MOVABLE)) continue;
#endif
#ifdef SC64_T6_RUNTIME
            if (u[i].order == ORDER_REPAIR) {
                u[i].carry = 0; u[i].work_timer = 0;
            }
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
            if (sc64_race_raw_unit(u[i].type)==12) u[i].work_timer&=~7;
#endif
	    u[i].order = order;
	#ifdef SC64_REACHABLE_FORMATION
	    if (group_order) {
	        int slot_x = limit(tx + ((n % 5) - 2) * 8, 8, WORLD_W - 8);
	        int slot_y = limit(ty + ((n / 5) - 1) * 8, 8, WORLD_H - 8);
	        if (!sc64_race_unit_flying(u[i].type)) {
	            int use_cursor = formation_route_pair(u[i].x, u[i].y,
                slot_x, slot_y, tx, ty, route_hint_slots, route_hint_roots,
                &route_hint_next);
	            if (use_cursor) {
	                slot_x = tx;
	                slot_y = ty;
	            }
	        }
	        u[i].tx = slot_x;
	        u[i].ty = slot_y;
	    } else {
	        u[i].tx = tx;
	        u[i].ty = ty;
	    }
	#else
	    if (group_order) {
		u[i].tx = limit(tx + ((n % 5) - 2) * 8, 8, WORLD_W - 8);
		u[i].ty = limit(ty + ((n / 5) - 1) * 8, 8, WORLD_H - 8);
	    } else {
		u[i].tx = tx;
		u[i].ty = ty;
	    }
	#endif
	    u[i].path_timer = 0;
	    u[i].path_retry = 0;
	    u[i].target = -1;
	    n++;
	}
    if (n)
	toast(0);
    else
	toast(5);
    return n;
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

/* Contextual mineral target shared by dispatch and HUD. */
static int
selected_worker_mineral_target(void)
{
    int patch = -1, best = 16 * 16;
    if (find_selected_worker() < 0 || selection_target() >= 0) return -1;
    for (int i = 0; i < MAX_MINERALS; ++i) {
        int d = dist2(cx, cy, m[i].x, m[i].y);
        if (d < best && currently_visible(m[i].x, m[i].y) && m[i].amount > 0) {
            patch = i; best = d;
        }
    }
    return patch;
}

/* Automatic construction selection must not steal an existing build order.
 * Explicit player orders may still interrupt construction normally. */
static int
build_worker_available(int worker, int owner, int type)
{
    return worker >= 0 && worker < MAX_UNITS && u[worker].alive &&
        u[worker].owner == owner &&
        (type < 0 ? IS_WORKER(u[worker].type) : u[worker].type == type) &&
        !SC64_TRANSPORT_LOADED(&u[worker]) && !SC64_BUILD_WORKER_ORDER(u[worker].order);
}

#ifdef SC64_RACES
static int
race_nexus_command_available(void)
{
    int found=0;
    if (race_human != R_PROTOSS || selected_building >= 0) return 0;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].owner==SIDE && u[i].selected &&
            !SC64_TRANSPORT_LOADED(&u[i])) {
            if (u[i].type!=U_PROBE) return 0;
            found=1;
        }
    return found;
}

/* Keep the extra defense command in the same Probe-only context as the
 * Nexus action, and reject mixed selections even if another owner is
 * selected through a debug or imported-control path. */
static int
race_probe_cannon_command_available(void)
{
    int found=0;
    if (race_human != R_PROTOSS || selected_building >= 0) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner != SIDE || u[i].type != U_PROBE ||
            SC64_TRANSPORT_LOADED(&u[i])) return 0;
        found=1;
    }
    return found;
}
#endif

#ifdef SC_DEFAULT_UI
#include "field_ui.h"
#endif
#ifdef SC64_FIELD_CONTROLS
#include "field_controls.h"
#endif

static void
start_build(int type)
{
#ifdef SC64_RACES
    {
        int site_x = cx, site_y = cy, worker = find_selected_worker();
#ifdef SC64_FIELD_CONTROLS
        /* Snap to the grid (gas to its geyser); without a selected worker
         * the nearest free one builds, so the army can stay selected. */
        field_ui_site(type, &site_x, &site_y);
        worker = field_ui_build_worker(type);
#endif
        if(race_start_build(SIDE,worker,type,site_x,site_y)){
            mode=MODE_NORMAL;toast(3);
#ifdef SC64_FIELD_CONTROLS
            field_ui_ping(site_x,site_y,C_GOLD);
#endif
        }else{
#ifdef SC64_FIELD_CONTROLS
            /* The first problem the placement HUD names, funds before site. */
            const char *need;
            int ground, problem = ui_site_problem(type, site_x, site_y, &need, &ground);
            if (problem == UI_SITE_MINERALS || problem == UI_SITE_GAS)
                toast_refusal(problem == UI_SITE_MINERALS ? 1 : 12);
            else
#endif
            toast(8);
        }
        return;
    }
#endif
    int cost = building_cost(type), i, slot, px = cx, py = cy;
#ifdef SC_DEFAULT_UI
    field_ui_site(type, &px, &py);
#endif
    if (!has_tech_for(type)) {
	toast(type == B_ACADEMY ? 10 : 8);
	return;
    }
    if (minerals < cost) {
	toast_refusal(1);
	return;
    }
    if (gas < building_gas_cost(type)) {
	toast_refusal(12);
	return;
    }
    if (!can_place(type, px, py)) {
	toast(6);
	return;
    }
#ifdef SC_DEFAULT_UI
    i = field_ui_build_worker();
#else
    i = find_selected_worker();
#endif
    if (!build_worker_available(i, SIDE, -1)) {
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
    slot = spawn_build(SIDE, type, px, py, 1);
    if (slot < 0) {
	minerals += cost;
	return;
    }
    u[i].order = ORDER_BUILD;
    u[i].target = slot;
    u[i].tx = px;
    u[i].ty = py;
    u[i].path_timer = 0;
    u[i].path_retry = 0;
    toast(3);
    mode = MODE_NORMAL;
#ifdef SC_DEFAULT_UI
    field_ui_ping(px, py, C_GOLD);
#endif
}

static void
stop_selected(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].selected) {
#ifdef SC64_Z7_EGG_PRODUCTION
            /* STOP is not Egg cancellation: retain its paid product/timer so
             * the contextual CANCEL EGG action remains the only cancel path. */
            if (u[i].type == U_Z7_EGG) continue;
#endif
	    u[i].order = ORDER_IDLE;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
            if (sc64_race_raw_unit(u[i].type)==12) u[i].work_timer&=~7;
#endif
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
            if (sc64_race_raw_unit(u[i].type) != 12)
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
            if (!race_z5_cloak_actor(&u[i]))
#endif
#ifdef SC64_P6_STORM_ENABLED
            if (!(race_p6_active() && u[i].raw_type == 79 && u[i].raw_owner == 2))
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
            if (!race_z6_cloak_actor(&u[i]))
#endif
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
	toast_refusal(gas < 100 ? 12 : 1);
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

#if defined(SC64_P6_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
static int race_selected_storm_hero(void)
{
    int found=0;
    if (selected_building>=0) return 0;
    for (int i=0; i<MAX_UNITS; ++i) if (u[i].alive && u[i].owner==SIDE && u[i].selected) {
        int eligible=0;
#ifdef SC64_P6_STORM_ENABLED
        eligible |= race_p6_active() && u[i].raw_type==79 && u[i].raw_owner==2;
#endif
#ifdef SC64_Z6_STORM_ENABLED
        eligible |= race_z6_active() && u[i].raw_type==51 && u[i].raw_owner==3;
#endif
        if (!eligible || ++found>1) return 0;
    }
    return found==1;
}
#endif

#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
static int race_z7_colony_command_available(void)
{
    if (!race_z7_active() || race_human!=R_ZERG || selected_building<0 ||
        selected_building>=MAX_BUILDINGS) return 0;
    for (unsigned i=0; i<MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected) return 0;
    const Building *q=&b[selected_building];
    return q->alive && q->owner==SIDE && q->type==B_CREEP_COLONY &&
        q->raw_type==sc64_race_raw_building(B_CREEP_COLONY) && q->raw_owner==3;
}

static int race_z7_spore_morph_command_available(void)
{
    return race_z7_colony_command_available() &&
        race_z7_colony_morph_available_for(selected_building,B_SPORE_COLONY);
}

static int race_z7_creep_command_available(void)
{
    int found=0;
    if (!race_z7_active() || race_human!=R_ZERG || selected_building>=0) return 0;
    for (unsigned i=0; i<MAX_UNITS; ++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner!=SIDE || u[i].type!=U_DRONE) return 0;
        found=1;
    }
    return found;
}
#endif

#ifdef SC64_SHARED_BUNKER
static int __attribute__((noinline)) race_bunker_menu_available(void)
{
    return selected_building>=0 && selected_building<MAX_BUILDINGS &&
        b[selected_building].owner==SIDE && race_bunker_cargo_space(selected_building)>0;
}
#endif

#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
static int race_bc_lab_menu_available(void)
{
    int i=selected_building;
    return i>=0 && i<MAX_BUILDINGS && b[i].alive && b[i].owner==SIDE &&
        building_complete(i) && sc64_race_raw_building(b[i].type)==118;
}
static int race_bc_menu_base(void)
{ return race_mode ? (int)sc64_skirmish_menu_count((unsigned)race_human) : MENU_STOP+1; }
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
static int race_bc_facility_menu_available(void)
{
    int i=selected_building;
    return i>=0 && i<MAX_BUILDINGS && b[i].alive && b[i].owner==SIDE &&
        !b[i].under_construction && sc64_race_raw_building(b[i].type)==116;
}
#endif
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
static int race_bc_yamato_target_valid(int target,const Unit *caster,int require_visible);
static int race_bc_yamato_issue(int slot,int target);
static int race_bc_yamato_menu_base(void)
{ return race_mode?(int)sc64_skirmish_menu_count((unsigned)race_human):MENU_STOP+1; }
static int race_bc_yamato_command_available(void)
{
    if (selected_building>=0) return 0;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE && u[i].selected &&
        sc64_race_raw_unit(u[i].type)==12 && !SC64_TRANSPORT_LOADED(&u[i])) {
        int key=sc64_battlecruiser_owner_key(race_mode,race_selected_mission,u[i].owner,u[i].raw_owner);
        if(key>=0 && sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,(unsigned)key,SC64_BC_TECH_YAMATO)) return 1;
    }
    return 0;
}
static int race_bc_yamato_target_at(int x,int y)
{
    int best=-1,best_distance=0x7fffffff;
    for(int target=0;target<MAX_UNITS+MAX_BUILDINGS;++target) {
        if(!target_alive(target)) continue;
        int d=dist2(x,y,target_x(target),target_y(target)),radius=target<MAX_UNITS?12:24,allowed=0;
        if(d>=radius*radius || d>=best_distance) continue;
        for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE && u[i].selected &&
            sc64_race_raw_unit(u[i].type)==12 && race_bc_yamato_target_valid(target,&u[i],1)) {allowed=1;break;}
        if(allowed){best=target;best_distance=d;}
    }
    return best;
}
#endif
static void
choose_command(int choice)
{
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    if (race_bc_yamato_command_available() && choice==race_bc_yamato_menu_base()) {
        menu_open=0;mode=MODE_YAMATO;return;
    }
#endif
#ifdef SC_FIELD_FEEDBACK
    menu_last = (u8)choice;
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_menu_available() && choice==race_bc_menu_base()) {
        menu_open=0; mode=MODE_NORMAL;
        if (race_bc_facility_airborne(selected_building)) mode=MODE_LAND;
        else {
#ifdef SC64_TERRAN_LATE_RUNTIME
            if (race_late_active()) toast(race_late_lift(selected_building)?0:8);
            else
#endif
            toast(race_bc_facility_lift(selected_building)?0:8);
        }
        return;
    }
#endif
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
    if (race_bc_lab_menu_available() && choice>=race_bc_menu_base() &&
        choice<race_bc_menu_base()+(b[selected_building].level ? 1 : 2)) {
        int result = b[selected_building].level ? race_bc_lab_cancel(selected_building,0) :
            race_bc_lab_start(selected_building,choice==race_bc_menu_base() ?
                              SC64_BC_TECH_YAMATO : SC64_BC_TECH_REACTOR);
        menu_open=0; mode=MODE_NORMAL; toast(result?0:8); return;
    }
#endif
#ifdef SC64_SHARED_BUNKER
    if (choice==(race_mode?(int)sc64_skirmish_menu_count((unsigned)race_human):MENU_STOP+1) &&
        race_bunker_menu_available()) {
        menu_open=0;mode=MODE_NORMAL;
        toast(race_bunker_request_unload(selected_building)?0:8);return;
    }
#endif
#ifdef SC64_RACES
    if (race_mode) {
        Sc64SkirmishMenuRow row;
#ifdef SC64_TRANSPORT_RUNTIME
        if (choice == (int)sc64_skirmish_menu_count((unsigned)race_human) &&
            race_skirmish_dropship_menu_available()) {
            menu_open = 0; mode = MODE_UNLOAD; return;
        }
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
        if (choice == (int)sc64_skirmish_menu_count((unsigned)race_human) &&
            race_battery_command_available()) {
            menu_open=0; mode=MODE_RECHARGE; return;
        }
#endif
        if (!sc64_skirmish_menu_at((unsigned)race_human, (unsigned)choice, &row)) {
            menu_open = 0; mode = MODE_NORMAL; toast(8); return;
        }
        if (row.record) {
            menu_open = 0;
            if (!race_skirmish_choose_record(row.record)) { mode = MODE_NORMAL; toast(8); }
            return;
        }
        /* Fixed first-ten control rows deliberately retain their old IDs. */
    }
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (choice==MENU_STOP+1 && race_p7_carrier_training_menu_available()) {
        menu_open=0; mode=MODE_NORMAL;
        toast(race_p7_carrier_training_queue_selected()?4:8); return;
    }
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    if (choice == MENU_STOP + 1 && race_z7_egg_cancel_available()) {
        menu_open = 0; mode = MODE_NORMAL;
        toast(race_z7_egg_cancel_selected() ? 0 : 8); return;
    }
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    if (choice==MENU_STOP+1 && race_z7_colony_command_available()) {
        menu_open=0; mode=MODE_NORMAL;
        toast(race_z7_colony_start_sunken(selected_building)?3:8); return;
    }
    if (choice==MENU_STOP+2 && race_z7_spore_morph_command_available()) {
        menu_open=0; mode=MODE_NORMAL;
        toast(race_z7_colony_start_spore(selected_building)?3:8); return;
    }
    if (choice==MENU_Z7_CREEP) {
        menu_open=0;
        if (!race_z7_creep_command_available()) { mode=MODE_NORMAL; toast(8); return; }
        mode=MODE_BUILD; menu=B_CREEP_COLONY; return;
    }
#endif
#ifdef SC64_RACES
    /* Contextual eleventh row: spell and transport rows belong to their
       selected actors, while a selected Probe exposes base expansion. */
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (choice==MENU_STOP+1 && race_battery_command_available()) {
        menu_open=0; mode=MODE_RECHARGE; return;
    }
    if (choice==MENU_STOP+3 && race_battery_build_available()) {
        menu_open=0; mode=MODE_BUILD; menu=sc64_race_building_from_raw(172); return;
    }
#endif
    if (choice == MENU_STOP + 1 && race_nexus_command_available()) {
        menu_open = 0; mode = MODE_BUILD; menu = B_NEXUS; return;
    }
    if (choice == MENU_STOP + 2 && race_probe_cannon_command_available()) {
        menu_open = 0; mode = MODE_BUILD; menu = B_PHOTON_CANNON; return;
    }
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (choice == MENU_STOP + 1 && race_z6_transport_research_selected()) {
        int result = race_z6_transport_start_research_selected();
        menu_open=0; mode=MODE_NORMAL;
        toast(result == SC64_Z6_TRANSPORT_TECH_RESEARCHING ? 4 : 8); return;
    }
    if (choice == MENU_STOP + 1 && race_z6_overlord_menu_available()) {
        menu_open=0; mode=MODE_UNLOAD; return;
    }
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
    if (choice == MENU_STOP + 2 && race_z5_cloak_menu_available()) {
        menu_open = 0; mode = MODE_Z5_CONSUME; return;
    }
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (choice == MENU_STOP + 1 && race_p5_shuttle_menu_available()) {
        menu_open = 0;
        mode = MODE_UNLOAD;
        return;
    }
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
    if (choice == MENU_STOP + 1 && race_z6_cloak_menu_available()) {
        int result=race_z6_cloak_toggle_selected();
        menu_open=0; mode=MODE_NORMAL;
        toast(result==SC64_Z5_CLOAK_ADVANCED ? 0 : 8); return;
    }
#endif
#ifdef SC64_Z6_STORM_ENABLED
    if (choice == MENU_STOP + 2 && race_z6_cloak_menu_available()) {
        menu_open=0; mode=MODE_P4_STORM; toast(0); return;
    }
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
    if (choice == MENU_STOP + 1 && race_z5_cloak_menu_available()) {
        int result = race_z5_cloak_toggle_selected();
        menu_open = 0;
        mode = MODE_NORMAL;
        toast(result == SC64_Z5_CLOAK_ADVANCED ? 0 : 8);
        return;
    }
#endif
#ifdef SC64_P6_STORM_ENABLED
    if (choice == MENU_STOP + 1 && race_p6_storm_menu_available()) {
        menu_open = 0; mode = MODE_P4_STORM; toast(0); return;
    }
#endif
#ifdef SC64_P4_STORM_ENABLED
    if (choice == MENU_STOP + 1 && race_p4_storm_menu_available()) {
        menu_open = 0;
        mode = MODE_P4_STORM;
        toast(0);
        return;
    }
#endif
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
    #ifdef SC64_ZP_CAMPAIGN_STORAGE
    if (choice == MENU_ZP_LAIR || choice == MENU_ZP_SPIRE) {
        menu_open = 0;
        if (race_human != R_ZERG) { toast(8); return; }
        if (choice == MENU_ZP_LAIR) {
            toast(race_start_lair_morph(selected_building) ? 3 : 8);
            return;
        }
        mode = MODE_BUILD; menu = B_SPIRE; return;
    }
    #endif
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
#ifndef SC_FIELD_FEEDBACK
	toast(0); /* the default HUD names the armed order instead */
#endif
	break;
    case MENU_ATTACK:
	mode = MODE_ATTACK;
#ifndef SC_FIELD_FEEDBACK
	toast(0);
#endif
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

#ifdef SC64_RACES
static int race_skirmish_command_count(void)
{
    int count = (int)sc64_skirmish_menu_count((unsigned)race_human);
#ifdef SC64_TRANSPORT_RUNTIME
    count += race_skirmish_dropship_menu_available();
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
    count += race_battery_command_available();
#endif
#ifdef SC64_SHARED_BUNKER
    count += race_bunker_menu_available();
#endif
    return count;
}
#endif

/* Advanced Terran commands are available in the T5 development profile.
 * Other factions retain their existing ten-row command list. */
#ifdef SC64_ZP_CAMPAIGN_STORAGE
static int field_menu_count(void)
{
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    if (race_bc_yamato_command_available()) return race_bc_yamato_menu_base()+1;
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_menu_available()) return race_bc_menu_base()+1;
#endif
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
    if (race_bc_lab_menu_available()) return race_bc_menu_base() + (b[selected_building].level ? 1 : 2);
#endif
    if (race_mode) return race_skirmish_command_count();
#ifdef SC64_SHARED_BUNKER
    if (race_bunker_menu_available()) return MENU_STOP+2;
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (race_p7_carrier_training_menu_available()) return MENU_STOP+2;
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (race_battery_command_available()) return MENU_STOP+2;
    if (race_battery_build_available()) return MENU_STOP+4;
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    if (race_z7_egg_cancel_available()) return MENU_STOP + 2;
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    if (race_z7_colony_command_available())
        return MENU_STOP+2+race_z7_spore_morph_command_available();
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (race_z6_transport_research_selected() || race_z6_overlord_menu_available())
        return MENU_STOP + 2;
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (race_p5_shuttle_menu_available()) return MENU_STOP + 2;
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
    if (race_z6_cloak_menu_available()) return MENU_STOP + 3;
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
    if (race_z5_cloak_menu_available()) return MENU_STOP + 2
#ifdef SC64_Z5_CONSUME_ENABLED
        + 1
#endif
        ;
#endif
#ifdef SC64_P6_STORM_ENABLED
    if (race_p6_storm_menu_available()) return MENU_STOP + 2;
#endif
#ifdef SC64_P4_STORM_ENABLED
    if (race_p4_storm_menu_available()) return MENU_STOP + 2;
#endif
    if (race_probe_cannon_command_available()) return MENU_STOP + 3;
    if (race_nexus_command_available()) return MENU_STOP + 2;
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    if (race_human==R_ZERG) {
        return race_z7_creep_command_available()?MENU_COUNT:MENU_Z7_CREEP;
    }
    return MENU_STOP+1;
#else
    return race_human == R_ZERG ? MENU_COUNT : MENU_STOP + 1;
#endif
}
#elif defined(SC64_T5_RUNTIME)
static int field_menu_count(void)
{
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    if (race_bc_yamato_command_available()) return race_bc_yamato_menu_base()+1;
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_menu_available()) return race_bc_menu_base()+1;
#endif
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
    if (race_bc_lab_menu_available()) return race_bc_menu_base() + (b[selected_building].level ? 1 : 2);
#endif
    if (race_mode) return race_skirmish_command_count();
#ifdef SC64_SHARED_BUNKER
    if (race_bunker_menu_available()) return MENU_STOP+2;
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (race_battery_command_available()) return MENU_STOP+2;
#endif
    if (race_probe_cannon_command_available()) return MENU_STOP + 3;
    if (race_nexus_command_available()) return MENU_STOP + 2;
    return race_human == R_TER ? MENU_COUNT : MENU_STOP + 1;
}
#elif defined(SC64_RACES)
static int field_menu_count(void)
{
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    if (race_bc_yamato_command_available()) return race_bc_yamato_menu_base()+1;
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_menu_available()) return race_bc_menu_base()+1;
#endif
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
    if (race_bc_lab_menu_available()) return race_bc_menu_base() + (b[selected_building].level ? 1 : 2);
#endif
    if (race_mode) return race_skirmish_command_count();
#ifdef SC64_SHARED_BUNKER
    if (race_bunker_menu_available()) return MENU_STOP+2;
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (race_battery_command_available()) return MENU_STOP+2;
#endif
    return MENU_STOP + 1 + race_nexus_command_available() + race_probe_cannon_command_available();
}
#else
#define field_menu_count() MENU_COUNT
#endif

#ifdef SC64_RACES
static const char *field_menu_base_label(int choice, const char *const *labels)
{
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if (choice==MENU_STOP+1 && race_p7_carrier_training_menu_available()) return "BUILD INTERCEPTOR";
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    if (choice == MENU_STOP + 1 && race_z7_egg_cancel_available()) return "CANCEL EGG";
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    if (choice==MENU_STOP+1 && race_z7_colony_command_available()) return "SUNKEN";
    if (choice==MENU_STOP+2 && race_z7_spore_morph_command_available()) return "SPORE";
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (choice==MENU_STOP+1 && race_battery_command_available()) return "RECHARGE";
    if (choice==MENU_STOP+3 && race_battery_build_available()) return "SHIELD BATTERY";
#endif
    if (choice == MENU_STOP + 2 && race_probe_cannon_command_available())
        return "CANNON";
    return choice == MENU_STOP + 1 && race_nexus_command_available() ?
        "NEXUS" : labels[choice];
}

/* Row i of the command list, with every profile's contextual rows. */
static const char *race_menu_label(int i)
{
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    if (race_bc_yamato_command_available() && i==race_bc_yamato_menu_base()) return "YAMATO GUN";
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_menu_available() && i==race_bc_menu_base())
        return race_bc_facility_airborne(selected_building) ? "LAND" : "LIFT OFF";
#endif
#ifdef SC64_BATTLECRUISER_RESEARCH_EXPERIMENT
    if (race_bc_lab_menu_available() && i>=race_bc_menu_base()) {
        if (b[selected_building].level) return i==race_bc_menu_base() ? "CANCEL RESEARCH" : "";
        if (i==race_bc_menu_base()) return "YAMATO GUN";
        if (i==race_bc_menu_base()+1) return "COLOSSUS REACTOR";
    }
#endif
#ifdef SC64_SHARED_BUNKER
    if (i==(race_mode?(int)sc64_skirmish_menu_count((unsigned)race_human):MENU_STOP+1) &&
        race_bunker_menu_available()) return "UNLOAD CARGO";
#endif
    if (race_mode) {
        Sc64SkirmishMenuRow row;
#ifdef SC64_BATTERY_SERVICE_ENABLED
        if (i == (int)sc64_skirmish_menu_count((unsigned)race_human) &&
            race_battery_command_available()) return "RECHARGE";
#endif
#ifdef SC64_TRANSPORT_RUNTIME
        if (i == (int)sc64_skirmish_menu_count((unsigned)race_human) &&
            race_skirmish_dropship_menu_available()) return "UNLOAD CARGO";
#endif
        return sc64_skirmish_menu_at((unsigned)race_human, (unsigned)i, &row) ? row.label : "";
    }
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (i==MENU_STOP+1 && race_battery_command_available()) return "RECHARGE";
#endif
    static const char *const race_labels[3][
#ifdef SC64_ZP_CAMPAIGN_STORAGE
        MENU_COUNT
#else
        MENU_STOP + 1
#endif
    ]={
        {"MOVE ORDER","ATTACK MOVE","BARRACKS","SUPPLY DEPOT","REFINERY","ACADEMY","ENGINEERING BAY","WEAPONS","ARMOR","STOP / HOLD"},
        {"MOVE ORDER","ATTACK MOVE","SPAWNING POOL","HATCHERY","EXTRACTOR","HYDRALISK DEN","EVOLUTION","WEAPONS","ARMOR","STOP / HOLD"
#ifdef SC64_ZP_CAMPAIGN_STORAGE
         ,"UPGRADE TO LAIR","SPIRE"
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
         ,"CREEP COLONY"
#endif
        },
        {"MOVE ORDER","ATTACK MOVE","GATEWAY","PYLON","ASSIMILATOR","CYBERNETICS","FORGE","WEAPONS","ARMOR","STOP / HOLD"}};
    const char *const *labels=race_labels[race_human];
#ifdef SC64_T5_RUNTIME
    static const char *const advanced_labels[] = {
        "FACTORY", "STARPORT", "CONTROL TOWER", "UNLOAD CARGO"
#ifdef SC64_TERRAN_LATE_RUNTIME
        , "LIFT / LAND", "COMMAND CENTER"
#endif
    };
    return race_nexus_command_available() && i == MENU_STOP + 1 ?
        "NEXUS" : race_probe_cannon_command_available() && i == MENU_STOP + 2 ?
        "CANNON" : i >= MENU_FACTORY ?
        advanced_labels[i - MENU_FACTORY] : labels[i];
#else
    const char *label = field_menu_base_label(i, labels);
#ifdef SC64_ZP_CAMPAIGN_STORAGE
#ifdef SC64_P4_STORM_ENABLED
    if (i == MENU_STOP + 1 && race_p4_storm_menu_available()) label = "STORM";
#ifdef SC64_P6_STORM_ENABLED
    if (i == MENU_STOP + 1 && race_p6_storm_menu_available()) label = "STORM";
#endif
#ifdef SC64_P5_SHUTTLE_ENABLED
    if (i == MENU_STOP + 1 && race_p5_shuttle_menu_available()) label = "UNLOAD CARGO";
#endif
#else
#ifdef SC64_Z5_CLOAK_ENABLED
    label = i == MENU_STOP + 1 && race_z5_cloak_menu_available() ?
        (race_z5.cloak.active ? "UNCLOAK" : "CLOAK") :
#ifdef SC64_Z5_CONSUME_ENABLED
        i == MENU_STOP + 2 && race_z5_cloak_menu_available() ? "CONSUME" :
#endif
        label;
#endif
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (i==MENU_STOP+1 && race_z6_transport_research_selected()) label="VENTRAL SACS";
    if (i==MENU_STOP+1 && race_z6_overlord_menu_available()) label="UNLOAD CARGO";
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
    if (race_z6_cloak_menu_available()) {
        if (i==MENU_STOP+1) label=race_z6.cloak.active ? "UNCLOAK" : "CLOAK";
        if (i==MENU_STOP+2) label="STORM";
    }
#endif
#endif
#endif
    return label;
#endif
}
#endif

static void
queue_unit(int building, int type)
{
#ifdef SC64_RACES
    race_refusal = 0;
    if (race_queue_unit(SIDE,building,type)) toast(4);
    else if (race_refusal) toast_refusal(race_refusal);
    else toast(8);
    return;
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
	toast_refusal(1);
	return;
    }
    if (gas < gas_cost) {
	toast_refusal(12);
	return;
    }
    if (reserved_supply() + unit_supply(type) > supply_cap) {
	toast_refusal(2);
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
#if defined(SC64_TERRAN_LATE_RUNTIME) || defined(SC64_BATTLECRUISER_FLIGHT_EXPERIMENT)
    if (mode == MODE_LAND) {
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (race_late_active()) toast(race_late_flight_order(selected_building,cx,cy,1)?0:8);
        else
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        toast(race_bc_facility_land(selected_building,cx,cy)?0:8);
#else
        toast(8);
#endif
        mode = MODE_NORMAL; return;
    }
#endif
#if defined(SC64_P4_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
    if (mode == MODE_P4_STORM) {
        int cast=0;
#ifdef SC64_P4_STORM_ENABLED
        if (race_p4_active()) cast=race_p4_storm_cast_selected(cx,cy);
#endif
#ifdef SC64_P6_STORM_ENABLED
        if (race_p6_active()) cast=race_p6_storm_cast_selected(cx,cy);
#endif
#ifdef SC64_Z6_STORM_ENABLED
        if (race_z6_active()) cast=race_z6_storm_cast_selected(cx,cy);
#endif
        mode=MODE_NORMAL; toast(cast ? 0 : 8); return;
    }
#endif
    #ifdef SC64_Z5_CONSUME_ENABLED
    if (mode == MODE_Z5_CONSUME) {
        toast(race_z5_consume_issue_selected(cx, cy) ? 0 : 8);
        mode = MODE_NORMAL; return;
    }
    #endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (mode==MODE_RECHARGE) {
        int issued=race_battery_issue_command(selected_building,cx,cy);
        mode=MODE_NORMAL; toast(issued ? 0 : 8); return;
    }
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
    if (mode==MODE_YAMATO) {
        int target=race_bc_yamato_target_at(cx,cy),issued=0;
        if(target>=0) for(int i=0;i<MAX_UNITS;++i)
            if(u[i].alive && u[i].owner==SIDE && u[i].selected && sc64_race_raw_unit(u[i].type)==12)
                issued+=race_bc_yamato_issue(i,target);
        mode=MODE_NORMAL;toast(issued?0:8);return;
    }
#endif
    if (mode == MODE_MOVE) {
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (race_late_airborne(selected_building)) {
            toast(race_late_flight_order(selected_building, cx, cy, 0) ? 0 : 8);
            mode = MODE_NORMAL; return;
        }
#endif
	int issued=issue_order(ORDER_MOVE, cx, cy);
	mode = MODE_NORMAL;
#ifdef SC_FIELD_FEEDBACK
	if (issued) field_ui_ping(cx, cy, C_GREEN);
#else
    (void)issued;
#endif
	return;
    }
    if (mode == MODE_ATTACK) {
#ifdef SC64_DIRECT_ATTACK
        int direct_target = sc64_direct_attack_find_target(cx, cy);
        if (direct_target >= 0) {
            int tx=target_x(direct_target),ty=target_y(direct_target);
            int issued=sc64_direct_attack_issue_target(direct_target);
#ifdef SC_FIELD_FEEDBACK
            if (issued) field_ui_ping(tx,ty,C_RED);
#else
            (void)tx;(void)ty;(void)issued;
#endif
            mode = MODE_NORMAL;
            return;
        }
#endif
	int issued=issue_order(ORDER_ATTACK_MOVE, cx, cy);
	mode = MODE_NORMAL;
#ifdef SC_FIELD_FEEDBACK
	if (issued) field_ui_ping(cx, cy, C_RED);
#else
    (void)issued;
#endif
	return;
    }
    if (mode == MODE_BUILD) {
	start_build(menu);
	return;
    }
#ifdef SC64_NYDUS_ENABLED
    /* Ordinary A on an owned linked endpoint sends eligible selected units. */
    if (mode == MODE_NORMAL && race_nydus_issue_selected(cx, cy)) {
        toast(0); return;
    }
#endif
#ifdef SC64_SHARED_BUNKER
    /* Selected infantry use the same contextual A action as transports. */
    if (mode==MODE_NORMAL)
    for (int slot=0;slot<MAX_BUILDINGS;++slot)
        if (race_bunker_carrier_valid(slot) && b[slot].owner==SIDE &&
            dist2(cx,cy,b[slot].x,b[slot].y)<20*20) {
            int issued=0;
            for (int i=0;i<MAX_UNITS;++i)
                if (u[i].alive && u[i].selected && u[i].owner==SIDE &&
                    race_bunker_request_board(i,slot)) ++issued;
            if (issued) {toast(0);return;}
        }
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    if (mode == MODE_UNLOAD) {
        int issued = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].selected &&
                race_transport_request_unload(i, cx, cy)) ++issued;
        mode = MODE_NORMAL; toast(issued ? 0 : 8); return;
    }
    /* Select passengers, then press A on a friendly transport to board.
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
#ifdef SC64_NEAREST_RESOURCE_DEPOT
            u[worker].build = -1;
#endif
	    toast(0);
	    return;
	}
        /* A selected, empty worker can be sent to a visible mineral patch.
         * Loaded workers keep their current delivery order and cargo. */
#ifdef SC_FIELD_FEEDBACK
        if (field_ui_smart_attack()) return;
#endif
        if (worker >= 0) {
            int patch = selected_worker_mineral_target();
            if (patch >= 0) {
                int issued = 0;
                for (int i = 0; i < MAX_UNITS; ++i) {
                    if (!u[i].alive || !u[i].selected || u[i].owner != SIDE ||
                        SC64_TRANSPORT_LOADED(&u[i]) || !IS_WORKER(u[i].type) || u[i].carry) continue;
                    u[i].order = ORDER_MINE;
                    u[i].target = patch;
                    u[i].work_timer = 0;
                    u[i].path_timer = u[i].path_retry = 0;
#ifdef SC64_NEAREST_RESOURCE_DEPOT
                    u[i].build = -1;
#endif
                    ++issued;
                }
                toast(issued ? 0 : 15);
                return;
            }
        }
    }
    if (selected_building >= 0
#ifdef SC_FIELD_FEEDBACK
        /* A trains only on the selected building; elsewhere it reselects,
         * and on empty ground it clears the selection. */
        && field_ui_on_selected_building()
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
        && (!race_late_active() ||
            dist2(cx, cy, b[selected_building].x, b[selected_building].y) < 26 * 26)
#endif
        ) {
#ifdef SC64_RACES
        if (race_mode) {
            int producer_raw = sc64_race_raw_building(b[selected_building].type);
            if (sc64_race_is_zerg_townhall(b[selected_building].type)) producer_raw = 35;
            const Sc64SkirmishRecord *product = 0;
            int current_raw = sc64_race_raw_unit(train_type);
            unsigned count = sc64_skirmish_product_count((unsigned)producer_raw);
            for (unsigned p = 0; p < count; ++p) {
                const Sc64SkirmishRecord *candidate = sc64_skirmish_product_at((unsigned)producer_raw, p);
                if (candidate->raw_id == current_raw &&
                    race_skirmish_product_available(candidate, &selected_building)) product = candidate;
            }
            if (!product) {
                int raw = sc64_skirmish_menu_next_product((unsigned)producer_raw, SC64_SK_NONE,
                    race_skirmish_product_available, &selected_building);
                if (raw == SC64_SK_NONE || sc64_race_unit_from_raw(raw) < 0) { toast(8); return; }
                train_type = sc64_race_unit_from_raw(raw);
            }
            if (train_type < 0) { toast(8); return; }
            queue_unit(selected_building, train_type); return;
        }
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
#ifdef SC64_NEAREST_RESOURCE_DEPOT
                u[worker].build = -1;
#endif
		toast(0);
	    } else
		toast(5);
	}
	return;
    }
#ifdef SC_FIELD_FEEDBACK
    if (field_ui_smart_attack() || field_ui_smart_move()) return;
#endif
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
        ) && (!b[i].under_construction
#ifdef SC64_RACES
        || b[i].type == B_LAIR || b[i].type == B_HIVE
#endif
        )
#ifdef SC64_TERRAN_LATE_RUNTIME
        && !race_late_airborne(i)
#endif
        ) return i;
    return -1;
}

/* Reuse the serialized waypoint fields; a depot circle is re-queried when
 * the waypoint expires, never replaced by the depot center on long trips. */
static void __attribute__((noinline))
worker_return_to_depot(Unit *worker, int home)
{
    if (worker->path_retry == 0 && worker->path_timer == 0) {
        int nx, ny;
        if (!worker_depot_next(worker->x, worker->y, home, &nx, &ny)) {
            worker->path_x = worker->x; worker->path_y = worker->y;
            worker->path_retry = 12;
            return;
        }
        worker->path_x = nx; worker->path_y = ny; worker->path_timer = 24;
    }
    move_toward(worker, worker->path_x, worker->path_y);
}
#ifdef SC64_NEAREST_RESOURCE_DEPOT
static int
race_resource_depot_type(int type)
{
    return type == B_COMMAND || type == B_NEXUS || sc64_race_is_zerg_townhall(type);
}

static int
race_resource_depot_valid_for(const Unit *worker, int slot)
{
    int morphing_zerg_depot;
    if (!worker || slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive ||
        b[slot].owner != worker->owner ||
        !race_resource_depot_type(b[slot].type)) return 0;
    morphing_zerg_depot = b[slot].under_construction &&
        (b[slot].type == B_LAIR || b[slot].type == B_HIVE);
    if (b[slot].under_construction && !morphing_zerg_depot) return 0;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active() && race_late_airborne(slot)) return 0;
#endif
    return 1;
}

static int
race_resource_depot_reachable(const Unit *worker, int slot)
{
    int next_x, next_y;
    return dist2(worker->x, worker->y, b[slot].x, b[slot].y) <= 32 * 32 ||
        worker_depot_next(worker->x, worker->y, slot, &next_x, &next_y);
}

static void
race_depot_cache_invalidate_topology(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && IS_WORKER(u[i].type) &&
            (u[i].build == -2 || u[i].order == ORDER_MINE ||
             u[i].order == ORDER_GAS))
            u[i].build = -1;
}

static void
race_depot_cache_clear_no_home(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && IS_WORKER(u[i].type) && u[i].build == -2)
            u[i].build = -1;
}

/* Cache one home depot for the current resource trip. The cache stores only
 * a building slot in Unit.build (already serialized). Reachability is checked
 * when selecting a home and cached routes are invalidated on topology changes;
 * destination ownership/type/completion is checked whenever the slot is used. */
/* Keep the bounded route-selection scan in ROM. Inlining it into the hot
 * order loop consumes the reserved IWRAM stack space on campaign builds. */
static int __attribute__((noinline))
race_nearest_resource_depot(Unit *worker)
{
    int cached, best = -1, best_distance = 0x7fffffff;
    if (!worker) return -1;
    cached = worker->build;
    if (cached == -2) return -1; /* No reachable depot until a new one appears. */
    if (race_resource_depot_valid_for(worker, cached)) return cached;
    worker->build = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int distance;
        if (!race_resource_depot_valid_for(worker, i)) continue;
        distance = dist2(worker->x, worker->y, b[i].x, b[i].y);
        /* Slots are scanned in ascending order, so equal-distance candidates
         * cannot win the existing tie. Avoid a route search for them too. */
        if (distance >= best_distance ||
            !race_resource_depot_reachable(worker, i)) continue;
        if (distance < best_distance) {
            best = i;
            best_distance = distance;
        }
    }
    worker->build = best >= 0 ? best : -2;
    return best;
}
#endif
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

#ifdef SC64_NEAREST_RESOURCE_DEPOT
/* Leave combat orders in fast RAM. Keeping the less frequent worker return
 * state machine in ROM prevents GCC's inlining decision from placing the
 * entire enlarged order loop in IWRAM and consuming the stack reserve. */
static void __attribute__((noinline))
race_update_resource_worker(Unit *q)
{
    int home, tx, ty;
    if (!q->carry) {
        if ((q->order == ORDER_GAS &&
             (q->target < 0 || q->target >= MAX_BUILDINGS ||
              !building_complete(q->target))) ||
            (q->order == ORDER_MINE &&
             (q->target < 0 || q->target >= MAX_MINERALS))) {
            q->order = ORDER_IDLE;
            q->build = -1;
            return;
        }
    }
    home = race_nearest_resource_depot(q);
    /* Preserve the trip and its cargo while the no-home cache is asleep.
     * A topology change or completed depot makes it eligible for selection. */
    if (home < 0) return;
    if (q->carry) {
        if (dist2(q->x, q->y, b[home].x, b[home].y) > 32 * 32) {
            worker_return_to_depot(q, home);
        } else {
            add_owner_resources(q->owner, q->order == ORDER_MINE ? q->carry : 0,
                                q->order == ORDER_GAS ? q->carry : 0);
            q->carry = 0;
            q->build = -1;
            q->work_timer = 20;
        }
        return;
    }
    /* A loaded worker can return even if its former resource disappeared;
     * only an empty worker needs to read the resource target coordinates. */
    tx = q->order == ORDER_MINE ? m[q->target].x : b[q->target].x;
    ty = q->order == ORDER_MINE ? m[q->target].y : b[q->target].y;
    if (dist2(q->x, q->y, tx, ty) >
        (q->order == ORDER_GAS ? 24 * 24 : 7 * 7)) {
        move_toward(q, tx, ty);
    } else if (q->work_timer > 0) {
        --q->work_timer;
    } else if (q->order == ORDER_MINE) {
        q->carry = m[q->target].amount < 8 ? m[q->target].amount : 8;
        m[q->target].amount -= q->carry;
        if (q->carry > 0) q->build = -1;
        q->work_timer = 45;
        if (q->carry <= 0) q->order = ORDER_IDLE;
    } else {
        q->carry = 8;
        q->build = -1;
        q->work_timer = 60;
    }
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
#ifdef SC64_DIRECT_FIRING_POINTS
/* When ordinary movement has no route to the target center, look for a
 * walkable tile center in the exact weapon band with a clear normal movement
 * corridor. This is bounded to a 33x33 tile box, retains eight nearest
 * candidates, checks at most eight direct corridors, and then tries at most
 * two one-corner paths per nearby candidate. The cache stores the final firing
 * tile; each origin computes and revalidates its own first-leg waypoint. */
static int __attribute__((noinline))
sc64_direct_firing_corner(int x, int y, int px, int py, int *nx, int *ny)
{
    if (dist2(x, y, px, py) > 128 * 128) return 0;
    /* Horizontal-first: move to the candidate's x coordinate, then turn. */
    if (direct_path_clear(x, y, px, y) &&
        direct_path_clear(px, y, px, py)) {
        *nx = px; *ny = y;
        return 1;
    }
    /* Vertical-first: move to the candidate's y coordinate, then turn. */
    if (direct_path_clear(x, y, x, py) &&
        direct_path_clear(x, py, px, py)) {
        *nx = x; *ny = py;
        return 1;
    }
    return 0;
}

static int __attribute__((noinline))
sc64_direct_firing_approach(int x, int y, int px, int py, int *nx, int *ny)
{
    if (direct_path_clear(x, y, px, py)) {
        *nx = px; *ny = py;
        return 1;
    }
    return sc64_direct_firing_corner(x, y, px, py, nx, ny);
}

static int __attribute__((noinline))
sc64_direct_firing_waypoint(int x, int y, int tx, int ty,
                            int minimum, int maximum, int *nx, int *ny)
{
    int col, row, count = 0;
    int points[8], costs[8];
    if (minimum < 0 || maximum < minimum || maximum > 256 ||
        tx < 0 || ty < 0 || tx >= MAP_COLS * 16 || ty >= MAP_ROWS * 16)
        return 0;
    if (!path_grid_valid) build_path_grid();
    if (sc64_direct_firing_cache.valid &&
        sc64_direct_firing_cache.tx == tx && sc64_direct_firing_cache.ty == ty &&
        sc64_direct_firing_cache.minimum == minimum &&
        sc64_direct_firing_cache.maximum == maximum) {
        int px = sc64_direct_firing_cache.x;
        int py = sc64_direct_firing_cache.y;
        int cell = px / 16 + (py / 16) * MAP_COLS;
        if (px >= 0 && py >= 0 && px < MAP_COLS * 16 && py < MAP_ROWS * 16 &&
            SC64_GRID_GET(path_walkable, cell)) {
            int distance = dist2(px, py, tx, ty);
            if (distance >= minimum * minimum && distance <= maximum * maximum &&
                sc64_direct_firing_approach(x, y, px, py, nx, ny))
                return 1;
        }
    }
    {
        int left = limit((tx - maximum) / 16, 0, MAP_COLS - 1);
        int right = limit((tx + maximum) / 16, 0, MAP_COLS - 1);
        int top = limit((ty - maximum) / 16, 0, MAP_ROWS - 1);
        int bottom = limit((ty + maximum) / 16, 0, MAP_ROWS - 1);
        int minimum2 = minimum * minimum, maximum2 = maximum * maximum;
        for (row = top; row <= bottom; ++row) for (col = left; col <= right; ++col) {
            int px = col * 16 + 8, py = row * 16 + 8;
            int d = dist2(px, py, tx, ty), cost, i;
            int cell = col + row * MAP_COLS;
            if (d < minimum2 || d > maximum2 ||
                !SC64_GRID_GET(path_walkable, cell)) continue;
            cost = dist2(x, y, px, py);
            if (count == 8 && cost >= costs[7]) continue;
            i = count < 8 ? count++ : 7;
            while (i > 0 && cost < costs[i - 1]) {
                points[i] = points[i - 1]; costs[i] = costs[i - 1]; --i;
            }
            points[i] = cell; costs[i] = cost;
        }
    }
    /* Preserve the fast corridor path for every retained candidate first. */
    for (int i = 0; i < count; ++i) {
        int cell = points[i];
        int px = (cell % MAP_COLS) * 16 + 8;
        int py = (cell / MAP_COLS) * 16 + 8;
        if (!direct_path_clear(x, y, px, py)) continue;
        *nx = px; *ny = py;
        sc64_direct_firing_cache.tx = (u16)tx;
        sc64_direct_firing_cache.ty = (u16)ty;
        sc64_direct_firing_cache.minimum = (u16)minimum;
        sc64_direct_firing_cache.maximum = (u16)maximum;
        sc64_direct_firing_cache.x = (u16)px;
        sc64_direct_firing_cache.y = (u16)py;
        sc64_direct_firing_cache.valid = 1;
        return 1;
    }
    /* For nearby final points, allow one axis-aligned turn around a corner.
     * The final in-band point remains cached; only the returned first leg can
     * be outside the firing band. Longer paths retain baseline retry behavior. */
    for (int i = 0; i < count; ++i) {
        int cell = points[i];
        int px = (cell % MAP_COLS) * 16 + 8;
        int py = (cell / MAP_COLS) * 16 + 8;
        if (!sc64_direct_firing_corner(x, y, px, py, nx, ny)) continue;
        sc64_direct_firing_cache.tx = (u16)tx;
        sc64_direct_firing_cache.ty = (u16)ty;
        sc64_direct_firing_cache.minimum = (u16)minimum;
        sc64_direct_firing_cache.maximum = (u16)maximum;
        sc64_direct_firing_cache.x = (u16)px;
        sc64_direct_firing_cache.y = (u16)py;
        sc64_direct_firing_cache.valid = 1;
        return 1;
    }
    return 0;
}

static void __attribute__((noinline))
move_toward_weapon_range(Unit *q, int tx, int ty, int minimum, int maximum)
{
    int may_plan = q->path_retry == 0 && q->path_timer <= 0;
    int nx, ny;
    move_toward(q, tx, ty);
    if (!may_plan || q->path_retry != 12 || maximum < 32 ||
        sc64_race_unit_flying(q->type)) return;
    if (!sc64_direct_firing_waypoint(q->x, q->y, tx, ty, minimum, maximum, &nx, &ny))
        return;
    q->path_x = nx; q->path_y = ny;
    q->path_retry = 0; q->path_timer = 24;
}
#endif
#ifdef SC64_UNIT_FACING
#if !defined(SC64_HAS_SPRITES) || !defined(SC64_RACES)
#error "SC64_UNIT_FACING requires imported race sprites"
#endif
#include "sc64_unit_facing.h"
#endif

#ifdef SC64_Z6_RUNTIME
/* The large weapon body is already in ROM; keep its validation wrapper
 * there as well rather than duplicating it in every ARM order branch. */
static void __attribute__((noinline))
#else
static void
#endif
tick_combat(Unit * q, int target)
{
    int dx, dy, range, damage;
#ifdef SC64_P7_PROJECTILES_ENABLED
    /* Only the child weapon-event adapter may fire weapon 79. Until that
     * lifecycle is connected, never fall back to instant generic damage. */
    if (q->type==U_P7_INTERCEPTOR) return;
#endif
#if (defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)) || \
    defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
    if (q->type == U_SCOURGE && !q->alive) return;
    /* Scourge target validation must precede target_alive(), which indexes
     * the unit or building arrays from this combined target namespace. */
    if (q->type == U_SCOURGE &&
        (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS)) return;
#endif
    if (target < 0 || !target_alive(target))
	return;
#ifdef SC64_P6_COMBAT_ENABLED
    if (race_p6_combat_infested_attack(q, target)) return;
#endif
#ifdef SC64_RACES
    if(!SC64_CAN_ATTACK_FROM_SOURCE(q->type,target,q->owner,q->raw_owner))return;
#endif
#ifdef SC64_P7_ARCHON_SPLASH
    /* Archon weapon 70 measures reach between source collision rectangles,
     * independently of the generic runtime-center approximation below. */
    if (q->type==U_P7_ARCHON) {
        (void)race_p7_archon_splash_attack(q,target);
        return;
    }
#endif
    dx = target_x(target) - q->x;
    dy = target_y(target) - q->y;
    range = unit_range(q->type);
    damage = unit_damage(q->type);
#ifdef SC64_RACES
    int flying=target<MAX_UNITS&&sc64_race_unit_flying(u[target].type);
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (target>=MAX_UNITS && race_bc_facility_airborne(target-MAX_UNITS)) flying=1;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (target >= MAX_UNITS && race_late_airborne(target - MAX_UNITS)) flying = 1;
#endif
    range=(sc64_race_unit_range_vs(q->type,flying)+1)/2;
    /* Weapon reach is measured from a structure's footprint edge. The
       navigation grid keeps actors outside the same 18px collision radius. */
    if(target>=MAX_UNITS)range+=18;
#if (defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)) || \
    defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
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
    Sc64StockAttack stock_attack={0};
    int attack_raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    int stock_weapon=sc64_stock_attack_select(attack_raw,flying,&stock_attack);
#ifdef SC64_SIEGE_SPLASH
    int siege_splash_weapon=stock_weapon &&
        (stock_attack.weapon_id==27 || stock_attack.weapon_id==28);
#endif
    if (!stock_weapon) {
        const Sc64StockUnitProfile *identity=sc64_stock_unit_profile(attack_raw);
        /* An alias may advertise another raw unit's weapon eligibility.
         * A known source None weapon must not acquire that alias's attack.
         * The existing Reaver adapter models ground Scarab ammunition. */
        if (identity && (flying?identity->air_weapon:identity->ground_weapon)==130 &&
            (flying || q->type!=U_REAVER)) return;
    }
    /* Firebat's maximum hits describe spatial flame segments, not three
     * guaranteed impacts on the selected target. The stock weapon still
     * supplies its damage class, upgrade and per-impact values. */
    int minimum_range=0;
    if (stock_weapon) {
        range=(stock_attack.weapon->max_range+1)/2;
        minimum_range=(stock_attack.weapon->min_range+1)/2;
        if (target>=MAX_UNITS) {
            range+=18;
            if (minimum_range) minimum_range+=18;
        } else if (!flying && !sc64_race_unit_flying(q->type) &&
                   stock_attack.weapon->min_range==0 &&
                   stock_attack.weapon->max_range<=32) {
            /* Ground melee needs to reach the center spacing reserved by
             * unit_step_clear(). Campaign actors can reserve up to ten
             * pixels from source bounds; retain the six-pixel fallback in
             * the standard roster and do not widen ranged/min-range weapons. */
            int contact_range=6;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
            if (target<MAX_UNITS) {
                int mover_radius=sc64_game_unit_collision_radius(q->type);
                int target_radius=sc64_game_unit_collision_radius(u[target].type);
                if (mover_radius>contact_range) contact_range=mover_radius;
                if (target_radius>contact_range) contact_range=target_radius;
            }
#endif
            if (range<contact_range) range=contact_range;
        }
        damage=sc64_stock_attack_damage_per_impact(&stock_attack,
                     q->owner==SIDE?weapons_level:0);
    }
#ifdef SC64_SHARED_BUNKER
    if (q->order==ORDER_TRANSPORTED && q->target>=MAX_UNITS) range+=32;
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
#ifdef SC64_RACES
        if (dx*dx+dy*dy<minimum_range*minimum_range) return;
#endif
	if (!q->cd) {
#ifdef SC64_IMPACT_EVENTS
            /* Decide before any launch side effect: a full queue leaves the
             * weapon ready for the next update instead of losing damage. */
            unsigned impact_steps=race_impact_unit_steps(q,target,flying);
#ifdef SC64_SIEGE_SPLASH
            if (siege_splash_weapon) impact_steps=0;
#endif
#ifdef SC64_Z6_RUNTIME
            if (!race_z6_hallucination_can_deal_damage((int)(q-u))) impact_steps=0;
#endif
            if (impact_steps && race_impact_full()) return;
#endif
#ifdef SC64_UNIT_FACING
	    SC64_FACING_FIRE(q, target);
#endif
	    int visual_color=sc64_weapon_vfx_unit_profile(
#ifdef SC64_RACES
                    q->type==U_RAYNOR && q->raw_type==19 ? U_VULTURE :
#endif
                    q->type,
#ifdef SC64_RACES
                    flying
#else
                    0
#endif
                );
#ifdef SC64_RACES
#ifdef SC64_TANK_SOURCE_VFX
            if (q->type==U_TANK || q->type==U_SIEGE_TANK) {
                int source_style=q->type==U_SIEGE_TANK ? SC64_TANK_SOURCE_SIEGE : SC64_TANK_SOURCE_TANK;
                int dir=sc64_private_tank_direction(target_x(target)-q->x,
                                                    target_y(target)-q->y);
                visual_color=sc64_weapon_vfx_pack(source_style,dir);
            }
#endif
#endif
#ifdef SC64_WEAPON_SOURCE_VFX
            if (!sc64_source_unit_vfx(q,target_x(target),target_y(target),flying))
#endif
            add_tracer(q->x,q->y,target_x(target),target_y(target),visual_color);
	#ifdef SC64_RACES
#ifdef SC64_SIEGE_SPLASH
        if (siege_splash_weapon) race_siege_splash_attack(q,target,damage);
        else
#endif
        {
        int hits=stock_weapon?((stock_attack.weapon_id==25 || stock_attack.weapon_id==26) ?
                              (int)stock_attack.weapon->damage_factor :
                              (int)sc64_stock_attack_impact_count(&stock_attack)):
                              sc64_race_unit_hits_vs(q->type,flying);
#ifdef SC64_IMPACT_EVENTS
        /* Nonzero only for a packet that can deal damage (see above). */
        if (impact_steps)
            race_impact_launch(target,damage,hits,q->owner,
                               stock_weapon?stock_attack.weapon->damage_class:-1,
                               q->type,impact_steps);
        else
#endif
#ifdef SC64_Z6_RUNTIME
        if (race_z6_hallucination_can_deal_damage((int)(q-u)))
#endif
        for(int hit=0;hit<hits&&target_alive(target);hit++) {
            if (stock_weapon) hit_target_impl(target,damage,q->type,q->owner,
                                             stock_attack.weapon->damage_class);
            else hit_target(target,damage,q->type,q->owner);
        }
        }
#if (defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)) || \
    defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
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
        q->cd=((stock_weapon?stock_attack.weapon->cooldown:
                sc64_race_unit_cooldown_vs(q->type,flying))*5+3)/4;
#else
        q->cd = q->type == U_ZERGLING ? 10 : q->type == U_FIREBAT ? 28 : q->type == U_RAYNOR ? 30 : 19;
#endif
	}
    } else {
#ifdef SC64_SHARED_BUNKER
        if (q->order==ORDER_TRANSPORTED && q->target>=MAX_UNITS) return;
#endif
#ifdef SC64_DIRECT_FIRING_POINTS
	move_toward_weapon_range(q, target_x(target), target_y(target),
                                 minimum_range, range);
#else
	move_toward(q, target_x(target), target_y(target));
#endif
    }
}

#ifdef SC64_SHARED_BUNKER
#include "sc64_bunker_combat.h"
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
#include "sc64_battlecruiser_yamato.h"
#include "sc64_battlecruiser_yamato_projectile.h"
#endif

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

#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
/* Preserve pre-order, battery service and caster ordering while keeping the
 * rare Battlecruiser work out of the hot ordinary order loop. */
static int __attribute__((noinline)) race_bc_order_dispatch(Unit *q,int slot,int recharge)
{
    if (sc64_race_is_battlecruiser(q->type) && sc64_battlecruiser_source_tick_due()) {
        if (q->build>=0) --q->build;
        race_bc_yamato_heading_tick(q);
    }
#ifdef SC64_BATTERY_SERVICE_ENABLED
    if (recharge && race_battery_tick_unit(slot)) return 1;
#else
    (void)recharge;
#endif
    return race_bc_yamato_recovery_tick(q) || race_bc_yamato_tick_caster(slot);
}
/* A non-BC can still own a stale charging reservation after slot reuse;
 * dispatch it so the existing caster check clears that reservation. */
static inline int race_bc_order_dispatch_needed(const Unit *q,int slot,int recharge)
{
    const Sc64BcYamatoShot *a=&sc64_bc_shots[0], *b=&sc64_bc_shots[1];
    return sc64_race_is_battlecruiser(q->type) || recharge ||
        (a->state && a->state!=SC64_BC_SHOT_FLIGHT && a->caster==(unsigned)slot) ||
        (b->state && b->state!=SC64_BC_SHOT_FLIGHT && b->caster==(unsigned)slot);
}
#endif

#if defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED) && !defined(GBA_HOST)
static void __attribute__((noinline,long_call))
#else
static void GBA_HOT
#endif
update_orders(void)
{
    int i, target;
#ifdef SC64_TRANSPORT_RUNTIME
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
#ifdef SC64_OWNER_LIST_TEST
#ifndef GBA_HOST
#error "Owner-list assertion hook is host-test only"
#endif
    sc64_owner_lists_test_probe();
#ifdef SC64_OWNER_LIST_TEST_ONLY
    return;
#endif
#endif
#ifdef SC64_RACES
    if (target_lists_valid) sc64_target_index_begin_owner_lists();
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (race_protoss_services_active()) sc64_p7_arbiter_cache_process_due();
#endif
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
    sc64_target_hostility_snapshot();
#endif
#endif
#ifdef SC64_DIAGNOSTIC_ORDERS
    sc64_order_setup_cycles = gba_clock() - sc64_setup_start;
#endif
    for (i = 0; i < MAX_UNITS; i++) {
	Unit *q = &u[i];
	if (!q->alive)
	    continue;
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        if (SC64_CARRIER_CHILD_SCRIPTED(q)) continue;
#endif
#ifdef SC64_P7_INTERCEPTOR_FLIGHT_ENABLED
        /* An engaged Carrier stops and launches instead of closing in. */
        if (q->type == U_P7_CARRIER && race_p7_interceptor_flight_carrier_order(i))
            continue;
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
        if (q->type == U_Z7_EGG) {
            (void)race_z7_egg_tick_unit(i);
            continue;
        }
#endif
#ifdef SC64_P6_COMBAT_ENABLED
        if (race_p6_combat_unit_burrowed(i)) continue;
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
        int p7_recharge_order=race_battery_order_is_recharge(q->order);
#endif
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
	#ifdef SC64_TRANSPORT_RUNTIME
        if (q->order >= ORDER_BOARD
#ifdef SC64_BATTERY_SERVICE_ENABLED
            && !race_battery_order_is_recharge(q->order)
#endif
            && !SC64_NYDUS_ORDER(q->order)
            && !SC64_PENDING_BUILD_ORDER(q->order)
            ) {
            race_transport_activity = 1;
            if (race_transport_tick_unit(i)) continue;
        }
	#endif
	#ifdef SC64_T5_RUNTIME
        /* Preplaced mines own their arm/acquire/chase state. The ordinary
         * actor loop must not clear their target or consume their timer. */
        if (q->type == U_SPIDER_MINE) continue;
	#endif
	#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (sc64_game_unit_actor_flags(q->type) &&
            !(sc64_game_unit_actor_flags(q->type)&SC64_GAME_ACTOR_MOVABLE)) {
            q->order = ORDER_IDLE;
            q->target = -1;
            continue;
        }
	#endif
	if (q->cd > 0)
	    q->cd--;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
#ifdef SC64_BATTERY_SERVICE_ENABLED
        if (race_bc_order_dispatch_needed(q,i,p7_recharge_order) &&
            race_bc_order_dispatch(q,i,p7_recharge_order)) continue;
#else
        if (race_bc_order_dispatch_needed(q,i,0) && race_bc_order_dispatch(q,i,0)) continue;
#endif
#elif defined(SC64_BATTERY_SERVICE_ENABLED)
        if (p7_recharge_order && race_battery_tick_unit(i)) continue;
#endif
#ifdef SC64_DIRECT_ATTACK
        if (q->order == ORDER_ATTACK_TARGET && q->owner == SIDE) {
            sc64_direct_attack_tick_unit(i);
            continue;
        }
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
        if (q->order == ORDER_Z5_CONSUME && race_z5_consume_tick_unit(i)) continue;
#endif
#ifdef SC64_NYDUS_ENABLED
        if (q->order == ORDER_NYDUS_ENTER && race_nydus_tick_unit(i)) continue;
#endif

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
                ,q->type SC64_P7_SOURCE_OWNER_ARG(q->raw_owner));
            if (target >= 0) tick_combat(q, target);
            continue;
        }
#endif

#ifdef SC64_RACES
        if (q->order == ORDER_BUILD_PENDING) {
            race_pending_build_tick(i);
            if (!q->alive || q->order != ORDER_BUILD) continue;
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
	    }
#ifdef SC64_CONSTRUCTION_HP
            else {
                race_construction_worker_tick(i);
            }
#else
            else if (++b[q->target].build_progress >= building_build_time(b[q->target].type)) {
		b[q->target].under_construction = 0;
		b[q->target].hp = b[q->target].maxhp;
		b[q->target].build_progress = 0;
#ifdef SC64_NEAREST_RESOURCE_DEPOT
                if (race_resource_depot_type(b[q->target].type))
                    race_depot_cache_clear_no_home();
#endif
		if (b[q->target].type == B_DEPOT && q->owner == SIDE)
		    supply_cap += 16;
		q->order = ORDER_IDLE;
		q->target = -1;
	    }
#endif
	    continue;
	}

#ifdef SC64_NEAREST_RESOURCE_DEPOT
        if (q->order == ORDER_MINE || q->order == ORDER_GAS) {
            race_update_resource_worker(q);
            continue;
        }
#else
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
		/* Mineral trips can collect before a base exists. Hold loaded cargo
		 * until a completed friendly depot can receive it. */
		if (q->order != ORDER_MINE) {
		    q->order = ORDER_IDLE;
		    continue;
		}
		if (q->carry) continue;
	    }
	    tx = q->order == ORDER_MINE ? m[q->target].x : b[q->target].x;
	    ty = q->order == ORDER_MINE ? m[q->target].y : b[q->target].y;
	    if (q->carry) {
		/* Stop at the approach ring outside the structure's blocked cells. */
		if (dist2(q->x, q->y, b[home].x, b[home].y) > 32 * 32) {
		    worker_return_to_depot(q, home);
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

#endif

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
                64,q->type SC64_P7_SOURCE_OWNER_ARG(q->raw_owner));
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
		target = nearest_enemy_typed(q->x, q->y, SIDE, 52,q->type
                                             SC64_P7_SOURCE_OWNER_ARG(q->raw_owner));
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
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
    target_hostile_masks_valid = 0;
#endif
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
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    if (!race_z7_colony_pre_building_tick()) {
        ++mission_unsupported_count; screen=SCREEN_PAUSE; toast(14); return;
    }
#endif
    race_tick_buildings();
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
    if (!race_z7_colony_post_building_tick()) {
        ++mission_unsupported_count; screen=SCREEN_PAUSE; toast(14); return;
    }
#endif
    return;
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
#ifdef SC64_IMPACT_EVENTS
    /* Combat launches before this end-of-update step, like tracers. */
    race_impact_tick();
#endif
    for (i = 0; i < MAX_TRACERS; i++)
	if (tracers[i].life)
	    tracers[i].life--;
}

#ifndef SC64_DEATH_VFX_DISABLED
/* Effect clock: game frames, plus outro ticks once the simulation stops. */
static u32
death_vfx_now(void)
{
    return (u32)frame + result_outro_get();
}

static int
death_vfx_raw(int slot)
{
#ifdef SC64_RACES
    if (slot < MAX_UNITS)
        return u[slot].raw_type >= 0 ? u[slot].raw_type : sc64_race_raw_unit(u[slot].type);
    const Building *q = &b[slot - MAX_UNITS];
    return q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type);
#else
    /* Original-art types use the matching StarCraft unit ids. */
    static const unsigned char units[] = { 7, 0, 32, 34, 37, 38, 19 };
    static const unsigned char buildings[] = { 106, 111, 109, 110, 112, 122, 131,
                                               125, 130, 143 };
    int type = slot < MAX_UNITS ? u[slot].type : b[slot - MAX_UNITS].type;
    if (slot < MAX_UNITS)
        return type >= 0 && type < (int)sizeof(units) ? units[type] : -1;
    return type >= 0 && type < (int)sizeof(buildings) ? buildings[type] : -1;
#endif
}

#if defined(SC64_HAS_SPRITES) && !defined(SC64_RACES)
static const Sc64SpriteFrame *mission_unit_sprite(int type);
static const Sc64SpriteFrame *mission_building_sprite(int type);
#endif
static int
death_vfx_extent(int slot, int raw)
{
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite =
#ifdef SC64_RACES
        raw >= 0 ? sc64_p8_unit_sprite_for_raw((unsigned short)raw) : 0;
#else
        slot < MAX_UNITS ? mission_unit_sprite(u[slot].type) :
                           mission_building_sprite(b[slot - MAX_UNITS].type);
#endif
    (void)raw;
    if (sprite) return (sprite->w > sprite->h ? sprite->w : sprite->h) / 2;
#else
    (void)raw;
#endif
    return slot < MAX_UNITS ? 5 : 14;
}

static void
death_vfx_spawn_slot(Sc64DeathVfx *pool, int slot, int x, int y, int owner)
{
    int raw = death_vfx_raw(slot), building = slot >= MAX_UNITS;
    int style = sc64_death_vfx_style_for_raw(raw, building), source = 0;
    Sc64DeathVfxTiming timing = sc64_death_vfx_timing(style);
#ifdef SC64_DEATH_SOURCE_ART
    const Sc64DeathSource *art = raw >= 0 ? sc64_death_source(raw) : 0;
    if (sc64_death_source_valid(art)) {
        timing.burst_su = art->burst_su;
        timing.remnant_start = art->remnant_start;
        timing.total_su = art->total_su;
        source = 1;
    }
#endif
    sc64_death_vfx_spawn(pool, SC64_DEATH_VFX_POOL, death_vfx_now(), style, raw,
                         x, y, death_vfx_extent(slot, raw), &timing, source,
                         owner_color(owner));
}

/* Observe alive-to-dead transitions with non-positive HP after a simulation
 * step. This reads actor rows only; scripted removals keep their HP, and
 * units that die loaded in a transport or hidden in a hangar were not on the
 * battlefield, so none of them plays a death. */
static void __attribute__((noinline))
death_vfx_scan(void)
{
    int primed = death_vfx_primed_get();
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
    Sc64DeathVfx pool[SC64_DEATH_VFX_POOL];
    death_vfx_test_load_pool(pool);
#else
    Sc64DeathVfx *pool = death_vfx;
#endif
    result_outro_set(0); result_outro_end_set(0); /* playing: no outro offset */
    /* Build each 8-row alive mask in a register and compare it with the
     * shadow byte once: the common no-change path stays a short scan. */
    for (int base = 0; base < MAX_UNITS; base += 8) {
        unsigned now = 0, gone;
        int end = base + 8 < MAX_UNITS ? base + 8 : MAX_UNITS;
        for (int i = base; i < end; ++i)
            if (u[i].alive) now |= 1u << (i - base);
        gone = death_vfx_seen_get_byte(base >> 3) & ~now;
        death_vfx_seen_set_byte(base >> 3, now);
        if (!gone || !primed) continue;
        /* Cargo destroyed with its transport and hangar-hidden children
         * still carry those orders/types: they were off the battlefield. */
        for (int i = base; i < end; ++i)
            if ((gone >> (i - base)) & 1u && u[i].hp <= 0 &&
                !SC64_TRANSPORT_LOADED(&u[i]) && !SC64_CARRIER_HANGAR_HIDDEN(&u[i]))
                death_vfx_spawn_slot(pool, i, u[i].x, u[i].y, u[i].owner);
    }
    /* Building shadow bits follow the unit bits (byte aligned). */
    for (int base = 0; base < MAX_BUILDINGS; base += 8) {
        unsigned now = 0, gone;
        int end = base + 8 < MAX_BUILDINGS ? base + 8 : MAX_BUILDINGS;
        unsigned seen_byte = (MAX_UNITS + 7) / 8 + (base >> 3);
        for (int i = base; i < end; ++i)
            if (b[i].alive) now |= 1u << (i - base);
        gone = death_vfx_seen_get_byte(seen_byte) & ~now;
        death_vfx_seen_set_byte(seen_byte, now);
        if (!gone || !primed) continue;
        for (int i = base; i < end; ++i)
            if ((gone >> (i - base)) & 1u && b[i].hp <= 0)
                death_vfx_spawn_slot(pool, MAX_UNITS + i, b[i].x, b[i].y, b[i].owner);
    }
    death_vfx_primed_set(1);
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
    death_vfx_test_store_pool(pool);
#endif
}

/* An effect still in its collapse/explosion phase keeps the outro running. */
static int
death_vfx_bursting(u32 now)
{
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i) {
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
        Sc64DeathVfx e;
        death_vfx_test_load_row(i, &e);
        const Sc64DeathVfx *row = &e;
#else
        const Sc64DeathVfx *row = &death_vfx[i];
#endif
        if (!sc64_death_vfx_expired(row, now) &&
            sc64_death_vfx_age_su(row->birth, now) < row->burst_su) return 1;
    }
    return 0;
}

/* Advance the presentation clock on result-screen updates. The outro ends at
 * the cap, or after the minimum once every final burst has finished. */
static void
result_outro_tick(void)
{
    u16 outro = result_outro_get(), end = result_outro_end_get();
    if (outro >= SC64_RESULT_OUTRO_MAX + SC64_RESULT_PANEL_OPEN) return;
    ++outro;
    result_outro_set(outro);
    if (end) return;
    if (outro >= SC64_RESULT_OUTRO_MAX ||
        (outro >= SC64_RESULT_OUTRO_MIN && !death_vfx_bursting(death_vfx_now())))
        result_outro_end_set(outro);
}

/* Runs at the top of every update, before input and simulation. During play
 * it scans on alternate updates, so a death appears one or two updates late.
 * The first result-screen update scans the outcome update's final deaths
 * (the outro counter is still zero there), then the outro clock advances. */
static void __attribute__((noinline))
death_vfx_update(void)
{
    if (screen == SCREEN_PLAY) {
        if (!(frame & 1)) death_vfx_scan();
        return;
    }
    if (screen != SCREEN_RESULT) return;
    if (!result_outro_get() && !result_outro_end_get()) death_vfx_scan();
    result_outro_tick();
}
#endif

static void GBA_HOT
update_exploration(void)
{
#ifdef SC64_IMPORTED_MISSION
    sc64_update_exploration();
#else
    int i, col, row;
    SC64_GRID_CLEAR(visible);
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) && u[i].owner == SIDE) {
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            &&sc64_game_unit_selectable(u[i].type)
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            &&sc64_game_unit_selectable(u[i].type)
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            &&sc64_game_unit_selectable(u[i].type)
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
             || sc64_game_unit_has_flag(u[i].type, SC64_GAME_ACTOR_WEAPON)
#endif
	     )
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            && sc64_game_unit_selectable(u[i].type)
            && (!sc64_game_unit_actor_flags(u[i].type) ||
                sc64_game_unit_has_flag(u[i].type, SC64_GAME_ACTOR_WEAPON))
#endif
	    )
	    u[i].selected = 1;
}

static void
select_next_train_type(void)
{
#ifdef SC64_RACES
    if (race_mode && selected_building >= 0 && selected_building < MAX_BUILDINGS &&
        b[selected_building].alive && b[selected_building].owner == SIDE) {
        unsigned producer = (unsigned)sc64_race_raw_building(b[selected_building].type);
        if (sc64_race_is_zerg_townhall(b[selected_building].type)) producer = 35;
        int raw = sc64_skirmish_menu_next_product(producer,
            (unsigned)sc64_race_raw_unit(train_type), race_skirmish_product_available, &selected_building);
        if (raw != SC64_SK_NONE) train_type = sc64_race_unit_from_raw(raw);
        return;
    }
#endif
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
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    if (race_human == R_ZERG) {
#ifdef SC64_Z7_SCOURGE_COMBAT
        static const int zerg_types[] = {U_DRONE, U_OVERLORD, U_ZERGLING, U_HYDRALISK, U_MUTALISK
            , U_SCOURGE
        };
        const int zerg_count = (int)(sizeof(zerg_types) / sizeof(zerg_types[0]));
        int n;
        for (n = 0; n < zerg_count; ++n) if (zerg_types[n] == train_type) break;
        for (int step = 1; step <= zerg_count; ++step) {
            int type = zerg_types[(n + step) % zerg_count];
#else
        static const int zerg_types[] = {U_DRONE, U_OVERLORD, U_ZERGLING, U_HYDRALISK, U_MUTALISK};
        int n;
        for (n = 0; n < 5; ++n) if (zerg_types[n] == train_type) break;
        for (int step = 1; step <= 5; ++step) {
            int type = zerg_types[(n + step) % 5];
#endif
#ifdef SC64_Z7_SCOURGE_COMBAT
            if (type == U_SCOURGE && !race_z7_active()) continue;
#endif
            if (selected_building < 0 || sc64_race_producer_compatible(
                    b[selected_building].type, sc64_race_unit_rules[type].producer)) {
                train_type = type; return;
            }
        }
        return;
    }
#endif
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
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
            &&sc64_game_unit_selectable(u[i].type)
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

#ifdef SC64_SHOWCASE_ENABLED
#include "sc64_showcase_adapter.h"
#endif
#ifdef SC_COMMAND_RING
#include "command_ring.h"
#endif

#ifdef SC_FIELD_FEEDBACK
/* SELECT in the field drops the selection (A on open ground now moves the
 * selected units there); with nothing selected it pauses, like START. */
static __attribute__((noinline)) int
field_ui_select_key(void)
{
    if (!selected_count() && selected_building < 0) return 0;
    clear_selection();
    mode = MODE_NORMAL;
    menu_open = 0;
    return 1;
}
#endif

static void
pause_action(u16 pressed)
{
#if defined(SC_DEFAULT_UI) || defined(SC64_FIELD_CONTROLS)
    field_ui_pause_input(pressed);
    return;
#endif
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
#ifndef SC64_DEATH_VFX_DISABLED
    death_vfx_update(); /* bare call: game_update's size steers IWRAM inlining */
#endif
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) {
        if (pressed & KEY_SELECT) { sc64_showcase_leave(held); return; }
        if (screen == SCREEN_PAUSE) {
            if (pressed & KEY_B) sc64_showcase_leave(held);
            else if (pressed & KEY_R) sc64_showcase_start(sc64_showcase_scene);
            else if (pressed & KEY_A) screen = SCREEN_PLAY;
            return;
        }
        if (screen == SCREEN_RESULT) {
            if (pressed & KEY_B) sc64_showcase_leave(held);
            else if (pressed & KEY_A) sc64_showcase_start(sc64_showcase_scene);
            return;
        }
    }
#endif
    if (screen != SCREEN_PLAY) reset_selection_gestures();
    if (screen == SCREEN_TITLE) {
#ifdef SC64_RACES
        int old_page=race_front.page,old_row=race_front.row;
        race_front_briefing_configure();
        int action=sc64_frontend_update(&race_front,held);
        race_front_briefing_configure();
        if(race_front_load_error)race_front_load_error--;
        if(pressed){race_front_load_error=0;audio_tone((pressed&KEY_A)?660:440,2,2);}
        if(old_page!=race_front.page)race_front_transition=race_front.ticks;
        if(old_row!=race_front.row)audio_tone(440,2,2);
#ifdef SC64_SHOWCASE_ENABLED
        if (action == SC_FRONT_START_SHOWCASE) {
            sc64_display_palette(sc64_palette+32);
            sc64_showcase_start(race_front.row);
        } else
#endif
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
#ifdef SC_DEFAULT_UI
        field_ui_title_input(pressed);
        return;
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
#ifdef SC64_Z5_RUNTIME
    /* Z5's source program contains PauseGame without an UnpauseGame. A
     * deliberate A press acknowledges that authored pause and resumes this
     * adaptation; no timer, trigger, combat, or order advances while waiting. */
    if (race_z5_world_paused()) {
        if (pressed & KEY_A) race_z5.pause_acknowledged = 1;
        else if (pressed & KEY_B) save_game();
        else if (pressed & KEY_START) screen = SCREEN_PAUSE;
        return;
    }
#endif
    if (pressed & KEY_START) {
	reset_selection_gestures();
	screen = SCREEN_PAUSE;
	return;
    }
    if (pressed & KEY_SELECT) {
	reset_selection_gestures();
#ifdef SC_FIELD_FEEDBACK
	if (field_ui_select_key()) return;
#endif
	screen = SCREEN_PAUSE;
	return;
    }
#ifdef SC64_P4_RUNTIME
    /* Advance the saved source action lists through PauseGame/UnpauseGame,
     * while combat, production, orders and player commands remain frozen. */
    if (race_p4_world_paused()) {
        ++frame;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
        sc64_bc_pause_offset=(u8)((sc64_bc_pause_offset+1u)%5u);
#endif
        update_imported_mission();
        update_exploration();
        if (won || lost) {
            if (won) race_completed |= 1 << race_selected_mission;
            save_game();
            reset_selection_gestures();
            screen = SCREEN_RESULT;
            result_timer = 0;
        }
        return;
    }
#endif
#ifdef SC64_P8_RUNTIME
    /* P8 PauseGame freezes combat, production, orders and commands while the
     * source trigger clock and its waits continue (native source pause). */
    if (race_p8_world_paused()) {
        ++frame;
        update_imported_mission();
        update_exploration();
        if (won || lost) {
            if (won) race_completed |= 1 << race_selected_mission;
            save_game();
            reset_selection_gestures();
            screen = SCREEN_RESULT;
            result_timer = 0;
        }
        return;
    }
#endif
#ifdef SC_FIELD_FEEDBACK
    if (ping_life) --ping_life;
#endif
#if defined(SC64_P6_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
    /* A spell hotkey keeps real-time targeting practical on the GBA pad. */
    if ((held&KEY_B) && (pressed&KEY_R) && !box_state &&
        !(held&(KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN)) &&
        race_selected_storm_hero()) {
        int available=0;
#ifdef SC64_P6_STORM_ENABLED
        available |= race_p6_storm_menu_available();
#endif
#ifdef SC64_Z6_STORM_ENABLED
        available |= race_z6_storm_menu_available();
#endif
        reset_selection_gestures(); menu_open=0;
        mode=available ? MODE_P4_STORM : MODE_NORMAL;
        if (!available) toast(8);
        pressed&=(u16)~(KEY_A|KEY_B|KEY_R);
    }
#endif
    /* Menus own their inputs. Selection gestures start only in the field. */
#ifdef SC64_P4_STORM_ENABLED
    /* B+R is an authored GBA escort-group shortcut. Keep the two objective
     * heroes separate from bulk route orders; regular R still selects them. */
    if (race_p4_active() && (held & KEY_B) && (pressed & KEY_R) &&
        !box_state && !(held & (KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN))) {
        reset_selection_gestures();
        select_combat_group();
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].raw_type == SC64_P4_RAW_TASSADAR ||
                u[i].raw_type == SC64_P4_RAW_RAYNOR_VULTURE) u[i].selected = 0;
        mode = MODE_NORMAL;
        menu_open = 0;
        pressed &= (u16)~(KEY_A | KEY_B | KEY_R);
    }
#endif
#ifdef SC64_Z6_STORM_ENABLED
    /* Authored GBA army shortcut for this escort mission. The sole selected
     * Kerrigan spell hotkey above takes precedence; regular R includes her. */
    if (race_z6_active() && (held & KEY_B) && (pressed & KEY_R) &&
        !box_state && !(held & (KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN))) {
        reset_selection_gestures();
        select_combat_group();
        for (int i=0;i<MAX_UNITS;++i)
            if (u[i].type==U_ZP_INFESTED_KERRIGAN)
                u[i].selected=0;
        mode=MODE_NORMAL;
        menu_open=0;
        pressed&=(u16)~(KEY_A|KEY_B|KEY_R);
    }
#endif
    if (menu_open) reset_selection_gestures();
#ifdef SC_COMMAND_RING
    if (command_ring_input(held, pressed)) return;
#endif
    if ((pressed & KEY_B) && !box_state && !(pressed & KEY_L)) {
	if (menu_open)
	    menu_open = 0;
	else if (mode != MODE_NORMAL) {
	    mode = MODE_NORMAL;
	    menu_open = 1;
	} else {
	    menu_open = 1;
#ifdef SC_FIELD_FEEDBACK
	    menu_cursor = menu_last; /* B, A repeats the last command */
#ifdef SC64_RACES
	    if (menu_cursor >= field_menu_count()) menu_cursor = 0; /* row left with its unit */
#endif
#else
	    menu_cursor = 0;
#endif
	}
    }
    if (menu_open) {
	reset_selection_gestures();
#ifdef SC64_COMMAND_ICONS
	if (!field_card_navigate(pressed))
#endif
	{
	    if (pressed & KEY_UP)
		menu_cursor = (menu_cursor + field_menu_count() - 1) % field_menu_count();
	    if (pressed & KEY_DOWN)
		menu_cursor = (menu_cursor + 1) % field_menu_count();
	}
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
#ifdef SC_FIELD_FEEDBACK
    if (field_ui_r_tap(held, pressed)
#else
    if ((pressed & KEY_R)
#endif
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
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) ++sc64_showcase_ticks;
#endif
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (race_protoss_services_active()) sc64_p7_arbiter_cache_frame_tick();
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    race_p7_carrier_training_frame_tick();
#endif
#ifdef SC64_P6_COMBAT_ENABLED
    (void)race_p6_combat_burrow_release_tick();
#endif
#ifdef SC64_IMPORTED_MISSION
    profile_start = gba_clock();
#endif
#ifdef SC64_NYDUS_ENABLED
    race_nydus_tick_world();
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    race_bc_yamato_sync_references();
#endif
    update_orders();
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    if (sc64_battlecruiser_source_tick_due())
        for (int i=0;i<SC64_BC_SHOT_LIMIT;++i)
            if (sc64_bc_shots[i].state==SC64_BC_SHOT_FLIGHT)
                (void)race_bc_yamato_projectile_step(i);
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    race_p7_projectile_tick_if_due();
#endif
#ifdef SC64_TRANSPORT_RUNTIME
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
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    race_bc_facility_flight_tick();
#endif
    update_production();
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_UPDATE)
    sc64_update_production_cycles = gba_clock() - sc64_update_stage_start;
    sc64_update_stage_start = gba_clock();
#endif
#ifdef SC64_RACES
    race_tick_state();
#ifdef SC64_P6_COMBAT_ENABLED
    (void)race_p6_combat_tick_traps();
#endif
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
#endif
#ifdef SC64_TRANSPORT_RUNTIME
    race_transport_tick_after_orders();
#endif
#ifdef SC64_T6_RUNTIME
    race_t6_tech_tick();
#endif
#if defined(SC64_IMPORTED_MISSION) && defined(SC64_DIAGNOSTIC_UPDATE)
    sc64_update_research_cycles = gba_clock() - sc64_update_stage_start;
    sc64_update_remainder_start = gba_clock();
#endif
#ifndef SC64_IMPORTED_MISSION
    if (frame % 90 == 0) {
	int i;
	for (i = 0; i < MAX_UNITS; i++)
	    if (u[i].alive && u[i].owner == SIDE && IS_WORKER(u[i].type) && u[i].hp < u[i].maxhp)
		u[i].hp++;
    }
#endif
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
#ifdef SC64_P7_PROJECTILES_ENABLED
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    race_p7_carrier_training_end_frame();
#endif
    if (race_protoss_services_active()) race_p7_projectile_clear_dead_references();
#endif
#ifdef SC64_IMPORTED_MISSION
    sc64_sight_cycles = gba_clock() - profile_sight_start;
#endif
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) sc64_showcase_reveal();
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
#ifdef SC64_Z5_ACTORS
    int raw = sc64_race_raw_unit(type);
    /* Z5 actor enums share raw identities with T4 actors, but retain their
     * own selected source art. Keep the raw T4 enums on the generic path. */
    if (type >= U_Z5_GHOST && type <= U_Z5_DATA_DISK && raw >= 0) {
        const Sc64SpriteFrame *z5 = sc64_z5_actor_sprite((unsigned short)raw);
        if (z5) return z5;
    }
    return raw >= 0 ? sc64_p8_unit_sprite_for_raw((unsigned short)raw) : 0;
#else
    return sc64_p8_unit_sprite_for_raw(sc64_race_raw_unit(type));
#endif
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

#ifdef SC64_RACES
static const Sc64SpriteFrame *
sc64_race_building_art_sprite(int raw)
{
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    /* ZP reuses several stock raw identities for buildings with distinct
       campaign art (including Queen's Nest and Ultralisk Cavern). Prefer the
       profile's selected building frame bank before the generic unit atlas. */
    const Sc64SpriteFrame *zp_sprite =
        raw >= 0 ? sc64_race_building_sprite((unsigned short)raw, 0) : 0;
    if (zp_sprite) return zp_sprite;
#ifdef SC64_BATTLECRUISER_EXPERIMENT
    if (raw==116 || raw==118) {
        static Sc64SpriteFrame bc_body;
        extern int sc64_bc_body_art(unsigned,short *,short *,unsigned short *,
                                    unsigned short *,unsigned short *,unsigned short *,
                                    const unsigned char **);
        if (sc64_bc_body_art((unsigned)raw,&bc_body.offset_x,&bc_body.offset_y,
                             &bc_body.w,&bc_body.h,&bc_body.canvas_w,&bc_body.canvas_h,
                             &bc_body.pixels)) return &bc_body;
    }
#endif
#endif
    const Sc64SpriteFrame *sprite =
        raw >= 0 ? sc64_p8_unit_sprite_for_raw((unsigned short)raw) : 0;
    if (sprite) return sprite;
    switch (raw) {
    /* These late Terran structures have dedicated source building frames,
       rather than entries in the ordinary unit-sprite table. */
    case 108: case 117: case 120: case 123: case 127:
    case 116: case 118: case 171: case 172:
        return sc64_race_building_sprite((unsigned short)raw, 0);
    default:
        return 0;
    }
}
#endif

static const Sc64SpriteFrame *
mission_building_sprite(int type)
{
#ifdef SC64_RACES
    return sc64_race_building_art_sprite(sc64_race_raw_building(type));
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

static const Sc64SpriteFrame *
mission_geyser_sprite(void)
{
#ifdef SC64_RACES
    return sc64_p8_unit_sprite_for_raw(188);
#else
    return &sc64_sprite_geyser;
#endif
}

static void
draw_mission_sprite(const Sc64SpriteFrame *s, int cx, int cy)
{
    gfx_blit_sprite(cx - s->canvas_w / 2 + s->offset_x,
	cy - s->canvas_h / 2 + s->offset_y, s->w, s->h, s->pixels);
}
#ifdef SC64_CONSTRUCTION_SOURCE_ART
static int
draw_construction_image(unsigned short image_id, unsigned short frame_index,
                        const Sc64SpriteFrame *fallback, int cx, int cy)
{
    unsigned rows = 0;
    const SC64ConstructionModifierFrame *modifier_frame =
        sc64_construction_modifier_frame(image_id, frame_index);
    const unsigned char *lookup =
        sc64_construction_modifier_lookup(image_id, &rows);
    if (sc64_construction_image_modifier(image_id) == 9u &&
        sc64_construction_image_color_shift(image_id) == 3u &&
        modifier_frame && lookup && rows == 40u) {
        int x = cx - modifier_frame->canvas_w / 2 + modifier_frame->x;
        int y = cy - modifier_frame->canvas_h / 2 + modifier_frame->y;
        int top = 0, bottom = modifier_frame->h;
        /* Map art always stays between the HUD and command bar, even in
         * profiles that don't enable the general scene-clip API. */
        if (y + top < 12) top = 12 - y;
        if (y + bottom > 120) bottom = 120 - y;
#ifdef GBA_SCENE_CLIP
        unsigned short clip = gfx_get_clip_y();
        int clip_top = clip & 255, clip_bottom = clip >> 8;
        if (y + top < clip_top) top = clip_top - y;
        if (y + bottom > clip_bottom) bottom = clip_bottom - y;
#endif
        if (top < 0) top = 0;
        if (bottom > modifier_frame->h) bottom = modifier_frame->h;
        if (top < bottom)
            gfx_blit_sprite_mapped(x, y + top, modifier_frame->w, bottom - top,
                                   modifier_frame->strengths + top * modifier_frame->w,
                                   lookup, rows);
        return 1;
    }
    if (fallback) draw_mission_sprite(fallback, cx, cy);
    return fallback != 0;
}
#endif
#endif

/* Use the same footprint in the painter and its cached-object key. Geyser
 * art can remain visible when its center has left the playfield. */
static int
geyser_in_view(int x, int y)
{
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite = mission_geyser_sprite();
    if (sprite) {
        int left = x - sprite->canvas_w / 2 + sprite->offset_x;
        int top = y - sprite->canvas_h / 2 + sprite->offset_y;
        return left < 240 && left + sprite->w > 0 &&
               top < 120 && top + sprite->h > 12;
    }
#endif
    return x > 0 && x < 240 && y > 12 && y < 120;
}

/* Presentation queries the canonical shield capacity, independently of HP.
 * In particular an Archon's tiny HP pool must not scale its large shields. */
static int __attribute__((noinline))
sc64_actor_max_shields(int type,int building)
{
#ifdef SC64_RACES
    if (building) {
        if (type<0 || type>=SC_RB_COUNT) return 0;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
        const Sc64ZpBuildingFact *fact=sc64_zp_building_fact(type);
        if (fact) return fact->shields;
#endif
        return sc64_race_building_shields(type);
    }
    if (type<0 || type>=SC_RU_COUNT) return 0;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const Sc64ZpActorFact *fact=sc64_zp_actor_fact(type);
    if (fact) return fact->shields;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    const Sc64TerranLateActorFact *late=sc64_terran_late_unit_fact(type);
    if (late) return late->shields;
#endif
    return sc64_race_unit_rules[type].shields;
#else
    (void)type; (void)building; return 0;
#endif
}

/* Caster energy in Q8, where a campaign adapter persists it; 0 otherwise. */
static int
sc64_unit_energy(const Unit *q,int *maximum)
{
    *maximum=0;
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    if (sc64_race_is_battlecruiser(q->type)) {
        int key=sc64_battlecruiser_owner_key(race_mode, race_selected_mission,q->owner,q->raw_owner);
        int reactor=key>=0 && sc64_battlecruiser_tech_has(
            sc64_battlecruiser_tech_bits,(unsigned)key,SC64_BC_TECH_REACTOR);
        *maximum=sc64_battlecruiser_energy_cap(reactor);
    }
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
    if (race_z6_cloak_actor(q)) *maximum=SC64_Z5_CLOAK_MAX_ENERGY_Q8;
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
    if (race_z5_cloak_actor(q)) *maximum=SC64_Z5_CLOAK_MAX_ENERGY_Q8;
#endif
#ifdef SC64_P6_STORM_ENABLED
    if (race_p6_active() && q->raw_type==79 && q->raw_owner==2)
        *maximum=SC64_STORM_HERO_MAX_ENERGY*SC64_STORM_ENERGY_ONE;
#endif
#ifdef SC64_Z6_STORM_ENABLED
    if (race_z6_active() && q->raw_type==51 && q->raw_owner==3)
        *maximum=SC64_STORM_HERO_MAX_ENERGY*SC64_STORM_ENERGY_ONE;
#endif
#ifdef SC64_P4_STORM_ENABLED
    if (race_p4_active() && q->owner==SIDE &&
        (q->raw_type==67 || q->raw_type==SC64_P4_RAW_TASSADAR))
        *maximum=(q->raw_type==67 ? SC64_STORM_MAX_ENERGY :
                  SC64_STORM_HERO_MAX_ENERGY)*SC64_STORM_ENERGY_ONE;
#endif
    return *maximum ? q->carry : 0;
}

/* StarCraft's selected-unit vitals, after OpenBW's draw_health_bars: a black
 * frame of 2-pixel cells split by 1-pixel dividers and filled in whole cells.
 * Shields stack above HP and energy sits below. Each strip keeps two shade
 * rows instead of three to suit the GBA sprite scale. Battlefield rows only. */
static void
vital_rect(int x,int y,int w,int h,u8 color)
{
    if (y<12) { h-=12-y; y=12; }
    if (y+h>120) h=120-y;
    gfx_rect(x,y,w,h,color);
}

static int
vital_fill(int value,int maximum,int width)
{
    if (value<=0 || maximum<=0) return 0;
    if (value>=maximum) return width;
    int r=(int)((u32)value*100u/(unsigned)maximum)*width/100;
    if (r<3) return 3;
    return r%3>1 ? r+3-r%3 : r-r%3;
}

static void
vital_strip(int left,int top,int width,int fill,u8 light,u8 dark)
{
    vital_rect(left,top,fill,1,light);
    vital_rect(left,top+1,fill,1,dark);
    vital_rect(left+fill,top,width-fill,1,C_STONE);
    vital_rect(left+fill,top+1,width-fill,1,C_UI);
}

static void __attribute__((noinline))
draw_actor_vitals(int x,int top,int width,int hp,int maxhp,
                   int shield,int maxshield,int energy,int maxenergy)
{
    width-=(width-1)%3;
    if (width<13) width=13;
    if (maxhp<=0) return;
    int left=x-width/2,hp_top=top+(maxshield>0?3:0),bottom=hp_top+4;
    int percent=hp>0?(int)((u32)hp*100u/(unsigned)maxhp):0;
    vital_rect(left,top,width,bottom-top,C_BLACK);
    if (maxshield>0)
        vital_strip(left,top+1,width,vital_fill(shield,maxshield,width),
                    C_BLUE,C_BLUE_DARK);
    vital_strip(left,hp_top+1,width,vital_fill(hp,maxhp,width),
                percent>=66 ? C_GREEN : percent>=33 ? C_GOLD : C_RED,
                percent>=66 ? C_GREEN_DARK : percent>=33 ? C_GOLD_DARK : C_RED_DARK);
    if (maxenergy>0) {
        vital_rect(left,bottom+1,width,4,C_BLACK);
        vital_strip(left,bottom+2,width,vital_fill(energy,maxenergy,width),
                    C_PURPLE,C_ZERG);
    }
    for (int dx=0;dx<width;dx+=3) {
        vital_rect(left+dx,top+1,1,bottom-top-2,C_BLACK);
        if (maxenergy>0) vital_rect(left+dx,bottom+2,1,2,C_BLACK);
    }
}

/* A compact UI cue for carried resources. The stock unit bank has no cargo
 * attachment frames, so this deliberately stays separate from source art. */
static void
draw_worker_cargo_badge(int x, int y, int gas)
{
    if (gas) {
        static const u8 widths[5] = {3,5,5,5,3};
        for (int row = 0; row < 5; ++row) {
            int py = y + row - 2;
            if (py < 12 || py >= 120) continue;
            int width = widths[row];
            gfx_rect(x - width / 2, py, width, 1,
                     row == 0 || row == 4 ? C_GREEN_DARK : C_GREEN);
        }
        if (y - 1 >= 12 && y - 1 < 120)
            gfx_pixel(x - 1, y - 1, C_WHITE);
    } else {
        static const u8 widths[7] = {1,3,5,5,3,2,1};
        static const u8 colors[7] = {
            C_CYAN,C_BLUE,C_BLUE_DARK,C_BLUE,C_BLUE_DARK,C_BLUE_DARK,C_CYAN
        };
        for (int row = 0; row < 7; ++row) {
            int py = y + row - 3;
            if (py < 12 || py >= 120) continue;
            int width = widths[row];
            gfx_rect(x - width / 2, py, width, 1, colors[row]);
        }
        if (y - 2 >= 12 && y - 2 < 120)
            gfx_pixel(x, y - 2, C_WHITE);
    }
}

#if defined(SC64_WEAPON_SOURCE_VFX) && defined(SC64_UNIT_ATTACK_POSES_ENABLED)
static const Sc64SpriteFrame *
sc64_source_unit_attack_pose(const Unit *q)
{
    int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    int slot=(int)(q-u)+1, newest=0x7fffffff;
    const Sc64SpriteFrame *pose=0;
    if(!q->cd) return 0;
    for(int i=0;i<MAX_TRACERS;++i) {
        const Tracer *t=&tracers[i]; Sc64WeaponSourceVfxKey key;
        if(t->life<=0 || !sc64_weapon_source_vfx_unpack((uint32_t)t->color,&key) ||
           key.actor_slot_plus1!=slot || key.actor_raw!=raw ||
           q->x!=t->x0 || q->y!=t->y0) continue;
        int age=sc64_tracer_duration(i)-t->life;
        if(age<0) age=0;
        if(age>=newest) continue;
        const Sc64SpriteFrame *candidate=sc64_unit_attack_source_full_frame(
            (unsigned short)raw,key.target_air,key.direction,
            sc64_weapon_source_vfx_source_tick((unsigned)age));
        if(candidate) {pose=candidate;newest=age;}
    }
    return pose;
}
#endif

static void
draw_unit(Unit * q)
{
    if (SC64_TRANSPORT_LOADED(q)) return;
#ifdef SC64_P6_COMBAT_ENABLED
    if (race_p6_combat_unit_burrowed((int)(q - u))) return;
#endif
    int x = q->x - camx, y = q->y - camy + 12;
    int col = q->owner == SIDE ? C_TERRAN : C_ZERG;
    if (x < -8 || x > 247 || y < 10 || y > 125)
	return;
    if (q->owner != SIDE && !currently_visible(q->x, q->y))
	return;
#ifdef SC64_P7_OBSERVER_ENABLED
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (q->owner != SIDE &&
        !sc64_p7_observer_target_revealed_to((int)(q-u),7,SIDE)) return;
#else
    if (race_p7_active() && q->owner != SIDE && q->raw_type == 84 &&
        !sc64_p7_observer_detector_sees((int)(q-u),7,SIDE)) return;
#endif
#endif
#ifdef SC64_Z8_RUNTIME
    if (q->owner != SIDE && race_z8_unit_hidden_from_side((int)(q - u))) return;
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active() && race_t6_unit_burrowed((int)(q - u))) return;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    unsigned actor_flags = sc64_game_unit_actor_flags(q->type);
    if ((actor_flags & SC64_GAME_ACTOR_HIDDEN) || SC64_CARRIER_STATE_HIDDEN(q)) return;
    if (actor_flags && !(actor_flags & SC64_GAME_ACTOR_DRAWABLE)) return;
#endif
    int walk = sc64_walk_track(q);
    int body_y = y - SC64_WALK_LIFT(walk);
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite =
#ifdef SC64_RACES
        q->raw_type>=0?sc64_p8_unit_sprite_for_raw(q->raw_type):
#endif
        mission_unit_sprite(q->type);
#ifdef SC64_UNIT_FACING
    int facing_raw = q->raw_type >= 0 ? q->raw_type :
        sc64_race_raw_unit(q->type);
    /* Units missing from the race bank (e.g. the sieged Siege Tank) can still
     * have per-direction art; its north frame also sizes the selection box. */
    if (!sprite && facing_raw >= 0)
        sprite = sc64_unit_facing_frames((unsigned short)facing_raw);
#endif
#if defined(SC64_WEAPON_SOURCE_VFX) && defined(SC64_UNIT_ATTACK_POSES_ENABLED)
    const Sc64SpriteFrame *attack_pose=sc64_source_unit_attack_pose(q);
    if(attack_pose) sprite=attack_pose;
#endif
    int sprite_x = sprite ? x - sprite->canvas_w / 2 + sprite->offset_x : x - 4;
    int sprite_y = sprite ? y - sprite->canvas_h / 2 + sprite->offset_y : y - 4;
    int sprite_w = sprite ? sprite->w : 8;
    int sprite_h = sprite ? sprite->h : 8;
#else
    int sprite_x = x - 4, sprite_y = y - 4, sprite_w = 8, sprite_h = 8;
#endif
#ifdef SC64_HAS_SPRITES
#ifdef SC64_UNIT_FACING
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    if (sc64_race_is_battlecruiser(q->type)) SC64_FACING_SET((int)(q-u),sc64_bc_heading_face((u8)((unsigned)q->work_timer>>3)));
    else
#endif
    sc64_facing_track(q);
#if defined(SC64_WEAPON_SOURCE_VFX) && defined(SC64_UNIT_ATTACK_POSES_ENABLED)
    if(attack_pose) draw_mission_sprite(attack_pose,x,body_y);
    else
#endif
    if (sprite && sc64_facing_draw(q, facing_raw, x, body_y)) {}
    else
#endif
    if (sprite) {
#if defined(SC64_UNIT_WORD_SPRITES) && defined(SC64_RACE_UNIT_WORD_SPRITE_METADATA)
        int sprite_raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
        const GbaSpriteWordFrame *word_sprite = sprite_raw >= 0 ?
            sc64_race_unit_words_sprite((unsigned short)sprite_raw,sprite) : 0;
        if (word_sprite)
#ifdef SC64_UNIT_WORD_SPRITE_DIAGNOSTICS
        {
            if(q->raw_type>=0)++sc64_unit_words_explicit_raw_calls;
            else ++sc64_unit_words_derived_raw_calls;
#endif
            gfx_blit_sprite_words(sprite_x,sprite_y+body_y-y,word_sprite);
#ifdef SC64_UNIT_WORD_SPRITE_DIAGNOSTICS
        }
#endif
        else
            draw_mission_sprite(sprite,x,body_y);
#else
        draw_mission_sprite(sprite,x,body_y);
#endif
    }
    else
#endif
    {
    /* Legs stay on the ground while the body rises between footfalls. */
    int feet = y;
    y = body_y;
    if (IS_WORKER(q->type)) {
	gfx_rect(x - 3, y - 2, 7, 5, col);
	gfx_pixel(x + 2, y - 3, C_SILVER);
	gfx_pixel(x - 3, y + 3, C_GOLD);
	if (walk) sc64_walk_treads(x, feet + 3, walk, C_SHADOW);
    } else if (q->type == U_MARINE) {
	gfx_rect(x - 2, y - 4, 5, 8, col);
	gfx_rect(x - 3, y - 5, 6, 2, C_SILVER);
	gfx_line(x + 1, y, x + 5, y - 2, C_SILVER);
	if (walk) sc64_walk_legs(x, feet + 4, walk, C_SILVER);
    } else if (q->type == U_FIREBAT) {
	gfx_rect(x - 3, y - 4, 7, 8, col);
	gfx_rect(x - 4, y - 5, 8, 2, C_ORANGE);
	gfx_pixel(x + 5, y, C_ORANGE);
	if (walk) sc64_walk_legs(x, feet + 4, walk, C_ORANGE);
    } else if (q->type == U_MEDIC) {
	gfx_rect(x - 3, y - 4, 7, 8, C_WHITE);
	gfx_pixel(x, y - 1, C_RED);
	gfx_pixel(x, y, C_RED);
	gfx_pixel(x, y + 1, C_RED);
	if (walk) sc64_walk_legs(x, feet + 4, walk, C_SILVER);
    } else if (q->type == U_ZERGLING) {
	gfx_diamond(x, y, 5, 3, col);
	gfx_line(x - 3, y - 1, x - 6, y - 4, C_BLOOD);
	gfx_line(x + 3, y - 1, x + 6, y - 4, C_BLOOD);
	if (walk) sc64_walk_scuttle(x, feet + 3, 4, walk, C_BLOOD);
	} else if (q->type == U_RAYNOR) {
	gfx_diamond(x, y, 6, 5, C_GOLD);
	gfx_rect(x - 2, y - 5, 5, 3, C_SILVER);
	gfx_pixel(x, y, C_WHITE);
	/* The hover bike has no legs; its exhaust flickers instead. */
	if (walk) gfx_pixel(x, y + 6, (walk & 1) ? C_ORANGE : C_GOLD);
    } else {
	gfx_diamond(x, y, 5, 4, col);
	gfx_pixel(x - 4, y - 3, C_PURPLE);
	gfx_pixel(x + 4, y - 3, C_PURPLE);
	if (walk) sc64_walk_scuttle(x, feet + 4, 4, walk, C_PURPLE);
    }
    y = feet;
    }
    if (q->carry > 0 && IS_WORKER(q->type) &&
        (q->order == ORDER_MINE || q->order == ORDER_GAS))
        draw_worker_cargo_badge(sprite_x + sprite_w + 2,
            sprite_y + sprite_h / 2, q->order == ORDER_GAS);
    if (!q->selected) return;
    gfx_frame(sprite_x - 2, sprite_y - 2, sprite_w + 4, sprite_h + 4, C_HIGHLIGHT);
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (actor_flags & SC64_GAME_ACTOR_INVINCIBLE) return;
#endif
    int maxenergy, energy = sc64_unit_energy(q, &maxenergy);
    draw_actor_vitals(x, sprite_y + sprite_h + 3, sprite_w + 4, q->hp, q->maxhp,
#ifdef SC64_RACES
        q->shield,
#else
        0,
#endif
        sc64_actor_max_shields(q->type, 0), energy, maxenergy);
}

#define SC64_WEAPON_VFX_VISIBLE(x,y) currently_visible((x)+camx,(y)+camy-12)
#include "sc64_weapon_vfx_render.h"
#undef SC64_WEAPON_VFX_VISIBLE
#ifdef SC64_TANK_SOURCE_VFX
#include "sc64_tank_source_vfx_render.h"
#endif

#ifdef SC64_WEAPON_SOURCE_VFX
#include "sc64_weapon_source_vfx_render.h"
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
#include "sc64_yamato_raster_adapter.h"
#endif
#endif


static void
draw_tracers(void)
{
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
    u32 diag_start=gba_clock();
    diag_lookup=diag_cull=diag_raster=diag_motion=diag_actor=diag_projectile=diag_children=0;
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
    for (int i=0;i<SC64_BC_SHOT_LIMIT;++i) {
        const Sc64BcYamatoShot *shot=&sc64_bc_shots[i];
        if (shot->state==SC64_BC_SHOT_CHARGING && shot->caster<MAX_UNITS && u[shot->caster].alive) {
            int f=sc64_yamato_raster_charge_frame(shot->age);
            if (f>=0) sc64_yamato_raster_draw(543,(unsigned)f,0,u[shot->caster].x,u[shot->caster].y);
        } else if (shot->state==SC64_BC_SHOT_FLIGHT) {
            Sc64BcMotionFields m;
            if (sc64_bc_motion_unpack(shot,&m))
                sc64_yamato_raster_draw(541,0,(((unsigned)m.heading+4u)>>3)&31u,(int)(m.x_q8/512u),(int)(m.y_q8/512u));
        }
    }
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && sc64_race_is_battlecruiser(u[i].type) &&
        u[i].order==ORDER_ATTACK_TARGET && (u[i].work_timer&7)>0 && (u[i].work_timer&7)<=6) {
        int f=sc64_yamato_raster_charge_frame(50u-((unsigned)u[i].work_timer&7u));
        if (f>=0) sc64_yamato_raster_draw(543,(unsigned)f,0,u[i].x,u[i].y);
    }
#endif
    for (int i=0;i<MAX_TRACERS;++i) if (tracers[i].life) {
#ifdef SC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT
        int ys=sc64_weapon_vfx_style(tracers[i].color);
        if (ys==SC64_YAMATO_TRAIL_STYLE || ys==SC64_YAMATO_IMPACT_STYLE) {
            int age=sc64_tracer_duration(i)-tracers[i].life;
            unsigned p=(unsigned)tracers[i].color&7u;
            unsigned source_age=(unsigned)(age<=0?0:age-1);
            source_age-= (source_age+p)/5u;
            if (ys==SC64_YAMATO_TRAIL_STYLE && source_age<6u)
                sc64_yamato_raster_draw(542,(source_age/2u)*17u,
                                         ((unsigned)tracers[i].color>>3)&31u,tracers[i].x0,tracers[i].y0);
            if (ys==SC64_YAMATO_IMPACT_STYLE && source_age<28u)
                sc64_yamato_raster_draw(544,source_age/2u,0,tracers[i].x0,tracers[i].y0);
            continue;
        }
#endif
#ifdef SC64_WEAPON_SOURCE_VFX
        if(sc64_weapon_source_vfx_is_tagged((uint32_t)tracers[i].color)) {
            uint32_t packed=(uint32_t)tracers[i].color;
            Sc64WeaponSourceVfxKey key;
            if (!sc64_weapon_source_vfx_unpack(packed,&key)) continue;
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
            u32 dt=gba_clock();
#endif
            const Sc64WeaponSourceAttackProfile *profile=0;
#if defined(SC64_TRACE_PROFILE_INDEX_CACHE) && defined(SC64_TRACE_DURATION_CACHE)
            unsigned cached=sc64_trace_durations[i]>>8;
            profile=sc64_trace_cached_profile(cached,key);
#endif
            if(!profile) profile=sc64_weapon_source_profile_lookup(
                key.actor_raw,key.weapon_id,key.target_air,key.variant);
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
            diag_lookup+=gba_clock()-dt; dt=gba_clock();
#endif
#ifdef SC64_WEAPON_SOURCE_VFX_OFFSCREEN_CULL
            if (!sc64_weapon_source_vfx_anchor_may_be_visible(
                    tracers[i].x0, tracers[i].y0, camx, camy) &&
                !sc64_weapon_source_vfx_anchor_may_be_visible(
                    tracers[i].x1, tracers[i].y1, camx, camy) &&
                sc64_weapon_source_vfx_trace_provably_offscreen_resolved(
                    key,profile,
                    tracers[i].x0, tracers[i].y0, tracers[i].x1, tracers[i].y1,
                    camx, camy)) {
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
                diag_cull+=gba_clock()-dt;
#endif
                continue;
            }
#endif
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
            diag_cull+=gba_clock()-dt; dt=gba_clock();
#endif
            int age=sc64_tracer_duration(i)-tracers[i].life;
            sc64_weapon_source_vfx_draw_trace_resolved(key,profile,
                tracers[i].x0,tracers[i].y0,tracers[i].x1,tracers[i].y1,
                tracers[i].life,(unsigned)(age<0?0:age));
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
            diag_raster+=gba_clock()-dt;
#endif
            continue;
        }
#endif
        int style=sc64_weapon_vfx_style(tracers[i].color);
        /* Legacy original-art saves may contain an untagged tracer color. */
        if (!style) style=SC64_WEAPON_VFX_STYLE_MUZZLE;
#ifdef SC64_TANK_SOURCE_VFX
        if (style==SC64_TANK_SOURCE_TANK || style==SC64_TANK_SOURCE_SIEGE) {
            sc64_private_tank_vfx_draw(style,sc64_weapon_vfx_color(tracers[i].color),
                tracers[i].x0,tracers[i].y0,tracers[i].x1,tracers[i].y1,tracers[i].life);
            continue;
        }
#endif
        sc64_weapon_vfx_draw(style,sc64_weapon_vfx_color(tracers[i].color),
            tracers[i].x0-camx,tracers[i].y0-camy+12,
            tracers[i].x1-camx,tracers[i].y1-camy+12,tracers[i].life);
    }
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
    diag_tracer_cycles=gba_clock()-diag_start;
#endif
}

#ifndef SC64_DEATH_VFX_DISABLED
#include "sc64_death_vfx_render.h"
#ifdef SC64_DEATH_SOURCE_ART
/* Clip source frames to the battlefield rows above the cached lower HUD. */
static void
death_vfx_blit(const Sc64SpriteFrame *s, int cx, int cy)
{
    int x = cx - s->canvas_w / 2 + s->offset_x;
    int y = cy - s->canvas_h / 2 + s->offset_y, h = s->h;
    const u8 *pixels = s->pixels;
    if (y < 12) {
        int skip = 12 - y;
        if (skip >= h) return;
        pixels += skip * s->w; h -= skip; y = 12;
    }
    if (y + h > 120) h = 120 - y;
    if (h > 0) gfx_blit_sprite(x, y, s->w, h, pixels);
}

static int
death_vfx_draw_source(const Sc64DeathVfx *e, int x, int y, u32 age, int under)
{
    const Sc64DeathSource *art = sc64_death_source(e->raw);
    if (!sc64_death_source_valid(art)) return 0;
    for (unsigned i = 0; i < art->layer_count; ++i) {
        const Sc64DeathSourceLayer *layer = &art->layers[i];
        unsigned frame_index;
        if (layer->under != under ||
            !sc64_death_source_layer_frame(layer, age, &frame_index)) continue;
        const Sc64SpriteFrame *sprite =
            sc64_death_image_frame(layer->image_id, (unsigned short)frame_index);
        if (sprite) {
#if defined(SC64_UNIT_WORD_SPRITES) && !defined(SC64_DEATH_INDEXED_REMNANTS)
            const GbaSpriteWordFrame *words =
                sc64_death_image_word_frame(layer->image_id,(unsigned short)frame_index);
            int left=x+layer->x-sprite->canvas_w/2+sprite->offset_x;
            int top=y+layer->y-sprite->canvas_h/2+sprite->offset_y;
            /* Packed collapse/remnant frames use the word compositor and no extra
             * RAM. The indexed fallback explicitly clips partial HUD-edge
             * frames even in builds without the scene-clip API. */
            if (words && top>=12 && top+(int)sprite->h<=120)
                gfx_blit_sprite_words(left,top,words);
            else
#endif
                death_vfx_blit(sprite, x + layer->x, y + layer->y);
        }
    }
    return 1;
}
#endif

/* under=1 draws remnants and collapse frames beneath units; under=0 draws
 * explosions above them. Effects in unseen cells stay hidden, as units do. */
static void __attribute__((noinline))
draw_death_vfx(int under)
{
    u32 now = death_vfx_now();
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i) {
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
        Sc64DeathVfx e;
        death_vfx_test_load_row(i, &e);
#else
        const Sc64DeathVfx *row = &death_vfx[i];
#endif
#ifdef SC64_DEATH_VFX_VRAM_STORAGE
        const Sc64DeathVfx *row = &e;
#endif
        if (!row->style) continue;
        u32 age = sc64_death_vfx_age_su(row->birth, now);
        if (age >= row->total_su) continue;
        /* Skip a pass this effect has nothing for before the fog lookup:
         * authored remnants start late, and bursts end early. Source
         * collapse frames draw beneath units from the start. */
        if (under ? !row->source && age < row->remnant_start : age >= row->burst_su) continue;
        int x = row->x - camx, y = row->y - camy + 12;
        if (x < -64 || x > 304 || y < -52 || y > 172 || !currently_visible(row->x, row->y)) continue;
#ifdef SC64_DEATH_SOURCE_ART
        if (row->source && death_vfx_draw_source(row, x, y, age, under)) continue;
#endif
        if (under) sc64_death_vfx_draw_remnant(row, x, y, age);
        else sc64_death_vfx_draw_burst(row, x, y, age);
    }
}
#endif

#ifdef SC64_IMPORTED_MISSION
static void sc64_hud_bottom_band_invalidate(void);
#endif

static void
sc64_minimap_dot(int x, int y, u8 color)
{
    gfx_pixel(x, y, color);
#ifdef SC64_MINIMAP_RASTER_ACTIVE
    /* A mark outside the 24x24 map lands on the cached panel, which only a
     * band restore clears; the per-frame panel fill used to do that. */
    if ((unsigned)(x - 197) >= 24u || (unsigned)(y - 129) >= 24u)
        sc64_hud_bottom_band_invalidate();
#endif
}

static void
draw_minimap(void)
{
#ifdef SC64_IMPORTED_MISSION
    int x, y, i;
    const int map_x = 197, map_y = 129, map_size = 24;
#else
    int row, col, x, y, i;
#endif
#ifndef SC64_MINIMAP_RASTER_ACTIVE
    gfx_rect(181, 124, 56, 34, C_BLACK);
    gfx_frame(181, 124, 56, 34, C_EDGE);
#endif
#ifdef SC64_IMPORTED_MISSION
#ifdef SC64_MINIMAP_RASTER_ACTIVE
    /* Every map pixel is replaced, including marks left on this page. */
    gfx_blit_opaque(map_x, map_y, 24, 24, sc64_minimap_raster);
#else
#ifdef SC64_TILE_RENDERER
    if (gfx_tile_active()) {
        /* The panel fill above already marked these tiles; store the same
         * colors straight into the linear software layer. */
        u8 *panel = gfx_tile_buffer_pixels() + map_y * 240 + map_x;
        for (int row = 0; row < 24; row++, panel += 240)
            for (int col = 0; col < 24; col++) {
                int cell = row * 24 + col;
                if (sc64_minimap_visible[cell]) panel[col] = C_GRASS;
                else if (sc64_minimap_explored[cell]) panel[col] = C_FOG;
            }
    } else
#endif
    sc64_minimap_draw_terrain(map_x, map_y,
                              sc64_minimap_visible, sc64_minimap_explored);
#endif
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
    for (i = 0; i < MAX_BUILDINGS; i++) {
	if (!b[i].alive) continue;
	if (b[i].owner != SIDE &&
	    (b[i].x < 0 || b[i].y < 0 || b[i].x >= WORLD_W || b[i].y >= WORLD_H ||
	     !currently_visible(b[i].x, b[i].y))) continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (sc64_game_building_actor_flags(b[i].type) &&
            !(sc64_game_building_actor_flags(b[i].type) & SC64_GAME_ACTOR_DRAWABLE)) continue;
#endif
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
	    sc64_minimap_dot(x, y,
#if defined(SC64_SKIRMISH) || defined(SC64_RACES)
                owner_color(b[i].owner)
#else
                b[i].owner == SIDE ? C_TERRAN : C_RED
#endif
                );
	}
    for (i = 0; i < MAX_UNITS; i++)
	{
	if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i])) continue;
	if (u[i].owner != SIDE &&
	    (u[i].x < 0 || u[i].y < 0 || u[i].x >= WORLD_W || u[i].y >= WORLD_H ||
	     !currently_visible(u[i].x, u[i].y))) continue;
#ifdef SC64_P7_OBSERVER_ENABLED
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
        if (u[i].owner != SIDE &&
            !sc64_p7_observer_target_revealed_to(i,7,SIDE)) continue;
#else
        if (race_p7_active() && u[i].owner != SIDE && u[i].raw_type == 84 &&
            !sc64_p7_observer_detector_sees(i,7,SIDE)) continue;
#endif
#endif
#ifdef SC64_Z8_RUNTIME
        if (u[i].owner != SIDE && race_z8_unit_hidden_from_side(i)) continue;
#endif
#ifdef SC64_T6_RUNTIME
        if (race_t6_unit_burrowed(i)) continue;
#endif
#ifdef SC64_P6_COMBAT_ENABLED
        if (race_p6_combat_unit_burrowed(i)) continue;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (sc64_game_unit_has_flag(u[i].type, SC64_GAME_ACTOR_HIDDEN) ||
            SC64_CARRIER_STATE_HIDDEN(&u[i]) ||
            (sc64_game_unit_actor_flags(u[i].type) &&
             !(sc64_game_unit_actor_flags(u[i].type) & SC64_GAME_ACTOR_DRAWABLE))) continue;
#endif
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
	    sc64_minimap_dot(x, y,
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
#ifdef SC64_MINIMAP_RASTER_ACTIVE
	    if (vx < map_x || vy < map_y || vx + vw > map_x + map_size ||
	        vy + vh > map_y + map_size)
	        sc64_hud_bottom_band_invalidate();
#endif
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
    int x = q->x - camx, y = q->y - camy + 12;
    if (x < -20 || x > 260 || y < 2 || y > 130)
        return;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_building_actor_flags(q->type) &&
        !(sc64_game_building_actor_flags(q->type)&SC64_GAME_ACTOR_DRAWABLE)) return;
#endif
    int col = q->owner == SIDE ? C_TERRAN : C_ZERG;
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite =
#ifdef SC64_RACES
#ifdef SC64_T5_RUNTIME
#ifdef SC64_T6_RUNTIME
        q->type == B_CONTROL_TOWER && race_t6_tech_active() ? sc64_p8_unit_sprite_for_raw(115) :
#endif
        q->type == B_CONTROL_TOWER ? sc64_p8_unit_sprite_for_raw(sc64_race_t5_building_art_raw(q->type)) :
#endif
        q->raw_type>=0?sc64_race_building_art_sprite(q->raw_type):
#endif
        mission_building_sprite(q->type);
#endif
    if (q->owner != SIDE && !currently_visible(q->x, q->y))
	return;
#if defined(SC64_T4_SCENARIO_RUNTIME) && defined(SC64_HAS_SPRITES)
    if (race_t4_draw_prop(q, x, y)) return;
#endif
#if defined(SC64_Z5_RUNTIME) && defined(SC64_HAS_SPRITES)
    if (race_z5_draw_prop(q, x, y)) return;
#endif
#if defined(SC64_P6_RUNTIME) && defined(SC64_HAS_SPRITES)
    if (race_p6_draw_prop(q, x, y)) return;
#endif
#ifdef SC64_RACES
    /* Unfinished structures are drawn dynamically below, so cached terrain
       never freezes their construction effect or keeps their completed art. */
    if (q->under_construction) return;
#ifdef SC64_PHOTON_SOURCE_FRAMES
    /* Photon attack frames belong to the dynamic pass, not the terrain
     * cache. The cannon must also be drawn there while idle/unpowered. */
    if (q->type==B_PHOTON_CANNON) return;
#endif
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
    if (x < -128 || x > 368 || y < -128 || y > 248)
        return;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_building_has_flag(q->type, SC64_GAME_ACTOR_INVINCIBLE)) return;
#endif
    if (q->owner != SIDE && !currently_visible(q->x, q->y))
        return;
#ifdef SC64_HAS_SPRITES
    const Sc64SpriteFrame *sprite =
#ifdef SC64_RACES
        q->raw_type>=0?sc64_race_building_art_sprite(q->raw_type):
#endif
        mission_building_sprite(q->type);
    int sprite_x = sprite ? x - sprite->canvas_w / 2 + sprite->offset_x : x - 12;
    int sprite_y = sprite ? y - sprite->canvas_h / 2 + sprite->offset_y : y - 12;
    int sprite_w = sprite ? sprite->w : 24;
    int sprite_h = sprite ? sprite->h : 24;
#else
    int sprite_x = x - 12, sprite_y = y - 12, sprite_w = 24, sprite_h = 24;
#endif
#ifdef SC64_RACES
#if defined(SC64_PHOTON_SOURCE_FRAMES) && defined(SC64_HAS_SPRITES)
    if (!q->under_construction && q->type==B_PHOTON_CANNON) {
        unsigned elapsed=race_site_powered(q->owner,q->x,q->y) &&
                         q->queue_progress>0 && q->queue_progress<=28 ?
                         (unsigned)(28-q->queue_progress)*4u/5u : 24u;
        const Sc64SpriteFrame *cannon=sc64_race_building_sprite(162,
                                   sc64_photon_cannon_attack_frame(elapsed));
        if (cannon) draw_mission_sprite(cannon,x,y);
    }
#endif
#ifdef SC64_BUILDING_FIRE_ART
    if (!q->under_construction && q->hp>0 && q->hp<q->maxhp) {
        unsigned raw=(unsigned)(q->raw_type>=0?q->raw_type:sc64_race_raw_building(q->type));
        sc64_building_damage_draw(raw,x,y,(unsigned)q->hp,(unsigned)q->maxhp,
                                 (u32)frame/5u*4u+((u32)frame%5u)*4u/5u);
    }
#endif
    if (q->under_construction) {
        int duration=building_build_time(q->type);
        int progress=q->build_progress;
        int race=sc64_race_building_rules[q->type].race;
#ifdef SC64_CONSTRUCTION_SOURCE_ART
        int source_drawn = 0;
        unsigned source_stage, source_frame;
        uint32_t source_elapsed;
        if (sc64_construction_source_phase(race+1,progress,duration,
                                           &source_stage,&source_elapsed)) {
            int raw = q->raw_type >= 0 ? q->raw_type :
                      sc64_race_raw_building(q->type);
            const SC64ConstructionSourceStage *stage =
                sc64_construction_stage(raw,source_stage);
            if (stage && sc64_construction_animation_frame(stage->animation,
                                        source_elapsed,&source_frame)) {
                /* Source underlays must precede the main image. Each child
                 * has its own spawn clock; follow-main layers use the root
                 * frame instead. No extra saved actor state is required. */
                for (int pass=0;pass<2;++pass) {
                    if (pass) {
                        const Sc64SpriteFrame *construction =
                            sc64_construction_image_frame(stage->image_id,source_frame);
                        if (construction) {
                            draw_construction_image(stage->image_id,source_frame,
                                                    construction,x,y);
                            source_drawn=1;
                        }
                    }
                    for (unsigned layer=1;layer<stage->layer_count;++layer) {
                        const SC64ConstructionSourceLayer *child=&stage->layers[layer];
                        unsigned child_frame=source_frame;
                        if ((child->under!=0)!=(pass==0) ||
                            source_elapsed<child->start_source_updates) continue;
                        if (!child->follows_main_graphic &&
                            !sc64_construction_animation_frame(child->animation,
                                source_elapsed-child->start_source_updates,&child_frame)) continue;
                        const Sc64SpriteFrame *art=
                            sc64_construction_image_frame(child->image_id,child_frame);
                        if (art) {
                            draw_construction_image(child->image_id,child_frame,art,
                                                    x+child->x_offset,y+child->y_offset);
                            source_drawn=1;
                        }
                    }
                }
            }
        }
        if (!source_drawn) {
#endif
#ifdef SC64_HAS_SPRITES
        /* A bottom-up reveal uses the existing completed source art. The
           scaffold/morph/warp motion is an authored visual approximation. */
        if (sprite && duration>0 && progress>=0) {
            int bounded=progress>duration?duration:progress;
            int rows=(int)((u32)sprite->h*(unsigned)bounded/(unsigned)duration);
            int skip=sprite->h-rows;
            if (rows>0)
                gfx_blit_sprite(sprite_x,sprite_y+skip,sprite->w,rows,
                                sprite->pixels+skip*sprite->w);
        }
#endif
        sc64_construction_vfx_draw(race+1,x,y,sprite_w,sprite_h,
                                   progress,duration,(u32)frame);
#ifdef SC64_CONSTRUCTION_SOURCE_ART
        }
#endif
        if (duration>0 && progress>=0)
            gfx_bar(x-12,sprite_y+sprite_h+3,24,2,progress,duration,C_CYAN,C_PANEL);
    }
#endif
    if (selected_building >= 0 && q == &b[selected_building]) {
        int vitals_y = sprite_y + sprite_h + 3;
#ifdef SC64_RACES
        if (q->under_construction) vitals_y += 3;
#endif
        draw_actor_vitals(x, vitals_y, sprite_w > 46 ? 46 : sprite_w,
                          q->hp, q->maxhp,
#ifdef SC64_RACES
                          q->shield,
#else
                          0,
#endif
                          sc64_actor_max_shields(q->type, 1), 0, 0);
    }
#ifndef SC64_RACES
    if (q->under_construction)
        gfx_text(x - 3, y, "C", C_GOLD);
    if (q->under_construction)
        gfx_frame(sprite_x - 2, sprite_y - 2, sprite_w + 4, sprite_h + 4, C_GOLD);
#endif
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
    enum { FIELDS = MAX_MINERALS * 5 + MAX_RACE_GEYSERS * 3 + 1 + MAX_BUILDINGS *
#ifdef SC64_T4_SCENARIO_RUNTIME
                    7
#else
                    6
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
                    + 32 * 2
#endif
#ifdef SC64_Z5_RUNTIME
                    + SC64_Z5_UNIT_BACKED_DOODADS * 2
#endif
#ifdef SC64_P6_RUNTIME
                    + SC64_P6_DOODAD_SOURCE_COUNT * 2
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
    for (i = 0; i < MAX_RACE_GEYSERS; i++) {
        values[n++] = geysers[i].x;
        values[n++] = geysers[i].y;
        {
            int x = geysers[i].x - camx;
            int y = geysers[i].y - camy + 12;
            int in_bounds = i < geyser_count && geyser_in_view(x, y);
            values[n++] = in_bounds && currently_visible(geysers[i].x, geysers[i].y);
        }
    }
    for (i = 0; i < MAX_BUILDINGS; i++) {
        Building *q = &b[i];
        int x = q->x - camx;
        int y = q->y - camy + 12;
        int in_bounds = q->alive && x >= -20 && x <= 260 && y >= 2 && y <= 130;
        values[n++] = q->alive
#ifdef SC64_RACES
            | ((q->under_construction!=0)<<1)
#endif
            ;
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
#ifdef SC64_Z5_RUNTIME
    for (i = 0; i < SC64_Z5_UNIT_BACKED_DOODADS; i++) {
        values[n++] = (int)race_z5_prop_cache_field((unsigned)i, 0);
        values[n++] = (int)race_z5_prop_cache_field((unsigned)i, 1);
    }
#endif
#ifdef SC64_P6_RUNTIME
    for (i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; i++) {
        values[n++] = (int)race_p6_prop_cache_field((unsigned)i, 0);
        values[n++] = (int)race_p6_prop_cache_field((unsigned)i, 1);
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
#ifdef SC64_TILE_RENDERER
#include "sc64_tile_renderer.h"
#endif
#endif

#if defined(SC64_STATIC_DIRTY_ROWS)
#include "sc64_static_dirty_rows.h"
#ifdef GBA_HOST
static unsigned sc64_static_dirty_rows_host_bands;
#endif
#endif

/* Static objects are part of the saved terrain band. Keep their painter
 * order and existing visibility/viewport culls together for cache redraws. */
static void
draw_map_static_objects(void)
{
    int i, x, y;
    for (i = 0; i < MAX_MINERALS; i++)
	if (m[i].amount > 0) {
	    x = m[i].x - camx;
	    y = m[i].y - camy + 12;
	    if (x > -6 && x < 245 && y > 7 && y < 123 &&
	        currently_visible(m[i].x, m[i].y)) {
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
    for (i = 0; i < geyser_count; i++) {
	x = geysers[i].x - camx;
	y = geysers[i].y - camy + 12;
	if (geyser_in_view(x, y) &&
	    currently_visible(geysers[i].x, geysers[i].y)) {
#ifdef SC64_HAS_SPRITES
            const Sc64SpriteFrame *sprite = mission_geyser_sprite();
            if (sprite) draw_mission_sprite(sprite, x, y);
            else
#endif
	        gfx_diamond(x, y, 6, 4, C_GREEN);
        }
    }
#ifdef SC64_IMPORTED_MISSION
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive)
            draw_building_art(&b[i]);
#endif
}

static void
draw_map(void)
{
    int i, x, y, col, row, col0 = 0, row0 = 0, col1 = MAP_COLS, row1 = MAP_ROWS;
#ifdef SC64_IMPORTED_MISSION
    u32 profile_stage_start;
#endif
#if (defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)) && \
    defined(SC64_IMPORTED_MISSION)
    unsigned int terrain_static_key;
#endif
    camx = viewport_left();
    camy = viewport_top();
#ifdef SC64_IMPORTED_MISSION
    profile_stage_start = gba_clock();
#endif
#if (defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)) && \
    defined(SC64_IMPORTED_MISSION)
    /* Rendering below only reads world state and writes pixels. The exact
     * roster/visibility key therefore remains valid through cache capture. */
#ifdef SC64_DIAGNOSTIC_TRACER_PROFILE
    { u32 diag_t=gba_clock(); terrain_static_key = sc64_static_objects_key(); diag_key_cycles=gba_clock()-diag_t; }
#else
    terrain_static_key = sc64_static_objects_key();
#endif
#endif
#ifdef SC64_IMPORTED_MISSION
    col0 = limit(camx / 16, 0, MAP_COLS - 1);
    col1 = limit((camx + 240) / 16 + 1, 0, MAP_COLS);
    row0 = limit(camy / 16, 0, MAP_ROWS - 1);
    row1 = limit((camy + 108) / 16 + 1, 0, MAP_ROWS);
#endif
#ifdef SC64_TILE_RENDERER
    if (gfx_tile_active()) {
        unsigned int tile_static_key = SC64_TERRAIN_STATIC_KEY;
        int refresh;
#ifdef SC64_DIAGNOSTIC_TILES
        u32 tile_diag = gba_clock();
        ++sc64_tile_diag_frames;
#endif
        gfx_tile_clear_world();
#ifdef SC64_DIAGNOSTIC_TILES
        SC64_TILE_DIAG(sc64_tile_diag_clear, tile_diag); tile_diag = gba_clock();
#endif
        sc64_tile_world_update(camx, camy);
#ifdef SC64_DIAGNOSTIC_TILES
        SC64_TILE_DIAG(sc64_tile_diag_world, tile_diag); tile_diag = gba_clock();
#endif
        refresh = sc64_tile_art_needs_refresh(camx, camy, tile_static_key);
        if (refresh || sc64_tile_art_overflow) {
            /* The same static-object code as the mode-4 background below. */
            if (refresh) gfx_tile_art_capture(1);
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
            for (i = 0; i < MAX_BUILDINGS; i++)
                if (b[i].alive)
                    draw_building_art(&b[i]);
            if (refresh) {
                gfx_tile_art_capture(0);
#ifdef SC64_DIAGNOSTIC_TILES
                SC64_TILE_DIAG(sc64_tile_diag_art_draw, tile_diag); tile_diag = gba_clock();
#endif
                if (sc64_tile_art_lift(camx, camy, tile_static_key)) sc64_tile_art_clear_pixels();
                else sc64_tile_art_keep_pixels();
#ifdef SC64_DIAGNOSTIC_TILES
                SC64_TILE_DIAG(sc64_tile_diag_lift, tile_diag);
#endif
            }
        }
        goto sc64_tile_background_done;
    }
#endif
    /* Each tile gets one terrain/fog pass. Unknown space stays black from
     * gfx_begin, so it needs no second opaque fill after sprites are drawn. */
#ifdef SC64_IMPORTED_MISSION
#if defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)
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
#ifdef SC64_STATIC_DIRTY_ROWS
    {
        u16 saved_clip = gfx_get_clip_y();
        int view_col0, view_row0, view_col1, view_row1;
        Sc64StaticDirtyRowsPlan plan;
        int planned = 0;

        /* Only partial cache reuse has a complete current-view dirty mask.
         * The cache helper sets reused_base after a successful restore and
         * fills every dirty cell before returning. */
        if (saved_clip == (u16)(12u | (120u << 8)) &&
            sc64_terrain_cache_valid && sc64_terrain_cache_reused_base &&
            sc64_terrain_cache_view(camx, camy, &view_col0, &view_row0,
                                    &view_col1, &view_row1) &&
            view_col0 == col0 && view_row0 == row0 &&
            view_col1 == col1 && view_row1 == row1 &&
            view_col1 > view_col0 && view_row1 > view_row0 &&
            (view_col1 - view_col0) * (view_row1 - view_row0) <=
                SC64_TERRAIN_CACHE_CELLS)
            planned = sc64_static_dirty_rows_plan(
                sc64_terrain_cache_dirty,
                (unsigned)(view_col1 - view_col0),
                (unsigned)(view_row1 - view_row0), view_row0, camy,
                (int)(saved_clip & 0xffu), (int)(saved_clip >> 8), &plan);

        if (planned) {
            for (unsigned band = 0; band < plan.count; ++band) {
                gfx_set_clip_y(plan.bands[band].top, plan.bands[band].bottom);
                draw_map_static_objects();
#ifdef GBA_HOST
                ++sc64_static_dirty_rows_host_bands;
#endif
            }
            gfx_set_clip_y((int)(saved_clip & 0xffu), (int)(saved_clip >> 8));
        } else {
            /* Full misses and non-map caller clips retain the full pass. */
            draw_map_static_objects();
        }
    }
#else
    draw_map_static_objects();
#endif
#ifdef SC64_IMPORTED_MISSION
#if defined(SC64_T5_RUNTIME) || defined(SC64_RENDER_KEY_ONCE)
    sc64_terrain_store_key(camx, camy, terrain_static_key);
#else
    sc64_terrain_store(camx, camy);
#endif
    }
#ifdef SC64_TILE_RENDERER
sc64_tile_background_done:
#endif
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
#ifndef SC64_DEATH_VFX_DISABLED
    draw_death_vfx(1);
#endif
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive)
	    draw_unit(&u[i]);
#ifndef SC64_DEATH_VFX_DISABLED
    draw_death_vfx(0);
#endif
    draw_tracers();
#ifdef SC64_P7_PROJECTILES_ENABLED
    if (race_protoss_services_active()) race_p7_projectile_draw();
#endif
#ifdef SC64_P4_STORM_ENABLED
    race_p4_storm_draw();
#endif
#ifdef SC64_P6_STORM_ENABLED
    race_p6_storm_draw();
#endif
#ifdef SC64_Z6_STORM_ENABLED
    race_z6_storm_draw();
#endif
#ifdef SC64_IMPORTED_MISSION
    sc64_unit_draw_cycles = gba_clock() - profile_stage_start;
#endif
    x = cx - camx;
    y = cy - camy + 12;
#ifdef SC_DEFAULT_UI
    field_ui_draw_cursor(x, y);
#else
    gfx_frame(x - 4, y - 4, 9, 9, (mode == MODE_BUILD
#ifdef SC64_BATTERY_SERVICE_ENABLED
              || mode == MODE_RECHARGE
#endif
              ) ? C_GOLD :
#if defined(SC64_P4_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
              mode == MODE_P4_STORM ? C_CYAN :
#endif
              C_WHITE);
#if defined(SC64_P4_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
    if (mode == MODE_P4_STORM) {
        gfx_line(x, y-24, x+24, y, C_CYAN);
        gfx_line(x+24, y, x, y+24, C_CYAN);
        gfx_line(x, y+24, x-24, y, C_CYAN);
        gfx_line(x-24, y, x, y-24, C_CYAN);
    }
#endif
    gfx_line(x - 7, y, x + 7, y, C_HIGHLIGHT);
    gfx_line(x, y - 7, x, y + 7, C_HIGHLIGHT);
#ifdef SC64_FIELD_CONTROLS
    field_controls_draw_cursor(x, y);
#endif
#endif
    if (box_state == 2) {
        int left = box_anchor_x < box_end_x ? box_anchor_x : box_end_x;
        int top = box_anchor_y < box_end_y ? box_anchor_y : box_end_y;
        int width = FAST_ABS(box_anchor_x - box_end_x) + 1;
        int height = FAST_ABS(box_anchor_y - box_end_y) + 1;
        gfx_frame(left - camx, top - camy + 12, width, height, C_HIGHLIGHT);
    }
#ifdef SC_COMMAND_RING
    command_ring_draw(x, y);
#endif
#ifdef SC_DEFAULT_UI
    if (menu_open) field_ui_draw_command_menu();
#elif defined(SC64_FIELD_CONTROLS)
    /* The imported command panel is drawn above transient mission messages
     * in draw_hud(), after the world and its overlays. */
#else
    if (menu_open) {
        int first = menu_cursor >= 10 ? menu_cursor - 9 : 0;
#ifndef SC64_RACES
        static const char *labels[MENU_COUNT] = {
	    "MOVE ORDER", "ATTACK MOVE", "BUILD BARRACKS", "BUILD DEPOT",
	    "BUILD REFINERY", "BUILD ACADEMY", "BUILD ENG BAY",
	    "WEAPON UPGRADE", "ARMOR UPGRADE", "STOP / HOLD"
	};
#endif
	gfx_rect(137, 14, 100, 96, C_PANEL);
	gfx_frame(137, 14, 100, 96, C_EDGE);
	for (i = first; i < field_menu_count() && i < first + 10; i++) {
            const char *label =
#ifdef SC64_RACES
                race_menu_label(i);
#else
                labels[i];
#endif
	    gfx_text(142, 17 + (i - first) * 9, i == menu_cursor ? ">" : " ", C_GOLD);
	    gfx_text(149, 17 + (i - first) * 9, label, i == menu_cursor ? C_WHITE : C_DIM);
	}
    }
#endif
#if !defined(SC_DEFAULT_UI) && !defined(SC64_FIELD_CONTROLS) /* field_ui previews the snapped site */
    if (mode == MODE_BUILD && box_state != 2) {
        int build_x = cx, build_y = cy;
#ifdef SC64_RACES
        if (menu == B_REFINERY || menu == B_EXTRACTOR || menu == B_ASSIMILATOR) {
            int geyser = geyser_at(cx, cy);
            if (geyser >= 0) {
                build_x = geysers[geyser].x;
                build_y = geysers[geyser].y;
            }
        }
#endif
	gfx_frame(build_x - camx - 8, build_y - camy + 12 - 6, 17, 13, (can_place(menu, cx, cy)
#ifdef SC64_RACES
            &&race_can_site(SIDE,menu,cx,cy)
#endif
            ) ? C_GREEN : C_RED);
    }
#endif
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
#ifdef SC_DEFAULT_UI
    field_ui_draw_title();
    return;
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
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) {
        gfx_rect(23,25,194,105,C_PANEL);
        gfx_frame(23,25,194,105,C_CYAN);
        gfx_text(35,35,"SHOWCASE / STAGED SETUP",C_GOLD);
        gfx_text(35,49,"A ROLL / RESUME",C_WHITE);
        gfx_text(35,61,"R RESTART SCENE",C_WHITE);
        gfx_text(35,73,"B OR SELECT: SCENES",C_WHITE);
        gfx_text(35,91,"START PAUSES THE ACTION",C_CYAN);
        gfx_text(35,106,"REPLAYS DO NOT WRITE SAVES",C_DIM);
        return;
    }
#endif
#ifdef SC_DEFAULT_UI
    field_ui_draw_pause();
    return;
#elif defined(SC64_FIELD_CONTROLS)
    field_controls_draw_pause();
    return;
#endif
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
#ifdef SC64_SHOWCASE_ENABLED
    if (sc64_showcase_scene >= 0) {
        gfx_rect(26,39,188,67,C_PANEL);
        gfx_frame(26,39,188,67,lost ? C_RED : C_GOLD);
        gfx_text(49,49,lost ? "LAST OUTPOST LOST" : "SCENE COMPLETE",C_GOLD);
        gfx_text(42,66,"STAGED GAMEPLAY SETUP",C_WHITE);
        gfx_text(39,87,"A REPLAY  B/SEL SCENES",C_CYAN);
        return;
    }
#endif
#ifdef SC_DEFAULT_UI
    field_ui_draw_result();
    return;
#elif defined(SC64_FIELD_CONTROLS)
    field_controls_draw_result();
    return;
#endif
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

/* Result presentation: the final explosions play over the frozen
 * battlefield, then the panel grows open, as StarCraft 64 shows its
 * defeat/victory dialog after the last structures die. */
static void
draw_result_screen(void)
{
#ifndef SC64_DEATH_VFX_DISABLED
    u16 outro_end = result_outro_end_get(), outro = result_outro_get();
    if (!outro_end) return;
    unsigned open = outro >= outro_end ? (unsigned)(outro - outro_end) : 0u;
    if (open < SC64_RESULT_PANEL_OPEN) {
        int w = 16 + (int)(168u * (open + 1u) / (SC64_RESULT_PANEL_OPEN + 1u));
#if defined(SC_DEFAULT_UI) || defined(SC64_FIELD_CONTROLS)
        int h = 8 + (int)(78u * (open + 1u) / (SC64_RESULT_PANEL_OPEN + 1u));
#else
        int h = 8 + (int)(58u * (open + 1u) / (SC64_RESULT_PANEL_OPEN + 1u));
#endif
        gfx_rect(120 - w / 2, 73 - h / 2, w, h, C_PANEL);
        gfx_frame(120 - w / 2, 73 - h / 2, w, h, won ? C_GOLD : C_RED);
        return;
    }
#endif
    draw_result();
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
#ifdef SC64_HUD_ICONS
    card_resource_icon(SC64_HUD_SPRITE_MINERAL, 2);
    gfx_num(16, 3, minerals, C_WHITE);
    card_resource_icon(SC64_HUD_SPRITE_GAS, 60);
    gfx_num(74, 3, gas, C_WHITE);
    card_resource_icon(card_supply_sprite(), 126);
    {
        /* The slash follows the used count, so 100/200 keeps its spacing. */
        int used = (supply_used + race_owner_queued_supply(SIDE)) / 2, x = 140 + ui_num_width(used) + 3;
        gfx_num(140, 3, used, C_WHITE);
        gfx_text(x, 3, "/", C_WHITE);
        gfx_num(x + 8, 3, supply_cap / 2, C_WHITE);
    }
#else
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
#endif
}

static void
draw_hud_shields(int shield,int maximum)
{
#ifdef SC64_RACES
    if (maximum>0) { gfx_text(129,126,"SH",C_CYAN); gfx_num(147,126,shield,C_CYAN); }
#else
    (void)shield; (void)maximum;
#endif
}

static void draw_hud_commands_body(void);

/* The minimap's black panel and edge are static, so with the maintained
 * raster they join the cached bottom band after every text that can reach
 * x >= 181; the per-frame opaque raster copy then covers the map area. */
static void
draw_hud_commands(void)
{
    draw_hud_commands_body();
#ifdef SC64_MINIMAP_RASTER_ACTIVE
    gfx_rect(181, 124, 56, 34, C_BLACK);
    gfx_frame(181, 124, 56, 34, C_EDGE);
#endif
}

static void
draw_hud_commands_body(void)
{
    int i, n = selected_count();
    gfx_rect(0, 121, 240, 39, C_PANEL);
    gfx_line(0, 120, 240, 120, C_EDGE);
#ifdef SC_COMMAND_RING
    if (command_ring_draw_hud()) return;
#endif
#ifdef SC_DEFAULT_UI
    field_ui_draw_hud();
    return;
#endif
    int hud_left=5;
#if defined(SC64_RACES) && defined(SC64_FIELD_CONTROLS)
    if(box_state!=2 && !menu_open && mode==MODE_NORMAL &&
#ifdef SC64_COMMAND_ICONS
       /* The command card's selection header draws unit portraits and keeps
          the unit hint at x 52; only buildings take the source icon here. */
       selected_building>=0 && b[selected_building].alive &&
#endif
       field_controls_draw_selected_icon(14,135,22))hud_left=30;
#endif
    if (box_state == 2) {
        gfx_text(5, 126, "BOX SELECT", C_CYAN);
        gfx_text(5, 137, "RELEASE L SELECT B CANCEL", C_GOLD);
#ifdef SC64_FIELD_CONTROLS
    } else if (field_controls_draw_hud()) { /* list, placement, armed orders */
#endif
    } else if (selected_building >= 0 && b[selected_building].alive) {
	Building *q = &b[selected_building];
	#ifdef SC64_RACES
        char title[64];
        int title_width=q->shield>0?124-hud_left:165-hud_left;
        sc64_font_ellipsis(sc64_race_building_name(q->type),title_width,title,sizeof(title));
        sc64_font_text(hud_left,124,title_width,title,C_WHITE);
        draw_hud_shields(q->shield,sc64_actor_max_shields(q->type,1));
#else
	gfx_text(5, 126, q->type == B_COMMAND ? "COMMAND CENTER" :
		 q->type == B_BARRACKS ? "BARRACKS" : q->type == B_DEPOT ? "SUPPLY DEPOT" :
		 q->type == B_REFINERY ? "REFINERY" : q->type == B_ACADEMY ? "ACADEMY" :
                 q->type == B_BUNKER ? "BUNKER" : q->type == B_INFESTED_COMMAND ? "INFESTED COMMAND" :
                 q->type == B_CREEP_COLONY ? "CREEP COLONY" : "ENGINEERING BAY", C_WHITE);
#endif
#ifdef SC64_RACES
#ifdef SC64_Z6_TRANSPORT_ENABLED
        if (race_z6_active() && race_z6.transport.ticks_remaining &&
            race_z6.transport.producer_slot_plus1==(unsigned)selected_building+1u) {
            gfx_text(5,137,"VENTRAL SACS",C_CYAN);
            gfx_num(95,137,(race_z6.transport.ticks_remaining+29u)/30u,C_WHITE);
            gfx_text(119,137,"SECONDS",C_GOLD);
        } else
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
        if (!q->under_construction && sc64_race_raw_building(q->type)==172) {
            gfx_text(hud_left,137,"EN",C_CYAN);
            gfx_num(hud_left+18,137,q->queue_progress/256,C_WHITE);
            sc64_font_text(85,135,94,race_battery_powered_live(selected_building) ?
                           "B RECHARGE" : "UNPOWERED",C_GOLD);
        } else
#endif
#ifdef SC64_SHARED_BUNKER
        if (!q->under_construction && sc64_race_raw_building(q->type)==125) {
            int cargo=race_bunker_cargo_space(selected_building);
            gfx_text(hud_left,137,"LOAD",C_CYAN);
            gfx_num(hud_left+30,137,cargo>0?cargo:0,C_WHITE);
            gfx_text(hud_left+38,137,"/4",C_WHITE);
            sc64_font_text(85,135,94,cargo>0?"B UNLOAD":"SELECT INFANTRY",C_GOLD);
        } else
#endif
        if(!q->under_construction) {
            char training[48];
            sc64_font_ellipsis(unit_name(train_type),81-hud_left-12,training,sizeof(training));
            gfx_text(hud_left,137,"A",C_GOLD);
            sc64_font_text(hud_left+12,135,81-hud_left-12,training,C_WHITE);
            sc64_font_text(85,135,94,"R Type L Cancel",C_GOLD);
        } else gfx_text(hud_left,137,"UNDER CONSTRUCTION",C_GOLD);
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
#ifdef SC64_COMMAND_ICONS
		card_selection_header(i, n);
#else
#ifdef SC64_RACES
                char title[64];
                int title_width=u[i].shield>0?124-hud_left:165-hud_left;
                sc64_font_ellipsis(n>1?"Combat group":unit_name(u[i].type),title_width,title,sizeof(title));
                sc64_font_text(hud_left,124,title_width,title,C_WHITE);
                draw_hud_shields(u[i].shield,sc64_actor_max_shields(u[i].type,0));
#else
                gfx_text(5,126,n>1?"COMBAT GROUP":unit_name(u[i].type),C_WHITE);
#endif
		gfx_text(hud_left, 137, "HP", C_CYAN);
		gfx_num(hud_left+19, 137, u[i].hp, C_GREEN);
#endif
		if (IS_WORKER(u[i].type))
#ifdef SC64_FIELD_CONTROLS
		    gfx_text(hud_left+47, 137, ui_action == UI_A_MOVE ? "A MOVE  B BUILD" :
			     ui_action == UI_A_GAS ? "A GAS  B BUILD" :
                             ui_action == UI_A_MINE ? "A MINE B BUILD" : "B BUILD", C_GOLD);
#elif defined(SC_DEFAULT_UI)
		    gfx_text(hud_left+47, 137, ui_action == UI_A_MINE ? "A MINE / B BUILD" : "B BUILD", C_GOLD);
#else
                    gfx_text(hud_left+47, 137, "B BUILD", C_GOLD);
#endif
		else
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
                if (race_protoss_services_active() && u[i].type==U_P7_CARRIER) {
                    gfx_text(hud_left+47,137,"IC",C_CYAN);
                    gfx_num(hud_left+65,137,(int)race_p7_carrier_training_child_count(i),C_WHITE);
                    gfx_text(hud_left+79,137,"HANGAR / FLIGHT WIP",C_GOLD);
                } else
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
                if (race_z6_cloak_actor(&u[i])) {
                    gfx_text(hud_left+47,137,"EN",C_CYAN); gfx_num(hud_left+65,137,u[i].carry/256,C_WHITE);
                    gfx_text(hud_left+95,137,race_z6.cloak.active ? "CLOAKED" : "B SPELLS",C_GOLD);
                } else
#endif
#ifdef SC64_P6_STORM_ENABLED
                if (race_p6_active() && u[i].raw_type==79 && u[i].raw_owner==2) {
                    gfx_text(hud_left+47,137,"EN",C_CYAN); gfx_num(hud_left+65,137,u[i].carry/256,C_WHITE);
                    gfx_text(hud_left+95,137,"B STORM",C_GOLD);
                } else
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
                if (race_z5_cloak_actor(&u[i])) {
                    gfx_text(hud_left+47, 137, "EN", C_CYAN);
                    gfx_num(hud_left+65, 137, u[i].carry / 256, C_WHITE);
                    gfx_text(hud_left+95, 137, race_z5.cloak.active ? "CLOAKED" : "B CLOAK", C_GOLD);
                } else
#endif
#ifdef SC64_TRANSPORT_RUNTIME
#ifdef SC64_T5_RUNTIME
                if (u[i].type == U_DROPSHIP) {
#else
                if (race_transport_carrier_type(&u[i])) {
#endif
                    gfx_text(hud_left+47, 137, "CARGO", C_GOLD);
                    gfx_num(hud_left+85, 137, race_transport_cargo_space(i), C_WHITE);
                    gfx_text(hud_left+99, 137, "/8  B UNLOAD", C_GOLD);
                } else
#endif
#ifdef SC_COMMAND_RING
		    gfx_text(hud_left+47, 137, "HOLD B ORDERS", C_GOLD);
#elif defined(SC64_FIELD_CONTROLS)
		    gfx_text(hud_left+47, 137, field_controls_unit_hint(), C_GOLD);
#else
		    gfx_text(hud_left+47, 137, "A ORDER B COMMANDS", C_GOLD);
#endif
		break;
	    }
    } else {
	gfx_text(5, 126, mode == MODE_BUILD ? "BUILD PLACEMENT" :
#ifdef SC64_TERRAN_LATE_RUNTIME
                 mode == MODE_LAND ? "LANDING SITE" :
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
                 mode == MODE_RECHARGE ? "RECHARGE SHIELDS" :
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_AUTOLAUNCH_EXPERIMENT
                 mode == MODE_YAMATO ? "YAMATO TARGET" :
#endif
#if defined(SC64_P4_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
                 mode == MODE_P4_STORM ? "PSIONIC STORM TARGET" :
#endif
#ifdef SC64_Z5_CONSUME_ENABLED
                 mode == MODE_Z5_CONSUME ? "CONSUME FRIENDLY ZERGLING" :
#endif
		 mode == MODE_ATTACK ? "ATTACK MOVE" : mode == MODE_MOVE ? "MOVE ORDER" : "FIELD COMMAND",
#ifdef SC64_FIELD_CONTROLS
                 field_controls_title_color());
#else
                 C_WHITE);
#endif
#if defined(SC64_P4_STORM_ENABLED) || defined(SC64_Z6_STORM_ENABLED)
        if (mode == MODE_P4_STORM)
            gfx_text(5, 137, "A CAST  B CANCEL", C_GOLD);
        else
#endif
#ifdef SC64_FIELD_CONTROLS
        if (field_controls_hint())
            gfx_text(5, 137, field_controls_hint(), C_GOLD);
        else
#endif
	gfx_text(5, 137,
#ifdef SC64_P4_STORM_ENABLED
                 race_p4_active() ? "B COMMANDS  B+R ESCORTS" :
#endif
#ifdef SC_COMMAND_RING
                 mode == MODE_NORMAL ? "HOLD B ORDERS  TAP B LIST" :
#endif
                 "B COMMANDS  A SELECT / PLACE", C_GOLD);
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
#ifdef SC64_Z4_RUNTIME
        else if (race_selected_mission == 15) {
            unsigned seconds = race_z4_hud_seconds();
            char timer[6] = {(char)('0' + seconds / 600u),
                (char)('0' + seconds / 60u % 10u), ':',
                (char)('0' + seconds / 10u % 6u), (char)('0' + seconds % 10u), 0};
            gfx_text(5, 149, "PROTECT / CLEAR COMMAND CENTERS", C_GOLD);
            gfx_text(194, 149, timer, C_WHITE);
        }
#endif
#ifdef SC64_P3_RUNTIME
        else if(race_selected_mission==SC64_P3_GLOBAL_ID)
            gfx_text(5,149,race_p3.sequence?"HIGHER GROUND / MISSION REPORT":"DESTROY BOTH ZERG COLONIES",C_GOLD);
#endif
#ifdef SC64_P4_RUNTIME
        else if (race_selected_mission == SC64_P4_GLOBAL_ID)
            gfx_text(5, 149, "RESCUE HEROES / REACH EXTRACTION", C_GOLD);
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
#if (defined(SC64_TERRAN_LATE_RUNTIME) || defined(SC64_HUD_BUFFER_REUSE)) && defined(SC64_NO_FRAME_CLEAR)
#define SC64_HUD_BAND_REUSE 1
#endif
/* Optional: with reusable bands, each page keeps its own HUD pixels and a
 * stale page copies from the one on screen, so no EWRAM copy is needed. */
#if defined(SC64_HUD_VRAM_BANDS) && defined(SC64_HUD_BAND_REUSE) && !defined(SC64_TILE_RENDERER)
#define SC64_HUD_PAGE_BANDS 1
#endif
#if !defined(SC64_TILE_RENDERER) && !defined(SC64_HUD_PAGE_BANDS)
static _Alignas(4) u8 sc64_hud_top[240 * 12], sc64_hud_bottom[240 * 40];
#endif
#ifdef SC64_FIELD_CONTROLS
enum { SC64_HUD_KEY_COUNT=30 }; /* 26-28: commands; 29: source icon identity */
#else
enum { SC64_HUD_KEY_COUNT=26 };
#endif
static int sc64_hud_previous[SC64_HUD_KEY_COUNT], sc64_hud_valid;
#ifdef SC64_RACES
/* Keep race presentation key construction in ROM: canonical Z3 has very
 * little fast-RAM code headroom. Key values and invalidation stay exact. */
static void __attribute__((noinline))
sc64_hud_race_keys(int *values,int first)
{
    if (first>=0) {
        values[24]=u[first].shield;
        values[25]=sc64_actor_max_shields(u[first].type,0);
    }
    if (selected_building>=0 && b[selected_building].alive) {
        values[24]=b[selected_building].shield;
        values[25]=sc64_actor_max_shields(b[selected_building].type,1);
    }
    values[19]=race_campaign.phase;values[20]=race_campaign.message;
#ifdef SC64_FIELD_CONTROLS
    /* Source icons depend on raw identity even when the engine type is equal. */
    values[29]=first>=0?u[first].raw_type:-1;
    if(selected_building>=0 && b[selected_building].alive)
        values[29]=b[selected_building].raw_type;
    /* The hint follows the A action; the open list follows its cursor and
       wallet; placement status follows the snapped site and wallet. */
    values[26]=ui_action|menu_open<<4|menu_cursor<<5;
    if (menu_open || mode == MODE_BUILD) {
        int x=cx,y=cy;
        if (mode == MODE_BUILD) {
            field_ui_site(menu,&x,&y);
            values[27]=(x>>3)|(y>>3)<<12;
        }
        values[28]=(int)((u32)minerals|((u32)gas<<16));
    }
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    if(first>=0 && race_protoss_services_active() && u[first].type==U_P7_CARRIER)
        values[20]|=((int)race_p7_carrier_training_child_count(first)+(u[first].carry<<3))<<8;
#endif
#ifdef SC64_Z5_CLOAK_ENABLED
    if (first >= 0 && race_z5_cloak_actor(&u[first]))
        values[20] |= (u[first].carry / 256 + ((int)race_z5.cloak.active << 9)) << 8;
#endif
#ifdef SC64_Z4_RUNTIME
    if (!race_mode && race_selected_mission == 15)
        values[20] |= (int)race_z4_hud_seconds() << 8;
#endif
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
}
#endif
#ifdef SC64_HUD_PAGE_BANDS
/* Band content versions, and per page the version it holds and the clear
 * count it held it at. A page band is current only when both still match. */
static u32 sc64_hud_band_version[2];
static u32 sc64_hud_page_version[2][2] = {{~0u, ~0u}, {~0u, ~0u}};
static u32 sc64_hud_page_clear_generation[2];
#elif defined(SC64_HUD_BAND_REUSE)
static u8 sc64_hud_buffer_dirty[2];
static u32 sc64_hud_buffer_clear_generation[2];
#endif

/* Something drew over the current page's bottom band after its restore. */
static void
sc64_hud_bottom_band_invalidate(void)
{
#ifdef SC64_HUD_PAGE_BANDS
    sc64_hud_page_version[gfx_backbuffer_index()][1] = ~0u;
#elif defined(SC64_HUD_BAND_REUSE)
    sc64_hud_buffer_dirty[gfx_backbuffer_index()] |= 2;
#endif
}

#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
/* Keep the expanded actor loop and target search in IWRAM. This once-per-
 * render HUD invalidation pass can execute from ROM without using its stack
 * reserve. Its timing impact is measured with the late-profile replay. */
static int __attribute__((noinline))
#else
static int GBA_HOT __attribute__((noinline))
#endif
sc64_hud_changed(void)
{
    int values[SC64_HUD_KEY_COUNT] = {minerals, gas, supply_used, supply_cap, mode, selected_building,
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
    sc64_hud_race_keys(values,first);

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
    for (i = 0; i < SC64_HUD_KEY_COUNT; i++) {
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
#ifdef SC64_HUD_PAGE_BANDS
    int changed = sc64_hud_changed();
    u8 buffer = gfx_backbuffer_index(), other = (u8)(buffer ^ 1);
    u32 generation = gfx_clear_generation(buffer);
    u32 other_generation = gfx_clear_generation(other);
    int band;
    if (changed & 1) sc64_hud_band_version[0]++;
    if (changed & 2) sc64_hud_band_version[1]++;
    for (band = 0; band < 2; band++) {
        u32 version = sc64_hud_band_version[band];
        if (sc64_hud_page_version[buffer][band] == version &&
            sc64_hud_page_clear_generation[buffer] == generation) continue;
        if (!(changed & (1 << band)) &&
            sc64_hud_page_version[other][band] == version &&
            sc64_hud_page_clear_generation[other] == other_generation)
            gfx_copy_band_from_front(band ? 120 : 0, band ? 40 : 12);
        else if (band) draw_hud_commands();
        else draw_hud_resources();
        sc64_hud_page_version[buffer][band] = version;
    }
    sc64_hud_page_clear_generation[buffer] = generation;
    sc64_hud_valid = 1;
#elif defined(SC64_HUD_BAND_REUSE)
    int changed = sc64_hud_changed();
    u8 buffer = gfx_backbuffer_index();
    u8 dirty;
    u32 generation = gfx_clear_generation(buffer);
    if (changed) {
        sc64_hud_buffer_dirty[0] |= (u8)changed;
        sc64_hud_buffer_dirty[1] |= (u8)changed;
    }
    dirty = sc64_hud_buffer_dirty[buffer];
#ifndef GBA_SCENE_CLIP
    /* Unclipped map primitives can paint either HUD band. Restore both on
     * each frame unless the scene clip guarantees those pixels stay intact. */
    dirty |= 3;
#endif
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
#elif defined(SC64_TILE_RENDERER)
    /* The software layer keeps the HUD bands. Redraw a band only when its
     * values changed or anything else drew over it, exactly as the mode-4
     * band restore would have overwritten those pixels. */
    int changed = sc64_hud_changed();
    if (!gfx_tile_active()) changed = 3;
    if (gfx_tile_band_top || sc64_tile_hud_invalid) changed |= 1;
    if (gfx_tile_band_bottom || sc64_tile_hud_invalid) changed |= 2;
#ifdef SC64_DIAGNOSTIC_TILES
    u32 hud_diag = gba_clock();
    sc64_tile_diag_hud_top += (changed & 1) != 0;
    sc64_tile_diag_hud_bottom += (changed & 2) != 0;
#endif
    if (changed & 1) draw_hud_resources();
    if (changed & 2) draw_hud_commands();
#ifdef SC64_DIAGNOSTIC_TILES
    SC64_TILE_DIAG(sc64_tile_diag_hud_bands, hud_diag);
#endif
    sc64_tile_hud_invalid = 0;
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
#ifdef SC64_DIAGNOSTIC_TILES
    u32 minimap_diag = gba_clock();
#endif
    draw_minimap();
#ifdef SC64_DIAGNOSTIC_TILES
    SC64_TILE_DIAG(sc64_tile_diag_minimap, minimap_diag);
#endif
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
#ifdef SC64_FIELD_CONTROLS
    if (menu_open) field_controls_draw_command_menu();
#endif
    if (toast_timer && toast_id >= 0 && (unsigned)toast_id < sizeof(toast_text) / sizeof(toast_text[0])) {
	gfx_rect(54, 105, 132, 12, C_PANEL);
	gfx_frame(54, 105, 132, 12, C_EDGE);
	gfx_text(59, 108, toast_text[toast_id], C_GOLD);
    }
#ifdef SC64_TILE_RENDERER
    /* Anything drawn over a band after this point forces a redraw next frame. */
    gfx_tile_band_top = gfx_tile_band_bottom = 0;
#endif
}

#include "game_audio.h"

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
#ifdef SC64_TILE_RENDERER
    {
        int want_tiles = screen != SCREEN_TITLE && sc64_tile_map_supported();
        if (want_tiles && (!gfx_tile_active() ||
                           sc64_tile_map != &sc64_race_maps[race_selected_mission])) {
            gfx_tile_enter();
            sc64_tile_reset();
            gfx_tile_present_hook = sc64_tile_present;
        } else if (!want_tiles && gfx_tile_active()) gfx_tile_leave();
    }
#endif
    if (screen == SCREEN_TITLE) gfx_begin(C_BLACK);
    else gfx_begin_preserve();
#else
    gfx_begin(C_BLACK);
#endif
    game_audio_observe();
    if (screen == SCREEN_TITLE)
	draw_title();
    else {
#ifdef SC64_IMPORTED_MISSION
        profile_start = gba_clock();
#endif
#ifdef SC_FIELD_FEEDBACK
        field_ui_refresh();
#endif
#ifdef GBA_SCENE_CLIP
        gfx_set_clip_y(12,120);
#endif
	draw_map();
#ifdef GBA_SCENE_CLIP
        gfx_set_clip_y(0,160);
#endif
#ifdef SC64_IMPORTED_MISSION
        sc64_map_cycles = gba_clock() - profile_start;
        profile_start = gba_clock();
#endif
	draw_hud();
#ifdef SC64_SHOWCASE_ENABLED
        if (sc64_showcase_scene >= 0) sc64_showcase_draw_label();
#endif
#ifdef SC64_IMPORTED_MISSION
        sc64_hud_cycles = gba_clock() - profile_start;
#endif
	if (screen == SCREEN_PAUSE) {
	    draw_pause();
#ifdef SC64_IMPORTED_MISSION
	    /* The pause panel crosses the bottom cached HUD band. */
	    sc64_hud_bottom_band_invalidate();
#endif
	} else if (screen == SCREEN_RESULT)
	    draw_result_screen();
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
#ifdef SC64_P7_ARCHON_SPLASH
#include "sc64_p7_archon_adapter.h"
#endif
#ifdef SC64_SIEGE_SPLASH
#include "sc64_siege_splash.h"
#endif
#ifdef SC64_T5_RUNTIME
#include "sc64_t5_availability_adapter.h"
#endif
#ifdef SC64_T6_RUNTIME
#include "sc64_t6_availability_adapter.h"
#endif
#ifdef SC64_P8_RUNTIME
#include "sc64_p8_availability_adapter.h"
#endif
#include "race_adapter.h"
#ifdef SC64_P7_OBSERVER_ENABLED
#include "sc64_p7_runtime_adapter.h"
#include "sc64_p7_observer_adapter.h"
#endif
#ifdef SC64_BATTERY_SERVICE_ENABLED
#include "sc64_battery_adapter.h"
#endif
#ifdef SC64_P7_BATTERY_ENABLED
#include "sc64_p7_runtime_adapter.h"
#include "sc64_p7_battery_adapter.h"
#include "sc64_p7_projectile_adapter.h"
#include "sc64_p7_carrier_training.h"
#include "sc64_p7_interceptor_flight.h"
#endif
#ifdef SC64_Z7_COLONY_DEFENSE_ENABLED
#include "sc64_z7_colony_adapter.h"
#endif
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
#include "sc64_z8_game_bridge.h"
#ifdef SC64_P9_RUNTIME
/* P9 builds dispatch the shared-service hooks between P8 and P9 (see
 * sc64_p9_runtime_adapter.h); P8's own definitions take P8-only names. */
#define race_protoss_later_services_active race_p8_later_services_active
#define race_protoss_later_source_owner race_p8_later_source_owner
#define race_protoss_later_service_clock race_p8_later_service_clock
#define race_protoss_later_service_mission race_p8_later_service_mission
#define race_protoss_later_mission_source_owner race_p8_later_mission_source_owner
#define race_protoss_later_battery_initialize race_p8_later_battery_initialize
#define race_protoss_later_battery_world_valid race_p8_later_battery_world_valid
#define race_protoss_later_battery_candidate_valid race_p8_later_battery_candidate_valid
#endif
#ifdef SC64_P8_RUNTIME
#include "sc64_p8_runtime_adapter.h"
#endif
#ifdef SC64_P9_RUNTIME
#undef race_protoss_later_services_active
#undef race_protoss_later_source_owner
#undef race_protoss_later_service_clock
#undef race_protoss_later_service_mission
#undef race_protoss_later_mission_source_owner
#undef race_protoss_later_battery_initialize
#undef race_protoss_later_battery_world_valid
#undef race_protoss_later_battery_candidate_valid
#endif
#ifdef SC64_P9_RUNTIME
#include "sc64_p9_runtime_adapter.h"
#endif
#ifdef SC64_NYDUS_ENABLED
static int race_save_decode_unit(const RaceSave *, unsigned, unsigned, unsigned, Unit *);
static int race_save_decode_building(const RaceSave *, unsigned, Building *);
#ifdef SC64_Z8_RUNTIME
/* Save preflight and a fresh-boot load resolve Z8 canal links from its
 * program table before the mission adapter has run. */
#include "sc64_nydus_rules.h"
static int race_z8_nydus_source(int mission, const Sc64NydusSourceRecord **records,
                                unsigned *count);
#define SC64_NYDUS_SOURCE_FOR_MISSION(mission, records, count) \
    race_z8_nydus_source((mission), (records), (count))
#endif
#include "sc64_nydus_adapter.h"
#endif
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
#include "sc64_battlecruiser_facility_flight.h"
#endif
#ifdef SC64_Z8_RUNTIME
#include "sc64_z8_runtime_adapter.h"
#endif
#include "race_save_adapter.h"
#ifdef SC64_IMPACT_EVENTS
#include "sc64_impact_adapter.h"
#endif
#endif

#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
static int sc64_battlecruiser_owner_key(int custom, int mission, int owner, int raw_owner)
{
    if (custom) return owner >= 0 && owner < 5 ? owner : -1;
    if (owner == SIDE) {
        if (mission < 0 || mission >= SC64_RACE_MAP_COUNT ||
            sc64_race_maps[mission].human_slot >= 8) return -1;
        return sc64_race_maps[mission].human_slot;
    }
    return raw_owner >= 0 && raw_owner < 8 ? raw_owner : -1;
}
/* Called before a real control transfer, while the previous owner key is
 * still available. Source raw_owner provenance on the actor is preserved. */
static int race_bc_before_unit_transfer(Unit *q,int new_owner,int new_raw_owner)
{
    unsigned bits;
    int old_key,new_key;
    if (!q || !q->alive || sc64_race_raw_unit(q->type)!=12) return 1;
    if (q->owner==new_owner) return 1;
    old_key=sc64_battlecruiser_owner_key(race_mode,race_selected_mission,q->owner,q->raw_owner);
    new_key=sc64_battlecruiser_owner_key(race_mode,race_selected_mission,new_owner,new_raw_owner);
    if (old_key<0 || new_key<0) return 0;
    bits=sc64_battlecruiser_tech_bits;
    /* Pinned OpenBW give_unit_to grants Yamato unconditionally; reactor is
     * inherited from the previous owner only when present. */
    (void)sc64_battlecruiser_tech_complete(&bits,(unsigned)new_key,SC64_BC_TECH_YAMATO);
    if (sc64_battlecruiser_tech_has(bits,(unsigned)old_key,SC64_BC_TECH_REACTOR))
        (void)sc64_battlecruiser_tech_complete(&bits,(unsigned)new_key,SC64_BC_TECH_REACTOR);
    sc64_battlecruiser_tech_bits=(u16)bits;
    return 1;
}
static int sc64_battlecruiser_source_tick_due(void)
{
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (race_p7_active()) return race_p7.reserved[0] != 4u;
#endif
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    return (((unsigned)frame%5u)+5u-sc64_bc_pause_offset)%5u != 0u;
#else
    return (unsigned)frame % 5u != 0u;
#endif
}
#endif
