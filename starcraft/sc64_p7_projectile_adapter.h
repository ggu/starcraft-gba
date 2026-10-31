/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_PROJECTILE_ADAPTER_H
#define SC64_P7_PROJECTILE_ADAPTER_H
#if defined(SC64_P7_PROJECTILES_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)

/* Mode 4 leaves the upper 16 KiB of OBJ VRAM unused by the display. This
 * pool owns its first 4816 bytes through the map in sc64_obj_vram.h, which
 * places the VRAM terrain cache after it; on the host both share one backing
 * array so an overlap would show in fixtures. All access goes through the
 * halfword arena API; never memcpy/memset a byte buffer onto video memory.
 * The experimental Mode 0 renderer uses this region and is excluded there. */
#include "sc64_obj_vram.h"
static Sc64Arena race_p7_projectiles={SC64_OBJ_VRAM_PROJECTILES,SC64_ARENA_WORDS};
#ifdef GBA_HOST
/* Host fixtures snapshot the arena as plain words; it now starts the shared
 * OBJ VRAM backing array, so keep that storage reachable by its old name. */
#define race_p7_projectile_host_words ((u16 *)(void *)GBA_VRAM_SCRATCH)
#endif
static u8 race_p7_projectile_fault;
#ifdef SC64_SHARED_PROJECTILE_STAGE
static u32 sc64_shared_interceptor_handles[SC64_POOL_CAPACITY];
static void sc64_shared_interceptor_clear(void)
{
    for(unsigned i=0;i<SC64_POOL_CAPACITY;++i) if(sc64_shared_interceptor_handles[i]) {
        (void)sc64_shared_projectile_release(&sc64_shared_projectiles,
            sc64_shared_interceptor_handles[i],SC64_SHARED_PROJECTILE_INTERCEPTOR,i);
        sc64_shared_interceptor_handles[i]=0;
    }
}
#endif

static void race_p7_projectile_reset(void)
{
#ifdef SC64_SHARED_PROJECTILE_STAGE
    sc64_shared_interceptor_clear();
#endif
    race_p7_projectile_fault=(u8)!sc64_pool_init(&race_p7_projectiles);
}

static int race_p7_projectile_encode(u8 *packet)
{
    return !race_p7_projectile_fault &&
        sc64_codec_encode(&race_p7_projectiles,packet,SC64_CODEC_BYTES);
}

static int race_p7_projectile_restore(const u8 *packet)
{
#ifdef SC64_SHARED_PROJECTILE_STAGE
    /* Do not overwrite another family's live reservation on an experimental
     * load. The shared typed codec is not implemented yet. */
    if(sc64_shared_projectiles.active) return 0;
#endif
    if (!sc64_codec_decode(&race_p7_projectiles,packet,SC64_CODEC_BYTES)) return 0;
#ifdef SC64_SHARED_PROJECTILE_STAGE
    {
        u8 id=sc64_pool_active_head(&race_p7_projectiles); unsigned steps=0;
        memset(sc64_shared_interceptor_handles,0,sizeof(sc64_shared_interceptor_handles));
        while(id!=SC64_END && steps++<SC64_POOL_CAPACITY) {
            Sc64ProjectileRow row; u32 handle;
            if(!sc64_pool_get_row(&race_p7_projectiles,id,&row)) return 0;
            handle=sc64_shared_projectile_acquire(&sc64_shared_projectiles,SC64_SHARED_PROJECTILE_INTERCEPTOR,id);
            if(!handle) return 0;
            sc64_shared_interceptor_handles[id]=handle; id=row.next_link;
        }
        if(id!=SC64_END) return 0;
    }
#endif
    race_p7_projectile_fault=0;
    return 1;
}

static int race_p7_projectile_target_alive(unsigned ref)
{
    return ref<MAX_UNITS+MAX_BUILDINGS && target_alive((int)ref);
}

/* Slot reuse must clear both namespaces before a new actor is installed.
 * A shot keeps its source-owner identity even after its shooter disappears. */
static void race_p7_projectile_clear_references(int ref)
{
    u8 id=sc64_pool_active_head(&race_p7_projectiles);
    unsigned steps=0;
    while (id!=SC64_END && steps++<SC64_POOL_CAPACITY) {
        Sc64ProjectileRow row;
        if (!sc64_pool_get_row(&race_p7_projectiles,id,&row)) break;
        if (row.target_ref==ref) row.target_ref=511;
        if (ref<MAX_UNITS && row.owner_ref==ref) row.owner_ref=255;
        if (!sc64_pool_set_row(&race_p7_projectiles,id,&row)) break;
        id=row.next_link;
    }
    if (id!=SC64_END) race_p7_projectile_fault=1;
}

/* Also cover source-script removals that do not pass through hit_target.
 * Runs at the completed-update boundary, before pause/save may be accepted. */
static void race_p7_projectile_clear_dead_references(void)
{
    u8 id=sc64_pool_active_head(&race_p7_projectiles);
    unsigned steps=0;
    while (id!=SC64_END && steps++<SC64_POOL_CAPACITY) {
        Sc64ProjectileRow row;
        if (!sc64_pool_get_row(&race_p7_projectiles,id,&row)) break;
        if (!race_p7_projectile_target_alive(row.target_ref)) row.target_ref=511;
        if (row.owner_ref>=MAX_UNITS || !u[row.owner_ref].alive) row.owner_ref=255;
        if (!sc64_pool_set_row(&race_p7_projectiles,id,&row)) break;
        id=row.next_link;
    }
    if (id!=SC64_END) race_p7_projectile_fault=1;
}

/* Entry point for the pending child weapon-event adapter, with native heading and source
 * Q8 position. No damage is applied here. A full 100-shot pool rejects the
 * new shot instead of recycling an in-flight projectile. */
static int race_p7_projectile_fire(int shooter,int target,u8 heading,
                                   s32 source_x_q8,s32 source_y_q8)
{
    Sc64ProjectileRow row;
    u8 id,native_index;
#if defined(SC64_P7_CARRIER_TRAINING_ENABLED) && \
    !defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) && !defined(SC64_SHARED_CARRIER_EXPERIMENT)
    /* All children in this profile are still inside the hangar. */
    (void)shooter; (void)target; (void)heading;
    (void)source_x_q8; (void)source_y_q8;
    return 0;
#endif
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
    /* Only launched Interceptors fire; hangar rows stay inert. */
    if (shooter>=0 && shooter<MAX_UNITS && SC64_CARRIER_HANGAR_HIDDEN(&u[shooter]))
        return 0;
#endif
    if (!race_carrier_services_active() || race_p7_projectile_fault || shooter<0 ||
        shooter>=MAX_UNITS || !u[shooter].alive ||
        u[shooter].type!=SC64_INTERCEPTOR_TYPE || RACE_PROTOSS_RAW(&u[shooter])!=73 ||
        RACE_PROTOSS_SLOT(&u[shooter])<0 || RACE_PROTOSS_SLOT(&u[shooter])>=8 ||
        race_protoss_source_owner((unsigned)RACE_PROTOSS_SLOT(&u[shooter]))!=u[shooter].owner ||
        source_x_q8<0 || source_y_q8<0 ||
        source_x_q8>=race_cols*8192 || source_y_q8>=race_rows*8192 ||
        source_x_q8/512!=u[shooter].x || source_y_q8/512!=u[shooter].y ||
        target<0 || !race_p7_projectile_target_alive((unsigned)target) ||
        !sc64_interceptor_projectile_launch(&row,source_x_q8,source_y_q8,
            (s16)(target_x(target)*2),(s16)(target_y(target)*2),heading,
            (u16)target,(u8)shooter,(u8)RACE_PROTOSS_SLOT(&u[shooter]))) return 0;
    if (!sc64_pool_acquire(&race_p7_projectiles,&id,&native_index)) return 0;
#ifdef SC64_SHARED_PROJECTILE_STAGE
    u32 shared_handle=sc64_shared_projectile_acquire(&sc64_shared_projectiles,
        SC64_SHARED_PROJECTILE_INTERCEPTOR,id);
    if(!shared_handle) { (void)sc64_pool_retire(&race_p7_projectiles,id); return 0; }
    sc64_shared_interceptor_handles[id]=shared_handle;
#endif
    (void)native_index;
    if (!sc64_pool_set_row(&race_p7_projectiles,id,&row)) {
#ifdef SC64_SHARED_PROJECTILE_STAGE
        (void)sc64_shared_projectile_release(&sc64_shared_projectiles,shared_handle,
            SC64_SHARED_PROJECTILE_INTERCEPTOR,id);
        sc64_shared_interceptor_handles[id]=0;
        (void)sc64_pool_retire(&race_p7_projectiles,id);
#endif
        race_p7_projectile_fault=1; return 0;
    }
    return 1;
}

/* Unit orders run before this pass. The caller supplies the existing shared
 * source-24 due event; a second accumulator would drift across save/load. */
static void race_p7_projectile_source_tick(void)
{
    u8 id=sc64_pool_active_head(&race_p7_projectiles);
    unsigned steps=0;
    if (race_p7_projectile_fault) return;
    while (id!=SC64_END && steps++<SC64_POOL_CAPACITY) {
        Sc64ProjectileRow row;
        Sc64InterceptorMotionContext context;
        u8 next;
        int impact;
        if (!sc64_pool_get_row(&race_p7_projectiles,id,&row)) break;
        next=row.next_link;
        context.map_width_px=(u16)(race_cols*32);
        context.map_height_px=(u16)(race_rows*32);
        context.target_alive=(u8)race_p7_projectile_target_alive(row.target_ref);
        context.owner_alive=(u8)(row.owner_ref<MAX_UNITS && u[row.owner_ref].alive);
        if (!sc64_interceptor_motion_tick(&row,&context)) break;
        impact=row.pending_damage_event;
        /* The pulse is consumed inside this complete update. A saved DYING
         * row must never reapply it after load. Commit before callbacks. */
        row.pending_damage_event=0;
        if (row.state==3) {
#ifdef SC64_SHARED_PROJECTILE_STAGE
            if(!sc64_shared_projectile_release(&sc64_shared_projectiles,
               sc64_shared_interceptor_handles[id],SC64_SHARED_PROJECTILE_INTERCEPTOR,id)) break;
            sc64_shared_interceptor_handles[id]=0;
#endif
            if(!sc64_pool_retire(&race_p7_projectiles,id)) break;
        }
        if (!sc64_pool_set_row(&race_p7_projectiles,id,&row)) break;
        if (impact && race_p7_projectile_target_alive(row.target_ref)) {
            int owner=race_protoss_source_owner(row.raw_source_owner);
            int damage=6+(owner==SIDE?weapons_level:0);
            if (owner>=0) hit_target(row.target_ref,damage,SC64_INTERCEPTOR_TYPE,owner);
        }
        id=next;
    }
    if (id!=SC64_END) race_p7_projectile_fault=1;
}

static void race_p7_projectile_tick_if_due(void)
{
    if (race_carrier_services_active() && RACE_PROTOSS_SERVICE_CLOCK[0]!=4u)
        race_p7_projectile_source_tick();
}

static void race_p7_projectile_draw(void)
{
    u8 id=sc64_pool_active_head(&race_p7_projectiles);
    unsigned steps=0;
    while (id!=SC64_END && steps++<SC64_POOL_CAPACITY) {
        Sc64ProjectileRow row;
        if (!sc64_pool_get_row(&race_p7_projectiles,id,&row)) return;
        int x=row.x_q8_source>=0?row.x_q8_source/512:
              -(int)((511u+(unsigned)(-row.x_q8_source))/512u);
        int y=row.y_q8_source>=0?row.y_q8_source/512:
              -(int)((511u+(unsigned)(-row.y_q8_source))/512u);
        if (row.state<2 && x>=0 && y>=0 && x<WORLD_W && y<WORLD_H &&
            currently_visible(x,y)) {
            x-=camx; y=y-camy+12;
            if (x>=1 && x<239 && y>=13 && y<119) {
                /* Authored marker until the source projectile image bank is
                 * imported. Its position comes from the actual moving row. */
                gfx_rect(x-1,y,3,1,C_GOLD); gfx_pixel(x,y,C_WHITE);
            }
        }
        id=row.next_link;
    }
}
#endif
#endif
