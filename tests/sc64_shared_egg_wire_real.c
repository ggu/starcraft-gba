/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real compiled codecs; constructed HOST rows never written to guest SRAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_shared_egg_wire_host.h"
#if defined(SC64_ZP_FULL_STORAGE)
#if defined(SC64_P7_TYPED_COMPACT_ENABLED)
typedef Sc64P7TypedUnitSave Row;
#define ENC sc64_p7_typed_unit_encode
#define DEC sc64_p7_typed_unit_decode
#define ENC_STAGE(r,q,w,h) sc64_p7_typed_unit_encode_for(r,q,w,h,1)
#define DEC_STAGE(q,r,w,h) sc64_p7_typed_unit_decode_for(q,r,w,h,1)
#elif defined(SC64_Z7_DERIVED_SAVE)
typedef Sc64DerivedUnitSave Row;
#define ENC sc64_derived_unit_encode
#define DEC sc64_derived_unit_decode
#define ENC_STAGE(r,q,w,h) ENC(r,q,w,h)
#define DEC_STAGE(q,r,w,h) DEC(q,r,w,h)
#else
typedef Sc64ZpPackedUnitSave Row;
#define ENC sc64_zp_unit_save_encode
#define DEC sc64_zp_unit_save_decode
#define ENC_STAGE(r,q,w,h) sc64_zp_unit_save_encode_for(r,q,w,h,1)
#define DEC_STAGE(q,r,w,h) sc64_zp_unit_save_decode_for(q,r,w,h,1)
#endif
#else
typedef Sc64PackedUnitSave Row;
#define ENC sc64_unit_save_encode
#define DEC sc64_unit_save_decode
#define ENC_STAGE(r,q,w,h) sc64_unit_save_encode_for(r,q,w,h,1)
#define DEC_STAGE(q,r,w,h) sc64_unit_save_decode_for(q,r,w,h,1)
#endif
static int custom_owner(int owner,int raw_owner,void *ctx)
{ (void)ctx;return owner>=0 && owner<2 && raw_owner==-1; }
int main(void)
{
    Unit q={0},out;Row row;
    q.x=q.tx=q.path_x=30;q.y=q.ty=q.path_y=40;
    q.alive=1;q.selected=1;q.owner=SIDE;q.type=SC64_SHARED_EGG_TYPE;
    q.raw_type=36;q.raw_owner=-1;q.hp=q.maxhp=200;q.armor=10;
    q.target=U_DRONE;q.work_timer=1;q.build=-1;
    assert(sc64_shared_egg_wire_valid(&q,1,custom_owner,0));
    assert(!sc64_shared_egg_wire_valid(&q,0,custom_owner,0));
#ifndef SC64_Z7_EGG_PRODUCTION
    assert(!ENC(&row,&q,2048,2048));
#endif
    assert(ENC_STAGE(&row,&q,2048,2048));assert(DEC_STAGE(&out,&row,2048,2048));
#ifndef SC64_Z7_EGG_PRODUCTION
    assert(!DEC(&out,&row,2048,2048));
#endif
    assert(!memcmp(&q,&out,sizeof(q)));
    assert(sc64_shared_egg_wire_valid(&out,1,custom_owner,0));
    /* Candidate decoding has a legacy-only default; the explicit detached
     * context is required for a new appended Egg identity. */
    static RaceSave candidate,other;
    candidate.version=SC64_SHARED_EGG_SAVE_VERSION;candidate.units[0]=row;
    Unit sentinel;memset(&sentinel,0x5a,sizeof(sentinel));out=sentinel;
#ifndef SC64_Z7_EGG_PRODUCTION
    assert(!race_save_decode_unit(&candidate,0,2048,2048,&out));
    assert(!memcmp(&out,&sentinel,sizeof(out)));
#else
    assert(race_save_decode_unit(&candidate,0,2048,2048,&out)); /* old Z7 identity */
#endif
    Sc64StageSaveAuth auth;
    const Sc64StageTagPair pairs[]={
        {SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION,0},
        {SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_SAVE_VERSION,SC64_STAGE_SAVE_EGG}
    };
    out=sentinel;assert(!race_save_decode_unit_authenticated(&candidate,0,2048,2048,&out,0));
    assert(!memcmp(&out,&sentinel,sizeof(out)));
    assert(sc64_stage_save_authenticate(&auth,&candidate,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,
                                        candidate.version,pairs,2));
    assert(race_save_decode_unit_authenticated(&candidate,0,2048,2048,&out,&auth));
    assert(!memcmp(&out,&q,sizeof(q)));
    out=sentinel;assert(!race_save_decode_unit_authenticated(&other,0,2048,2048,&out,&auth));
    assert(!memcmp(&out,&sentinel,sizeof(out)));
    candidate.version=SAVE_VERSION;out=sentinel;
    assert(!race_save_decode_unit_authenticated(&candidate,0,2048,2048,&out,&auth));
    assert(!memcmp(&out,&sentinel,sizeof(out)));
    Unit ordinary=q;ordinary.type=U_OVERLORD;ordinary.target=-1;ordinary.work_timer=0;
    ordinary.armor=0;assert(ENC(&row,&ordinary,2048,2048));candidate.units[0]=row;
    assert(race_save_decode_unit(&candidate,0,2048,2048,&out));
    assert(out.type==U_OVERLORD && out.raw_type==36);
#define BAD(field,value) do { Unit bad=q;bad.field=(value);assert(!sc64_shared_egg_wire_valid(&bad,1,custom_owner,0)); } while(0)
    BAD(raw_type,59);BAD(raw_owner,3);BAD(type,U_OVERLORD);BAD(target,U_MARINE);
    BAD(work_timer,0);BAD(work_timer,2048);BAD(hp,0);BAD(hp,201);BAD(armor,0);
    BAD(build,0);BAD(carry,1);BAD(flags,1);BAD(alive,0);BAD(selected,2);
#undef BAD
    printf("Egg HOST real codec roundtrip row=%zu passed\n",sizeof(row));return 0;
}
