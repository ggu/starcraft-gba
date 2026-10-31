/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHARED_PROJECTILE_STAGE_H
#define SC64_SHARED_PROJECTILE_STAGE_H
#include <stdint.h>
#include <string.h>
/* Experimental typed reservations only. Motion and damage remain owned by
 * each existing family. No reservation is evicted to make space. */
#define SC64_SHARED_PROJECTILE_CAPACITY 100u
enum { SC64_SHARED_PROJECTILE_FREE=0, SC64_SHARED_PROJECTILE_INTERCEPTOR=1,
       SC64_SHARED_PROJECTILE_YAMATO=2, SC64_SHARED_PROJECTILE_SWARM=3,
       SC64_SHARED_PROJECTILE_IMPACT=4 };
typedef struct { uint16_t generation; uint8_t type, owner; } Sc64SharedProjectileSlot;
typedef struct { Sc64SharedProjectileSlot slots[SC64_SHARED_PROJECTILE_CAPACITY]; uint8_t active; } Sc64SharedProjectileArena;
/* A reset invalidates outstanding handles as well as clearing live rows. */
static inline void sc64_shared_projectile_reset(Sc64SharedProjectileArena *a)
{
    if(!a) return;
    for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++) {
        uint16_t g=(uint16_t)(a->slots[i].generation+1u); if(!g) g=1;
        a->slots[i].generation=g; a->slots[i].type=0; a->slots[i].owner=0;
    }
    a->active=0;
}
/* Handles include a generation and one-based slot; stale handles cannot
 * release a replacement before the 16-bit generation wraps. Zero is always an invalid handle. */
static inline int sc64_shared_projectile_has_capacity(const Sc64SharedProjectileArena *a)
{ return a && a->active<SC64_SHARED_PROJECTILE_CAPACITY; }
static inline uint32_t sc64_shared_projectile_acquire(Sc64SharedProjectileArena *a,unsigned type,unsigned owner)
{
    if(!a || type<1 || type>SC64_SHARED_PROJECTILE_IMPACT || owner>255) return 0;
    for(unsigned i=0;i<SC64_SHARED_PROJECTILE_CAPACITY;i++) if(!a->slots[i].type) {
        uint16_t g=(uint16_t)(a->slots[i].generation+1u); if(!g) g=1;
        a->slots[i].generation=g; a->slots[i].type=(uint8_t)type;
        a->slots[i].owner=(uint8_t)owner; ++a->active;
        return ((uint32_t)g<<8)|(i+1u);
    }
    return 0;
}
static inline int sc64_shared_projectile_release(Sc64SharedProjectileArena *a,uint32_t handle,unsigned type,unsigned owner)
{
    unsigned index=handle&255u, generation=handle>>8;
    if(!a || !index || index>SC64_SHARED_PROJECTILE_CAPACITY || !generation || generation>65535u) return 0;
    Sc64SharedProjectileSlot *s=&a->slots[index-1u];
    if(s->generation!=generation || s->type!=type || s->owner!=owner || !a->active) return 0;
    s->type=SC64_SHARED_PROJECTILE_FREE;s->owner=0;--a->active;return 1;
}
#endif
