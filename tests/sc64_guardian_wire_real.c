/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Test-only adapter exercises actual compiled product types and codecs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
static int encode_wire(const Unit *in, Unit *out, int stage)
{
    if (!in || !out) return 0;
    if (in->type != U_GUARDIAN_COCOON) { if (in->raw_type==59) return 0; *out=*in; return 1; }
    if (!stage || in->raw_type!=59 || in->hp<1 || in->hp>200 || in->maxhp!=200 ||
        in->target!=44 || in->work_timer<1 || in->work_timer>750 || in->build!=-1 ||
        in->carry || in->flags || in->order || in->armor || in->shield || !in->alive) return 0;
    *out=*in; out->type=U_OVERLORD; return 1;
}
static int decode_wire(const Unit *in, Unit *out, int stage)
{
    if (!in || !out) return 0;
    if (in->raw_type!=59) { *out=*in; return 1; }
    if (!stage || in->type!=U_OVERLORD) return 0;
    Unit x=*in, check; x.type=U_GUARDIAN_COCOON;
    if (!encode_wire(&x,&check,1)) return 0;
    *out=x; return 1;
}
#if defined(SC64_ZP_FULL_STORAGE)
#if defined(SC64_P7_TYPED_COMPACT_ENABLED)
typedef Sc64P7TypedUnitSave Row;
#define ENC sc64_p7_typed_unit_encode
#define DEC sc64_p7_typed_unit_decode
#elif defined(SC64_Z7_DERIVED_SAVE)
typedef Sc64DerivedUnitSave Row;
#define ENC sc64_derived_unit_encode
#define DEC sc64_derived_unit_decode
#else
typedef Sc64ZpPackedUnitSave Row;
#define ENC sc64_zp_unit_save_encode
#define DEC sc64_zp_unit_save_decode
#endif
#else
typedef Sc64PackedUnitSave Row;
#define ENC sc64_unit_save_encode
#define DEC sc64_unit_save_decode
#endif
int main(void)
{
    assert(sc64_race_raw_unit(U_GUARDIAN_COCOON)==59);
    assert(sc64_race_unit_rule(U_GUARDIAN_COCOON)->hp==200);
    assert(sc64_race_unit_armor(U_GUARDIAN_COCOON)==0);
    assert(sc64_race_unit_flying(U_GUARDIAN_COCOON));
    assert(sc64_game_unit_actor_flags(U_GUARDIAN_COCOON)==(SC64_GAME_ACTOR_SELECTABLE|SC64_GAME_ACTOR_COLLIDABLE|SC64_GAME_ACTOR_DRAWABLE));
    assert(sc64_race_unit_damage(U_GUARDIAN_COCOON)==0 && sc64_race_unit_damage_vs(U_GUARDIAN_COCOON,1)==0);
    assert(sc64_race_unit_range(U_GUARDIAN_COCOON)==0 && sc64_race_unit_hits_vs(U_GUARDIAN_COCOON,0)==0);
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    assert(sc64_game_unit_collision_radius(U_GUARDIAN_COCOON)==8);
#endif
#ifdef SC64_Z7_SOURCE_SIGHT
    assert(sc64_z7_source_unit_sight_raw(sc64_race_raw_unit(U_GUARDIAN_COCOON))==128);
#endif
    Unit q={0},wire,out,again; Row row;
    q.x=q.tx=q.path_x=30; q.y=q.ty=q.path_y=40; q.hp=q.maxhp=200;
    q.owner=SIDE;q.type=U_GUARDIAN_COCOON;q.raw_type=59;q.raw_owner=3;
    q.alive=1;q.selected=1;q.build=-1;q.target=44;q.work_timer=750;
    printf("legacy=%d cocoon=%d count=%d overlord=%d row=%zu\n",SC_RU_LEGACY_COUNT,U_GUARDIAN_COCOON,SC_RU_COUNT,U_OVERLORD,sizeof(row));
    for(int i=0;i<2;i++) {
      if(i){q.hp=37;q.work_timer=1;q.selected=0;q.owner=ENEMY;}
      assert(encode_wire(&q,&wire,1));assert(ENC(&row,&wire,2048,2048));assert(DEC(&out,&row,2048,2048));
      assert(decode_wire(&out,&again,1));assert(!memcmp(&q,&again,sizeof(q)));
      assert(!decode_wire(&out,&again,0));assert(!ENC(&row,&q,2048,2048));
    }
#define BAD(f,v) do { Unit b=q;b.f=(v);assert(!encode_wire(&b,&wire,1)); } while(0)
    BAD(raw_type,44);BAD(target,43);BAD(work_timer,0);BAD(work_timer,751);
    BAD(build,0);BAD(carry,1);BAD(flags,1);BAD(order,1);BAD(maxhp,150);BAD(hp,0);BAD(hp,201);
#undef BAD
    return 0;
}
