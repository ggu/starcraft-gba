/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_WAYPOINT_BRIDGE_H
#define SC64_SCARAB_WAYPOINT_BRIDGE_H
#include <stdint.h>
/* Unpublished typed/derived feature-15 candidate only: caller authenticates
 * the exact candidate/tag pair and chooses the actual row codec. Never use
 * this for Terran compact rows or a historical feature-15 producer. A full
 * source waypoint is twice an integral GBA coordinate; current scratch may
 * also contain an odd, freshly initialized engine coordinate. Source Scarabs
 * are allocated with flags=0 and shields 0..10. The adapter stores all four
 * path fields and the real shield in the row's existing bounded fields.
 * The remaining mixed-radix values are noncanonical and fail closed. */
enum { SC64_SCARAB_WAYPOINT_DERIVED=1, SC64_SCARAB_WAYPOINT_TYPED=2 };
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
_Static_assert((uint64_t)2048u*2048u*25u*13u*512u*8192u >=
               ((uint64_t)1u<<46)*11u*4u,
               "typed row must cover every bounded waypoint and motion flag");
#endif
static int sc64_scarab_waypoint_radices(int codec, unsigned *shield, unsigned *flags)
{
    if (!shield || !flags) return 0;
    if (codec==SC64_SCARAB_WAYPOINT_DERIVED) { *shield=131072u; *flags=8u; return 1; }
    if (codec==SC64_SCARAB_WAYPOINT_TYPED) {
        *shield=512u;
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
        *flags=8192u; /* two extra motion bits, within typed 14-bit flags */
#else
        *flags=2048u;
#endif
        return 1;
    }
    return 0;
}
static int sc64_scarab_waypoint_to_wire(const Unit *in, Unit *out,
                                        unsigned width, unsigned height,
                                        unsigned features, int codec)
{
    Unit q; unsigned phase, sr, fr, previous; uint64_t n;
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    unsigned motion;
#endif
    if (!in || !out || !width || !height || width>2048 || height>2048 ||
        features!=15u || !sc64_scarab_waypoint_radices(codec,&sr,&fr)) return 0;
    if (in->type!=U_SCARAB && in->raw_type!=85) { *out=*in; return 1; }
    if (in->type!=U_SCARAB || in->raw_type!=85) return 0;
    phase=((unsigned)in->carry>>8)&3u;
    if (phase<2) { *out=*in; return 1; }
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    if (codec!=SC64_SCARAB_WAYPOINT_TYPED || ((unsigned)in->path_timer & 0x3000u)) return 0;
    motion=(unsigned)in->path_timer>>14;
    previous=(unsigned)in->path_timer&4095u;
#else
    if ((unsigned)in->weapon>3u) return 0;
    previous=(unsigned)in->path_timer;
#endif
    if (in->flags || in->shield<0 || in->shield>10 ||
        (previous&1u) || (in->path_retry&1) ||
        previous/2u>=width || (unsigned)in->path_retry/2u>=height ||
        in->path_x<0 || in->path_y<0 || (unsigned)in->path_x>=width*2u ||
        (unsigned)in->path_y>=height*2u) return 0;
    n=previous/2u;
    n=n*2048u+(unsigned)in->path_retry/2u;
    n=n*4096u+(unsigned)in->path_x;
    n=n*4096u+(unsigned)in->path_y;
    n=n*11u+(unsigned)in->shield;
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    n=n*4u+motion;
#endif
    q=*in;
    q.path_x=(int)(n%2048u); n/=2048u;
    q.path_y=(int)(n%2048u); n/=2048u;
    q.path_timer=(int)(n%25u); n/=25u;
    q.path_retry=(int)(n%13u); n/=13u;
    q.shield=(int)(n%sr); n/=sr;
    if (n>=fr) return 0;
    q.flags=(int)n;
    *out=q; return 1;
}
static int sc64_scarab_waypoint_from_wire(const Unit *in, Unit *out,
                                          unsigned width, unsigned height,
                                          unsigned features, int codec)
{
    Unit q; unsigned phase,sr,fr,prevx,prevy,curx,cury,shield; uint64_t n;
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    unsigned motion;
#endif
    if (!in || !out || !width || !height || width>2048 || height>2048 ||
        features!=15u || !sc64_scarab_waypoint_radices(codec,&sr,&fr)) return 0;
    if (in->type!=U_SCARAB && in->raw_type!=85) { *out=*in; return 1; }
    if (in->type!=U_SCARAB || in->raw_type!=85) return 0;
    phase=((unsigned)in->carry>>8)&3u;
    if (phase<2) { *out=*in; return 1; }
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    if (codec!=SC64_SCARAB_WAYPOINT_TYPED) return 0;
#else
    if ((unsigned)in->weapon>3u) return 0;
#endif
    if (in->path_x<0 || in->path_x>2047 ||
        in->path_y<0 || in->path_y>2047 || (unsigned)in->path_timer>=25u ||
        (unsigned)in->path_retry>=13u || in->shield<0 || (unsigned)in->shield>=sr ||
        (unsigned)in->flags>=fr) return 0;
    n=(unsigned)in->flags;
    n=n*sr+(unsigned)in->shield;
    n=n*13u+(unsigned)in->path_retry;
    n=n*25u+(unsigned)in->path_timer;
    n=n*2048u+(unsigned)in->path_y;
    n=n*2048u+(unsigned)in->path_x;
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    if (n>=(((uint64_t)1u<<46)*11u*4u)) return 0;
    motion=(unsigned)(n%4u); n/=4u;
#else
    if (n>=(((uint64_t)1u<<46)*11u)) return 0;
#endif
    shield=(unsigned)(n%11u); n/=11u;
    cury=(unsigned)(n%4096u); n/=4096u;
    curx=(unsigned)(n%4096u); n/=4096u;
    prevy=(unsigned)(n%2048u); n/=2048u; prevx=(unsigned)n;
    if (prevx>=width || prevy>=height || curx>=width*2u || cury>=height*2u) return 0;
    q=*in; q.path_timer=(int)(prevx*2u);
#if defined(SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    q.path_timer |= (int)(motion<<14);
#endif
    q.path_retry=(int)(prevy*2u);
    q.path_x=(int)curx; q.path_y=(int)cury; q.shield=(int)shield; q.flags=0;
    *out=q; return 1;
}
#endif
