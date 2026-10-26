#ifndef SC64_P4_ADAPTER_H
#define SC64_P4_ADAPTER_H

#if defined(SC64_P4_COUNT_CENSUS_ENABLED) && !defined(SC64_P4_RUNTIME)
#error "SC64_P4_COUNT_CENSUS_ENABLED requires the P4 runtime"
#endif
#if defined(SC64_P4_CREATE_PREFIX) && !defined(SC64_P4_RUNTIME)
#error "SC64_P4_CREATE_PREFIX requires the P4 runtime"
#endif
#if defined(SC64_P4_VALIDATOR_HOT) && !defined(SC64_P4_RUNTIME)
#error "SC64_P4_VALIDATOR_HOT requires the P4 runtime"
#endif

#if defined(SC64_P4_RUNTIME)

#include "sc64_p4.h"
#include "sc64_p4_scenario_generated.h"

#if defined(SC64_P4_CREATE_PREFIX)
#if !defined(SC64_P4_CREATE_PREFIX_VERSION) || SC64_P4_CREATE_PREFIX_VERSION != 1
#error "SC64_P4_CREATE_PREFIX requires generated create-prefix version 1"
#endif
_Static_assert(sizeof(sc64_p4_source_create_prefix) /
               sizeof(sc64_p4_source_create_prefix[0]) ==
               sizeof(sc64_p4_source_actions) /
               sizeof(sc64_p4_source_actions[0]) + 1,
               "P4 create-prefix endpoints must cover every source action");
#endif

/* P4's presentation payloads and original AI script are not included. Numeric
 * source action order, waits, actor creation and predicates are retained; text
 * and transmissions use a bounded three-second hold, while the opaque AI
 * command is represented by a documented, bounded authored pressure action. */
enum {
    SC64_P4_TICKS_PER_SECOND = 30,
    SC64_P4_PRESENTATION_TICKS = 90,
    SC64_P4_SOURCE_CREATE_COUNT = 87,
    SC64_P4_FAULT_STATE = 1,
    SC64_P4_FAULT_ACTION = 2,
    SC64_P4_FAULT_CAPACITY = 3,
    SC64_P4_FAULT_ACTOR = 4,
    SC64_P4_FAULT_SELECTOR = 5,
    SC64_P4_FAULT_LOCATION = 6,
    SC64_P4_FAULT_PROPERTY = 7,
    SC64_P4_FAULT_RESOURCE = 8,
    SC64_P4_FAULT_SWITCH = 9
};

static Sc64P4RuntimeState race_p4;

static unsigned race_p4_ticks(u32 ms);
#ifdef SC64_P4_STORM_ENABLED
static void race_p4_storm_init_actor(int i);
#endif

static int race_p4_active(void)
{
    return !race_mode && race_selected_mission == SC64_P4_GLOBAL_ID;
}

static const char *race_p4_objective_text(void)
{
    return "Rescue Tassadar and Raynor. Bring them to the extraction point.";
}
static const char *race_p4_victory_text(void) { return "TASSADAR AND RAYNOR EXTRACTED"; }
static const char *race_p4_failure_text(void) { return "THE HEROES WERE LOST"; }

/* Largest legal outstanding timer for the action currently at cursor-1.
 * This reads only immutable generated metadata and timing policy. */
static u16 race_p4_action_duration_limit(unsigned trigger, unsigned index)
{
    const Sc64P4SourceTrigger *tr;
    const Sc64P4SourceAction *a;
    if (trigger >= SC64_P4_TRIGGER_COUNT) return 0;
    tr = &sc64_p4_source_triggers[trigger];
    if (index >= tr->action_count) return 0;
    a = &sc64_p4_source_actions[tr->first_action + index];
    if (a->trigger != trigger || a->index != index) return 0;
    switch (a->opcode) {
    case 4: return (u16)race_p4_ticks(a->time);
    case 7: case 8: case 9: return SC64_P4_PRESENTATION_TICKS;
    case 1: case 2: case 5: case 6: case 10: case 11: case 12:
    case 13: case 16: case 26: case 30: case 31: return 1;
    default: return 0;
    }
}

static unsigned race_p4_reserved_units(void)
{
    return race_p4.created_count <= SC64_P4_SOURCE_CREATE_COUNT ?
        SC64_P4_SOURCE_CREATE_COUNT - race_p4.created_count : 0;
}

#if defined(SC64_P4_VALIDATOR_HOT)
static int GBA_HOT __attribute__((noinline))
#else
static int
#endif
race_p4_runtime_state_valid(const Sc64P4RuntimeState *s)
{
    if (!s || !sc64_p4_state_valid(&s->model) || s->faults ||
        s->switch_bits & (u8)~3u || s->source_paused > 1 ||
        s->speech_muted > 1 || s->switch13_set > 1 ||
        s->ai_script_mask & (u8)~1u ||
        s->created_count > SC64_P4_SOURCE_CREATE_COUNT ||
        s->message_ticks > SC64_P4_PRESENTATION_TICKS) return 0;
#ifdef SC64_P4_STORM_ENABLED
    if (!sc64_psionic_storm_state_valid(&s->storm)) return 0;
#endif
    unsigned expected_creates = 0;
    for (unsigned t = 0; t < SC64_P4_TRIGGER_COUNT; ++t) {
        unsigned n = sc64_p4_source_triggers[t].action_count;
        if (s->action_cursor[t] > n ||
            ((s->action_cursor[t] == 0) != (s->action_ticks[t] == 0)))
            return 0;
        if (s->action_cursor[t] && !(s->model.fired & (1u << t))) return 0;
        if (s->action_ticks[t]) {
            u16 limit = race_p4_action_duration_limit(t, s->action_cursor[t] - 1u);
            if (!limit || s->action_ticks[t] > limit) return 0;
        }
        if (s->model.fired & (1u << t)) {
            unsigned dispatched = s->action_cursor[t] ? s->action_cursor[t] : n;
#ifdef SC64_P4_CREATE_PREFIX
            unsigned first = sc64_p4_source_triggers[t].first_action;
            expected_creates += sc64_p4_source_create_prefix[first + dispatched] -
                                sc64_p4_source_create_prefix[first];
#else
            for (unsigned a = 0; a < dispatched; ++a) {
                const Sc64P4SourceAction *row = &sc64_p4_source_actions[
                    sc64_p4_source_triggers[t].first_action + a];
                if (row->opcode == 11) ++expected_creates;
            }
#endif
        }
    }
    if (expected_creates != s->created_count) return 0;
    if (s->model.sequence_active &&
        s->action_cursor[s->model.sequence_trigger] != s->model.action_index + 1u)
        return 0;
    if (s->message_ticks) {
        int presentation_pending = 0;
        for (unsigned t = 0; t < SC64_P4_TRIGGER_COUNT; ++t) {
            u8 cursor = s->action_cursor[t];
            if (!cursor) continue;
            u8 op = sc64_p4_source_actions[
                sc64_p4_source_triggers[t].first_action + cursor - 1u].opcode;
            if (op == 7 || op == 8 || op == 9) presentation_pending = 1;
        }
        if (!presentation_pending) return 0;
    }
    return 1;
}

static int race_p4_source_owner(unsigned slot)
{
    if (slot == 0) return ENEMY;       /* Force 2 */
    if (slot == 3) return 4;           /* Force 3, a second Zerg force */
    if (slot == 1 || slot == 2) return RESCUABLE; /* OWNR rescue-passive Protoss */
    if (slot == 7) return SIDE;        /* Force 1 */
    return -1;
}

static int race_p4_source_for_trigger(unsigned t)
{
    /* The active trigger selectors are Force 1 for 0..5 and 11, Force 2 for
     * 6..10 and 12. P4 setup resolves these active forces to slots 7 and 0. */
    return ((t <= 5u) || t == 11u) ? 7 : 0;
}

static int race_p4_location(unsigned id, int *l, int *t, int *r, int *b0)
{
    const Sc64P4SourceLocation *p;
    if (!id || id > 64 || !l || !t || !r || !b0) return 0;
    p = &sc64_p4_source_locations[id - 1u];
    if (p->left >= p->right || p->top >= p->bottom ||
        p->right > SC64_P4_WIDTH_TILES * 32u ||
        p->bottom > SC64_P4_HEIGHT_TILES * 32u) return 0;
    *l = (int)(p->left / 2u); *t = (int)(p->top / 2u);
    *r = (int)(p->right / 2u); *b0 = (int)(p->bottom / 2u);
    return *l < *r && *t < *b0;
}

static int race_p4_location_center(unsigned id, int *x, int *y)
{
    int l, t, r, b0;
    if (!race_p4_location(id, &l, &t, &r, &b0)) return 0;
    *x = (l + r) / 2; *y = (t + b0) / 2;
    return *x >= 0 && *y >= 0 && *x < WORLD_W && *y < WORLD_H;
}

static int race_p4_source_owner_matches(unsigned selector, int raw_owner,
                                       int runtime_owner)
{
    int slot = raw_owner;
    /* Current Player is runtime ownership: a rescued actor now belongs to the
     * human even though its raw_owner remains the source provenance slot. */
    if (selector == SC64_P4_SELECTOR_CURRENT_PLAYER)
        return runtime_owner == SIDE;
    if (selector <= 7) return slot == (int)selector;
    if (selector == SC64_P4_SELECTOR_ALL_PLAYERS) return slot >= 0 && slot < 8;
    if (selector == SC64_P4_SELECTOR_FORCE1) return slot == 7;
    if (selector == SC64_P4_SELECTOR_FORCE2) return slot == 0;
    return -1;
}

static int __attribute__((unused))
race_p4_count_type(unsigned selector, unsigned raw, int buildings, int in_escape)
{
    int count = 0, l = 0, t = 0, r = WORLD_W, b0 = WORLD_H;
    if (in_escape && !race_p4_location(SC64_P4_LOCATION_ESCAPE,&l,&t,&r,&b0))
        return -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && !buildings) {
        int match = race_p4_source_owner_matches(selector, u[i].raw_owner, u[i].owner);
        int type = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        if (match < 0) return -1;
        if (match && (raw == 229 || type == (int)raw) && (!in_escape ||
            (u[i].x >= l && u[i].x < r && u[i].y >= t && u[i].y < b0))) ++count;
    }
    if (raw == 229 || raw == 231) for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int match = race_p4_source_owner_matches(selector, b[i].raw_owner, b[i].owner);
        int type = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        if (match < 0) return -1;
        if (match && (raw == 229 || type == (int)raw) && (!in_escape ||
            (b[i].x >= l && b[i].x < r && b[i].y >= t && b[i].y < b0))) ++count;
    }
    return count;
}

#ifdef SC64_P4_COUNT_CENSUS_ENABLED
typedef struct {
    int side_any;
    int all_ray;
    int all_tass;
    int cur_ray;
    int cur_tass;
    int esc_ray;
    int esc_tass;
} RaceP4CountSnapshot;

/* P4 evaluates seven related count conditions each active update. Preserve
 * the source's two ownership models in one pass: Current Player follows live
 * owner after rescue, while All Players follows raw source-owner provenance.
 * Source location tests use the actor center and half-open runtime rectangle. */
static int race_p4_count_snapshot(RaceP4CountSnapshot *out)
{
    int l, t, r, b0;
    if (!out || !race_p4_location(SC64_P4_LOCATION_ESCAPE, &l, &t, &r, &b0))
        return 0;
    out->side_any = 0;
    out->all_ray = 0;
    out->all_tass = 0;
    out->cur_ray = 0;
    out->cur_tass = 0;
    out->esc_ray = 0;
    out->esc_tass = 0;

    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int type = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        int current_player = u[i].owner == SIDE;
        int all_players = u[i].raw_owner >= 0 && u[i].raw_owner < 8;
        int in_escape = u[i].x >= l && u[i].x < r &&
                        u[i].y >= t && u[i].y < b0;
        if (current_player) {
            ++out->side_any;
            if (type == SC64_P4_RAW_RAYNOR_VULTURE) {
                ++out->cur_ray;
                if (in_escape) ++out->esc_ray;
            }
            if (type == SC64_P4_RAW_TASSADAR) {
                ++out->cur_tass;
                if (in_escape) ++out->esc_tass;
            }
        }
        if (all_players) {
            if (type == SC64_P4_RAW_RAYNOR_VULTURE) ++out->all_ray;
            if (type == SC64_P4_RAW_TASSADAR) ++out->all_tass;
        }
    }
    /* The Current Player Any Unit predicate includes buildings as in the
     * source selector; its other six predicates concern unit types only. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) ++out->side_any;
    return 1;
}
#endif

static unsigned race_p4_ticks(u32 ms)
{
    u32 n = (ms * SC64_P4_TICKS_PER_SECOND + 999u) / 1000u;
    if (!n) n = 1;
    return n > 36000u ? 36000u : (unsigned)n;
}

static int race_p4_create(const Sc64P4SourceAction *a)
{
    unsigned slot = (unsigned)race_p4_source_for_trigger(a->trigger);
    int x, y, type, owner;
    if (a->modifier != 0 || a->group2 > 64 ||
        (a->group2 && !sc64_p4_source_property_used[a->group2 - 1u]))
        return -SC64_P4_FAULT_PROPERTY;
    if (find_unit_slot() < 0 || race_p4.created_count >= SC64_P4_SOURCE_CREATE_COUNT)
        return -SC64_P4_FAULT_CAPACITY;
    if (!race_p4_location_center(a->location, &x, &y)) return -SC64_P4_FAULT_LOCATION;
    /* The startup Raynor is authored by the direct rescue player, while later
     * Current Player creates inherit their trigger's active force owner. */
    if (a->group1 <= 7u) slot = a->group1;
    else if (a->group1 != SC64_P4_SELECTOR_CURRENT_PLAYER)
        return -SC64_P4_FAULT_SELECTOR;
    owner = race_p4_source_owner(slot);
    type = sc64_race_unit_from_raw((int)a->unit);
    if (owner < 0 || type < 0) return -SC64_P4_FAULT_ACTOR;
    int n = spawn_unit(owner, type, x, y);
    if (n < 0) return -SC64_P4_FAULT_CAPACITY;
    u[n].raw_type = (int)a->unit;
    u[n].raw_owner = (int)slot;
    if (a->group2) {
        const unsigned char *p = sc64_p4_source_properties[a->group2 - 1u];
        u[n].hp = u[n].maxhp * p[5] / 100;
        /* P4 rows 3..6 request the burrowed state with special bit 0x02.
         * The map has no property row requesting invincibility. */
        if (p[14] & 0x02u) u[n].flags |= 2;
    }
#ifdef SC64_P4_STORM_ENABLED
    race_p4_storm_init_actor(n);
#endif
    ++race_p4.created_count;
    return 1;
}

static int race_p4_dispatch(const Sc64P4SourceAction *a, u16 *duration)
{
    int x, y;
    if (!a || !duration || a->opcode == 0) return -SC64_P4_FAULT_ACTION;
    *duration = 1;
    switch (a->opcode) {
    case 1: case 2: return 1;
    case 4: *duration = (u16)race_p4_ticks(a->time); return 1;
    /* Preserve PauseGame/UnpauseGame in the saved source cursor, but keep the
     * mission scheduler running through its authored list so it can reach the
     * matching Unpause. The outer game pause still freezes calls to this API. */
    case 5: race_p4.source_paused = 1; return 1;
    case 6: race_p4.source_paused = 0; return 1;
    case 7: case 8: case 9:
        *duration = SC64_P4_PRESENTATION_TICKS;
        race_p4.message_ticks = *duration;
        return 1;
    case 10:
        if (!race_p4_location_center(a->location,&x,&y)) return -SC64_P4_FAULT_LOCATION;
        cx=x; cy=y; return 1;
    case 11: return race_p4_create(a);
    case 12: race_campaign.objective_stage = a->trigger; return 1;
    case 13:
        if (a->group1 != 0 || (a->modifier != 4 && a->modifier != 5))
            return -SC64_P4_FAULT_SWITCH;
        if (a->group2 == 13u) race_p4.switch13_set = (a->modifier == 4);
        else if (a->group2 <= 1u) {
            if (a->modifier == 4) race_p4.switch_bits |= (u8)(1u << a->group2);
            else race_p4.switch_bits &= (u8)~(1u << a->group2);
        } else return -SC64_P4_FAULT_SWITCH;
        return 1;
    case 16:
        if (a->group2 != 879718992u) return -SC64_P4_FAULT_ACTION;
        race_p4.ai_script_mask |= 1u;
        /* The source script is opaque. Authored policy: send up to two idle
         * enemy combat units to the nearest human, without claiming original
         * script equivalence. */
        for (int i = 0, sent = 0; i < MAX_UNITS && sent < 2; ++i) if
            (u[i].alive && u[i].owner == ENEMY && u[i].order == ORDER_IDLE &&
             !sc64_race_is_worker(u[i].type) && sc64_race_unit_damage(u[i].type) > 0) {
                int tx=cx, ty=cy;
                for (int j=0;j<MAX_UNITS;++j) if (u[j].alive && u[j].owner==SIDE) {tx=u[j].x;ty=u[j].y;break;}
                u[i].order=ORDER_ATTACK_MOVE; u[i].tx=tx; u[i].ty=ty; ++sent;
            }
        return 1;
    case 26: {
        int owner;
        if (a->modifier != 7 || a->unit > 2 || a->group2 > 65535u)
            return -SC64_P4_FAULT_RESOURCE;
        owner = a->group1 == 13 ? SIDE : (a->group1 == 19 ? ENEMY : -1);
        if (owner < 0) return -SC64_P4_FAULT_SELECTOR;
        if (a->unit == 0 || a->unit == 2) race_minerals[owner]=(int)a->group2;
        if (a->unit == 1 || a->unit == 2) race_gas[owner]=(int)a->group2;
        if (owner==SIDE) {
            if (a->unit == 0 || a->unit == 2) minerals=(int)a->group2;
            if (a->unit == 1 || a->unit == 2) gas=(int)a->group2;
        }
        return 1;
    }
    case 30: race_p4.speech_muted=1; return 1;
    case 31: race_p4.speech_muted=0; return 1;
    default: return -SC64_P4_FAULT_ACTION;
    }
}

static int race_p4_start_action(unsigned trigger, unsigned index)
{
    const Sc64P4SourceTrigger *tr;
    const Sc64P4SourceAction *a;
    u16 duration;
    if (trigger >= SC64_P4_TRIGGER_COUNT) return 0;
    tr = &sc64_p4_source_triggers[trigger];
    if (index >= tr->action_count) return 0;
    a = &sc64_p4_source_actions[tr->first_action + index];
    if (a->trigger != trigger || a->index != index) return 0;
    int rc = race_p4_dispatch(a, &duration);
    if (rc <= 0) {
        unsigned f = (unsigned)(-rc);
        if (!f || f > 9) f = SC64_P4_FAULT_ACTION;
        race_p4.faults |= (u16)(1u << (f - 1u));
        return 0;
    }
    race_p4.action_cursor[trigger] = (u8)(index + 1u);
    race_p4.action_ticks[trigger] = duration;
    return 1;
}

static int race_p4_validate(void) { return race_p4_runtime_state_valid(&race_p4); }

static int race_p4_init(void)
{
    memset(&race_p4, 0, sizeof(race_p4));
    sc64_p4_init(&race_p4.model);
    if (!race_p4_active()) return 1;
    if (memcmp(SC64_P4_SOURCE_CHK_SHA256_HEX,
        "40f30ccedf87a3b9c88308dc3092611ceff10185ddaf32471104902042a93ecd",65))
        return 0;
    if (MAX_UNITS < 225 || race_p4_reserved_units() >
        (unsigned)(MAX_UNITS - 138)) {
        race_p4.faults |= 1u << (SC64_P4_FAULT_CAPACITY - 1u); return 0;
    }
    unsigned live = 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) ++live;
    /* Buildings have their own MAX_BUILDINGS pool and must not consume the
     * unit slots reserved for the 87 source-script actors. */
    if (live + SC64_P4_SOURCE_CREATE_COUNT > MAX_UNITS) {
        race_p4.faults |= 1u << (SC64_P4_FAULT_CAPACITY - 1u); return 0;
    }
#ifdef SC64_P4_CREATE_PREFIX
    unsigned source_create_rows = 0;
#endif
    unsigned base = 0;
    for (unsigned t=0; t<SC64_P4_TRIGGER_COUNT; ++t) {
        const Sc64P4SourceTrigger *tr=&sc64_p4_source_triggers[t];
#ifdef SC64_P4_CREATE_PREFIX
        unsigned action_total = sizeof(sc64_p4_source_actions) /
                                sizeof(sc64_p4_source_actions[0]);
        if (base > action_total || tr->first_action > action_total ||
            tr->action_count > action_total - tr->first_action ||
            tr->first_action != base)
            return 0;
#endif
        if (tr->first_action != base || tr->action_count==0) return 0;
#ifdef SC64_P4_CREATE_PREFIX
        if (!base && sc64_p4_source_create_prefix[0] != 0) return 0;
#endif
        for (unsigned i=0;i<tr->action_count;++i) {
            const Sc64P4SourceAction *a=&sc64_p4_source_actions[base+i];
            if (a->trigger != t || a->index != i || !a->opcode) return 0;
#ifdef SC64_P4_CREATE_PREFIX
            if (a->opcode == 11) ++source_create_rows;
            if (sc64_p4_source_create_prefix[base+i+1] != source_create_rows)
                return 0;
#endif
        }
        base += tr->action_count;
    }
#ifdef SC64_P4_CREATE_PREFIX
    if (base != sizeof(sc64_p4_source_actions) /
                sizeof(sc64_p4_source_actions[0]) ||
        source_create_rows != SC64_P4_SOURCE_CREATE_COUNT ||
        sc64_p4_source_create_prefix[sizeof(sc64_p4_source_actions) /
                                     sizeof(sc64_p4_source_actions[0])] !=
            source_create_rows)
        return 0;
#endif
    race_minerals[SIDE]=SC64_P4_START_MINERALS;
    minerals=SC64_P4_START_MINERALS;
    race_minerals[ENEMY]=SC64_P4_FORCE2_START_RESOURCES;
    race_gas[ENEMY]=SC64_P4_FORCE2_START_RESOURCES;
    return 1;
}

static int race_p4_campaign_tick(void)
{
    if (!race_p4_active()) return 1;
    if (!race_p4_runtime_state_valid(&race_p4)) {
        race_p4.faults |= 1u << (SC64_P4_FAULT_STATE-1u); return 0;
    }
    if (race_p4.model.outcome != SC64_P4_RUNNING) return 1;
    if (race_p4.message_ticks) --race_p4.message_ticks;
    u16 completed=0;
    for (unsigned t=0;t<SC64_P4_TRIGGER_COUNT;++t)
        if (race_p4.action_ticks[t] && !--race_p4.action_ticks[t]) completed |= (u16)(1u<<t);

    Sc64P4Input in; memset(&in,0,sizeof(in));
    int side_any, all_ray, all_tass, cur_ray, cur_tass, esc_ray, esc_tass;
#ifdef SC64_P4_COUNT_CENSUS_ENABLED
    RaceP4CountSnapshot counts;
    if (!race_p4_count_snapshot(&counts)) {
        race_p4.faults |= 1u << (SC64_P4_FAULT_SELECTOR-1u); return 0;
    }
    side_any=counts.side_any; all_ray=counts.all_ray; all_tass=counts.all_tass;
    cur_ray=counts.cur_ray; cur_tass=counts.cur_tass;
    esc_ray=counts.esc_ray; esc_tass=counts.esc_tass;
#else
    side_any=race_p4_count_type(13,229,0,0);
    all_ray=race_p4_count_type(17,19,0,0);
    all_tass=race_p4_count_type(17,79,0,0);
    cur_ray=race_p4_count_type(13,19,0,0);
    cur_tass=race_p4_count_type(13,79,0,0);
    esc_ray=race_p4_count_type(13,19,0,1);
    esc_tass=race_p4_count_type(13,79,0,1);
    if (side_any<0 || all_ray<0 || all_tass<0 || cur_ray<0 || cur_tass<0 || esc_ray<0 || esc_tass<0) {
        race_p4.faults |= 1u << (SC64_P4_FAULT_SELECTOR-1u); return 0;
    }
#endif
    in.current_any_units=(u16)side_any; in.all_ray_vultures=(u16)all_ray;
    in.all_tassadars=(u16)all_tass; in.current_ray_vultures=(u16)cur_ray;
    in.current_tassadars=(u16)cur_tass; in.escape_ray_vultures=(u16)esc_ray;
    in.escape_tassadars=(u16)esc_tass; in.switch0_set=!!(race_p4.switch_bits&1u);
    in.switch1_set=!!(race_p4.switch_bits&2u); in.switch13_set=race_p4.switch13_set;
    in.elapsed_time_ge_zero=1;

    u16 events=sc64_p4_step(&race_p4.model,&in);
    for (unsigned t=0;t<SC64_P4_TRIGGER_COUNT;++t) {
        if ((events & (1u<<t)) && t != 0u && t != 1u && t != 2u && t != 4u) {
            if (!race_p4_start_action(t,0)) return 0;
        } else if ((completed & (1u<<t)) && t != 1u && t != 2u && t != 4u) {
            unsigned next=race_p4.action_cursor[t];
            unsigned n=sc64_p4_source_triggers[t].action_count;
            if (next<n) { if (!race_p4_start_action(t,next)) return 0; }
            else race_p4.action_cursor[t]=0;
        }
    }
    if (events & (SC64_P4_EVENT_TRIGGER_1 | SC64_P4_EVENT_TRIGGER_2 |
                  SC64_P4_EVENT_TRIGGER_4)) {
        unsigned t = (events & SC64_P4_EVENT_TRIGGER_1) ? 1u :
                     (events & SC64_P4_EVENT_TRIGGER_2) ? 2u : 4u;
        if (!race_p4_start_action(t,0)) return 0;
    }
    if (race_p4.model.sequence_active) {
        unsigned t = race_p4.model.sequence_trigger;
        if (t < SC64_P4_TRIGGER_COUNT && (completed & (1u << t))) {
            u16 seq = sc64_p4_complete_action(&race_p4.model, 0);
            events |= seq;
            if (seq & SC64_P4_EVENT_ACTION) {
                if (!race_p4_start_action(t, race_p4.model.action_index)) return 0;
            } else if (race_p4.action_cursor[t]) {
                race_p4.action_cursor[t] = 0;
            }
        }
    }
    if (events & SC64_P4_EVENT_WON) {
        race_campaign.phase=SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags|=SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << SC64_P4_GLOBAL_ID; won=1; lost=0;
    } else if (events & SC64_P4_EVENT_LOST) {
        race_campaign.phase=SC64_RACE_PHASE_FAILED; won=0; lost=1;
    }
    return race_p4_runtime_state_valid(&race_p4);
}

#endif /* SC64_P4_RUNTIME */
#endif /* SC64_P4_ADAPTER_H */
