#ifndef SC64_P5_SOURCE_MODEL_H
#define SC64_P5_SOURCE_MODEL_H

/* Source-fact checks and trigger semantics for the authenticated P5 CHK.
 * This is a compact authored model, not a CHK interpreter.  Include the
 * staged sc64_p5_scenario_generated.h before this file. */
#include "../common/gba.h"

enum {
    SC64_P5_SOURCE_ID = 24,
    SC64_P5_SOURCE_PLAYERS = 27,
    SC64_P5_MODEL_TRIGGER_COUNT = 11,
    SC64_P5_MODEL_ACTION_COUNT = 37,
    SC64_P5_MODEL_CONDITION_COUNT = 12,
    SC64_P5_ACTION_CREATE_UNIT = 11,
    SC64_P5_ACTION_SET_INVINCIBILITY = 43,
    SC64_P5_ACTION_VICTORY = 1,
    SC64_P5_ACTION_DEFEAT = 2,
    SC64_P5_CONDITION_BRING = 3,
    SC64_P5_CONDITION_COMMAND = 2,
    SC64_P5_CONDITION_DEATHS = 15,
    SC64_P5_MODEL_RAW_HERO = 79,
    SC64_P5_MODEL_RAW_ZEALOT = 65,
    SC64_P5_MODEL_RAW_ANY = 229,
    SC64_P5_VICTORY_LOCATION = 2
};

typedef struct {
    u8 trigger;
    u8 action_index;
    u8 opcode;
    u8 count;
    u8 location;
    u8 owner_selector;
    u16 raw_unit;
} Sc64P5SourceEffect;

static inline u8 sc64_p5_source_create_count(const Sc64P5SourceAction *a)
{
    if (!a || a->opcode != SC64_P5_ACTION_CREATE_UNIT) return 0;
    /* CHK Create Unit modifier zero means one.  The count is otherwise the
     * raw modifier byte; source opcode 43 is never classified as creation. */
    return a->modifier ? a->modifier : 1u;
}

static inline int sc64_p5_source_effect(unsigned trigger, unsigned index,
                                         Sc64P5SourceEffect *out)
{
    const Sc64P5SourceAction *a;
    if (!out || trigger >= SC64_P5_MODEL_TRIGGER_COUNT ||
        index >= sc64_p5_source_triggers[trigger].action_count) return 0;
    a = &sc64_p5_source_actions[
        sc64_p5_source_triggers[trigger].first_action + index];
    if (a->trigger != trigger || a->index != index) return 0;
    out->trigger = (u8)trigger;
    out->action_index = (u8)index;
    out->opcode = a->opcode;
    out->count = sc64_p5_source_create_count(a);
    out->location = (u8)a->location;
    out->owner_selector = (u8)a->group1;
    out->raw_unit = a->unit;
    return 1;
}

/* Validate the staged source table's structure and the P5 facts that the
 * authored runtime relies on.  Opaque AI script actions are accepted as rows
 * but this check makes no claim to execute their native payloads. */
static inline int sc64_p5_source_model_valid(void)
{
    unsigned i, creates = 0, victory_count = 0, defeat_count = 0;
    if (SC64_P5_SOURCE_GLOBAL_ID != SC64_P5_SOURCE_ID ||
        SC64_P5_SOURCE_TRIGGER_COUNT != SC64_P5_MODEL_TRIGGER_COUNT ||
        SC64_P5_SOURCE_ACTION_COUNT != SC64_P5_MODEL_ACTION_COUNT ||
        SC64_P5_SOURCE_CONDITION_COUNT != SC64_P5_MODEL_CONDITION_COUNT) return 0;
    for (i = 0; i < SC64_P5_MODEL_TRIGGER_COUNT; ++i) {
        const Sc64P5SourceTrigger *t = &sc64_p5_source_triggers[i];
        unsigned j;
        if (t->first_action + t->action_count > SC64_P5_MODEL_ACTION_COUNT ||
            t->first_condition + t->condition_count > SC64_P5_MODEL_CONDITION_COUNT)
            return 0;
        for (j = 0; j < t->action_count; ++j) {
            const Sc64P5SourceAction *a =
                &sc64_p5_source_actions[t->first_action + j];
            if (a->trigger != i || a->index != j || !a->opcode) return 0;
            if (a->opcode == SC64_P5_ACTION_CREATE_UNIT) ++creates;
            if (a->opcode == SC64_P5_ACTION_VICTORY) ++victory_count;
            if (i < 2 && a->opcode == SC64_P5_ACTION_DEFEAT) ++defeat_count;
            /* The final victory action follows the two Bring predicates. */
            if (i == 4 && j == 2 &&
                (a->opcode != SC64_P5_ACTION_VICTORY || a->unit != 0)) return 0;
        }
        for (j = 0; j < t->condition_count; ++j) {
            const Sc64P5SourceCondition *c =
                &sc64_p5_source_conditions[t->first_condition + j];
            if (c->trigger != i || c->index != j) return 0;
        }
    }
    if (creates != 11 || victory_count != 1 || defeat_count != 2) return 0;
    /* The two terminal loss records are source Command predicates at
     * zero: Current Player has no raw any-unit count, or All Players has no
     * raw Tassadar count.  Their action programs each end with Defeat. */
    {
        const Sc64P5SourceCondition *any = &sc64_p5_source_conditions[0];
        const Sc64P5SourceCondition *hero = &sc64_p5_source_conditions[1];
        const Sc64P5SourceCondition *death = &sc64_p5_source_conditions[6];
        if (any->trigger != 0 || any->player != 13 ||
            any->unit != SC64_P5_MODEL_RAW_ANY ||
            any->opcode != SC64_P5_CONDITION_COMMAND || any->comparison != 1 ||
            any->amount != 0 || hero->trigger != 1 || hero->player != 17 ||
            hero->unit != SC64_P5_MODEL_RAW_HERO ||
            hero->opcode != SC64_P5_CONDITION_COMMAND || hero->comparison != 1 ||
            hero->amount != 0 || sc64_p5_source_actions[0].opcode != SC64_P5_ACTION_DEFEAT ||
            sc64_p5_source_actions[5].opcode != SC64_P5_ACTION_DEFEAT ||
            death->trigger != 5 || death->player != 6 ||
            death->unit != SC64_P5_MODEL_RAW_ANY ||
            death->opcode != SC64_P5_CONDITION_DEATHS || death->amount != 1)
            return 0;
    }
    /* Source victory requires Tassadar and two Zealots at location 2. */
    {
        const Sc64P5SourceCondition *hero = &sc64_p5_source_conditions[4];
        const Sc64P5SourceCondition *carriers = &sc64_p5_source_conditions[5];
        if (hero->trigger != 4 || hero->opcode != SC64_P5_CONDITION_BRING ||
            hero->location != SC64_P5_VICTORY_LOCATION || hero->player != 13 ||
            hero->unit != SC64_P5_MODEL_RAW_HERO || hero->amount != 1 ||
            carriers->trigger != 4 || carriers->opcode != SC64_P5_CONDITION_BRING ||
            carriers->location != SC64_P5_VICTORY_LOCATION || carriers->player != 13 ||
            carriers->unit != SC64_P5_MODEL_RAW_ZEALOT || carriers->amount != 2) return 0;
    }
    /* Opcode 43 modifies invincibility on an existing source actor. */
    {
        const Sc64P5SourceAction *a = &sc64_p5_source_actions[26];
        if (a->opcode != SC64_P5_ACTION_SET_INVINCIBILITY ||
            sc64_p5_source_create_count(a) != 0 || a->unit != 229) return 0;
    }
    return 1;
}

#endif
