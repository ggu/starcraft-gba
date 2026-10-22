#ifndef SC64_Z3_H
#define SC64_Z3_H

#include "../common/gba.h"

/* Bounded rule model for the source Zerg 3 / New Dominion trigger records.
 * This is not a trigger VM or gameplay adapter. Counts are supplied by an
 * adapter that resolves CHK selectors (Force 2, Foes, Current Player). */
enum {
    SC64_Z3_FORCE_2 = 19,
    SC64_Z3_SELECTOR_FOES = 14,
    SC64_Z3_SELECTOR_CURRENT_PLAYER = 13,
    SC64_Z3_SELECTOR_ANY_UNIT = 229,
    SC64_Z3_SELECTOR_BUILDINGS = 231,
    SC64_Z3_UNIT_MATURE_CHRYSALIS = 150,
    SC64_Z3_MISSION_ORE = 150,
    SC64_Z3_MISSION_GAS = 50,
    SC64_Z3_FORCE2_RESOURCES = 8000,
    SC64_Z3_FORCE2_AI_SCRIPT_LOCATION = 3,
    SC64_Z3_FORCE2_AI_SCRIPT_RAW = 863135066,
    SC64_Z3_MISSION_OBJECTIVES_STRING_ID = 31,
    SC64_Z3_WIN_WAIT_FIRST = 1000,
    SC64_Z3_WIN_WAIT_LAST = 500,
    SC64_Z3_LOSS_WAIT_FIRST = 1000,
    SC64_Z3_LOSS_WAV_ID = 29,
    SC64_Z3_LOSS_WAV_DURATION = 403,
    SC64_Z3_LOSS_MESSAGE_ID = 23,
    SC64_Z3_LOSS_MESSAGE_FLAGS = 4,
    SC64_Z3_LOSS_WAIT_LAST = 500,
    SC64_Z3_FORCE2_RESOURCE_SELECTOR_ORE_AND_GAS = 2,
    SC64_Z3_WIN_TX1_STRING_ID = 13,
    SC64_Z3_WIN_TX2_STRING_ID = 18,
    SC64_Z3_WIN_TX3_STRING_ID = 19,
    SC64_Z3_WIN_TX1_LOCATION = 3,
    SC64_Z3_WIN_TX2_LOCATION = 1,
    SC64_Z3_WIN_TX3_LOCATION = 2,
    SC64_Z3_WIN_TX_WAV_ID = 25,
    SC64_Z3_WIN_TX_TIME_FIELD = 0,
    SC64_Z3_WIN_TX_GROUP1 = 0,
    SC64_Z3_WIN_TX_GROUP2 = 100,
    SC64_Z3_WIN_TX1_UNIT_FIELD = 23,
    SC64_Z3_WIN_TX2_UNIT_FIELD = 152,
    SC64_Z3_WIN_TX3_UNIT_FIELD = 151,
    SC64_Z3_WIN_TX_MODIFIER_FIELD = 9,
    SC64_Z3_WIN_TX_FLAGS_FIELD = 16,
    SC64_Z3_WIN_FINAL_WAIT_FLAGS = 4,
    SC64_Z3_WIN_FIRST_WAIT_FLAGS = 0,
    SC64_Z3_LOSS_WAIT_FLAGS = 0,
    SC64_Z3_WIN_TX_OPCODE = 7,
    SC64_Z3_WIN_WAIT_OPCODE = 4,
    SC64_Z3_WIN_RESULT_OPCODE = 1,
    SC64_Z3_LOSS_WAIT_OPCODE = 4,
    SC64_Z3_LOSS_WAV_OPCODE = 8,
    SC64_Z3_LOSS_MESSAGE_OPCODE = 9,
    SC64_Z3_LOSS_RESULT_OPCODE = 2
};

enum {
    SC64_Z3_RUNNING = 0,
    SC64_Z3_WON = 1,
    SC64_Z3_LOST = 2
};

/* A sequence phase identifies the action currently waiting for adapter
 * completion. Raw Wait values are preserved; the ROM's conversion from those
 * values to this engine's 30 Hz ticks is deliberately not guessed here. */
enum {
    SC64_Z3_SEQ_NONE = 0,
    SC64_Z3_SEQ_WIN_WAIT_1000,
    SC64_Z3_SEQ_WIN_TX_1,
    SC64_Z3_SEQ_WIN_TX_2,
    SC64_Z3_SEQ_WIN_TX_3,
    SC64_Z3_SEQ_WIN_WAIT_500,
    SC64_Z3_SEQ_LOSS_WAIT_1000,
    SC64_Z3_SEQ_LOSS_WAV,
    SC64_Z3_SEQ_LOSS_MESSAGE,
    SC64_Z3_SEQ_LOSS_WAIT_500,
    SC64_Z3_SEQ_DONE_WIN,
    SC64_Z3_SEQ_DONE_LOSS
};

enum {
    SC64_Z3_EVENT_MISSION_START = 1u << 0,
    SC64_Z3_EVENT_FORCE2_START = 1u << 1,
    SC64_Z3_EVENT_WIN_WAIT = 1u << 2,
    SC64_Z3_EVENT_WIN_TX = 1u << 3,
    SC64_Z3_EVENT_LOSS_WAIT = 1u << 4,
    SC64_Z3_EVENT_LOSS_WAV = 1u << 5,
    SC64_Z3_EVENT_LOSS_MESSAGE = 1u << 6,
    SC64_Z3_EVENT_WON = 1u << 7,
    SC64_Z3_EVENT_LOST = 1u << 8
};

enum {
    SC64_Z3_F_MISSION_START = 1u << 0,
    SC64_Z3_F_FORCE2_START = 1u << 1,
    SC64_Z3_F_SWITCH_7 = 1u << 2,
    SC64_Z3_F_RESULT = 1u << 3,
    SC64_Z3_F_SEQUENCE_ACTION = 1u << 4
};

typedef struct {
    u16 fired;
    u8 switch7;
    u8 outcome;
    u8 sequence;
    u8 transmission; /* 1..3 for current win transmission, else 0. */
} Sc64Z3State;

typedef struct {
    u16 foes_any_units;      /* CHK Foes selector 14, raw selector 229. */
    u16 foes_buildings;      /* CHK Foes selector 14, raw selector 231. */
    u16 force2_any_units;    /* CHK Force 2 selector 19, raw selector 229. */
    u16 human_chrysalis;     /* CHK Current Player selector 13, raw unit 150. */
    u8 paused;
} Sc64Z3Input;

/* Events describe CHK actions. The startup event corresponds to TRIG 3's
 * objective/resource actions; the parent engine supplies the objective text.
 * Mission initial amounts are the source values 150 ore / 50 gas. */
static inline u16 sc64_z3_init(Sc64Z3State *s)
{
    u8 *p;
    u32 i;
    if (!s) return 0;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
    s->fired = SC64_Z3_F_MISSION_START;
    return SC64_Z3_EVENT_MISSION_START;
}

static inline int sc64_z3_state_valid(const Sc64Z3State *s)
{
    const u16 known = SC64_Z3_F_MISSION_START | SC64_Z3_F_FORCE2_START |
                      SC64_Z3_F_SWITCH_7 | SC64_Z3_F_RESULT |
                      SC64_Z3_F_SEQUENCE_ACTION;
    if (!s || (s->fired & (u16)~known) || s->outcome > SC64_Z3_LOST ||
        s->switch7 > 1 ||
        s->sequence > SC64_Z3_SEQ_DONE_LOSS || s->transmission > 3 ||
        !(s->fired & SC64_Z3_F_MISSION_START)) return 0;
    if (!!s->switch7 != !!(s->fired & SC64_Z3_F_SWITCH_7)) return 0;
    if (!!s->outcome != !!(s->fired & SC64_Z3_F_RESULT)) return 0;
    if (s->sequence != SC64_Z3_SEQ_NONE &&
        !(s->fired & SC64_Z3_F_SEQUENCE_ACTION)) return 0;
    if (s->outcome == SC64_Z3_RUNNING) {
        if (s->sequence >= SC64_Z3_SEQ_DONE_WIN ||
            ((s->sequence != SC64_Z3_SEQ_NONE) !=
             !!(s->fired & SC64_Z3_F_SEQUENCE_ACTION))) return 0;
    } else if (!(s->fired & SC64_Z3_F_SEQUENCE_ACTION) ||
               (s->outcome == SC64_Z3_WON && s->sequence != SC64_Z3_SEQ_DONE_WIN) ||
               (s->outcome == SC64_Z3_LOST && s->sequence != SC64_Z3_SEQ_DONE_LOSS))
        return 0;
    if (s->sequence >= SC64_Z3_SEQ_WIN_TX_1 &&
        s->sequence <= SC64_Z3_SEQ_WIN_TX_3) {
        if (s->transmission != (u8)(s->sequence - SC64_Z3_SEQ_WIN_TX_1 + 1)) return 0;
    } else if (s->transmission != 0) return 0;
    if (s->outcome == SC64_Z3_WON && !s->switch7) return 0;
    if (s->sequence >= SC64_Z3_SEQ_WIN_WAIT_1000 &&
        s->sequence <= SC64_Z3_SEQ_WIN_WAIT_500 && !s->switch7) return 0;
    if (s->sequence >= SC64_Z3_SEQ_LOSS_WAIT_1000 &&
        s->sequence <= SC64_Z3_SEQ_LOSS_WAIT_500 && s->transmission) return 0;
    return 1;
}

/* Evaluate enabled source predicates once per unpaused simulation update.
 * The map's Force 1 triggers SetSwitch(7) for either (Foes Any Units <= 6) or
 * (Foes Buildings <= 0). Current Player's Mature Chrysalis <= 0 starts defeat.
 * If loss and victory become true on the same sample, this model chooses loss
 * first as an explicit adapter policy; separate trigger records don't prove
 * a same-frame ordering in the source game. */
static inline u16 sc64_z3_step(Sc64Z3State *s, const Sc64Z3Input *in)
{
    u16 events = 0;
    if (!sc64_z3_state_valid(s) || !in || in->paused || s->outcome) return 0;
    /* This compact model serializes the result actions. Once the first Wait
     * begins, only complete_action may advance it; the source trigger records
     * do not establish an ordering between simultaneously-running triggers. */
    if (s->sequence != SC64_Z3_SEQ_NONE) return 0;

    if (!(s->fired & SC64_Z3_F_FORCE2_START) && in->force2_any_units >= 1) {
        s->fired |= SC64_Z3_F_FORCE2_START;
        events |= SC64_Z3_EVENT_FORCE2_START;
    }
    if (!s->switch7 && (in->foes_any_units <= 6 || in->foes_buildings == 0)) {
        s->switch7 = 1;
        s->fired |= SC64_Z3_F_SWITCH_7;
    }

    if (!in->human_chrysalis) {
        s->sequence = SC64_Z3_SEQ_LOSS_WAIT_1000;
        s->fired |= SC64_Z3_F_SEQUENCE_ACTION;
        return (u16)(events | SC64_Z3_EVENT_LOSS_WAIT);
    }
    if (s->switch7) {
        s->sequence = SC64_Z3_SEQ_WIN_WAIT_1000;
        s->fired |= SC64_Z3_F_SEQUENCE_ACTION;
        events |= SC64_Z3_EVENT_WIN_WAIT;
    }
    return events;
}

/* Acknowledge completion of the current action. In a future adapter this is
 * called when its selected presentation/timing policy says the action ended.
 * Raw CHK Wait amounts and the transmission order are retained by phase. */
static inline u16 sc64_z3_complete_action(Sc64Z3State *s, u8 paused)
{
    if (!sc64_z3_state_valid(s) || paused || s->outcome ||
        s->sequence == SC64_Z3_SEQ_NONE)
        return 0;
    switch (s->sequence) {
    case SC64_Z3_SEQ_WIN_WAIT_1000:
        s->sequence = SC64_Z3_SEQ_WIN_TX_1; s->transmission = 1;
        return SC64_Z3_EVENT_WIN_TX;
    case SC64_Z3_SEQ_WIN_TX_1:
        s->sequence = SC64_Z3_SEQ_WIN_TX_2; s->transmission = 2;
        return SC64_Z3_EVENT_WIN_TX;
    case SC64_Z3_SEQ_WIN_TX_2:
        s->sequence = SC64_Z3_SEQ_WIN_TX_3; s->transmission = 3;
        return SC64_Z3_EVENT_WIN_TX;
    case SC64_Z3_SEQ_WIN_TX_3:
        s->sequence = SC64_Z3_SEQ_WIN_WAIT_500; s->transmission = 0;
        return SC64_Z3_EVENT_WIN_WAIT;
    case SC64_Z3_SEQ_WIN_WAIT_500:
        s->sequence = SC64_Z3_SEQ_DONE_WIN; s->outcome = SC64_Z3_WON;
        s->fired |= SC64_Z3_F_RESULT;
        return SC64_Z3_EVENT_WON;
    case SC64_Z3_SEQ_LOSS_WAIT_1000:
        s->sequence = SC64_Z3_SEQ_LOSS_WAV;
        return SC64_Z3_EVENT_LOSS_WAV;
    case SC64_Z3_SEQ_LOSS_WAV:
        s->sequence = SC64_Z3_SEQ_LOSS_MESSAGE;
        return SC64_Z3_EVENT_LOSS_MESSAGE;
    case SC64_Z3_SEQ_LOSS_MESSAGE:
        s->sequence = SC64_Z3_SEQ_LOSS_WAIT_500;
        return SC64_Z3_EVENT_LOSS_WAIT;
    case SC64_Z3_SEQ_LOSS_WAIT_500:
        s->sequence = SC64_Z3_SEQ_DONE_LOSS; s->outcome = SC64_Z3_LOST;
        s->fired |= SC64_Z3_F_RESULT;
        return SC64_Z3_EVENT_LOST;
    default:
        return 0;
    }
}

#endif /* SC64_Z3_H */
