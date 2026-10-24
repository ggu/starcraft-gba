#ifndef SC64_Z4_ADAPTER_H
#define SC64_Z4_ADAPTER_H

#if defined(SC64_Z4_RUNTIME)

#include "sc64_z4.h"
#include "sc64_z4_scenario_generated.h"

/* Z4 source slots and bounded engine owners are deliberately explicit. The
 * authenticated setup assigns source slot 3 to the human, slot 1 to Force 2,
 * and slot 2 to Force 3. The runtime keeps the source slot on every actor. */
enum {
    SC64_Z4_GLOBAL_MISSION_ID = 15,
    SC64_Z4_TICKS_PER_SECOND = 30,
    SC64_Z4_TX_HOLD_TICKS = 90,
    SC64_Z4_FAULT_BAD_STATE = 1,
    SC64_Z4_FAULT_BAD_ACTION = 2,
    SC64_Z4_FAULT_CAPACITY = 3,
    SC64_Z4_FAULT_BAD_ACTOR = 4,
    SC64_Z4_FAULT_BAD_LOCATION = 5,
    SC64_Z4_FAULT_BAD_PROPERTY = 6,
    SC64_Z4_FAULT_BAD_SELECTOR = 7,
    SC64_Z4_FAULT_BAD_RESOURCE = 8,
    SC64_Z4_FAULT_BAD_SWITCH = 9,
    SC64_Z4_FAULT_BAD_SCRIPT = 10
};

static Sc64Z4RuntimeState race_z4;

static unsigned race_z4_ceil_ticks(u32 milliseconds);

static int
race_z4_active(void)
{
    return !race_mode && race_selected_mission == SC64_Z4_GLOBAL_MISSION_ID;
}

static const char *
race_z4_objective_text(void)
{
    return "Protect the Chrysalis until it hatches. Destroy the Force 3 Command Centers.";
}

static const char *
race_z4_victory_text(void)
{
    return "AGENT OF THE SWARM SECURED";
}

static const char *
race_z4_failure_text(void)
{
    return "THE CHRYSALIS OR KERRIGAN WAS LOST";
}

/* Derive the largest legal outstanding timer solely from the authenticated
 * action row and adapter timing policy. This is intentionally world-free so
 * save validation cannot perform an engine effect. */
static u16
race_z4_action_duration_limit(unsigned trigger, unsigned index)
{
    const Sc64Z4SourceTrigger *tr;
    const Sc64Z4SourceAction *a;
    if (trigger >= SC64_Z4_TRIGGER_COUNT) return 0;
    tr = &sc64_z4_source_triggers[trigger];
    if (index >= tr->action_count) return 0;
    a = &sc64_z4_source_actions[tr->first_action + index];
    if (a->trigger != trigger || a->index != index) return 0;
    switch (a->opcode) {
    case 4: case 8: return (u16)race_z4_ceil_ticks(a->time);
    case 7: case 9: return SC64_Z4_TX_HOLD_TICKS;
    case 1: case 2: case 3: case 10: case 11: case 12: case 13:
    case 14: case 15: case 16: case 22: case 23: case 25: case 26:
        return 1;
    default: return 0;
    }
}

static int
race_z4_runtime_state_valid(const Sc64Z4RuntimeState *s)
{
    if (!s || !sc64_z4_state_valid(&s->model) || s->faults ||
        (s->switch_bits & (u8)~1u) || s->countdown_active > 1 ||
        s->countdown_ticks > SC64_Z4_MISSION_COUNTDOWN * SC64_Z4_TICKS_PER_SECOND ||
        (!s->countdown_active && s->countdown_ticks) ||
        (s->ai_script_mask & (u8)~3u) || (s->hatch_effects & (u8)~3u) ||
        ((s->hatch_effects & 2u) && !(s->hatch_effects & 1u))) return 0;
    u8 startup_cursor = s->model.action_cursor[SC64_Z4_TRIGGER_STARTUP];
    int startup_fired = !!(s->model.fired & (1u << SC64_Z4_TRIGGER_STARTUP));
    if (s->countdown_active && (!startup_fired ||
        (startup_cursor && startup_cursor < 3))) return 0;
    if (startup_fired && (!startup_cursor || startup_cursor >= 3) &&
        !s->countdown_active) return 0;
    u8 hatch_cursor = s->model.action_cursor[SC64_Z4_TRIGGER_HATCH];
    int hatch_fired = !!(s->model.fired & (1u << SC64_Z4_TRIGGER_HATCH));
    if (!hatch_fired && s->hatch_effects) return 0;
    if (hatch_fired) {
        /* Cursor is index+1 after dispatch. Action 3 removes Chrysalis; action
         * 6 creates Kerrigan. A cleared cursor means the whole list completed. */
        if (!hatch_cursor) {
            if (s->hatch_effects != 3u) return 0;
        } else {
            if ((hatch_cursor >= 4u) != !!(s->hatch_effects & 1u)) return 0;
            if ((hatch_cursor >= 7u) != !!(s->hatch_effects & 2u)) return 0;
        }
    }
    for (unsigned i = 0; i < SC64_Z4_TRIGGER_COUNT; ++i) {
        if ((s->action_ticks[i] != 0) !=
            (s->model.action_cursor[i] != 0)) return 0;
        if (s->action_ticks[i]) {
            u8 cursor = s->model.action_cursor[i];
            u16 limit = race_z4_action_duration_limit(i, cursor - 1u);
            if (!limit || s->action_ticks[i] > limit) return 0;
        }
    }
    if (s->message_ticks > SC64_Z4_TX_HOLD_TICKS) return 0;
    if (s->message_ticks) {
        int presentation_pending = 0;
        for (unsigned i = 0; i < SC64_Z4_TRIGGER_COUNT; ++i) {
            u8 cursor = s->model.action_cursor[i];
            if (cursor && (sc64_z4_source_actions[
                    sc64_z4_source_triggers[i].first_action + cursor - 1u].opcode == 7 ||
                sc64_z4_source_actions[
                    sc64_z4_source_triggers[i].first_action + cursor - 1u].opcode == 9))
                presentation_pending = 1;
        }
        if (!presentation_pending) return 0;
    }
    return 1;
}

static int
race_z4_source_owner(unsigned source_slot)
{
    switch (source_slot) {
    case 1: return ENEMY; /* Force 2 */
    case 2: return 4;     /* Force 3 */
    case 3: return SIDE;  /* human */
    default: return -1;
    }
}

static int
race_z4_selector_matches(unsigned selector, unsigned source_slot,
                         int runtime_owner, int raw_owner)
{
    int source = raw_owner >= 0 ? raw_owner : -1;
    if (selector <= 7) return source == (int)selector;
    if (selector == 13) return source == 3;
    if (selector == 17) return source >= 0 && source < 8;
    if (selector >= 18 && selector <= 21) {
        unsigned force = selector - 17u;
        if (source < 0 || source >= 8) return 0;
        return (unsigned)sc64_z4_source_forces[source] + 1u == force;
    }
    (void)source_slot;
    (void)runtime_owner;
    return -1;
}

static int
race_z4_owner_for_selector(unsigned selector, unsigned *source_slot)
{
    unsigned slot = 0xffu;
    if (selector <= 7) slot = selector;
    else if (selector == 13) slot = 3;
    else if (selector >= 18 && selector <= 21) {
        unsigned force = selector - 17u;
        for (unsigned i = 0; i < 8; ++i)
            if ((unsigned)sc64_z4_source_forces[i] + 1u == force &&
                race_z4_source_owner(i) >= 0) {
                if (slot != 0xffu) return 0; /* Ambiguous runtime owner. */
                slot = i;
            }
    }
    if (slot >= 8 || race_z4_source_owner(slot) < 0) return 0;
    *source_slot = slot;
    return 1;
}

static int
race_z4_location(unsigned id, int *left, int *top, int *right, int *bottom)
{
    const Sc64Z4SourceLocation *l;
    if (!id || id > 64 || !left || !top || !right || !bottom) return 0;
    l = &sc64_z4_source_locations[id - 1];
    if (l->left >= l->right || l->top >= l->bottom ||
        l->right > 96u * 32u || l->bottom > 96u * 32u) return 0;
    *left = (int)(l->left / 2u);
    *top = (int)(l->top / 2u);
    *right = (int)(l->right / 2u);
    *bottom = (int)(l->bottom / 2u);
    return *left < *right && *top < *bottom;
}

static int
race_z4_location_center(unsigned id, int *x, int *y)
{
    int left, top, right, bottom;
    if (!race_z4_location(id, &left, &top, &right, &bottom)) return 0;
    *x = (left + right) / 2;
    *y = (top + bottom) / 2;
    return *x >= 0 && *y >= 0 && *x < WORLD_W && *y < WORLD_H;
}

static unsigned
race_z4_ceil_ticks(u32 milliseconds)
{
    u32 ticks = (milliseconds * SC64_Z4_TICKS_PER_SECOND + 999u) / 1000u;
    if (!ticks) ticks = 1;
    return ticks > 36000u ? 36000u : (unsigned)ticks;
}

static int
race_z4_count_owner_type(unsigned selector, unsigned unit, int buildings_only)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && !buildings_only) {
        int selected = race_z4_selector_matches(selector, 3, u[i].owner, u[i].raw_owner);
        int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        if (selected < 0) return -1;
        if (selected && (unit == 229 || !unit || raw == (int)unit)) ++count;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int selected = race_z4_selector_matches(selector, 3, b[i].owner, b[i].raw_owner);
        int raw = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        if (selected < 0) return -1;
        if (selected && (unit == 229 || !unit || raw == (int)unit)) ++count;
    }
    return count;
}

static int
race_z4_property_valid(unsigned raw_index)
{
    if (!raw_index) return 1;
    if (raw_index > 64 || !sc64_z4_source_property_used[raw_index - 1]) return 0;
    /* The three Z4 property records are identical full-health, no-owner-override
     * records. Reject unknown fields rather than silently dropping semantics. */
    const unsigned char *p = sc64_z4_source_properties[raw_index - 1];
    return p[0] == 24 && p[1] == 0 && p[2] == 2 && p[3] == 0 &&
           p[4] == 255 && p[5] == 100 && p[6] == 0 && p[7] == 0 &&
           p[8] == 0 && p[9] == 0 && p[10] == 0 && p[11] == 0 &&
           p[12] == 0 && p[13] == 0 && p[14] == 16 && p[15] == 0 &&
           p[16] == 0 && p[17] == 0 && p[18] == 0 && p[19] == 0;
}

static int
race_z4_create_action(const Sc64Z4SourceAction *a)
{
    unsigned source_slot;
    int x, y, type, owner;
    if (!a) return -SC64_Z4_FAULT_BAD_ACTION;
    if (!race_z4_owner_for_selector(a->group1, &source_slot))
        return -SC64_Z4_FAULT_BAD_SELECTOR;
    if (!race_z4_location_center(a->location, &x, &y))
        return -SC64_Z4_FAULT_BAD_LOCATION;
    if (a->modifier != 0) return -SC64_Z4_FAULT_BAD_ACTION;
    if (!race_z4_property_valid(a->group2)) return -SC64_Z4_FAULT_BAD_PROPERTY;
    owner = race_z4_source_owner(source_slot);
    type = sc64_race_unit_from_raw((int)a->unit);
    if (type < 0) return -SC64_Z4_FAULT_BAD_ACTOR;
    /* For these authored rows group2 selects one-based UPRP (or zero for
     * defaults); modifier is the opcode-11 count and is zero on the source,
     * which the pinned handler defines as a one-unit default. */
    if (find_unit_slot() < 0) return -SC64_Z4_FAULT_CAPACITY;
    int slot = spawn_unit(owner, type, x, y);
    if (slot < 0) return -SC64_Z4_FAULT_CAPACITY;
    u[slot].raw_type = a->unit;
    u[slot].raw_owner = (int)source_slot;
    if (a->group2) {
        const unsigned char *property = sc64_z4_source_properties[a->group2 - 1u];
        u[slot].hp = u[slot].maxhp * property[5] / 100;
        /* The pinned OpenBW CHK loader sets invincibility when valid_flags
         * and flags both contain 0x10. The Z4 records have valid_special
         * 0x18 and special 0x10. Keep both engine representations aligned. */
        if ((property[0] & 0x10u) && (property[14] & 0x10u)) {
            u[slot].flags |= 16;
            invincible_units[slot] = 1;
        }
    }
    return 1;
}

static int
race_z4_remove_selected(unsigned selector, unsigned raw_type,
                        int at_location, unsigned location, int kill)
{
    int left = 0, top = 0, right = WORLD_W, bottom = WORLD_H;
    (void)kill; /* Native Kill/Remove outcomes share actor deletion in this engine. */
    if (selector != 17 && selector != 13 && !(selector <= 7) &&
        !(selector >= 18 && selector <= 21)) return -SC64_Z4_FAULT_BAD_SELECTOR;
    if (at_location && !race_z4_location(location, &left, &top, &right, &bottom))
        return -SC64_Z4_FAULT_BAD_LOCATION;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int selected = race_z4_selector_matches(selector, 3, u[i].owner, u[i].raw_owner);
        int raw = u[i].raw_type >= 0 ? u[i].raw_type : sc64_race_raw_unit(u[i].type);
        if (selected < 0) return -SC64_Z4_FAULT_BAD_SELECTOR;
        if (!selected || (raw_type != 229 && raw != (int)raw_type) ||
            (at_location && (u[i].x < left || u[i].x >= right ||
                             u[i].y < top || u[i].y >= bottom))) continue;
        if (u[i].owner == SIDE) supply_used -= unit_supply(u[i].type);
        u[i].alive = 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        int selected = race_z4_selector_matches(selector, 3, b[i].owner, b[i].raw_owner);
        int raw = b[i].raw_type >= 0 ? b[i].raw_type : sc64_race_raw_building(b[i].type);
        if (selected < 0) return -SC64_Z4_FAULT_BAD_SELECTOR;
        if (!selected || (raw_type != 229 && raw != (int)raw_type) ||
            (at_location && (b[i].x < left || b[i].x >= right ||
                             b[i].y < top || b[i].y >= bottom))) continue;
        b[i].alive = 0;
        if (selected_building == i) selected_building = -1;
    }
    supply_used = race_owner_supply(SIDE);
    supply_cap = race_owner_supply_cap(SIDE);
    return 1;
}

static int
race_z4_set_resources(const Sc64Z4SourceAction *a)
{
    int owner;
    if (!a || a->modifier != 7 || a->unit != 2) return 0;
    if (a->group1 == 3) owner = SIDE;
    else if (a->group1 == 19) owner = ENEMY;
    else return 0;
    if (a->group2 > 65535u) return 0;
    race_minerals[owner] = (int)a->group2;
    race_gas[owner] = (int)a->group2;
    if (owner == SIDE) { minerals = (int)a->group2; gas = (int)a->group2; }
    return 1;
}

static int
race_z4_ai_script(const Sc64Z4SourceAction *a)
{
    unsigned bit, location;
    u32 script;
    int left, top, right, bottom;
    if (!a || a->opcode != 16 || a->group1 != 0) return 0;
    if (a->group2 == 879912282u) { bit = 1; location = 4; script = a->group2; }
    else if (a->group2 == 1395942746u) { bit = 2; location = 5; script = a->group2; }
    else return 0;
    if (a->location != location || !race_z4_location(location, &left, &top, &right, &bottom))
        return 0;
    (void)script;
    race_z4.ai_script_mask |= (u8)bit;
    /* Authored bounded pressure in place of the opaque source AI script: at
     * most two existing idle Force 2 combat units move toward the human. */
    int tx = cx, ty = cy;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        tx = u[i].x; ty = u[i].y; break;
    }
    int sent = 0;
    for (int i = 0; i < MAX_UNITS && sent < 2; ++i) if (u[i].alive &&
        u[i].raw_owner == 1 && u[i].order == ORDER_IDLE &&
        !sc64_race_is_worker(u[i].type) && sc64_race_unit_damage(u[i].type) > 0 &&
        u[i].x >= left && u[i].x < right && u[i].y >= top && u[i].y < bottom) {
        u[i].order = ORDER_ATTACK_MOVE; u[i].tx = tx; u[i].ty = ty;
        ++sent; ++race_campaign.enemy_units_sent;
    }
    if (sent) ++race_campaign.enemy_wave;
    return 1;
}

static int
race_z4_dispatch_action(const Sc64Z4SourceAction *a, u16 *duration)
{
    if (!a || !duration || a->trigger >= SC64_Z4_TRIGGER_COUNT ||
        a->index >= sc64_z4_action_count(a->trigger)) return -SC64_Z4_FAULT_BAD_ACTION;
    *duration = 1; /* Even instant engine effects complete on the following tick. */
    switch (a->opcode) {
    case 1: case 2: case 3: case 12: case 15:
        return 1;
    case 4: case 8:
        *duration = (u16)race_z4_ceil_ticks(a->time);
        return 1;
    case 7: case 9:
        /* Dialogue/WAV bytes are intentionally absent. Preserve action order
         * with a documented 3-second presentation hold. */
        *duration = SC64_Z4_TX_HOLD_TICKS;
        race_z4.message_ticks = *duration;
        return 1;
    case 10: {
        int x, y;
        if (!race_z4_location_center(a->location, &x, &y)) return -SC64_Z4_FAULT_BAD_LOCATION;
        cx = x; cy = y; return 1;
    }
    case 11: {
        int result = race_z4_create_action(a);
        if (result > 0 && a->trigger == SC64_Z4_TRIGGER_HATCH && a->index == 6 &&
            a->unit == SC64_Z4_UNIT_KERRIGAN)
            race_z4.hatch_effects |= 2u;
        return result;
    }
    case 13:
        if (a->group1 != 0) return -SC64_Z4_FAULT_BAD_SWITCH;
        if (a->modifier == 4) race_z4.switch_bits |= 1u;
        else if (a->modifier == 5) race_z4.switch_bits &= (u8)~1u;
        else return -SC64_Z4_FAULT_BAD_SWITCH;
        return 1;
    case 14:
        if (a->modifier != 7 || a->time > SC64_Z4_MISSION_COUNTDOWN)
            return -SC64_Z4_FAULT_BAD_ACTION;
        race_z4.countdown_ticks = (u16)(a->time * SC64_Z4_TICKS_PER_SECOND);
        race_z4.countdown_active = 1;
        return 1;
    case 16:
        return race_z4_ai_script(a) ? 1 : -SC64_Z4_FAULT_BAD_SCRIPT;
    case 22: {
        int before = -1;
        if (a->trigger == SC64_Z4_TRIGGER_HATCH && a->index == 3 &&
            a->unit == SC64_Z4_UNIT_CHRYSALIS) {
            before = race_z4_count_owner_type(13, SC64_Z4_UNIT_CHRYSALIS, 0);
            if (before < 0) return -SC64_Z4_FAULT_BAD_SELECTOR;
        }
        int result = race_z4_remove_selected(a->group1, a->unit, 0, 0, 1);
        if (result > 0 && before > 0) {
            int after = race_z4_count_owner_type(13, SC64_Z4_UNIT_CHRYSALIS, 0);
            if (after < 0) return -SC64_Z4_FAULT_BAD_SELECTOR;
            if (!after) race_z4.hatch_effects |= 1u;
        }
        return result;
    }
    case 23: return race_z4_remove_selected(a->group1, a->unit, 1, a->location, 1);
    case 25: return race_z4_remove_selected(a->group1, a->unit, 1, a->location, 0);
    case 26:
        return race_z4_set_resources(a) ? 1 : -SC64_Z4_FAULT_BAD_RESOURCE;
    default:
        return -SC64_Z4_FAULT_BAD_ACTION;
    }
}

static int
race_z4_init(void)
{
    memset(&race_z4, 0, sizeof(race_z4));
    (void)sc64_z4_init(&race_z4.model);
    if (!race_z4_active()) return 1;
    static const unsigned char expected_counts[7] = {5,14,1,24,14,3,5};
    unsigned base = 0;
    if (memcmp(SC64_Z4_SOURCE_CHK_SHA256_HEX,
               "2cb8e516f07651801c1d6f519017d494af87c479a0ba8df9e33af34b14ec685a", 65) ||
        sc64_z4_source_owners[3] != 6 ||
        sc64_z4_source_forces[0] != 3 || sc64_z4_source_forces[1] != 1 ||
        sc64_z4_source_forces[2] != 2 || sc64_z4_source_forces[3] != 0) return 0;
    for (unsigned t = 0; t < 7; ++t) {
        const Sc64Z4SourceTrigger *tr = &sc64_z4_source_triggers[t];
        if (tr->first_action != base || tr->action_count != expected_counts[t]) return 0;
        for (unsigned a = 0; a < tr->action_count; ++a) {
            const Sc64Z4SourceAction *row = &sc64_z4_source_actions[base + a];
            if (row->trigger != t || row->index != a || !row->opcode) return 0;
        }
        base += tr->action_count;
    }
    if (base != 66) return 0;
    race_campaign.objective_stage = 0;
    return 1;
}

static int
race_z4_dispatch_events(const Sc64Z4Output *out)
{
    if (!out) return 0;
    for (unsigned i = 0; i < out->action_count; ++i) {
        const Sc64Z4ActionEvent *event = &out->actions[i];
        unsigned trig = event->trigger;
        unsigned first, count;
        u16 duration;
        int result;
        if (trig >= SC64_Z4_TRIGGER_COUNT) return 0;
        first = sc64_z4_source_triggers[trig].first_action;
        count = sc64_z4_source_triggers[trig].action_count;
        if (event->action_index >= count || first + event->action_index >= 66) return 0;
        const Sc64Z4SourceAction *action = &sc64_z4_source_actions[first + event->action_index];
        if (action->trigger != trig || action->index != event->action_index) return 0;
        result = race_z4_dispatch_action(action, &duration);
        if (result <= 0) {
            unsigned fault = (unsigned)(-result);
            if (!fault || fault > 10) fault = SC64_Z4_FAULT_BAD_ACTION;
            race_z4.faults |= 1u << (fault - 1u);
            return 0;
        }
        race_z4.action_ticks[trig] = duration;
    }
    return 1;
}

static void
race_z4_apply_result(u16 events)
{
    if (events & SC64_Z4_EVENT_WON) {
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << SC64_Z4_GLOBAL_MISSION_ID;
        won = 1; lost = 0;
    } else if (events & SC64_Z4_EVENT_LOST) {
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
        won = 0; lost = 1;
    }
}

static int
race_z4_campaign_tick(void)
{
    Sc64Z4Input in;
    Sc64Z4Output out;
    u8 completed = 0;
    if (!race_z4_active()) return 1;
    if (!race_z4_runtime_state_valid(&race_z4)) {
        race_z4.faults |= 1u << (SC64_Z4_FAULT_BAD_STATE - 1u);
        return 0;
    }
    if (race_z4.model.outcome != SC64_Z4_RUNNING) return 1;
    if (race_z4.message_ticks) --race_z4.message_ticks;
    if (race_z4.countdown_active && race_z4.countdown_ticks)
        --race_z4.countdown_ticks;
    for (unsigned i = 0; i < SC64_Z4_TRIGGER_COUNT; ++i) {
        if (race_z4.action_ticks[i] && !--race_z4.action_ticks[i])
            completed |= (u8)(1u << i);
    }
    memset(&in, 0, sizeof(in));
    int kerrigan_count = race_z4_count_owner_type(13, 51, 0);
    int chrysalis_count = race_z4_count_owner_type(13, 150, 0);
    int command_centers = race_z4_count_owner_type(20, 106, 1);
    int force2_units = race_z4_count_owner_type(19, 229, 0);
    if (kerrigan_count < 0 || chrysalis_count < 0 || command_centers < 0 ||
        force2_units < 0 || kerrigan_count > 65535 || chrysalis_count > 65535 ||
        command_centers > 65535 || force2_units > 65535) {
        race_z4.faults |= 1u << (SC64_Z4_FAULT_BAD_SELECTOR - 1u);
        return 0;
    }
    in.kerrigan_count = (u16)kerrigan_count;
    in.chrysalis_count = (u16)chrysalis_count;
    in.command_center_count = (u16)command_centers;
    in.force2_any_units = (u16)force2_units;
    in.countdown_remaining = (u16)((race_z4.countdown_ticks + 29u) / 30u);
    in.elapsed_seconds = (u8)(race_campaign.elapsed_frames / 30u > 255u ? 255u :
                               race_campaign.elapsed_frames / 30u);
    in.countdown_active = race_z4.countdown_active;
    /* CHK Switch comparison 2 means set; comparison 3 means cleared. */
    in.switch_condition_2 = !!(race_z4.switch_bits & 1u);
    in.switch_condition_3 = !(race_z4.switch_bits & 1u);
    in.suppress_chrysalis_loss = (race_z4.hatch_effects & 1u) &&
                                 !(race_z4.hatch_effects & 2u);
    in.kerrigan_survival_armed = !!(race_z4.hatch_effects & 2u);
    in.complete_actions = completed;
    in.paused = 0;
    out = sc64_z4_step(&race_z4.model, &in);
    if (!race_z4_dispatch_events(&out)) return 0;
    if (!race_z4_runtime_state_valid(&race_z4)) {
        race_z4.faults |= 1u << (SC64_Z4_FAULT_BAD_STATE - 1u);
        return 0;
    }
    if (out.events & SC64_Z4_EVENT_TRIGGER_HATCH)
        race_campaign.objective_stage = 1;
    race_z4_apply_result(out.events);
    return 1;
}

#endif /* SC64_Z4_RUNTIME */
#endif /* SC64_Z4_ADAPTER_H */
