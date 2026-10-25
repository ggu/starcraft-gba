/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_TYPED_BUILDING_SAVE_H
#define SC64_P7_TYPED_BUILDING_SAVE_H

/* Generation-40 P7 building row. Canonical type determines maxhp; source
 * owner/type remain explicit so the imported scenario roster is verifiable. */
static inline int sc64_zp_building_default_hp(int type);

#define SC64_P7_TYPED_BUILDING_SAVE_BYTES 25
typedef struct { u8 bytes[SC64_P7_TYPED_BUILDING_SAVE_BYTES]; }
    Sc64P7TypedBuildingSave;
_Static_assert(sizeof(Sc64P7TypedBuildingSave) == 25,
               "P7 typed building row budget");

static int sc64_p7_typed_building_put(Sc64P7TypedBuildingSave *r,
                                      unsigned *bit, unsigned width,
                                      u32 value)
{
    if (!r || !bit || !width || width > 32 || *bit > 200 ||
        width > 200 - *bit ||
        (width < 32 && value >= ((u32)1u << width))) return 0;
    for (unsigned i=0; i<width; ++i, ++*bit)
        if ((value >> i) & 1u)
            r->bytes[*bit>>3] |= (u8)(1u << (*bit&7u));
    return 1;
}

static int sc64_p7_typed_building_get(const Sc64P7TypedBuildingSave *r,
                                      unsigned *bit, unsigned width,
                                      u32 *value)
{
    u32 v=0;
    if (!r || !bit || !value || !width || width>32 || *bit>200 ||
        width>200-*bit) return 0;
    for (unsigned i=0; i<width; ++i, ++*bit)
        v |= (u32)((r->bytes[*bit>>3] >> (*bit&7u))&1u) << i;
    *value=v;
    return 1;
}

static int sc64_p7_typed_building_zero(const Sc64P7TypedBuildingSave *r)
{
    if (!r) return 0;
    for (unsigned i=0; i<sizeof(r->bytes); ++i) if (r->bytes[i]) return 0;
    return 1;
}

static int sc64_p7_typed_building_maxhp(int type)
{
    int hp=sc64_zp_building_default_hp(type);
    if (!hp) {
        const Sc64RaceBuildingRule *rule=sc64_race_building_rule(type);
        if (rule) hp=rule->hp;
    }
#ifdef SC64_T4_RUNTIME
    if (type==B_TERRAN_BEACON || type==B_UPPER_DOOR ||
        type==B_RIGHT_UPPER_DOOR || type==B_PIT_DOOR ||
        type==B_RIGHT_PIT_DOOR) hp=100000;
#endif
    return hp;
}

static int sc64_p7_typed_building_fields_valid(const Building *q,
                                                unsigned ww,unsigned wh)
{
    int maxhp;
    if (!q || !ww || !wh || ww>2048 || wh>2048 || q->x<0 ||
        (unsigned)q->x>=ww || q->y<0 || (unsigned)q->y>=wh ||
        q->hp < -131072 || q->hp >= 131072 || (q->alive && q->hp<0) ||
        q->owner<0 || q->owner>4 || q->type<0 || q->type>=SC_RB_COUNT ||
        (q->alive!=0 && q->alive!=1) || q->queue_head<0 || q->queue_head>4 ||
        q->queue_count<0 || q->queue_count>5 || q->queue_progress<0 ||
        q->queue_progress>65535 || q->build_progress < -1 ||
        q->build_progress>65534 || (q->under_construction!=0 &&
        q->under_construction!=1) || q->level<0 || q->level>127 ||
        q->shield<0 || q->shield>131071 || q->raw_type < -1 ||
        q->raw_type>254 || q->raw_owner < -1 || q->raw_owner>14 ||
        q->flags<0 || q->flags>65535 || q->larva<0 || q->larva>3 ||
        q->larva_timer<0 || q->larva_timer>511 ||
        q->shield_remainder<0 || q->shield_remainder>1279 ||
        SC_RU_COUNT>64) return 0;
    maxhp=sc64_p7_typed_building_maxhp(q->type);
    if (maxhp<=0 || maxhp>131071 || q->maxhp!=maxhp) return 0;
    for (unsigned i=0;i<5;++i)
        if (q->queue[i]<0 || q->queue[i]>=SC_RU_COUNT) return 0;
    return 1;
}

static int sc64_p7_typed_building_encode(Sc64P7TypedBuildingSave *row,
                                         const Building *q,unsigned ww,
                                         unsigned wh)
{
    Sc64P7TypedBuildingSave out;
    unsigned bit=0;
    if (!row || !q || !ww || !wh || ww>2048 || wh>2048) return 0;
    memset(&out,0,sizeof(out));
    static const Building zero={0};
    if (memcmp(q,&zero,sizeof(*q))==0) { *row=out; return 1; }
    if (!sc64_p7_typed_building_fields_valid(q,ww,wh)) return 0;
#define PUT(w,v) do { if(!sc64_p7_typed_building_put(&out,&bit,(w),(u32)(v))) return 0; } while(0)
    PUT(11,q->x); PUT(11,q->y); PUT(18,q->hp+131072);
    PUT(3,q->owner); PUT(6,q->type); PUT(1,q->alive);
    for(unsigned i=0;i<5;++i) PUT(6,q->queue[i]);
    PUT(3,q->queue_head); PUT(3,q->queue_count); PUT(16,q->queue_progress);
    PUT(16,q->build_progress+1); PUT(1,q->under_construction);
    PUT(7,q->level); PUT(17,q->shield); PUT(8,q->raw_type+1);
    PUT(4,q->raw_owner+1); PUT(16,q->flags); PUT(2,q->larva);
    PUT(9,q->larva_timer); PUT(11,q->shield_remainder); PUT(1,1);
#undef PUT
    if (bit!=194) return 0;
    *row=out;
    return 1;
}

static int sc64_p7_typed_building_decode(Building *out,
                                         const Sc64P7TypedBuildingSave *row,
                                         unsigned ww,unsigned wh)
{
    Building q;
    unsigned bit=0;
    u32 v;
    if (!out || !row || !ww || !wh || ww>2048 || wh>2048) return 0;
    memset(&q,0,sizeof(q));
    if (sc64_p7_typed_building_zero(row)) { *out=q; return 1; }
#define GET(w,d) do { if(!sc64_p7_typed_building_get(row,&bit,(w),&v)) return 0; (d)=(int)v; } while(0)
    GET(11,q.x); GET(11,q.y);
    if(!sc64_p7_typed_building_get(row,&bit,18,&v)) return 0;
    q.hp=(int)v-131072;
    GET(3,q.owner); GET(6,q.type); GET(1,q.alive);
    for(unsigned i=0;i<5;++i) GET(6,q.queue[i]);
    GET(3,q.queue_head); GET(3,q.queue_count); GET(16,q.queue_progress);
    GET(16,v); q.build_progress=(int)v-1; GET(1,q.under_construction);
    GET(7,q.level); GET(17,q.shield); GET(8,v); q.raw_type=(int)v-1;
    GET(4,v); q.raw_owner=(int)v-1; GET(16,q.flags); GET(2,q.larva);
    GET(9,q.larva_timer); GET(11,q.shield_remainder);
    if(!sc64_p7_typed_building_get(row,&bit,1,&v)||v!=1||bit!=194) return 0;
    for(;bit<200;++bit)
        if((row->bytes[bit>>3]>>(bit&7u))&1u) return 0;
    q.maxhp=sc64_p7_typed_building_maxhp(q.type);
    if(!sc64_p7_typed_building_fields_valid(&q,ww,wh)) return 0;
    *out=q;
    return 1;
#undef GET
}

#endif
