#ifndef SC64_P3_MODEL_H
#define SC64_P3_MODEL_H

#include "../common/gba.h"
#include "sc64_p3.h"

/* Bounded, asset-free model of Higher Ground's audited start, force-start,
 * defeat, and Foes-zero-Buildings trigger actions. This is not a CHK runtime
 * and does not make the mission playable. Counts are already aggregated by
 * a future adapter according to the source's player-group selectors. */
enum {
    SC64_P3_RUNNING = 0,
    SC64_P3_WON = 1,
    SC64_P3_LOST = 2
};

enum {
    SC64_P3_SEQ_NONE = 0,
    SC64_P3_SEQ_WIN_WAIT_2000,
    SC64_P3_SEQ_WIN_TX_1,
    SC64_P3_SEQ_WIN_TX_2,
    SC64_P3_SEQ_WIN_WAIT_3000,
    SC64_P3_SEQ_DONE_WIN,
    SC64_P3_SEQ_DONE_LOSS
};

enum {
    SC64_P3_EVENT_NONE = 0,
    SC64_P3_EVENT_MISSION_START = 1u << 0,
    SC64_P3_EVENT_FORCE2_START = 1u << 1,
    SC64_P3_EVENT_FORCE3_START = 1u << 2,
    SC64_P3_EVENT_WIN_WAIT = 1u << 3,
    SC64_P3_EVENT_WIN_TRANSMISSION = 1u << 4,
    SC64_P3_EVENT_WIN_POST_WAIT = 1u << 5,
    SC64_P3_EVENT_WON = 1u << 6,
    SC64_P3_EVENT_LOST = 1u << 7
};

enum {
    SC64_P3_F_MISSION_START = 1u << 0,
    SC64_P3_F_FORCE2_START = 1u << 1,
    SC64_P3_F_FORCE3_START = 1u << 2,
    SC64_P3_F_WIN_SEQUENCE = 1u << 3,
    SC64_P3_F_RESULT = 1u << 4
};

typedef struct {
    u16 fired;
    u8 outcome;
    u8 sequence;
    u8 transmission; /* 1 or 2 while that source transmission is current. */
    u8 reserved;
} Sc64P3State;

typedef struct {
    u16 human_any_units; /* Current Player / raw AnyUnit 229. */
    u16 foes_buildings;  /* Foes / raw Buildings 231, aggregated across Foes. */
    u16 force2_any_units;/* Force 2 / raw AnyUnit 229. */
    u16 force3_any_units;/* Force 3 / raw AnyUnit 229. */
    u8 paused;
    u8 complete_action;  /* Adapter says the currently presented action ended. */
} Sc64P3Input;

/* init reports the TRIG 14 one-shot. It represents source awards of exactly
 * 300 minerals and 100 gas; the model only emits the event, not a wallet. */
static inline u16 sc64_p3_model_init(Sc64P3State *s)
{
    u8 *p;
    u32 i;
    if (!s) return SC64_P3_EVENT_NONE;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    s->fired = SC64_P3_F_MISSION_START;
    return SC64_P3_EVENT_MISSION_START;
}

static inline int sc64_p3_model_state_valid(const Sc64P3State *s)
{
    const u16 known = SC64_P3_F_MISSION_START | SC64_P3_F_FORCE2_START |
                      SC64_P3_F_FORCE3_START | SC64_P3_F_WIN_SEQUENCE |
                      SC64_P3_F_RESULT;
    if (!s || (s->fired & (u16)~known) ||
        !(s->fired & SC64_P3_F_MISSION_START) ||
        s->outcome > SC64_P3_LOST || s->sequence > SC64_P3_SEQ_DONE_LOSS ||
        s->transmission > 2 || s->reserved != 0)
        return 0;
    if (s->outcome == SC64_P3_RUNNING) {
        if (s->fired & SC64_P3_F_RESULT) return 0;
        if ((s->sequence != SC64_P3_SEQ_NONE) !=
            !!(s->fired & SC64_P3_F_WIN_SEQUENCE)) return 0;
        if (s->sequence == SC64_P3_SEQ_DONE_WIN ||
            s->sequence == SC64_P3_SEQ_DONE_LOSS) return 0;
    } else if (!(s->fired & SC64_P3_F_RESULT)) {
        return 0;
    }

    if (s->sequence == SC64_P3_SEQ_NONE) {
        if (s->transmission != 0) return 0;
    } else if (s->sequence == SC64_P3_SEQ_WIN_TX_1) {
        if (s->transmission != 1) return 0;
    } else if (s->sequence == SC64_P3_SEQ_WIN_TX_2) {
        if (s->transmission != 2) return 0;
    } else if (s->transmission != 0) {
        return 0;
    }

    if (s->outcome == SC64_P3_WON) {
        if (s->sequence != SC64_P3_SEQ_DONE_WIN ||
            !(s->fired & SC64_P3_F_WIN_SEQUENCE)) return 0;
    } else if (s->outcome == SC64_P3_LOST) {
        if (s->sequence != SC64_P3_SEQ_DONE_LOSS) return 0;
    }
    if (s->sequence >= SC64_P3_SEQ_WIN_WAIT_2000 &&
        s->sequence <= SC64_P3_SEQ_DONE_WIN &&
        !(s->fired & SC64_P3_F_WIN_SEQUENCE)) return 0;
    return 1;
}

/* One update samples independent force triggers and the result trigger.
 * Deterministic adapter policy: fire eligible force awards before processing
 * results; if player defeat and Foes-zero-Buildings coincide, defeat wins.
 * The source stores force starts in separate trigger records and does not
 * establish which event wins a same-frame conflict. */
static inline u16 sc64_p3_model_step(Sc64P3State *s, const Sc64P3Input *in)
{
    u16 events = 0;
    if (!sc64_p3_model_state_valid(s) || !in || in->paused ||
        s->outcome != SC64_P3_RUNNING)
        return SC64_P3_EVENT_NONE;

    /* These two source triggers are independent and one-shot. They remain
     * eligible even while the serialized victory presentation is pending. */
    if (!(s->fired & SC64_P3_F_FORCE2_START) && in->force2_any_units >= 1) {
        s->fired |= SC64_P3_F_FORCE2_START;
        events |= SC64_P3_EVENT_FORCE2_START;
    }
    if (!(s->fired & SC64_P3_F_FORCE3_START) && in->force3_any_units >= 1) {
        s->fired |= SC64_P3_F_FORCE3_START;
        events |= SC64_P3_EVENT_FORCE3_START;
    }

    /* Source Current Player / raw AnyUnit <= 0. AnyUnit includes buildings.
     * The adapter policy lets defeat preempt an already-pending win sequence. */
    if (in->human_any_units == 0) {
        s->sequence = SC64_P3_SEQ_DONE_LOSS;
        s->transmission = 0;
        s->outcome = SC64_P3_LOST;
        s->fired |= SC64_P3_F_RESULT;
        return (u16)(events | SC64_P3_EVENT_LOST);
    }

    /* This model serializes the source's ordered win actions. A future adapter
     * supplies complete_action according to its chosen UI/timing policy;
     * source Wait fields are raw milliseconds and transmission durations are
     * unresolved, so this model converts none of them to simulation ticks. */
    if (s->sequence != SC64_P3_SEQ_NONE) {
        if (!in->complete_action) return events;
        switch (s->sequence) {
        case SC64_P3_SEQ_WIN_WAIT_2000:
            s->sequence = SC64_P3_SEQ_WIN_TX_1;
            s->transmission = 1;
            return (u16)(events | SC64_P3_EVENT_WIN_TRANSMISSION);
        case SC64_P3_SEQ_WIN_TX_1:
            s->sequence = SC64_P3_SEQ_WIN_TX_2;
            s->transmission = 2;
            return (u16)(events | SC64_P3_EVENT_WIN_TRANSMISSION);
        case SC64_P3_SEQ_WIN_TX_2:
            s->sequence = SC64_P3_SEQ_WIN_WAIT_3000;
            s->transmission = 0;
            return (u16)(events | SC64_P3_EVENT_WIN_POST_WAIT);
        case SC64_P3_SEQ_WIN_WAIT_3000:
            s->sequence = SC64_P3_SEQ_DONE_WIN;
            s->outcome = SC64_P3_WON;
            s->fired |= SC64_P3_F_RESULT;
            return (u16)(events | SC64_P3_EVENT_WON);
        default:
            return events;
        }
    }

    /* Source Foes / raw Buildings <= 0, then ordered TRIG 14 actions:
     * Wait 2000, transmissions at source locations 64 then 1, Wait 3000,
     * Victory. The source does not tell us the N64 UI duration of each TX. */
    if (in->foes_buildings == 0) {
        s->sequence = SC64_P3_SEQ_WIN_WAIT_2000;
        s->fired |= SC64_P3_F_WIN_SEQUENCE;
        events |= SC64_P3_EVENT_WIN_WAIT;
    }
    return events;
}

#endif
