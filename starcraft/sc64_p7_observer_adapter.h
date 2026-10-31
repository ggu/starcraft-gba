#ifndef SC64_P7_OBSERVER_ADAPTER_H
#define SC64_P7_OBSERVER_ADAPTER_H

#if defined(SC64_P7_SIGHT_AXIS_CULL) && !defined(SC64_P7_ACTORS)
#error "SC64_P7_SIGHT_AXIS_CULL requires SC64_P7_ACTORS"
#endif
#if defined(SC64_P7_ARBITER_AURA_ONCE) && \
    !defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED)
#error "SC64_P7_ARBITER_AURA_ONCE requires the P7 Arbiter cache"
#endif

#ifdef SC64_P7_OBSERVER_ENABLED
/* Current roster maxima after source-pixel conversion, including fallbacks.
 * The optional cull is conservative; the exact radius/circle test follows it.
 * tests/sc64_p7_sight_axis_cull.c checks all types and the boundary geometry. */
enum { SC64_P7_SIGHT_UNIT_AXIS_BOUND=176, SC64_P7_SIGHT_BUILDING_AXIS_BOUND=160 };
#include "sc64_p7_observer_rules.h"
#include "sc64_p7_geometry.h"
#include "sc64_p7_sight_runtime.h"
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
#include "sc64_p7_power_geometry.h"
#endif

/* Owner-specific optical visibility query. The world maintains a single
 * SIDE grid, so only source slot 7 can reuse it. Other P7 owners are evaluated
 * on demand using the same cell-center circles as sc64_visibility.h. */
static int sc64_p7_observer_source_matches(int raw_owner, int live_owner,
                                           int source_raw_owner,
                                           int source_live_owner)
{
    /* A known querying slot must never absorb another source player's
       unknown or collapsed live-owner records. Unknown query provenance alone
       triggers the documented legacy live-owner fallback. */
    if (source_raw_owner >= 0 && source_raw_owner < 8)
        return raw_owner == source_raw_owner;
    return live_owner == source_live_owner;
}


static int sc64_p7_observer_owner_sees_cell(int source_owner, int live_owner,
                                           int cell)
{
    int col = cell % MAP_COLS, row = cell / MAP_COLS;
    int cx = col * 16 + 8, cy = row * 16 + 8;
    if (source_owner == 7 || (source_owner < 0 && live_owner == SIDE))
        return SC64_GRID_GET(visible, cell);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        sc64_p7_observer_source_matches(RACE_PROTOSS_SLOT(&u[i]), u[i].owner,
                                        source_owner, live_owner)) {
#ifdef SC64_P7_SIGHT_AXIS_CULL
        int dx = cx - u[i].x, dy = cy - u[i].y;
        if (dx < -SC64_P7_SIGHT_UNIT_AXIS_BOUND || dx > SC64_P7_SIGHT_UNIT_AXIS_BOUND ||
            dy < -SC64_P7_SIGHT_UNIT_AXIS_BOUND || dy > SC64_P7_SIGHT_UNIT_AXIS_BOUND) continue;
        int radius = sc64_p7_observer_sight_radius_runtime(u[i].type, 0);
#else
        int radius = sc64_p7_observer_sight_radius_runtime(u[i].type, 0);
        int dx = cx - u[i].x, dy = cy - u[i].y;
#endif
        if (dx * dx + dy * dy <= radius * radius) return 1;
    }
    /* Match the current GBA sight grid: alive buildings contribute optical
     * sight while under construction. Native construction-sight differences
     * are not modeled by this approximation. */
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        sc64_p7_observer_source_matches(RACE_PROTOSS_SLOT(&b[i]), b[i].owner,
                                        source_owner, live_owner)) {
#ifdef SC64_P7_SIGHT_AXIS_CULL
        int dx = cx - b[i].x, dy = cy - b[i].y;
        if (dx < -SC64_P7_SIGHT_BUILDING_AXIS_BOUND || dx > SC64_P7_SIGHT_BUILDING_AXIS_BOUND ||
            dy < -SC64_P7_SIGHT_BUILDING_AXIS_BOUND || dy > SC64_P7_SIGHT_BUILDING_AXIS_BOUND) continue;
        int radius = sc64_p7_observer_sight_radius_runtime(b[i].type, 1);
#else
        int radius = sc64_p7_observer_sight_radius_runtime(b[i].type, 1);
        int dx = cx - b[i].x, dy = cy - b[i].y;
#endif
        if (dx * dx + dy * dy <= radius * radius) return 1;
    }
    return 0;
}

static int sc64_p7_observer_target_optically_visible(int target,
                                                      int source_owner,
                                                      int live_owner)
{
    int x, y, cell;
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS) return 0;
    if (target < MAX_UNITS) {
        if (!u[target].alive) return 0;
        x = u[target].x; y = u[target].y;
    } else {
        int i = target - MAX_UNITS;
        if (!b[i].alive) return 0;
        x = b[i].x; y = b[i].y;
    }
    int col = x / 16, row = y / 16;
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) return 0;
    cell = col + row * MAP_COLS;
    return sc64_p7_observer_owner_sees_cell(source_owner, live_owner, cell);
}

static int sc64_p7_observer_target_raw_type(int target)
{
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS) return -1;
    return target < MAX_UNITS ? RACE_PROTOSS_RAW(&u[target]) :
           RACE_PROTOSS_RAW(&b[target - MAX_UNITS]);
}

static int sc64_p7_observer_pylon_powers(int cannon, int source_owner,
                                          int live_owner)
{
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) &&
        b[i].type == B_PYLON &&
        sc64_p7_observer_source_matches(RACE_PROTOSS_SLOT(&b[i]), b[i].owner,
                                        source_owner, live_owner) &&
        sc64_p7_in_psi_range(b[cannon].x-b[i].x,b[cannon].y-b[i].y))
        return 1;
#else
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) &&
        b[i].type == B_PYLON &&
        sc64_p7_observer_source_matches(RACE_PROTOSS_SLOT(&b[i]), b[i].owner,
                                        source_owner, live_owner) &&
        dist2(b[cannon].x, b[cannon].y, b[i].x, b[i].y) <= 144 * 144)
        return 1;
#endif
    return 0;
}

static unsigned sc64_p7_observer_xy_length(unsigned x, unsigned y)
{
    unsigned major = x > y ? x : y, minor = x > y ? y : x;
    /* The runtime uses the pinned integer distance metric, approximated with
       shifts so this stays deterministic and cheap on the ARM target. */
    if (major / 4u < minor)
        major = major - major / 16u + minor * 3u / 8u - major / 64u +
                minor * 3u / 256u;
    return major;
}

static int sc64_p7_observer_rect_distance(unsigned raw_a, int x_a, int y_a,
                                          int building_a, unsigned raw_b,
                                          int x_b, int y_b, int building_b)
{
    int al, at, ar, ab, bl, bt, br, bb;
    if (!sc64_p7_actor_rect_source(raw_a, x_a, y_a, building_a,
                                  &al, &at, &ar, &ab) ||
        !sc64_p7_actor_rect_source(raw_b, x_b, y_b, building_b,
                                  &bl, &bt, &br, &bb)) return -1;
    /* Half-open helper rectangles correspond to inclusive source rectangles.
       Source rect_difference measures the gap between inclusive edge pixels. */
    int dx = al >= br ? al - (br - 1) : bl >= ar ? bl - (ar - 1) : 0;
    int dy = at >= bb ? at - (bb - 1) : bt >= ab ? bt - (ab - 1) : 0;
    return (int)sc64_p7_observer_xy_length((unsigned)dx,(unsigned)dy);
}

#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
/* Exact continuation of rect_distance after both source rectangles have
 * passed the same actor-rectangle validation as the legacy helper. */
static int sc64_p7_observer_rect_distance_rects(int al,int at,int ar,int ab,
                                                int bl,int bt,int br,int bb)
{
    int dx=al>=br ? al-(br-1) : bl>=ar ? bl-(ar-1) : 0;
    int dy=at>=bb ? at-(bb-1) : bt>=ab ? bt-(ab-1) : 0;
    return (int)sc64_p7_observer_xy_length((unsigned)dx,(unsigned)dy);
}

/* The caller validates the Arbiter rectangle once before reusing target
 * rectangles here. The clipped finder-square policy is unchanged. */
static int sc64_p7_arbiter_finder_square_rect(int arbiter,int tl,int tt,
                                              int tr,int tb)
{
    int cx=u[arbiter].x*2,cy=u[arbiter].y*2;
    int left=cx-160,top=cy-160,right=cx+160,bottom=cy+160;
    if(left<0)left=0; if(top<0)top=0;
    if(right>SC64_P7_MAP_COLS*32)right=SC64_P7_MAP_COLS*32;
    if(bottom>SC64_P7_MAP_ROWS*32)bottom=SC64_P7_MAP_ROWS*32;
    --tr; --tb;
    return tl<right && tr>=left && tt<bottom && tb>=top;
}
#endif

/* Generation-39 cache layout in the otherwise-unused Unit.weapon word:
 * bits 0..7 detected-and-optically-visible P7 owner mask;
 * bits 8..12 target's native secondary-order countdown (0..30);
 * bit 13 valid marker, bit 14 current Arbiter aura membership, bit 15 tracked
 * for detection (intrinsically cloaked Observer / Dark Templar / Zeratul, or
 * current Arbiter aura).
 * This state is accepted only by the new save generation and only in
 * missions that run the shared Protoss services (P7, and P8 in its
 * candidate). */
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
#define SC64_P7_ARBITER_DETECTED_MASK(v) ((unsigned)(v) & 0xffu)
#define SC64_P7_ARBITER_COUNTDOWN(v) (((unsigned)(v) >> 8) & 31u)
#define SC64_P7_ARBITER_VALID (1u << 13)
#define SC64_P7_ARBITER_AURA (1u << 14)
#define SC64_P7_ARBITER_TRACKED (1u << 15)
#define SC64_P7_ARBITER_CACHE_VALID(v) \
    (((unsigned)(v) & SC64_P7_ARBITER_VALID) != 0 && \
     SC64_P7_ARBITER_COUNTDOWN(v) <= 30u)
static unsigned sc64_p7_arbiter_pack(unsigned mask,unsigned timer,int aura,int tracked)
{
    return (mask & 255u) | ((timer & 31u) << 8) | SC64_P7_ARBITER_VALID |
           (aura ? SC64_P7_ARBITER_AURA : 0u) |
           (tracked ? SC64_P7_ARBITER_TRACKED : 0u);
}

/* U_SCV is enum value zero. Only a byte-for-byte zero Unit row is unused. */
#include "sc64_zero_bytes.h"
static int sc64_p7_arbiter_cache_empty_unit(const void *actor)
{
    return sc64_bytes_are_zero(actor, sizeof(Unit));
}

/* u[] supplies aligned, complete Unit objects. Unroll the representation
 * check for zero-filled unused slots; retain the generic byte-safe fallback
 * for other layouts and compilers. No padding or tail bytes are ignored. */
static int sc64_p7_arbiter_cache_empty_slot(unsigned slot)
{
#if defined(__GNUC__) || defined(__clang__)
    if (_Alignof(Unit) >= 4 && sizeof(Unit) % 4 == 0) {
        const Sc64ZeroAlias32 *p=(const Sc64ZeroAlias32 *)(const void *)&u[slot];
        unsigned n=(unsigned)(sizeof(Unit)/4), i=0;
        for (;i+4<=n;i+=4)
            if ((p[i] | p[i+1] | p[i+2] | p[i+3]) != 0) return 0;
        for (;i<n;++i) if (p[i] != 0) return 0;
        return 1;
    }
#endif
    return sc64_p7_arbiter_cache_empty_unit(&u[slot]);
}

static int sc64_p7_observer_arbiter_aura_candidate(int target)
{
    unsigned actor_flags;
    if (target < 0 || target >= MAX_UNITS || !u[target].alive ||
        SC64_TRANSPORT_LOADED(&u[target]) || RACE_PROTOSS_SLOT(&u[target]) < 0 ||
        RACE_PROTOSS_SLOT(&u[target]) >= 8 || RACE_PROTOSS_RAW(&u[target]) == 71u ||
        RACE_PROTOSS_RAW(&u[target]) == 86u || RACE_PROTOSS_RAW(&u[target]) == 14u ||
        (u[target].flags & 0x0008u))
        return 0; /* Arbiter / Danimoth / Nuclear Missile / hallucination */
    actor_flags=sc64_game_unit_actor_flags(u[target].type);
    if ((actor_flags & (SC64_GAME_ACTOR_BUILDING | SC64_GAME_ACTOR_HIDDEN)) ||
#ifdef SC64_Z5_ACTORS
        (actor_flags & SC64_ZP_ACTOR_POWERUP) ||
#endif
        0) return 0;
    return 1;
}

/* OpenBW first finds candidates whose INNER sprite box overlaps the clipped,
 * half-open square, then applies outer-box integer distance. Its inner box
 * omits right/bottom +1; the cached source rectangles include those edges. */
static int sc64_p7_arbiter_finder_square(int arbiter,int target)
{
    int al,at,ar,ab,tl,tt,tr,tb;
    int cx,cy,left,top,right,bottom;
    if (!sc64_p7_actor_rect_source(71u,u[arbiter].x,u[arbiter].y,0,
                                   &al,&at,&ar,&ab) ||
        !sc64_p7_actor_rect_source((unsigned)RACE_PROTOSS_RAW(&u[target]),u[target].x,
                                   u[target].y,0,&tl,&tt,&tr,&tb)) return 0;
    /* Runtime x/y is the sprite center. Convert GBA pixels to source pixels. */
    cx=u[arbiter].x*2; cy=u[arbiter].y*2;
    left=cx-160; top=cy-160; right=cx+160; bottom=cy+160;
    if (left<0) left=0; if (top<0) top=0;
    if (right>SC64_P7_MAP_COLS*32) right=SC64_P7_MAP_COLS*32;
    if (bottom>SC64_P7_MAP_ROWS*32) bottom=SC64_P7_MAP_ROWS*32;
    /* Convert half-open geometry to native inner inclusive endpoints. */
    --tr; --tb;
    (void)al; (void)at; (void)ar; (void)ab;
    return tl<right && tr>=left && tt<bottom && tb>=top;
}

static int sc64_p7_arbiter_same_source(int arbiter,int target)
{
    int raw=RACE_PROTOSS_SLOT(&u[arbiter]);
    if (raw<0 || raw>=8 || raw!=RACE_PROTOSS_SLOT(&u[target]) ||
        u[arbiter].owner!=u[target].owner) return 0;
    /* The compact owner map collapses P7 source forces 0 and 4 to ENEMY.
       Require both identities to match the authenticated runtime mapping. */
    return race_protoss_source_owner((unsigned)raw)==u[arbiter].owner &&
           race_protoss_source_owner((unsigned)RACE_PROTOSS_SLOT(&u[target]))==u[target].owner;
}

static int sc64_p7_arbiter_aura_cloaks(int target)
{
    return target>=0 && target<MAX_UNITS && u[target].alive &&
        SC64_P7_ARBITER_CACHE_VALID(u[target].weapon) &&
        (u[target].weapon & SC64_P7_ARBITER_AURA)!=0;
}
static int sc64_p7_observer_arbiter_aura_cloaks(int target)
{ return sc64_p7_arbiter_aura_cloaks(target); }


/* Recompute once per native source update, before game orders query cloaking.
 * The 4/5 accumulator converts 30 GBA frames to 24 source updates without
 * advancing in title/pause/result states (game_update returns before here).
 * Target timer refresh is 31 source updates; first aura application refreshes
 * immediately, matching cloak_unit's immediate update_detected_flags call. */
static unsigned sc64_p7_arbiter_detected_mask(int target,
                                                  const u8 powered_cannons[MAX_BUILDINGS])
{
    unsigned visible_owners=0,detected=0;
    int x,y,col,row,cx,cy;
    if(target<0||target>=MAX_UNITS||!u[target].alive) return 0;
    x=u[target].x; y=u[target].y; col=x/16; row=y/16;
    if(col<0||row<0||col>=MAP_COLS||row>=MAP_ROWS) return 0;
    cx=col*16+8; cy=row*16+8;
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
    int target_l,target_t,target_r,target_b;
    if(!sc64_p7_actor_rect_source((unsigned)RACE_PROTOSS_RAW(&u[target]),x,y,0,
                                  &target_l,&target_t,&target_r,&target_b))
        return 0;
#endif
    /* Calculate all non-human source visibility in one actor/building pass;
       slot 7 uses the engine's authenticated SIDE visibility grid. */
    {
        int cell=col+row*MAP_COLS;
        if(SC64_GRID_GET(visible,cell)) visible_owners|=1u<<7;
    }
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
        !SC64_TRANSPORT_LOADED(&u[i])) {
        int source=RACE_PROTOSS_SLOT(&u[i]);
#ifdef SC64_P7_SIGHT_AXIS_CULL
        int dx=cx-u[i].x,dy=cy-u[i].y;
        if(dx < -SC64_P7_SIGHT_UNIT_AXIS_BOUND || dx > SC64_P7_SIGHT_UNIT_AXIS_BOUND ||
            dy < -SC64_P7_SIGHT_UNIT_AXIS_BOUND || dy > SC64_P7_SIGHT_UNIT_AXIS_BOUND) continue;
#else
        int dx=cx-u[i].x,dy=cy-u[i].y;
#endif
        unsigned candidate=0;
        if(source>=0&&source<8) {
            int live=race_protoss_source_owner((unsigned)source);
            if(live>=0&&live==u[i].owner) candidate=1u<<(unsigned)source;
        } else for(unsigned owner=0;owner<7;++owner)
            if(race_protoss_source_owner(owner)==u[i].owner) candidate|=1u<<owner;
        if(!(candidate & ~visible_owners)) continue;
        int radius=sc64_p7_observer_sight_radius_runtime(u[i].type,0);
        if(dx*dx+dy*dy>radius*radius) continue;
        visible_owners|=candidate;
    }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive) {
        int source=RACE_PROTOSS_SLOT(&b[i]);
#ifdef SC64_P7_SIGHT_AXIS_CULL
        int dx=cx-b[i].x,dy=cy-b[i].y;
        if(dx < -SC64_P7_SIGHT_BUILDING_AXIS_BOUND || dx > SC64_P7_SIGHT_BUILDING_AXIS_BOUND ||
            dy < -SC64_P7_SIGHT_BUILDING_AXIS_BOUND || dy > SC64_P7_SIGHT_BUILDING_AXIS_BOUND) continue;
#else
        int dx=cx-b[i].x,dy=cy-b[i].y;
#endif
        unsigned candidate=0;
        if(source>=0&&source<8) {
            int live=race_protoss_source_owner((unsigned)source);
            if(live>=0&&live==b[i].owner) candidate=1u<<(unsigned)source;
        } else for(unsigned owner=0;owner<7;++owner)
            if(race_protoss_source_owner(owner)==b[i].owner) candidate|=1u<<owner;
        if(!(candidate & ~visible_owners)) continue;
        int radius=sc64_p7_observer_sight_radius_runtime(b[i].type,1);
        if(dx*dx+dy*dy>radius*radius) continue;
        visible_owners|=candidate;
    }
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        RACE_PROTOSS_RAW(&u[i])==SC64_P7_OBSERVER_RAW) {
        int source=RACE_PROTOSS_SLOT(&u[i]);
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        int ol,ot,or_,ob;
        int distance=-1;
        if(sc64_p7_actor_rect_source(SC64_P7_OBSERVER_RAW,u[i].x,u[i].y,0,
                                     &ol,&ot,&or_,&ob))
            distance=sc64_p7_observer_rect_distance_rects(
                ol,ot,or_,ob,target_l,target_t,target_r,target_b);
#else
        int distance=sc64_p7_observer_rect_distance(SC64_P7_OBSERVER_RAW,
            u[i].x,u[i].y,0,(unsigned)RACE_PROTOSS_RAW(&u[target]),u[target].x,u[target].y,0);
#endif
        if(distance<0||distance>SC64_P7_OBSERVER_BASE_SIGHT_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_protoss_source_owner((unsigned)source)==u[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
        else if(source<0||source>=8) for(unsigned owner=0;owner<7;++owner)
            if(race_protoss_source_owner(owner)==u[i].owner &&
               (visible_owners&(1u<<owner))) detected|=1u<<owner;
    }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive &&
        RACE_PROTOSS_RAW(&b[i])==SC64_P7_CANNON_RAW && building_complete(i) &&
        powered_cannons[i]) {
        int source=RACE_PROTOSS_SLOT(&b[i]);
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        int cl,ct,cr,cb;
        int distance=-1;
        if(sc64_p7_actor_rect_source(SC64_P7_CANNON_RAW,b[i].x,b[i].y,1,
                                     &cl,&ct,&cr,&cb))
            distance=sc64_p7_observer_rect_distance_rects(
                cl,ct,cr,cb,target_l,target_t,target_r,target_b);
#else
        int distance=sc64_p7_observer_rect_distance(SC64_P7_CANNON_RAW,
            b[i].x,b[i].y,1,(unsigned)RACE_PROTOSS_RAW(&u[target]),u[target].x,u[target].y,0);
#endif
        if(distance<0||distance>SC64_P7_CANNON_DETECTION_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_protoss_source_owner((unsigned)source)==b[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
        else if(source<0||source>=8) for(unsigned owner=0;owner<7;++owner)
            if(race_protoss_source_owner(owner)==b[i].owner &&
               (visible_owners&(1u<<owner))) detected|=1u<<owner;
    }
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
    /* Zerg detectors. As for the Observer, a mobile detector detects at its
     * sight range (Overlord, 11 tiles, no Antennae); a completed structure
     * at 7 tiles (Spore Colony, as the Photon Cannon). */
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive &&
        !SC64_TRANSPORT_LOADED(&u[i]) && RACE_PROTOSS_RAW(&u[i])==SC64_OVERLORD_RAW) {
        int source=RACE_PROTOSS_SLOT(&u[i]);
        int ol,ot,or_,ob,distance=-1;
        if(sc64_p7_actor_rect_source(SC64_OVERLORD_RAW,u[i].x,u[i].y,0,
                                     &ol,&ot,&or_,&ob))
            distance=sc64_p7_observer_rect_distance_rects(
                ol,ot,or_,ob,target_l,target_t,target_r,target_b);
        if(distance<0||distance>SC64_OVERLORD_DETECTION_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_protoss_source_owner((unsigned)source)==u[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
    }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive &&
        RACE_PROTOSS_RAW(&b[i])==SC64_SPORE_COLONY_RAW && building_complete(i)) {
        int source=RACE_PROTOSS_SLOT(&b[i]);
        int sl,st,sr,sb,distance=-1;
        if(sc64_p7_actor_rect_source(SC64_SPORE_COLONY_RAW,b[i].x,b[i].y,1,
                                     &sl,&st,&sr,&sb))
            distance=sc64_p7_observer_rect_distance_rects(
                sl,st,sr,sb,target_l,target_t,target_r,target_b);
        if(distance<0||distance>SC64_SPORE_DETECTION_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_protoss_source_owner((unsigned)source)==b[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
    }
#endif
    return detected&255u;
}

static void sc64_p7_arbiter_cache_source_tick(void)
{
    unsigned i,j;
    u8 powered_cannons[MAX_BUILDINGS];
    int powered_cannons_ready=0;
    unsigned global=(unsigned)RACE_PROTOSS_SERVICE_CLOCK[1] |
                    ((unsigned)RACE_PROTOSS_SERVICE_CLOCK[2]<<8);
    int stagger_reset=0;
    unsigned visible_ordinal=0;
    /* OpenBW decrements the global secondary-order timer and assigns timer
       offsets to every visible unit before movement, sprite detection, and
       unit orders. The GBA approximation seeds the source's 150-frame first
       interval, then applies each 300-source-frame reset before target work. */
    if (!global && !sc64_p7_arbiter_cache_initialized) global=150;
    if (global) --global;
    if (!global) { global=300; stagger_reset=1; }
    if (stagger_reset) for (i=0;i<MAX_UNITS;++i)
        if (u[i].alive && RACE_PROTOSS_RAW(&u[i])!=101u &&
            !SC64_TRANSPORT_LOADED(&u[i])) {
            unsigned old=(unsigned)u[i].weapon;
            if (!SC64_P7_ARBITER_CACHE_VALID(old)) old=sc64_p7_arbiter_pack(0,0,0,0);
            u[i].weapon=(u16)sc64_p7_arbiter_pack(
                SC64_P7_ARBITER_DETECTED_MASK(old),visible_ordinal%30u,
                (old&SC64_P7_ARBITER_AURA)!=0,
                (old&SC64_P7_ARBITER_TRACKED)!=0);
            ++visible_ordinal;
        }
    u8 aura[MAX_UNITS];
    for (i=0;i<MAX_UNITS;++i) aura[i]=0;
    for (i=0;i<MAX_UNITS;++i) if (u[i].alive && RACE_PROTOSS_RAW(&u[i])==71u &&
        RACE_PROTOSS_SLOT(&u[i])>=0 && RACE_PROTOSS_SLOT(&u[i])<8 && !(u[i].flags&8u) &&
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        race_protoss_source_owner((unsigned)RACE_PROTOSS_SLOT(&u[i]))==u[i].owner) {
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        int al,at,ar,ab;
        if(!sc64_p7_actor_rect_source(71u,u[i].x,u[i].y,0,
                                      &al,&at,&ar,&ab)) continue;
#endif
        if (sc64_target_index_active) {
            /* Target index cells are 64 GBA pixels. 160 source pixels is
             * 80 GBA pixels; 32 additional GBA pixels cover the maximum
             * authenticated P7 unit half-extent before exact broadphase. */
            int x0=(u[i].x-112)/64, x1=(u[i].x+112)/64;
            int y0=(u[i].y-112)/64, y1=(u[i].y+112)/64;
            int cols=(WORLD_W+63)/64, rows=(WORLD_H+63)/64;
            if (x0<0)x0=0; if(y0<0)y0=0;
            if(x1>=cols)x1=cols-1; if(y1>=rows)y1=rows-1;
            for (int cy=y0;cy<=y1;++cy) for(int cx=x0;cx<=x1;++cx)
                for(int owner=0;owner<5;++owner) {
                    int cell=cx+cy*SC64_TARGET_GRID_COLS;
                    Sc64TargetIndex ent=sc64_target_head[owner][cell];
                    while(ent!=SC64_TARGET_NONE) {
                        int slot=(int)ent;
                        if(slot<MAX_UNITS && slot!=(int)i &&
#ifdef SC64_P7_ARBITER_AURA_ONCE
                           !aura[slot] &&
#endif
                           sc64_p7_observer_arbiter_aura_candidate(slot) &&
                           sc64_p7_arbiter_same_source((int)i,slot) &&
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
                           1) {
                            int tl,tt,tr,tb;
                            if(sc64_p7_actor_rect_source((unsigned)RACE_PROTOSS_RAW(&u[slot]),
                                    u[slot].x,u[slot].y,0,&tl,&tt,&tr,&tb) &&
                               sc64_p7_arbiter_finder_square_rect((int)i,
                                    tl,tt,tr,tb)) {
                                int d=sc64_p7_observer_rect_distance_rects(
                                    al,at,ar,ab,tl,tt,tr,tb);
                                if(d>=0&&d<=160)aura[slot]=1;
                            }
                        }
#else
                           sc64_p7_arbiter_finder_square((int)i,slot)) {
                            int d=sc64_p7_observer_rect_distance(71u,u[i].x,u[i].y,0,
                                (unsigned)RACE_PROTOSS_RAW(&u[slot]),u[slot].x,u[slot].y,0);
                            if(d>=0&&d<=160)aura[slot]=1;
                        }
#endif
                        ent=sc64_target_next[slot];
                    }
                }
        } else {
            for (j=0;j<MAX_UNITS;++j) if (j!=i &&
#ifdef SC64_P7_ARBITER_AURA_ONCE
                !aura[j] &&
#endif
                sc64_p7_observer_arbiter_aura_candidate((int)j) &&
                sc64_p7_arbiter_same_source((int)i,(int)j) &&
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
                1) {
                int tl,tt,tr,tb;
                if(sc64_p7_actor_rect_source((unsigned)RACE_PROTOSS_RAW(&u[j]),
                        u[j].x,u[j].y,0,&tl,&tt,&tr,&tb) &&
                   sc64_p7_arbiter_finder_square_rect((int)i,tl,tt,tr,tb)) {
                    int d=sc64_p7_observer_rect_distance_rects(
                        al,at,ar,ab,tl,tt,tr,tb);
                    if(d>=0&&d<=160)aura[j]=1;
                }
            }
#else
                sc64_p7_arbiter_finder_square((int)i,(int)j)) {
                int d=sc64_p7_observer_rect_distance(71u,u[i].x,u[i].y,0,
                    (unsigned)RACE_PROTOSS_RAW(&u[j]),u[j].x,u[j].y,0);
                if (d>=0 && d<=160) aura[j]=1;
            }
#endif
        }
    }
    for (i=0;i<MAX_UNITS;++i) {
        unsigned old=(unsigned)u[i].weapon;
        unsigned mask=0,timer=0;
        if (!u[i].alive) {
            /* The old value is already the exact dead-row result. Zero rows
             * still need the complete representation check below. */
            if (old != sc64_p7_arbiter_pack(0,0,0,0) &&
                !sc64_p7_arbiter_cache_empty_slot(i))
                u[i].weapon=(u16)sc64_p7_arbiter_pack(0,0,0,0);
            continue;
        }
        int tracked=aura[i] || sc64_p7_observer_is_cloaked((unsigned)RACE_PROTOSS_RAW(&u[i]));
        int was_tracked=SC64_P7_ARBITER_CACHE_VALID(old) &&
                        (old&SC64_P7_ARBITER_TRACKED)!=0;
        if (!SC64_P7_ARBITER_CACHE_VALID(old)) {
            /* Fail closed on malformed in-memory rows; spawn initializes it. */
            u[i].weapon=(u16)sc64_p7_arbiter_pack(0,0,0,0);
            old=(unsigned)u[i].weapon;
        }
        timer=SC64_P7_ARBITER_COUNTDOWN(old);
        if (tracked) {
            if (timer==0 || (tracked && !was_tracked)) {
                /* Building and owner state are unchanged within this tick.
                 * Compute the shared snapshot only if a refresh needs it. */
                if (!powered_cannons_ready) {
                    memset(powered_cannons,0,sizeof(powered_cannons));
                    for(j=0;j<MAX_BUILDINGS;++j) if(b[j].alive &&
                        RACE_PROTOSS_RAW(&b[j])==SC64_P7_CANNON_RAW && building_complete((int)j) &&
                        RACE_PROTOSS_SLOT(&b[j])>=0 && RACE_PROTOSS_SLOT(&b[j])<8) {
                        int live=race_protoss_source_owner((unsigned)RACE_PROTOSS_SLOT(&b[j]));
                        if(live==b[j].owner)
                            powered_cannons[j]=(u8)sc64_p7_observer_pylon_powers((int)j,
                                RACE_PROTOSS_SLOT(&b[j]),live);
                    }
                    powered_cannons_ready=1;
                }
                mask=sc64_p7_arbiter_detected_mask((int)i,powered_cannons);
                timer=30;
            } else {
                mask=SC64_P7_ARBITER_DETECTED_MASK(old);
                --timer;
            }
        } else {
            /* Native reset assigns a countdown even to currently uncloaked
               visible units; retain it in case cloak begins before next reset. */
            mask=0;
        }
        u[i].weapon=(u16)sc64_p7_arbiter_pack(mask,timer,(int)aura[i],tracked);
    }
    RACE_PROTOSS_SERVICE_CLOCK[1]=(u8)global;
    RACE_PROTOSS_SERVICE_CLOCK[2]=(u8)(global>>8);
}

static void sc64_p7_arbiter_cache_frame_tick(void)
{
    unsigned phase=(unsigned)RACE_PROTOSS_SERVICE_CLOCK[0]+4u;
    if (phase>=5u) { phase-=5u; sc64_p7_arbiter_cache_due=1; }
    RACE_PROTOSS_SERVICE_CLOCK[0]=(u8)phase;
}

#ifndef GBA_HOST
static void __attribute__((noinline,long_call))
#else
static void
#endif
sc64_p7_arbiter_cache_process_due(void)
{
    /* The first ordinary play tick runs after map setup and target-index
       construction, before this tick's unit orders. Menu launch itself is
       between simulation ticks; this is not a launch-time native refresh. */
    if (!sc64_p7_arbiter_cache_initialized) {
        sc64_p7_arbiter_cache_due=0;
        sc64_p7_arbiter_cache_source_tick();
        sc64_p7_arbiter_cache_initialized=1;
        return;
    }
    if (!sc64_p7_arbiter_cache_due) return;
    sc64_p7_arbiter_cache_due=0;
    sc64_p7_arbiter_cache_source_tick();
}

static int sc64_p7_arbiter_cache_unit_valid_with_mask(const Unit *actor,
                                                      unsigned allowed_sources)
{
    unsigned w,mask;
    if (!actor) return 0;
    if (sc64_p7_arbiter_cache_empty_unit(actor)) return 1;
    w=(unsigned)actor->weapon;
    if (!SC64_P7_ARBITER_CACHE_VALID(w)) return 0;
    mask=SC64_P7_ARBITER_DETECTED_MASK(w);
    if (mask & ~allowed_sources) return 0;
    if (actor->alive && !(w&SC64_P7_ARBITER_TRACKED) && mask) return 0;
    if (actor->alive && (w&SC64_P7_ARBITER_AURA) &&
        !(w&SC64_P7_ARBITER_TRACKED)) return 0;
    if (actor->alive && (w&SC64_P7_ARBITER_TRACKED) &&
        !(w&SC64_P7_ARBITER_AURA) &&
        !sc64_p7_observer_is_cloaked((unsigned)RACE_PROTOSS_RAW(actor))) return 0;
    return 1;
}

static unsigned sc64_p7_valid_source_mask(void)
{
    unsigned mask=0;
    for (unsigned owner=0;owner<8;++owner)
        if (race_protoss_source_owner(owner)>=0) mask|=1u<<owner;
    return mask;
}


static int sc64_p7_arbiter_cache_unit_valid(const Unit *actor)
{
    return sc64_p7_arbiter_cache_unit_valid_with_mask(
        actor, sc64_p7_valid_source_mask());
}

static int sc64_p7_arbiter_cache_world_valid(void)
{
    unsigned allowed_sources=sc64_p7_valid_source_mask();
    for (int i=0;i<MAX_UNITS;++i)
        if (!sc64_p7_arbiter_cache_unit_valid_with_mask(&u[i],
                                                        allowed_sources)) return 0;
    return 1;
}
#endif /* SC64_P7_ARBITER_CLOAK_CACHE_ENABLED */

#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
static int sc64_p7_observer_target_is_cloaked(int target)
{
    int raw=sc64_p7_observer_target_raw_type(target);
    if (raw < 0) return 0;
    return sc64_p7_observer_is_cloaked((unsigned)raw) ||
           sc64_p7_observer_arbiter_aura_cloaks(target);
}
#endif

static int sc64_p7_observer_detector_sees(int target, int source_owner,
                                         int live_owner)
{
    int target_raw = sc64_p7_observer_target_raw_type(target);
    source_owner = race_protoss_query_slot(source_owner, live_owner);
    if (target_raw < 0 || !sc64_p7_observer_target_optically_visible(
            target, source_owner, live_owner)) return 0;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    /* Native detected_flags are target-owned and expire on source cadence;
       optical visibility is live and must be checked independently on every
       combat/render/minimap query. */
    if (!race_protoss_services_active() || target >= MAX_UNITS ||
        !SC64_P7_ARBITER_CACHE_VALID(u[target].weapon) ||
        source_owner < 0 || source_owner >= 8 ||
        race_protoss_source_owner((unsigned)source_owner) != live_owner)
        return 0;
    return (SC64_P7_ARBITER_DETECTED_MASK(u[target].weapon) &
            (1u << (unsigned)source_owner)) != 0;
#else
    unsigned raw_target = (unsigned)target_raw;
    int target_x = target < MAX_UNITS ? u[target].x : b[target-MAX_UNITS].x;
    int target_y = target < MAX_UNITS ? u[target].y : b[target-MAX_UNITS].y;
    int target_building = target >= MAX_UNITS;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        RACE_PROTOSS_RAW(&u[i]) == SC64_P7_OBSERVER_RAW &&
        sc64_p7_observer_source_matches(RACE_PROTOSS_SLOT(&u[i]), u[i].owner,
                                        source_owner, live_owner)) {
        Sc64P7DetectorState state = {1, 1, 0, 1, 0};
        int range = sc64_p7_detector_range_source_pixels(
            (unsigned)RACE_PROTOSS_RAW(&u[i]), 1, 0);
        int distance = sc64_p7_observer_rect_distance((unsigned)RACE_PROTOSS_RAW(&u[i]),
            u[i].x, u[i].y, 0, raw_target, target_x, target_y,
            target_building);
        if (sc64_p7_detector_usable(&state, 0) && distance >= 0 &&
            distance <= range) return 1;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        RACE_PROTOSS_RAW(&b[i]) == SC64_P7_CANNON_RAW && building_complete(i) &&
        sc64_p7_observer_pylon_powers(i, source_owner, live_owner) &&
        sc64_p7_observer_source_matches(RACE_PROTOSS_SLOT(&b[i]), b[i].owner,
                                        source_owner, live_owner)) {
        Sc64P7DetectorState state = {1, 1, 0, 1, 0};
        int range = sc64_p7_detector_range_source_pixels(
            (unsigned)RACE_PROTOSS_RAW(&b[i]), 1, 0);
        int distance = sc64_p7_observer_rect_distance((unsigned)RACE_PROTOSS_RAW(&b[i]),
            b[i].x, b[i].y, 1, raw_target, target_x, target_y,
            target_building);
        if (sc64_p7_detector_usable(&state, 1) && distance >= 0 &&
            distance <= range) return 1;
    }
    return 0;
#endif
}

static int sc64_p7_observer_can_attack_from(int attacker, int target,
                                            int live_owner,
                                            int source_owner)
{
    if (!SC64_CAN_ATTACK_FROM(attacker, target, live_owner)) return 0;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (!race_protoss_services_active() || !sc64_p7_observer_target_is_cloaked(target)) return 1;
    return sc64_p7_observer_detector_sees(target,source_owner,live_owner);
#else
    if (!race_protoss_services_active() || source_owner >= 8 ||
        !sc64_p7_observer_is_cloaked(
            (unsigned)sc64_p7_observer_target_raw_type(target))) return 1;
    return sc64_p7_observer_target_allowed(
        (unsigned)sc64_p7_observer_target_raw_type(target),
        sc64_p7_observer_target_optically_visible(target, source_owner,
                                                   live_owner),
        sc64_p7_observer_detector_sees(target, source_owner, live_owner));
#endif
}

/* Shared by main-view and minimap rendering. Both surfaces use this same
 * target-owned detection cache and live optical-visibility check. */
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
static int sc64_p7_observer_target_revealed_to(int target,int source_owner,
                                               int live_owner)
{
    return !race_protoss_services_active() ||
        !sc64_p7_observer_target_is_cloaked(target) ||
        sc64_p7_observer_detector_sees(target,source_owner,live_owner);
}
#endif

#endif /* SC64_P7_OBSERVER_ENABLED */
#endif /* SC64_P7_OBSERVER_ADAPTER_H */
