/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_UNIT_ADAPTER_H
#define SC64_SCARAB_UNIT_ADAPTER_H
#ifndef SC64_SCARAB_RUNTIME
#error "Scarab unit adapter requires the explicit experimental gate"
#endif
#if !defined(SC64_RACES) || (!defined(SC64_UNIFIED) && !defined(SC64_BACKWATER))
#error "Scarab unit adapter requires race identities and the invincibility array"
#endif
#include "sc64_scarab_fields.h"
#include "sc64_scarab_lifecycle.h"
_Static_assert(MAX_UNITS <= SC64_SCARAB_FIELD_SLOT_LIMIT, "Scarab slot encoding must cover the actor pool");

/* This header is deliberately not included by the game yet. Call only after
 * spawn_unit and race_owner_minerals are defined. The eventual integration must
 * exclude training/inside children from ordinary simulation, targeting,
 * rendering and selection, preserve Reaver carry/work_timer on STOP, and
 * validate the actual candidate world before loading. Slot reuse must run the
 * parent/child loss hooks before reuse; slot plus owner alone is not a
 * generation identity. No current save version claims these fields. */
enum {
    SC64_SCARAB_CHILD_TRAINING = 0,
    SC64_SCARAB_CHILD_INSIDE = 1,
    SC64_SCARAB_CHILD_OUTSIDE = 2,
    SC64_SCARAB_CHILD_SELF_DESTRUCT = 3
};
typedef struct { int parent_slot; } Sc64ScarabUnitContext;

static int race_scarab_parent_identity(int slot)
{
    return slot >= 0 && slot < MAX_UNITS && u[slot].alive &&
           u[slot].owner < 5 && u[slot].type < SC_RU_COUNT &&
           sc64_race_raw_unit(u[slot].type) == 83;
}

/* Returns 1 for a checked child, 0 for anything else. An attached parent must
 * still be alive and match engine and source owner. A detached outside child
 * returns parent -1. This is not a generation or candidate-save check. */
static int race_scarab_child_lookup(int child_slot, int *parent_slot, unsigned *phase)
{
    Sc64ScarabChildFields f;
    if (child_slot < 0 || child_slot >= MAX_UNITS || !u[child_slot].alive ||
        u[child_slot].type != U_SCARAB || u[child_slot].raw_type != 85 ||
        !sc64_scarab_child_fields_unpack(u[child_slot].carry,u[child_slot].work_timer,&f) ||
        u[child_slot].owner >= 5 || u[child_slot].build != -1) return 0;
    if (f.parent_slot != SC64_SCARAB_FIELD_NO_SLOT &&
        (f.parent_slot >= MAX_UNITS || !race_scarab_parent_identity((int)f.parent_slot) ||
         u[child_slot].owner != u[f.parent_slot].owner ||
         u[child_slot].raw_owner != u[f.parent_slot].raw_owner)) return 0;
    if ((f.phase == SC64_SCARAB_CHILD_TRAINING || f.phase == SC64_SCARAB_CHILD_INSIDE) &&
        (f.life || f.no_collision)) return 0;
    if (f.phase == SC64_SCARAB_CHILD_OUTSIDE &&
        !sc64_scarab_flight_valid(&(Sc64ScarabFlight){(uint8_t)f.life,(uint8_t)f.no_collision})) return 0;
    if (parent_slot) *parent_slot=f.parent_slot==SC64_SCARAB_FIELD_NO_SLOT ? -1 : (int)f.parent_slot;
    if (phase) *phase=f.phase;
    return 1;
}

static int race_scarab_child_set(int slot, unsigned parent, unsigned phase,
                                 unsigned life, unsigned no_collision)
{
    Sc64ScarabChildFields f={parent,phase,life,no_collision}; int carry,timer;
    if (slot<0 || slot>=MAX_UNITS || !u[slot].alive || u[slot].type!=U_SCARAB ||
        !sc64_scarab_child_fields_pack(&f,&carry,&timer)) return 0;
    int was_hidden=SC64_SCARAB_HIDDEN(&u[slot]);
    int was_noncolliding=!!SC64_SCARAB_NONCOLLIDING(&u[slot]);
    u[slot].carry=carry; u[slot].work_timer=(u16)timer;
    if (was_hidden!=!!SC64_SCARAB_HIDDEN(&u[slot]) ||
        was_noncolliding!=!!SC64_SCARAB_NONCOLLIDING(&u[slot])) {
        /* Previously hidden/grace rows may not be linked in the active
         * indexes. Fall back to the checked world scans until next setup. */
        target_lists_valid=0;
        sc64_target_index_end();
        sc64_unit_bucket_active=0;
#ifdef SC64_CONTACT_MEMO_ENABLED
        SC64_CONTACT_MEMO_INVALIDATE();
#endif
    }
    return 1;
}

/* A fresh Reaver only: no actor may already claim this slot. */
static int race_scarab_parent_init(int parent_slot)
{
    Sc64ScarabParentFields f={0,SC64_SCARAB_FIELD_NO_SLOT,0,0}; int carry,timer;
    if (!race_scarab_parent_identity(parent_slot)) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==U_SCARAB) {
        Sc64ScarabChildFields child;
        if (!sc64_scarab_child_fields_unpack(u[i].carry,u[i].work_timer,&child) ||
            child.parent_slot==(unsigned)parent_slot) return 0;
    }
    if (!sc64_scarab_parent_fields_pack(&f,&carry,&timer)) return 0;
    u[parent_slot].carry=carry; u[parent_slot].work_timer=(u16)timer;
    return 1;
}

static int race_scarab_parent_read(int slot, int upgraded,
                                   Sc64ScarabParent *parent, Sc64ScarabCounts *counts)
{
    Sc64ScarabParentFields f; int training=-1;
    if (!parent || !counts || !race_scarab_parent_identity(slot) ||
        !sc64_scarab_parent_fields_unpack(u[slot].carry,u[slot].work_timer,&f)) return 0;
    *parent=(Sc64ScarabParent){f.active_slot==SC64_SCARAB_FIELD_NO_SLOT ? -1 : (int)f.active_slot,
                              (uint16_t)f.progress,(uint8_t)f.queued,(uint8_t)f.cooldown};
    *counts=(Sc64ScarabCounts){0,0};
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].type==U_SCARAB) {
        Sc64ScarabChildFields child; int p; unsigned phase;
        if (!sc64_scarab_child_fields_unpack(u[i].carry,u[i].work_timer,&child)) return 0;
        if (child.parent_slot != (unsigned)slot) continue;
        if (!race_scarab_child_lookup(i,&p,&phase) || p!=slot) return 0;
        if (phase==SC64_SCARAB_CHILD_TRAINING) { if(training>=0)return 0; training=i; }
        else if (phase==SC64_SCARAB_CHILD_INSIDE) ++counts->inside;
        else ++counts->outside;
    }
    return training==parent->active_slot && sc64_scarab_parent_valid(parent,*counts,upgraded);
}
static int race_scarab_parent_write(int slot, const Sc64ScarabParent *p)
{
    Sc64ScarabParentFields f={p->queued,p->active_slot<0 ? SC64_SCARAB_FIELD_NO_SLOT : (unsigned)p->active_slot,
                             p->cooldown,p->progress}; int carry,timer;
    if(!sc64_scarab_parent_fields_pack(&f,&carry,&timer)) return 0;
    u[slot].carry=carry; u[slot].work_timer=(u16)timer; return 1;
}
static int *race_scarab_owner_bank(int owner)
{
    return owner==SIDE ? &minerals : owner>=0 && owner<5 ? &race_minerals[owner] : 0;
}
static int race_scarab_allocate(void *ctx,int *slot)
{
    Sc64ScarabUnitContext *c=ctx; int predicted=find_unit_slot(),carry,timer;
    Sc64ScarabChildFields f={(unsigned)c->parent_slot,SC64_SCARAB_CHILD_TRAINING,0,0};
    if(predicted<0 || predicted>=SC64_SCARAB_FIELD_SLOT_LIMIT ||
       !sc64_scarab_child_fields_pack(&f,&carry,&timer)) return 0;
    int i=spawn_unit(u[c->parent_slot].owner,U_SCARAB,u[c->parent_slot].x,u[c->parent_slot].y);
    /* The current single-threaded spawn_unit uses the immediately preceding
     * first-free search. Future slot/spawn hooks must preserve that invariant;
     * otherwise this callback cannot roll back arbitrary spawn side effects. */
    if(i!=predicted) return 0;
    u[i].raw_type=85; u[i].raw_owner=u[c->parent_slot].raw_owner;
    u[i].hp=2; /* Q8 source starts at 512; integer Unit stores the floor. */
    u[i].carry=carry; u[i].work_timer=(u16)timer; u[i].build=-1;
    invincible_units[i]=1; *slot=i; return 1;
}
static int race_scarab_complete(void *ctx,int slot)
{
    Sc64ScarabUnitContext *c=ctx; int p; unsigned phase;
    return race_scarab_child_lookup(slot,&p,&phase) && p==c->parent_slot &&
           phase==SC64_SCARAB_CHILD_TRAINING &&
           race_scarab_child_set(slot,(unsigned)p,SC64_SCARAB_CHILD_INSIDE,0,0);
}
static int race_scarab_launch_cb(void *ctx,int slot)
{
    Sc64ScarabUnitContext *c=ctx; int p; unsigned phase;
#ifdef SC64_SCARAB_FLIGHT_CONTROL_PROBE
    if (!race_scarab_child_lookup(slot,&p,&phase) || p!=c->parent_slot || phase!=SC64_SCARAB_CHILD_INSIDE ||
        !race_scarab_child_set(slot,(unsigned)p,SC64_SCARAB_CHILD_OUTSIDE,0,0)) return 0;
    u[slot].order=1; /* bridge code for source order state 0 */
    return 1;
#else
    return race_scarab_child_lookup(slot,&p,&phase) && p==c->parent_slot &&
           phase==SC64_SCARAB_CHILD_INSIDE &&
           race_scarab_child_set(slot,(unsigned)p,SC64_SCARAB_CHILD_OUTSIDE,
                                 SC64_SCARAB_LIFETIME,SC64_SCARAB_NO_COLLISION_TICKS);
#endif
}

#ifdef SC64_SCARAB_FLIGHT_CONTROL_PROBE
/* Source-order control slice only. Caller supplies an authoritative arrival
 * result and handles returned animation/removal requests. No movement,
 * animation, damage or automatic dispatch is implemented here. */
enum { SC64_SCARAB_CONTROL_INVALID=-1, SC64_SCARAB_CONTROL_CONTINUE=0,
       SC64_SCARAB_CONTROL_ARRIVED=1, SC64_SCARAB_CONTROL_REMOVE=2 };
static int race_scarab_flight_control_tick(int slot,int arrived,int signal)
{
    Sc64ScarabChildFields f;Unit *q;
    if (slot<0 || slot>=MAX_UNITS || (arrived!=0 && arrived!=1) || (signal!=0 && signal!=1)) return SC64_SCARAB_CONTROL_INVALID;
    q=&u[slot];
    if (!q->alive || q->type!=U_SCARAB || q->raw_type!=85 ||
        !sc64_scarab_child_fields_unpack(q->carry,q->work_timer,&f)) return SC64_SCARAB_CONTROL_INVALID;
    if (f.phase==SC64_SCARAB_CHILD_SELF_DESTRUCT) return SC64_SCARAB_CONTROL_REMOVE;
    if (f.phase!=SC64_SCARAB_CHILD_OUTSIDE) return SC64_SCARAB_CONTROL_INVALID;
    if (q->order==1) {
        if (f.life || f.no_collision) return SC64_SCARAB_CONTROL_INVALID;
        if (!race_scarab_child_set(slot,f.parent_slot,f.phase,90,7)) return SC64_SCARAB_CONTROL_INVALID;
        q->order=2;return SC64_SCARAB_CONTROL_CONTINUE;
    }
    if ((q->order!=2 && q->order!=3 && q->order!=4) || !f.life) return SC64_SCARAB_CONTROL_INVALID;
    if (f.life==1) return SC64_SCARAB_CONTROL_REMOVE;
    --f.life;if (f.no_collision) --f.no_collision;
    if (!race_scarab_child_set(slot,f.parent_slot,f.phase,f.life,f.no_collision)) return SC64_SCARAB_CONTROL_INVALID;
    if (q->order==2 && !f.no_collision) q->order=3;
    if ((q->order==2 || q->order==3) && arrived) { q->order=4;return SC64_SCARAB_CONTROL_ARRIVED; }
    if (q->order==4 && signal) return SC64_SCARAB_CONTROL_REMOVE;
    return SC64_SCARAB_CONTROL_CONTINUE;
}
#endif
static int race_scarab_cancel_cb(void *ctx,int slot)
{
    Sc64ScarabUnitContext *c=ctx; int p; unsigned phase;
    if (!race_scarab_child_lookup(slot,&p,&phase) || p!=c->parent_slot ||
        phase!=SC64_SCARAB_CHILD_TRAINING) return 0;
    /* Unintegrated hidden build child cannot have external references yet.
     * Real integration must clear all references through its death lifecycle. */
    u[slot].alive=0; return 1;
}
static const Sc64ScarabOps race_scarab_ops={race_scarab_allocate,race_scarab_complete,
                                            race_scarab_launch_cb,race_scarab_cancel_cb};
static int race_scarab_enqueue(int slot,int upgraded)
{
    Sc64ScarabParent p; Sc64ScarabCounts counts; int *bank;
    if (!race_scarab_parent_read(slot,upgraded,&p,&counts) ||
        !(bank=race_scarab_owner_bank(u[slot].owner)) || *bank<0 || *bank>65535) return 0;
    int next=*bank;
    if (!sc64_scarab_enqueue(&p,counts,upgraded,&next) || !race_scarab_parent_write(slot,&p)) return 0;
    *bank=next; return 1;
}
/* Invoke only on a source tick. No automatic game-loop registration. */
static int race_scarab_production_tick(int slot,int upgraded)
{
    Sc64ScarabParent p; Sc64ScarabCounts counts; Sc64ScarabUnitContext c={slot};
    if (!race_scarab_parent_read(slot,upgraded,&p,&counts)) return SC64_SCARAB_INVALID;
    int active=p.active_slot; unsigned hp_q8;
    /* This experiment only reconstructs undamaged construction. Refuse an
     * external HP mutation rather than silently overwriting it. */
    if (active>=0 && (!sc64_scarab_training_hp_q8(p.progress,&hp_q8) ||
                      u[active].hp!=(int)(hp_q8 >> 8))) return SC64_SCARAB_INVALID;
    int result=sc64_scarab_production_tick(&p,counts,upgraded,&race_scarab_ops,&c);
    if (result==SC64_SCARAB_INVALID || !race_scarab_parent_write(slot,&p)) return SC64_SCARAB_INVALID;
    if (result==SC64_SCARAB_PROGRESS) {
        if (!sc64_scarab_training_hp_q8(p.progress,&hp_q8)) return SC64_SCARAB_INVALID;
        u[active].hp=(int)(hp_q8 >> 8);
    }
    return result;
}
/* The caller invokes this exactly once per elapsed source tick. */
static int race_scarab_cooldown_tick(int slot,int upgraded)
{
    Sc64ScarabParent p; Sc64ScarabCounts counts;
    if (!race_scarab_parent_read(slot,upgraded,&p,&counts)) return 0;
    sc64_scarab_cooldown_tick(&p);
    return race_scarab_parent_write(slot,&p);
}
static int race_scarab_cancel(int slot,int upgraded,unsigned index)
{
    Sc64ScarabParent p; Sc64ScarabCounts counts; Sc64ScarabUnitContext c={slot}; int *bank;
    if (!race_scarab_parent_read(slot,upgraded,&p,&counts) ||
        !(bank=race_scarab_owner_bank(u[slot].owner))) return 0;
    int next=*bank;
    if (!sc64_scarab_cancel(&p,counts,upgraded,&race_scarab_ops,&c,index,&next,65535)) return 0;
    if (!race_scarab_parent_write(slot,&p)) return 0;
    *bank=next; return 1;
}
/* Transfer callers cancel every paid entry while the old owner still owns the
 * parent and children. Preflight the total refund before touching any slot. */
static int race_scarab_cancel_all(int slot,int upgraded)
{
    Sc64ScarabParent p; Sc64ScarabCounts counts; Sc64ScarabUnitContext c={slot}; int *bank;
    if (!race_scarab_parent_read(slot,upgraded,&p,&counts) ||
        !(bank=race_scarab_owner_bank(u[slot].owner))) return 0;
    int refund=(int)p.queued*SC64_SCARAB_COST;
    if (*bank<0 || *bank>65535 || refund>65535-*bank) return 0;
    if (p.active_slot>=0 && !race_scarab_cancel_cb(&c,p.active_slot)) return 0;
    p.queued=0; p.active_slot=-1; p.progress=0;
    if (!race_scarab_parent_write(slot,&p)) return 0;
    *bank+=refund; return 1;
}
static int race_scarab_launch_child(int slot,int upgraded,int child)
{
    Sc64ScarabParent p; Sc64ScarabCounts counts; Sc64ScarabUnitContext c={slot}; Sc64ScarabFlight f;
    if (!race_scarab_parent_read(slot,upgraded,&p,&counts) ||
        !sc64_scarab_launch(&p,counts,upgraded,&race_scarab_ops,&c,child,&f)) return 0;
    return race_scarab_parent_write(slot,&p);
}
/* Called only for the explicit STOP action, before generic STOP clears carry. */
static int race_scarab_stop_signal(int slot,int upgraded)
{
    Sc64ScarabParent p; Sc64ScarabCounts counts;
    if (!race_scarab_parent_read(slot,upgraded,&p,&counts)) return 0;
    for (int i=0;i<MAX_UNITS;++i) { int parent; unsigned phase;
        if (race_scarab_child_lookup(i,&parent,&phase) && parent==slot && phase==SC64_SCARAB_CHILD_OUTSIDE) {
            Sc64ScarabChildFields f;
            if (!sc64_scarab_child_fields_unpack(u[i].carry,u[i].work_timer,&f) ||
                !race_scarab_child_set(i,(unsigned)slot,SC64_SCARAB_CHILD_SELF_DESTRUCT,f.life,f.no_collision)) return 0;
        }
    }
    return 1;
}
/* Call before retiring/reusing the parent slot. Source behavior destroys
 * inside/training children without refund and detaches an outside child; its
 * owner, position, target and flight timers remain intact. */
static int race_scarab_parent_lost(int slot,int upgraded)
{
    Sc64ScarabParent p; Sc64ScarabCounts counts;
    if (!race_scarab_parent_read(slot,upgraded,&p,&counts)) return 0;
    for (int i=0;i<MAX_UNITS;++i) { int parent; unsigned phase;
        if (race_scarab_child_lookup(i,&parent,&phase) && parent==slot) {
            if (phase==SC64_SCARAB_CHILD_OUTSIDE || phase==SC64_SCARAB_CHILD_SELF_DESTRUCT) {
                Sc64ScarabChildFields f;
                if (!sc64_scarab_child_fields_unpack(u[i].carry,u[i].work_timer,&f) ||
                    !race_scarab_child_set(i,SC64_SCARAB_FIELD_NO_SLOT,phase,f.life,f.no_collision)) return 0;
            } else u[i].alive=0;
        }
    }
    return 1;
}
/* Notify only after a real child was removed; call before reusing its slot. */
static int race_scarab_child_lost(int slot,int upgraded,int lost_slot)
{
    Sc64ScarabParentFields f; Sc64ScarabParent p;
    Sc64ScarabChildFields child;
    if (!race_scarab_parent_identity(slot) || lost_slot<0 || lost_slot>=MAX_UNITS || u[lost_slot].alive ||
        u[lost_slot].type!=U_SCARAB || u[lost_slot].raw_type!=85 ||
        u[lost_slot].owner!=u[slot].owner || u[lost_slot].raw_owner!=u[slot].raw_owner ||
        !sc64_scarab_child_fields_unpack(u[lost_slot].carry,u[lost_slot].work_timer,&child) ||
        child.parent_slot!=(unsigned)slot || child.phase!=SC64_SCARAB_CHILD_TRAINING ||
        !sc64_scarab_parent_fields_unpack(u[slot].carry,u[slot].work_timer,&f)) return 0;
    p=(Sc64ScarabParent){f.active_slot==SC64_SCARAB_FIELD_NO_SLOT ? -1 : (int)f.active_slot,
                         (uint16_t)f.progress,(uint8_t)f.queued,(uint8_t)f.cooldown};
    if (!sc64_scarab_active_lost(&p,(Sc64ScarabCounts){0,0},upgraded,lost_slot)) return 0;
    return race_scarab_parent_write(slot,&p);
}
#endif
