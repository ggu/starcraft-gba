#ifndef SC64_P4_H
#define SC64_P4_H

#include "../common/gba.h"

/* Compact P4 rule model for authenticated BOLT 008/021. This is not a CHK
 * interpreter. Its input counts are already resolved by a future adapter
 * against P4's OWNR/SIDE/FORC setup and current owner policy. */
enum {
    SC64_P4_GLOBAL_ID = 23,
    SC64_P4_WIDTH_TILES = 128,
    SC64_P4_HEIGHT_TILES = 128,
    SC64_P4_TRIGGER_COUNT = 13,
    SC64_P4_HUMAN_SOURCE_SLOT = 7,

    /* CHK player-group selectors (not trigger execution-owner indices). */
    SC64_P4_SELECTOR_CURRENT_PLAYER = 13,
    SC64_P4_SELECTOR_FOES = 14,
    SC64_P4_SELECTOR_ALL_PLAYERS = 17,
    SC64_P4_SELECTOR_FORCE1 = 18,
    SC64_P4_SELECTOR_FORCE2 = 19,

    /* Raw unit IDs from the pinned StarCraft unit table. */
    SC64_P4_RAW_RAYNOR_VULTURE = 19,
    SC64_P4_RAW_TASSADAR = 79,
    SC64_P4_RAW_ALDARIS = 87,
    SC64_P4_RAW_HYDRALISK = 38,
    SC64_P4_RAW_ULTRALISK = 39,
    SC64_P4_RAW_ANY_UNIT = 229,
    SC64_P4_RAW_BUILDINGS = 231,

    /* The named source location predicates and raw switches used by rules. */
    SC64_P4_LOCATION_ESCAPE = 1,
    SC64_P4_SWITCH_GATE_A = 0,
    SC64_P4_SWITCH_GATE_B = 1,
    SC64_P4_SWITCH_INTRO = 13,

    /* Authored numeric action facts. Wait values are not converted to ticks. */
    SC64_P4_FORCE2_START_RESOURCES = 8000,
    SC64_P4_START_MINERALS = 50,
    SC64_P4_RESULT_WAIT_MS = 1000,
    SC64_P4_LOSS_AUDIO_DURATION = 403
};

enum { SC64_P4_RUNNING = 0, SC64_P4_WON = 1, SC64_P4_LOST = 2 };
enum {
    SC64_P4_SEQ_NONE = 0,
    SC64_P4_SEQ_LOSS_RAYNOR = 1,
    SC64_P4_SEQ_LOSS_TASSADAR = 2,
    SC64_P4_SEQ_ESCAPE = 4
};

/* Bits 0..12 report the corresponding source TRIG record becoming eligible
 * and one-shot in this model; they do not contain its action payload. */
enum {
    SC64_P4_EVENT_TRIGGER_0 = 1u << 0,
    SC64_P4_EVENT_TRIGGER_1 = 1u << 1,
    SC64_P4_EVENT_TRIGGER_2 = 1u << 2,
    SC64_P4_EVENT_TRIGGER_3 = 1u << 3,
    SC64_P4_EVENT_TRIGGER_4 = 1u << 4,
    SC64_P4_EVENT_TRIGGER_5 = 1u << 5,
    SC64_P4_EVENT_TRIGGER_6 = 1u << 6,
    SC64_P4_EVENT_TRIGGER_7 = 1u << 7,
    SC64_P4_EVENT_TRIGGER_8 = 1u << 8,
    SC64_P4_EVENT_TRIGGER_9 = 1u << 9,
    SC64_P4_EVENT_TRIGGER_10 = 1u << 10,
    SC64_P4_EVENT_TRIGGER_11 = 1u << 11,
    SC64_P4_EVENT_TRIGGER_12 = 1u << 12,
    SC64_P4_EVENT_ACTION = 1u << 13,
    SC64_P4_EVENT_WON = 1u << 14,
    SC64_P4_EVENT_LOST = 1u << 15
};

enum {
    SC64_P4_F_TRIGGER_0 = 1u << 0,
    SC64_P4_F_TRIGGER_1 = 1u << 1,
    SC64_P4_F_TRIGGER_2 = 1u << 2,
    SC64_P4_F_TRIGGER_3 = 1u << 3,
    SC64_P4_F_TRIGGER_4 = 1u << 4,
    SC64_P4_F_TRIGGER_5 = 1u << 5,
    SC64_P4_F_TRIGGER_6 = 1u << 6,
    SC64_P4_F_TRIGGER_7 = 1u << 7,
    SC64_P4_F_TRIGGER_8 = 1u << 8,
    SC64_P4_F_TRIGGER_9 = 1u << 9,
    SC64_P4_F_TRIGGER_10 = 1u << 10,
    SC64_P4_F_TRIGGER_11 = 1u << 11,
    SC64_P4_F_TRIGGER_12 = 1u << 12
};

typedef struct {
    u16 fired;
    u8 outcome;
    u8 sequence_trigger; /* 1, 2 or 4 while a result sequence is active. */
    u8 action_index;
    u8 sequence_active;
    u8 reserved;
} Sc64P4State;

typedef struct {
    u16 current_any_units;  /* Current Player 13 / Any Unit 229. */
    u16 all_ray_vultures;   /* All Players 17 / raw Raynor Vulture 19. */
    u16 all_tassadars;      /* All Players 17 / raw Tassadar 79. */
    u16 current_ray_vultures; /* Current Player / raw 19. */
    u16 current_tassadars;    /* Current Player / raw 79. */
    u16 escape_ray_vultures;  /* Bring Current Player, location 1, raw 19. */
    u16 escape_tassadars;     /* Bring Current Player, location 1, raw 79. */
    u8 switch0_set;
    u8 switch1_set;
    u8 switch13_set;
    u8 elapsed_time_ge_zero;
    u8 paused;
} Sc64P4Input;

/* The source model alone is not enough to resume a campaign action list. Keep
 * cursors and waits with it so a save made during a transmission/wait resumes
 * at the exact next numeric action. */
typedef struct {
    Sc64P4State model;
    u16 action_ticks[SC64_P4_TRIGGER_COUNT];
    u16 message_ticks;
    u16 created_count;
    u8 action_cursor[SC64_P4_TRIGGER_COUNT];
    u8 switch_bits;       /* source switches 0 and 1 */
    u8 source_paused;     /* authored PauseGame marker; outer pause freezes tick */
    u8 speech_muted;
    u8 ai_script_mask;    /* opaque source script seen; authored bounded policy */
    u8 switch13_set;      /* Raynor survival condition gate */
    u16 faults;
} Sc64P4RuntimeState;

static inline u16 sc64_p4_init(Sc64P4State *s)
{
    u8 *p;
    u32 i;
    if (!s) return 0;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    return 0;
}

static inline int sc64_p4_state_valid(const Sc64P4State *s)
{
    const u16 known = (u16)((1u << SC64_P4_TRIGGER_COUNT) - 1u);
    const u16 result_bits = SC64_P4_F_TRIGGER_0 | SC64_P4_F_TRIGGER_1 |
                            SC64_P4_F_TRIGGER_2 | SC64_P4_F_TRIGGER_4;
    u16 results;
    u8 action_count;
    if (!s || (s->fired & (u16)~known) || s->outcome > SC64_P4_LOST ||
        s->sequence_active > 1 || s->reserved != 0) return 0;
    results = s->fired & result_bits;
    if (results && (results & (u16)(results - 1u))) return 0;
    if (!s->sequence_active) {
        if (s->sequence_trigger != SC64_P4_SEQ_NONE || s->action_index != 0)
            return 0;
        if (s->outcome == SC64_P4_WON)
            return results == SC64_P4_F_TRIGGER_4;
        if (s->outcome == SC64_P4_LOST)
            return results == SC64_P4_F_TRIGGER_0 ||
                   results == SC64_P4_F_TRIGGER_1 ||
                   results == SC64_P4_F_TRIGGER_2;
        return results == 0;
    }
    if (s->outcome != SC64_P4_RUNNING) return 0;
    if (s->sequence_trigger == SC64_P4_SEQ_LOSS_RAYNOR ||
        s->sequence_trigger == SC64_P4_SEQ_LOSS_TASSADAR) action_count = 6;
    else if (s->sequence_trigger == SC64_P4_SEQ_ESCAPE) action_count = 8;
    else return 0;
    if (s->action_index >= action_count) return 0;
    if (s->sequence_trigger == SC64_P4_SEQ_LOSS_RAYNOR) {
        if (results != SC64_P4_F_TRIGGER_1) return 0;
    } else if (s->sequence_trigger == SC64_P4_SEQ_LOSS_TASSADAR) {
        if (results != SC64_P4_F_TRIGGER_2) return 0;
    } else if (results != SC64_P4_F_TRIGGER_4) return 0;
    return 1;
}

/* Record 0 is Current Player Any Unit <= 0. Records 1/2 are All Players raw
 * unit-type zero counts (record 1 also requires switch 13). Record 3 requires
 * both named actors alive; record 4 requires both in source location 1.
 * Records 5..12 are represented as one-shot gate/timer events for an adapter
 * to handle using private source metadata. Same-frame result precedence is an
 * explicit policy: immediate survival loss, record 1, record 2, then escape.
 * Separate source records do not establish their inter-trigger order. */
static inline u16 sc64_p4_step(Sc64P4State *s, const Sc64P4Input *in)
{
    u16 events = 0;
    if (!sc64_p4_state_valid(s) || !in || in->paused || s->outcome) return 0;

#define SC64_P4_FIRE_IF(rec, cond) do { \
    if (!(s->fired & (u16)(1u << (rec))) && (cond)) { \
        s->fired |= (u16)(1u << (rec)); \
        events |= (u16)(1u << (rec)); \
    } \
} while (0)
    SC64_P4_FIRE_IF(11, in->elapsed_time_ge_zero);
    SC64_P4_FIRE_IF(3, in->current_ray_vultures && in->current_tassadars);
    SC64_P4_FIRE_IF(5, in->switch0_set);
    SC64_P4_FIRE_IF(6, in->switch0_set);
    SC64_P4_FIRE_IF(7, in->switch0_set);
    SC64_P4_FIRE_IF(8, in->switch0_set);
    SC64_P4_FIRE_IF(9, in->switch1_set);
    SC64_P4_FIRE_IF(10, in->switch0_set);
    SC64_P4_FIRE_IF(12, in->switch0_set);
#undef SC64_P4_FIRE_IF

    if (s->sequence_active) return events;
    if (in->current_any_units == 0) {
        s->fired |= SC64_P4_F_TRIGGER_0;
        s->outcome = SC64_P4_LOST;
        return (u16)(events | SC64_P4_EVENT_TRIGGER_0 | SC64_P4_EVENT_LOST);
    }
    if (in->all_ray_vultures == 0 && in->switch13_set) {
        s->fired |= SC64_P4_F_TRIGGER_1;
        s->sequence_trigger = SC64_P4_SEQ_LOSS_RAYNOR;
        s->sequence_active = 1;
        return (u16)(events | SC64_P4_EVENT_TRIGGER_1 | SC64_P4_EVENT_ACTION);
    }
    if (in->all_tassadars == 0) {
        s->fired |= SC64_P4_F_TRIGGER_2;
        s->sequence_trigger = SC64_P4_SEQ_LOSS_TASSADAR;
        s->sequence_active = 1;
        return (u16)(events | SC64_P4_EVENT_TRIGGER_2 | SC64_P4_EVENT_ACTION);
    }
    if (in->escape_ray_vultures && in->escape_tassadars) {
        s->fired |= SC64_P4_F_TRIGGER_4;
        s->sequence_trigger = 4;
        s->sequence_active = 1;
        return (u16)(events | SC64_P4_EVENT_TRIGGER_4 | SC64_P4_EVENT_ACTION);
    }
    return events;
}

/* Action completion timing is adapter policy. Source result sequences have
 * two 1000 ms Wait actions (raw CHK values); this model does not equate those
 * milliseconds with 30 Hz simulation frames. */
static inline u16 sc64_p4_complete_action(Sc64P4State *s, u8 paused)
{
    u8 count;
    u8 trigger;
    if (!sc64_p4_state_valid(s) || paused || s->outcome || !s->sequence_active)
        return 0;
    trigger = s->sequence_trigger;
    count = trigger == SC64_P4_SEQ_ESCAPE ? 8 : 6;
    if ((u8)(s->action_index + 1) < count) {
        ++s->action_index;
        return SC64_P4_EVENT_ACTION;
    }
    s->sequence_active = 0;
    s->sequence_trigger = SC64_P4_SEQ_NONE;
    s->action_index = 0;
    s->outcome = trigger == SC64_P4_SEQ_ESCAPE ? SC64_P4_WON : SC64_P4_LOST;
    return trigger == SC64_P4_SEQ_ESCAPE ? SC64_P4_EVENT_WON : SC64_P4_EVENT_LOST;
}

static inline u16 sc64_p4_current_wait_ms(const Sc64P4State *s)
{
    if (!sc64_p4_state_valid(s) || !s->sequence_active) return 0;
    if ((s->sequence_trigger == SC64_P4_SEQ_LOSS_RAYNOR ||
         s->sequence_trigger == SC64_P4_SEQ_LOSS_TASSADAR) &&
        (s->action_index == 0 || s->action_index == 4))
        return SC64_P4_RESULT_WAIT_MS;
    if (s->sequence_trigger == SC64_P4_SEQ_ESCAPE &&
        (s->action_index == 2 || s->action_index == 6))
        return SC64_P4_RESULT_WAIT_MS;
    return 0;
}

#endif /* SC64_P4_H */
