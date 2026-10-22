#ifndef SC64_SKIRMISH_AI_H
#define SC64_SKIRMISH_AI_H

/* Small deterministic skirmish planner.  It owns only planning/economy state;
 * the game applies intents to its own units and acknowledges accepted work. */

enum {
    SC64_AI_MAX_PLAYERS = 8,
    SC64_AI_MAX_INTENTS = 4,
    SC64_AI_MAX_QUEUE = 4,
    SC64_AI_REL_DEFAULT = 0,
    SC64_AI_REL_ALLY = 1,
    SC64_AI_REL_HOSTILE = 2,
    SC64_AI_REL_NEUTRAL = 3,
    SC64_AI_TRAIN_WORKER = 1,
    SC64_AI_TRAIN_SOLDIER = 2,
    SC64_AI_TRAIN_HEAVY = 3,
    SC64_AI_BUILD_BARRACKS = 1,
    SC64_AI_INTENT_MINE = 1,
    SC64_AI_INTENT_TRAIN = 2,
    SC64_AI_INTENT_BUILD = 3,
    SC64_AI_INTENT_ATTACK = 4
};

typedef struct {
    unsigned char player;
    unsigned char team;
    unsigned char alive;
    unsigned char neutral;
    unsigned short army;
    unsigned short distance; /* Manhattan distance in caller-defined cells. */
} Sc64AiTarget;

typedef struct {
    unsigned char count;
    Sc64AiTarget targets[SC64_AI_MAX_PLAYERS];
} Sc64AiWorld;

typedef struct {
    unsigned char kind;
    unsigned char type;
    unsigned char target_player;
    unsigned short workers_gas;
    unsigned short amount;
    unsigned short minerals;
    unsigned short gas;
} Sc64AiIntent;

typedef struct {
    unsigned char player;
    unsigned char team;
    unsigned char active;
    unsigned char has_townhall;
    unsigned char barracks;
    unsigned char relation[SC64_AI_MAX_PLAYERS];
    unsigned short minerals;
    unsigned short gas;
    unsigned short supply_used;
    unsigned short supply_cap;
    unsigned short workers;
    unsigned short workers_mining;
    unsigned short workers_gas;
    unsigned short army;
    unsigned short queued_workers;
    unsigned short queued_army;
    unsigned short queued_buildings;
    unsigned short desired_workers;
    unsigned short desired_gas_workers;
    unsigned short attack_threshold;
    unsigned short income_remainder;
} Sc64AiState;

static inline void sc64_ai_init(Sc64AiState *ai, unsigned char player,
                         unsigned char team, unsigned short minerals,
                         unsigned short gas, unsigned short workers,
                         unsigned short supply_cap)
{
    unsigned int i;
    ai->player = player;
    ai->team = team;
    ai->active = 1;
    ai->has_townhall = 1;
    ai->barracks = 0;
    for (i = 0; i < SC64_AI_MAX_PLAYERS; ++i) ai->relation[i] = SC64_AI_REL_DEFAULT;
    ai->minerals = minerals;
    ai->gas = gas;
    ai->supply_used = workers;
    ai->supply_cap = supply_cap;
    ai->workers = workers;
    ai->workers_mining = workers;
    ai->workers_gas = 0;
    ai->army = 0;
    ai->queued_workers = 0;
    ai->queued_army = 0;
    ai->queued_buildings = 0;
    ai->desired_workers = 8;
    ai->desired_gas_workers = 0;
    ai->attack_threshold = 3;
    ai->income_remainder = 0;
}

static inline void sc64_ai_set_relation(Sc64AiState *ai, unsigned char other,
                                 unsigned char relation)
{
    if (other < SC64_AI_MAX_PLAYERS && other != ai->player &&
        relation <= SC64_AI_REL_NEUTRAL)
        ai->relation[other] = relation;
}

static inline int sc64_ai_hostile(const Sc64AiState *ai, const Sc64AiTarget *target)
{
    unsigned char relation;
    if (!ai->active || !target->alive || target->neutral ||
        target->player >= SC64_AI_MAX_PLAYERS || target->player == ai->player)
        return 0;
    relation = ai->relation[target->player];
    if (relation == SC64_AI_REL_HOSTILE) return 1;
    if (relation == SC64_AI_REL_ALLY || relation == SC64_AI_REL_NEUTRAL) return 0;
    return target->team != ai->team;
}

/* The example Terran costs below follow BWAPI's public stat table at
 * d727fed68558c506163048ea889131d8cbb33915, UnitType.cpp: SCV/Marine 50
 * minerals, Firebat 50 minerals/25 gas, Barracks 150 minerals. They are
 * lightweight intent defaults, not a reconstruction of the original AI. */

/* One economy tick is an abstract simulation pulse.  Each mineral worker
 * yields one mineral and each gas worker one gas, with saturated 16-bit banks. */
static inline void sc64_ai_economy_tick(Sc64AiState *ai)
{
    unsigned int m, g;
    if (!ai->active) return;
    m = ai->minerals + ai->workers_mining;
    g = ai->gas + ai->workers_gas;
    ai->minerals = m > 65535u ? 65535u : (unsigned short)m;
    ai->gas = g > 65535u ? 65535u : (unsigned short)g;
}

static inline unsigned char sc64_ai_plan(const Sc64AiState *ai, const Sc64AiWorld *world,
                                  Sc64AiIntent *out)
{
    unsigned char n = 0, i;
    unsigned short pending_workers, pending_army;
    unsigned short minerals, gas, supply;
    const Sc64AiTarget *best = 0;
    if (!ai->active) return 0;
    minerals = ai->minerals;
    gas = ai->gas;
    supply = ai->supply_used + ai->queued_workers + ai->queued_army;
    pending_workers = ai->workers + ai->queued_workers;
    pending_army = ai->army + ai->queued_army;

    if (ai->workers_mining + ai->workers_gas != ai->workers ||
        ai->workers_gas != ai->desired_gas_workers) {
        out[n].kind = SC64_AI_INTENT_MINE;
        out[n].amount = ai->workers - (ai->desired_gas_workers < ai->workers ? ai->desired_gas_workers : ai->workers);
        out[n].workers_gas = ai->workers - out[n].amount;
        out[n].type = 0;
        out[n].minerals = out[n].gas = 0;
        ++n;
    }
    if (ai->has_townhall && pending_workers < ai->desired_workers &&
        ai->supply_used + ai->queued_workers < ai->supply_cap &&
        minerals >= 50 && supply < ai->supply_cap &&
        ai->queued_workers < SC64_AI_MAX_QUEUE && n < SC64_AI_MAX_INTENTS) {
        out[n].kind = SC64_AI_INTENT_TRAIN;
        out[n].type = SC64_AI_TRAIN_WORKER;
        out[n].amount = 1;
        out[n].minerals = 50;
        out[n].gas = 0;
        minerals -= 50;
        ++supply;
        ++n;
    }
    if (ai->has_townhall && !ai->barracks && !ai->queued_buildings &&
        minerals >= 150 && n < SC64_AI_MAX_INTENTS) {
        out[n].kind = SC64_AI_INTENT_BUILD;
        out[n].type = SC64_AI_BUILD_BARRACKS;
        out[n].amount = 1;
        out[n].minerals = 150;
        out[n].gas = 0;
        minerals -= 150;
        ++n;
    }
    if (ai->barracks && pending_army < ai->attack_threshold + 2 &&
        supply < ai->supply_cap &&
        ai->queued_army < SC64_AI_MAX_QUEUE && n < SC64_AI_MAX_INTENTS) {
        unsigned char type = (gas >= 25 && minerals >= 50) ?
                             SC64_AI_TRAIN_HEAVY : SC64_AI_TRAIN_SOLDIER;
        unsigned short mc = 50;
        unsigned short gc = type == SC64_AI_TRAIN_HEAVY ? 25 : 0;
        if (minerals >= mc && gas >= gc) {
            out[n].kind = SC64_AI_INTENT_TRAIN;
            out[n].type = type;
            out[n].amount = 1;
            out[n].minerals = mc;
            out[n].gas = gc;
            minerals -= mc;
            gas -= gc;
            ++supply;
            ++n;
        }
    }
    if (world && pending_army >= ai->attack_threshold && n < SC64_AI_MAX_INTENTS) {
        for (i = 0; i < world->count && i < SC64_AI_MAX_PLAYERS; ++i) {
            const Sc64AiTarget *t = &world->targets[i];
            if (sc64_ai_hostile(ai, t) && (!best || t->distance < best->distance ||
                (t->distance == best->distance && t->player < best->player))) best = t;
        }
        if (best) {
            out[n].kind = SC64_AI_INTENT_ATTACK;
            out[n].type = 0;
            out[n].target_player = best->player;
            out[n].amount = pending_army;
            out[n].minerals = out[n].gas = 0;
            ++n;
        }
    }
    return n;
}

/* The host calls commit only when an intent was accepted.  Completion is a
 * separate event so training time remains in the game engine. */
static inline int sc64_ai_commit(Sc64AiState *ai, const Sc64AiIntent *intent)
{
    if (intent->kind == SC64_AI_INTENT_MINE) {
        if (intent->amount + intent->workers_gas != ai->workers) return 0;
        ai->workers_mining = intent->amount;
        ai->workers_gas = intent->workers_gas;
        return 1;
    }
    if (intent->minerals > ai->minerals || intent->gas > ai->gas) return 0;
    if (intent->kind == SC64_AI_INTENT_TRAIN) {
        if (intent->type == SC64_AI_TRAIN_WORKER) {
            if (ai->queued_workers >= SC64_AI_MAX_QUEUE) return 0;
            ++ai->queued_workers;
        } else if (intent->type == SC64_AI_TRAIN_SOLDIER || intent->type == SC64_AI_TRAIN_HEAVY) {
            if (ai->queued_army >= SC64_AI_MAX_QUEUE) return 0;
            ++ai->queued_army;
        } else return 0;
    } else if (intent->kind == SC64_AI_INTENT_BUILD) {
        if (intent->type != SC64_AI_BUILD_BARRACKS || ai->barracks || ai->queued_buildings) return 0;
        ++ai->queued_buildings;
    } else if (intent->kind == SC64_AI_INTENT_ATTACK) {
        return 1;
    } else return 0;
    ai->minerals -= intent->minerals;
    ai->gas -= intent->gas;
    return 1;
}

static inline void sc64_ai_building_completed(Sc64AiState *ai, unsigned char type)
{
    if (type == SC64_AI_BUILD_BARRACKS) {
        ai->barracks = 1;
        if (ai->queued_buildings) --ai->queued_buildings;
    }
}

static inline void sc64_ai_unit_completed(Sc64AiState *ai, unsigned char type)
{
    if (type == SC64_AI_TRAIN_WORKER && ai->queued_workers) {
        --ai->queued_workers;
        ++ai->workers;
        ++ai->supply_used;
        ++ai->workers_mining;
    } else if ((type == SC64_AI_TRAIN_SOLDIER || type == SC64_AI_TRAIN_HEAVY) && ai->queued_army) {
        --ai->queued_army;
        ++ai->army;
        ++ai->supply_used;
    }
}

#endif
