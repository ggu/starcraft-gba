/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_LIFECYCLE_H
#define SC64_SCARAB_LIFECYCLE_H

#include <stdint.h>

/* Source ticks, not GBA frames. A caller must supply the clock conversion.
 * These constants do not activate the gameplay feature. */
enum {
    SC64_SCARAB_COST = 15,
    SC64_SCARAB_BUILD_TICKS = 105,
    SC64_SCARAB_QUEUE_LIMIT = 5,
    SC64_SCARAB_CAPACITY = 5,
    SC64_SCARAB_UPGRADED_CAPACITY = 10,
    SC64_SCARAB_LAUNCH_COOLDOWN = 60,
    SC64_SCARAB_LIFETIME = 90,
    SC64_SCARAB_NO_COLLISION_TICKS = 7,
    SC64_SCARAB_NO_SLOT = -1
};

/* Queue includes the active entry until completion. Children, including the
 * partially built child, are real caller-owned actor slots, not an ammo count.
 * This standalone model does not claim any particular Unit/save field layout. */
typedef struct {
    int active_slot;
    uint16_t progress;
    uint8_t queued;
    uint8_t cooldown;
} Sc64ScarabParent;

typedef struct {
    uint8_t inside;  /* completed, hidden children; excludes active build */
    uint8_t outside; /* launched children; at most one */
} Sc64ScarabCounts;

typedef struct {
    uint8_t life;
    uint8_t no_collision;
} Sc64ScarabFlight;

/* Callbacks own the child actor and must validate its parent, owner and phase.
 * Return zero without side effects on failure. A successful allocation returns
 * a nonnegative actual slot. The caller guarantees it is not simultaneously
 * counted inside/outside or reused for any other actor. */
typedef struct {
    int (*allocate)(void *context, int *slot);
    int (*complete)(void *context, int slot);
    int (*launch)(void *context, int slot);
    int (*cancel_active)(void *context, int slot);
} Sc64ScarabOps;

enum {
    SC64_SCARAB_WAIT = 0,
    SC64_SCARAB_ALLOCATED = 1,
    SC64_SCARAB_PROGRESS = 2,
    SC64_SCARAB_COMPLETED = 3,
    SC64_SCARAB_INVALID = -1
};

static inline void sc64_scarab_parent_init(Sc64ScarabParent *parent)
{
    parent->active_slot = SC64_SCARAB_NO_SLOT;
    parent->progress = 0;
    parent->queued = 0;
    parent->cooldown = 0;
}

/* Shape tolerates an above-current-capacity inventory so admission can fail
 * safely. It does not implement source ownership-transfer policy; the caller
 * must cancel paid work before changing the parent owner. Loaders must validate actor slot
 * ranges, generations/identities, owner/parent bindings, child phases, queue
 * payment and cross-owner references against the candidate world before any
 * live mutation. */
static inline int sc64_scarab_parent_valid(const Sc64ScarabParent *parent,
                                           Sc64ScarabCounts counts,
                                           int upgraded)
{
    if (!parent || (upgraded != 0 && upgraded != 1) || counts.outside > 1 ||
        counts.inside > SC64_SCARAB_UPGRADED_CAPACITY || parent->queued > SC64_SCARAB_QUEUE_LIMIT ||
        (unsigned)counts.inside + parent->queued > SC64_SCARAB_UPGRADED_CAPACITY ||
        parent->cooldown > SC64_SCARAB_LAUNCH_COOLDOWN ||
        parent->active_slot < SC64_SCARAB_NO_SLOT) return 0;
    if (parent->active_slot == SC64_SCARAB_NO_SLOT) return parent->progress == 0;
    return parent->queued != 0 && parent->progress <= SC64_SCARAB_BUILD_TICKS;
}

/* Undamaged source child: 20 HP maximum, 2 HP at creation, 44/256 HP per
 * progress call, clamped to 20. This reconstructs Q8 from saved progress;
 * it does not model damage/healing or establish an external-mutation codec. */
static inline int sc64_scarab_training_hp_q8(unsigned progress, unsigned *out)
{
    unsigned hp;
    if (!out || progress > SC64_SCARAB_BUILD_TICKS) return 0;
    hp = 512u + 44u * progress;
    *out = hp < 5120u ? hp : 5120u;
    return 1;
}

/* Payment and admission are one atomic operation; no actor is allocated here.
 * Launched children do not count against inside+queued capacity. */
static inline int sc64_scarab_enqueue(Sc64ScarabParent *parent,
                                      Sc64ScarabCounts counts, int upgraded,
                                      int *minerals)
{
    unsigned capacity = upgraded ? SC64_SCARAB_UPGRADED_CAPACITY : SC64_SCARAB_CAPACITY;
    if (!minerals || !sc64_scarab_parent_valid(parent, counts, upgraded) ||
        *minerals < SC64_SCARAB_COST || parent->queued >= SC64_SCARAB_QUEUE_LIMIT ||
        (unsigned)counts.inside + parent->queued >= capacity) return 0;
    *minerals -= SC64_SCARAB_COST;
    ++parent->queued;
    return 1;
}

/* One source tick of the production queue. Allocation is attempted only for
 * the already-paid front entry; a full actor pool leaves it queued to retry.
 * Completion callback failure retains the slot and progress for a later retry. */
static inline int sc64_scarab_production_tick(Sc64ScarabParent *parent,
                                               Sc64ScarabCounts counts,
                                               int upgraded,
                                               const Sc64ScarabOps *ops,
                                               void *context)
{
    int slot;
    if (!sc64_scarab_parent_valid(parent, counts, upgraded) || !ops ||
        !ops->allocate || !ops->complete) return SC64_SCARAB_INVALID;
    if (!parent->queued) return SC64_SCARAB_WAIT;
    if (parent->active_slot == SC64_SCARAB_NO_SLOT) {
        slot = SC64_SCARAB_NO_SLOT;
        if (!ops->allocate(context, &slot)) return SC64_SCARAB_WAIT;
        if (slot < 0) return SC64_SCARAB_INVALID; /* callback contract violation */
        parent->active_slot = slot;
        return SC64_SCARAB_ALLOCATED;
    }
    /* Reference decrements 105 to zero and returns; completion happens on
     * the next production call, so progress 105 remains an active child. */
    if (parent->progress < SC64_SCARAB_BUILD_TICKS) {
        ++parent->progress;
        return SC64_SCARAB_PROGRESS;
    }
    if (!ops->complete(context, parent->active_slot)) return SC64_SCARAB_WAIT;
    --parent->queued;
    parent->active_slot = SC64_SCARAB_NO_SLOT;
    parent->progress = 0;
    return SC64_SCARAB_COMPLETED;
}

/* Cancel one paid queue entry and refund exactly once. A nonfront entry has
 * no actor; canceling the active front additionally retires its real child.
 * Validate the refund bound before any callback. The callback must be atomic,
 * enforce actual slot/parent/owner/phase, and have no side effects on failure.
 * A repeat index can name the next legitimate entry after queue compaction. */
static inline int sc64_scarab_cancel(Sc64ScarabParent *parent,
                                     Sc64ScarabCounts counts, int upgraded,
                                     const Sc64ScarabOps *ops, void *context,
                                     unsigned queue_index, int *minerals,
                                     int bank_max)
{
    if (!minerals || !sc64_scarab_parent_valid(parent, counts, upgraded) ||
        queue_index >= parent->queued || bank_max < 0 || *minerals < 0 ||
        *minerals > bank_max || bank_max - *minerals < SC64_SCARAB_COST) return 0;
    if (queue_index == 0 && parent->active_slot != SC64_SCARAB_NO_SLOT) {
        if (!ops || !ops->cancel_active ||
            !ops->cancel_active(context, parent->active_slot)) return 0;
        parent->active_slot = SC64_SCARAB_NO_SLOT;
        parent->progress = 0;
    }
    --parent->queued;
    *minerals += SC64_SCARAB_COST;
    return 1;
}

/* A child-loss notification must come from the real actor lifecycle after
 * validating slot identity/generation. The paid front entry remains queued;
 * the next production tick can attempt a fresh allocation with no new charge. */
static inline int sc64_scarab_active_lost(Sc64ScarabParent *parent,
                                          Sc64ScarabCounts counts, int upgraded,
                                          int lost_slot)
{
    if (!sc64_scarab_parent_valid(parent, counts, upgraded) || lost_slot < 0 ||
        parent->active_slot != lost_slot) return 0;
    parent->active_slot = SC64_SCARAB_NO_SLOT;
    parent->progress = 0;
    return 1;
}

/* An actual, completed inside actor is selected by the caller. The callback
 * must check identity/owner and atomically move that child outside. */
static inline int sc64_scarab_launch(Sc64ScarabParent *parent,
                                     Sc64ScarabCounts counts, int upgraded,
                                     const Sc64ScarabOps *ops, void *context,
                                     int child_slot, Sc64ScarabFlight *flight)
{
    if (!flight || !sc64_scarab_parent_valid(parent, counts, upgraded) || !ops ||
        !ops->launch || child_slot < 0 || !counts.inside || counts.outside ||
        parent->cooldown || !ops->launch(context, child_slot)) return 0;
    parent->cooldown = SC64_SCARAB_LAUNCH_COOLDOWN;
    flight->life = SC64_SCARAB_LIFETIME;
    flight->no_collision = SC64_SCARAB_NO_COLLISION_TICKS;
    return 1;
}

/* An active saved flight must still have life; expired children must be
 * retired before saving. Inactive/non-flight slots use their own validation. */
static inline int sc64_scarab_flight_valid(const Sc64ScarabFlight *flight)
{
    return flight && flight->life > 0 && flight->life <= SC64_SCARAB_LIFETIME &&
           flight->no_collision <= SC64_SCARAB_NO_COLLISION_TICKS &&
           flight->no_collision <= flight->life;
}

/* Advance each independently once per source tick; the caller owns projectile
 * movement, splash, collision and removal and must retire the child at zero. */
static inline void sc64_scarab_cooldown_tick(Sc64ScarabParent *parent)
{
    if (parent->cooldown) --parent->cooldown;
}
static inline int sc64_scarab_flight_tick(Sc64ScarabFlight *flight)
{
    if (flight->no_collision) --flight->no_collision;
    if (flight->life) --flight->life;
    return flight->life == 0;
}
#endif
