/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_TYPED_UNIT_SAVE_H

#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
#define SC64_TYPED_WIRE_CANONICAL sc64_p7_typed_unit_canonical_for
#define SC64_TYPED_WIRE_RAW_CANONICAL sc64_p7_typed_unit_canonical_raw_for
#define SC64_TYPED_WIRE_VALIDATE sc64_p7_typed_unit_fields_valid_for
#define SC64_TYPED_WIRE_ENCODE sc64_p7_typed_unit_encode_for
#define SC64_TYPED_WIRE_DECODE sc64_p7_typed_unit_decode_for
#define SC64_TYPED_WIRE_PARAM , int egg_stage
#define SC64_TYPED_WIRE_ARG , egg_stage
#else
#define SC64_TYPED_WIRE_CANONICAL sc64_p7_typed_unit_canonical
#define SC64_TYPED_WIRE_RAW_CANONICAL sc64_p7_typed_unit_canonical_raw
#define SC64_TYPED_WIRE_VALIDATE sc64_p7_typed_unit_fields_valid
#define SC64_TYPED_WIRE_ENCODE sc64_p7_typed_unit_encode
#define SC64_TYPED_WIRE_DECODE sc64_p7_typed_unit_decode
#define SC64_TYPED_WIRE_PARAM
#define SC64_TYPED_WIRE_ARG
#endif
#define SC64_P7_TYPED_UNIT_SAVE_H
#ifndef SC_RU_LEGACY_BOUND
#define SC_RU_LEGACY_BOUND SC_RU_COUNT
#endif

/* Generation-40 P7 row.  This codec is deliberately separate from both the
 * generation-22 ZP row and the Z7 derived row: the 16-bit weapon field remains
 * opaque so P7 cloak/cache state can survive a save exactly. */
static inline int sc64_zp_actor_default_hp(int type);
static inline int sc64_zp_actor_default_shields(int type);

#define SC64_P7_TYPED_UNIT_SAVE_BYTES 28
/* Canonical shield bound. The row's shield field is 9 bits; P10's Gantrithor
 * (500 shields) is the only actor above Zeratul's 400. */
#ifdef SC64_P10_DATA_ENABLED
#define SC64_P7_TYPED_UNIT_MAX_SHIELDS 511
#else
#define SC64_P7_TYPED_UNIT_MAX_SHIELDS 400
#endif
typedef struct { u8 bytes[SC64_P7_TYPED_UNIT_SAVE_BYTES]; }
    Sc64P7TypedUnitSave;
_Static_assert(sizeof(Sc64P7TypedUnitSave) == 28,
               "P7 typed unit row budget");

static int sc64_p7_typed_unit_put(Sc64P7TypedUnitSave *r, unsigned *bit,
                                  unsigned width, u32 value)
{
    if (!r || !bit || !width || width > 32 || *bit > 224 ||
        width > 224 - *bit ||
        (width < 32 && value >= ((u32)1u << width))) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        if ((value >> i) & 1u)
            r->bytes[*bit >> 3] |= (u8)(1u << (*bit & 7u));
    return 1;
}

static int sc64_p7_typed_unit_get(const Sc64P7TypedUnitSave *r,
                                  unsigned *bit, unsigned width, u32 *value)
{
    u32 v = 0;
    if (!r || !bit || !value || !width || width > 32 || *bit > 224 ||
        width > 224 - *bit) return 0;
    for (unsigned i = 0; i < width; ++i, ++*bit)
        v |= (u32)((r->bytes[*bit >> 3] >> (*bit & 7u)) & 1u) << i;
    *value = v;
    return 1;
}

static int sc64_p7_typed_unit_zero(const Sc64P7TypedUnitSave *r)
{
    if (!r) return 0;
    for (unsigned i = 0; i < sizeof(r->bytes); ++i)
        if (r->bytes[i]) return 0;
    return 1;
}

static int SC64_TYPED_WIRE_CANONICAL(int type, int *maxhp, int *armor,
                                        int *maxshield SC64_TYPED_WIRE_PARAM)
{
    const Sc64RaceUnitRule *rule;
    int hp, shields;
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
    (void)egg_stage;
#endif
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
    if ((egg_stage & 1) && type==SC64_SHARED_EGG_TYPE && type>=0 && type<64) {
        if (maxhp) *maxhp=200;
        if (armor) *armor=10;
        if (maxshield) *maxshield=0;
        return 1;
    }
#ifdef SC64_SCARAB_STABLE_ROSTER_HOST_STAGE
#ifdef SC64_GUARDIAN_DISTINCT_STAGE
    if ((egg_stage & 2) && type==U_GUARDIAN_STAGE && type<64) {
        if (maxhp) *maxhp=150;
        if (armor) *armor=2;
        if (maxshield) *maxshield=0;
        return 1;
    }
#endif
    if ((egg_stage & 4) && type==U_SCARAB && type<64) {
        if (maxhp) *maxhp=20;
        if (armor) *armor=0;
#ifdef SC64_SHARED_ACTOR_FULL_HOST_STAGE
        /* Feature-15 typed Scarab wire rows temporarily use shield bits as
         * waypoint digits; the candidate bridge restores real shields before
         * graph validation or publication. */
        if (maxshield) *maxshield=(egg_stage==15 ? 511 : 10);
#else
        if (maxshield) *maxshield=10;
#endif
        return 1;
    }
#endif
#endif
    if (type < 0 || type >= SC_RU_LEGACY_BOUND || type >= 64) return 0;
    rule = sc64_race_unit_rule(type);
    hp = sc64_zp_actor_default_hp(type);
    if (!hp && rule) hp = rule->hp;
    shields = sc64_zp_actor_default_shields(type);
    if (!shields && rule) shields = rule->shields;
    if (hp <= 0 || hp > 131071 || shields < 0 ||
        shields > SC64_P7_TYPED_UNIT_MAX_SHIELDS) return 0;
    if (maxhp) *maxhp = hp;
    if (armor) *armor = sc64_race_unit_armor(type);
    if (maxshield) *maxshield = shields;
    return 1;
}

/* Cocoon shares the old Overlord wire type but has its own source facts. */
static int SC64_TYPED_WIRE_RAW_CANONICAL(int type, int raw, int *maxhp,
                                         int *armor, int *maxshield SC64_TYPED_WIRE_PARAM)
{
#ifdef SC64_SHARED_ACTOR_FULL_HOST_STAGE
    if ((egg_stage & 2) && type == U_OVERLORD && raw == 59) {
        if (maxhp) *maxhp = 200;
        if (armor) *armor = 0;
        if (maxshield) *maxshield = 0;
        return 1;
    }
#else
    (void)raw;
#endif
    return SC64_TYPED_WIRE_CANONICAL(type,maxhp,armor,maxshield SC64_TYPED_WIRE_ARG);
}

/* This representation is valid only for the authenticated, unpublished
 * typed feature-15 generation. Dead rows retain the original signed HP wire
 * format and the original 8-bit cooldown restriction. */
#if defined(SC64_SHARED_ACTOR_FULL_HOST_STAGE) && defined(SC64_SHARED_EGG_WIRE_HOST_STAGE)
#define SC64_TYPED_LIVE_SCARAB(q) \
    (egg_stage==15 && (q)->type==U_SCARAB && (q)->raw_type==85 && (q)->alive==1)
#else
#define SC64_TYPED_LIVE_SCARAB(q) 0
#endif

static int SC64_TYPED_WIRE_VALIDATE(const Unit *q,
                                           unsigned world_width,
                                           unsigned world_height SC64_TYPED_WIRE_PARAM)
{
    int maxhp, armor, maxshield;
    if (!q || !world_width || !world_height || world_width > 2048 ||
        world_height > 2048 || q->x < 0 || (unsigned)q->x >= world_width ||
        q->y < 0 || (unsigned)q->y >= world_height ||
        q->hp < -131072 || q->hp >= 131072 || (q->alive && q->hp < 0) ||
        q->owner < 0 || q->owner > 4 || q->type < 0 ||
        !SC64_TYPED_WIRE_RAW_CANONICAL(q->type, q->raw_type, &maxhp, &armor, &maxshield SC64_TYPED_WIRE_ARG) ||
#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
        ((egg_stage & 1) && q->type==SC64_SHARED_EGG_TYPE && (q->raw_type!=36 || q->alive!=1)) ||
#ifdef SC64_GUARDIAN_DISTINCT_STAGE
        ((egg_stage & 2) && q->type==U_GUARDIAN_STAGE && q->raw_type!=44) ||
#endif
#ifdef SC64_SCARAB_STABLE_ROSTER_HOST_STAGE
#ifdef SC64_SHARED_ACTOR_FULL_HOST_STAGE
        ((egg_stage & 4) && q->type==U_SCARAB && (q->raw_type!=85 || (!q->alive && q->hp>0))) ||
#else
        ((egg_stage & 4) && q->type==U_SCARAB && (q->raw_type!=85 || q->alive!=1)) ||
#endif
#endif
#endif
        q->maxhp != maxhp || q->armor != armor ||
        (q->alive != 0 && q->alive != 1) ||
        (q->selected != 0 && q->selected != 1) || q->cd < 0 ||
        q->cd > (SC64_TYPED_LIVE_SCARAB(q) ? 65535 : 255) ||
        (SC64_TYPED_LIVE_SCARAB(q) && (q->hp < 1 || q->hp > 20)) ||
        q->order < 0 || q->order > 15 || q->tx < 0 || q->tx > 2047 ||
        q->ty < 0 || q->ty > 2047 ||
        q->carry < -65536 || q->carry >= 65536 || q->build < -2 ||
        q->build > 239 || q->work_timer < 0 || q->work_timer > 2047 ||
        q->weapon < 0 || q->weapon > 65535 || q->target < -1 ||
        q->target >= MAX_UNITS + MAX_BUILDINGS || q->path_timer < 0 ||
        q->path_timer > 24 || q->path_retry < 0 || q->path_retry > 12 ||
        q->path_x < 0 || q->path_x > 2047 || q->path_y < 0 ||
        q->path_y > 2047 || q->shield < 0 || q->shield > maxshield ||
        q->raw_type < -1 || q->raw_type > 254 || q->raw_owner < -1 ||
        q->raw_owner > 14 || q->flags < 0 || q->flags > 0x3fff ||
        q->shield_remainder < 0 || q->shield_remainder > 1279) return 0;
    return 1;
}

static int SC64_TYPED_WIRE_ENCODE(Sc64P7TypedUnitSave *row,
                                     const Unit *q, unsigned ww,
                                     unsigned wh SC64_TYPED_WIRE_PARAM)
{
    Sc64P7TypedUnitSave out;
    unsigned bit = 0;
    u32 hp_wire, cd_wire;
    if (!row || !q || !ww || !wh || ww > 2048 || wh > 2048) return 0;
    memset(&out, 0, sizeof(out));
    static const Unit zero = {0};
    if (memcmp(q, &zero, sizeof(*q)) == 0) { *row = out; return 1; }
    if (!SC64_TYPED_WIRE_VALIDATE(q, ww, wh SC64_TYPED_WIRE_ARG)) return 0;
    hp_wire = SC64_TYPED_LIVE_SCARAB(q) ?
        (((u32)q->cd >> 8) << 5) | (u32)q->hp : (u32)(q->hp + 131072);
    cd_wire = (u32)q->cd & 255u;
#define PUT(w, v) do { if (!sc64_p7_typed_unit_put(&out, &bit, (w), (u32)(v))) return 0; } while (0)
    PUT(11, q->x); PUT(11, q->y); PUT(18, hp_wire);
    PUT(3, q->owner); PUT(6, q->type); PUT(1, q->alive); PUT(1, q->selected);
    PUT(8, cd_wire); PUT(4, q->order); PUT(11, q->tx); PUT(11, q->ty);
    PUT(17, q->carry + 65536); PUT(8, q->build + 2); PUT(11, q->work_timer);
    PUT(16, q->weapon); PUT(9, q->target + 1); PUT(5, q->path_timer);
    PUT(4, q->path_retry); PUT(11, q->path_x); PUT(11, q->path_y);
    PUT(9, q->shield); PUT(8, q->raw_type + 1); PUT(4, q->raw_owner + 1);
    PUT(14, q->flags); PUT(11, q->shield_remainder); PUT(1, 1);
#undef PUT
    if (bit != 224) return 0;
    *row = out;
    return 1;
}

static int SC64_TYPED_WIRE_DECODE(Unit *out,
                                     const Sc64P7TypedUnitSave *row,
                                     unsigned ww, unsigned wh SC64_TYPED_WIRE_PARAM)
{
    Unit q;
    unsigned bit = 0;
    u32 v, hp_wire;
    int maxhp, armor, maxshield;
    if (!out || !row || !ww || !wh || ww > 2048 || wh > 2048) return 0;
    memset(&q, 0, sizeof(q));
    if (sc64_p7_typed_unit_zero(row)) { *out = q; return 1; }
#define GET(w, dst) do { if (!sc64_p7_typed_unit_get(row, &bit, (w), &v)) return 0; (dst) = (int)v; } while (0)
    GET(11, q.x); GET(11, q.y);
    if (!sc64_p7_typed_unit_get(row, &bit, 18, &v)) return 0;
    hp_wire = v;
    GET(3, q.owner); GET(6, q.type); GET(1, q.alive); GET(1, q.selected);
    GET(8, q.cd); GET(4, q.order); GET(11, q.tx); GET(11, q.ty);
    if (!sc64_p7_typed_unit_get(row, &bit, 17, &v)) return 0;
    q.carry = (int)v - 65536;
    GET(8, v); q.build = (int)v - 2; GET(11, q.work_timer);
    GET(16, q.weapon); GET(9, v); q.target = (int)v - 1;
    GET(5, q.path_timer); GET(4, q.path_retry); GET(11, q.path_x);
    GET(11, q.path_y); GET(9, q.shield); GET(8, v); q.raw_type = (int)v - 1;
    GET(4, v); q.raw_owner = (int)v - 1; GET(14, q.flags);
    GET(11, q.shield_remainder);
    if (SC64_TYPED_LIVE_SCARAB(&q)) {
        if ((hp_wire >> 13) || !(hp_wire & 31u) || (hp_wire & 31u) > 20u) return 0;
        q.hp = (int)(hp_wire & 31u);
        q.cd |= (int)((hp_wire >> 5) << 8);
    } else q.hp = (int)hp_wire - 131072;
    if (!sc64_p7_typed_unit_get(row, &bit, 1, &v) || v != 1 || bit != 224 ||
        !SC64_TYPED_WIRE_RAW_CANONICAL(q.type, q.raw_type, &maxhp, &armor, &maxshield SC64_TYPED_WIRE_ARG)) return 0;
    q.maxhp = maxhp; q.armor = armor;
    if (!SC64_TYPED_WIRE_VALIDATE(&q, ww, wh SC64_TYPED_WIRE_ARG)) return 0;
    *out = q;
    return 1;
#undef GET
}

#ifdef SC64_SHARED_EGG_WIRE_HOST_STAGE
static int sc64_p7_typed_unit_canonical(int t,int *hp,int *a,int *sh)
{ return sc64_p7_typed_unit_canonical_for(t,hp,a,sh,0); }
static int sc64_p7_typed_unit_fields_valid(const Unit *q,unsigned w,unsigned h)
{ return sc64_p7_typed_unit_fields_valid_for(q,w,h,0); }
static int sc64_p7_typed_unit_encode(Sc64P7TypedUnitSave *r,const Unit *q,unsigned w,unsigned h)
{ return sc64_p7_typed_unit_encode_for(r,q,w,h,0); }
static int sc64_p7_typed_unit_decode(Unit *q,const Sc64P7TypedUnitSave *r,unsigned w,unsigned h)
{ return sc64_p7_typed_unit_decode_for(q,r,w,h,0); }

#endif /* SC64_SHARED_EGG_WIRE_HOST_STAGE */
#undef SC64_TYPED_LIVE_SCARAB
#undef SC64_TYPED_WIRE_CANONICAL
#undef SC64_TYPED_WIRE_RAW_CANONICAL
#undef SC64_TYPED_WIRE_VALIDATE
#undef SC64_TYPED_WIRE_ENCODE
#undef SC64_TYPED_WIRE_DECODE
#undef SC64_TYPED_WIRE_PARAM
#undef SC64_TYPED_WIRE_ARG

#endif
