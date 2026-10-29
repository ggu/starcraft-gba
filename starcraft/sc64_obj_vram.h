/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ownership map of the OBJ tile VRAM, 0x06014000-0x06017FFF. Mode-4 products
 * keep objects off, so the area is free for data, but it is not free for
 * one owner: the P7 interceptor projectile arena lives at its start, an
 * optional fog shade map follows, and the terrain cache takes the remainder.
 * Every product takes its range
 * from this map, so the two never overlap by construction, and the host
 * build backs the whole area with one shared array (gba_vram_scratch) so an
 * overlap would corrupt host fixtures exactly as it would the hardware.
 *
 *   [0, SC64_OBJ_VRAM_PROJECTILE_BYTES)        P7 projectile arena (halfwords)
 *   [FOG_OFFSET, FOG_OFFSET + FOG_BYTES)       optional palette shade map
 *   [SC64_OBJ_VRAM_CACHE_OFFSET, 16 KiB)       SC64_TERRAIN_CACHE_VRAM rows */
#ifndef SC64_OBJ_VRAM_H
#define SC64_OBJ_VRAM_H

#ifdef SC64_P7_PROJECTILES_ENABLED
#include "sc64_interceptor_pool_codec.h"
#define SC64_OBJ_VRAM_PROJECTILE_BYTES (SC64_ARENA_WORDS * 2u)
#else
#define SC64_OBJ_VRAM_PROJECTILE_BYTES 0u
#endif
#define SC64_OBJ_VRAM_FOG_OFFSET ((SC64_OBJ_VRAM_PROJECTILE_BYTES + 15u) & ~15u)
#ifdef SC64_SOFT_FOG
#define SC64_OBJ_VRAM_FOG_BYTES 256u
#define SC64_OBJ_VRAM_FOG ((volatile u16 *)(void *)(GBA_VRAM_SCRATCH + SC64_OBJ_VRAM_FOG_OFFSET))
#else
#define SC64_OBJ_VRAM_FOG_BYTES 0u
#endif
#define SC64_OBJ_VRAM_CACHE_OFFSET (SC64_OBJ_VRAM_FOG_OFFSET + SC64_OBJ_VRAM_FOG_BYTES)
#define SC64_OBJ_VRAM_CACHE_BYTES (GBA_VRAM_SCRATCH_BYTES - SC64_OBJ_VRAM_CACHE_OFFSET)
#define SC64_OBJ_VRAM_PROJECTILES ((volatile u16 *)(void *)GBA_VRAM_SCRATCH)
#define SC64_OBJ_VRAM_CACHE ((u8 *)GBA_VRAM_SCRATCH + SC64_OBJ_VRAM_CACHE_OFFSET)

_Static_assert(SC64_OBJ_VRAM_PROJECTILE_BYTES <= SC64_OBJ_VRAM_CACHE_OFFSET,
               "the VRAM terrain cache starts inside the projectile arena");
_Static_assert(SC64_OBJ_VRAM_PROJECTILE_BYTES <= SC64_OBJ_VRAM_FOG_OFFSET,
               "fog shade map starts inside the projectile arena");
_Static_assert(SC64_OBJ_VRAM_FOG_OFFSET + SC64_OBJ_VRAM_FOG_BYTES <= SC64_OBJ_VRAM_CACHE_OFFSET,
               "fog shade map overlaps the terrain cache");
_Static_assert(SC64_OBJ_VRAM_CACHE_OFFSET <= GBA_VRAM_SCRATCH_BYTES,
               "the projectile arena exceeds the OBJ tile VRAM");
_Static_assert((SC64_OBJ_VRAM_CACHE_OFFSET & 3u) == 0u,
               "the VRAM terrain cache must stay word aligned for DMA");
#if defined(SC64_TILE_RENDERER) && \
    (defined(SC64_P7_PROJECTILES_ENABLED) || defined(SC64_TERRAIN_CACHE_VRAM) || defined(SC64_SOFT_FOG))
#error "the experimental tile renderer owns the whole OBJ tile VRAM"
#endif

#endif
