/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Test-only adapter exercises actual compiled product types and codecs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_guardian_wire.h"
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
    for(int i=0;i<3;i++) {
      if(i==1){q.hp=37;q.work_timer=1;q.selected=0;q.owner=ENEMY;}
      if(i==2){q.alive=0;q.hp=-1;q.work_timer=0;}
      assert(sc64_guardian_wire_encode(&q,&wire,1));assert(ENC(&row,&wire,2048,2048));assert(DEC(&out,&row,2048,2048));
      assert(sc64_guardian_wire_decode(&out,&again,1));assert(!memcmp(&q,&again,sizeof(q)));
      assert(!sc64_guardian_wire_decode(&out,&again,0));assert(!ENC(&row,&q,2048,2048));
    }
#ifdef SC64_GUARDIAN_DISTINCT_STAGE
    assert(U_GUARDIAN_STAGE == SC_RU_LEGACY_COUNT + 1 && U_GUARDIAN_STAGE < 64);
    assert(sc64_race_raw_unit(U_GUARDIAN_STAGE)==44);
    assert(sc64_race_unit_rule(U_GUARDIAN_STAGE)->hp==150);
    assert(sc64_race_unit_flying(U_GUARDIAN_STAGE));
    assert(sc64_race_unit_armor(U_GUARDIAN_STAGE)==2);
    assert(sc64_race_unit_damage_vs(U_GUARDIAN_STAGE,0)==20 && sc64_race_unit_damage_vs(U_GUARDIAN_STAGE,1)==0);
    assert(sc64_race_unit_range_vs(U_GUARDIAN_STAGE,0)==256 && sc64_race_unit_range_vs(U_GUARDIAN_STAGE,1)==0);
    assert(sc64_game_unit_actor_flags(U_GUARDIAN_STAGE)==(SC64_GAME_ACTOR_SELECTABLE|SC64_GAME_ACTOR_MOVABLE|
        SC64_GAME_ACTOR_COLLIDABLE|SC64_GAME_ACTOR_DRAWABLE|SC64_GAME_ACTOR_MEN|SC64_GAME_ACTOR_WEAPON));
    Unit guardian=q;guardian.type=U_GUARDIAN_STAGE;guardian.raw_type=44;
    guardian.alive=1;guardian.hp=101;guardian.maxhp=150;guardian.armor=2;
    guardian.target=-1;guardian.work_timer=0;
    assert(sc64_guardian_wire_encode(&guardian,&wire,1));
    assert(ENC(&row,&wire,2048,2048));assert(DEC(&out,&row,2048,2048));
    assert(sc64_guardian_wire_decode(&out,&again,1));assert(!memcmp(&guardian,&again,sizeof(guardian)));
    assert(!sc64_guardian_wire_decode(&out,&again,0));
    guardian.alive=0;guardian.hp=-1;
    assert(sc64_guardian_wire_encode(&guardian,&wire,1));assert(ENC(&row,&wire,2048,2048));
    assert(DEC(&out,&row,2048,2048));assert(sc64_guardian_wire_decode(&out,&again,1));
    assert(!memcmp(&guardian,&again,sizeof(guardian)));
    guardian.alive=1;guardian.hp=100;
#define BAD_GUARDIAN(f,v) do { Unit b=guardian;b.f=(v);assert(!sc64_guardian_wire_encode(&b,&wire,1)); } while(0)
    BAD_GUARDIAN(raw_type,42);BAD_GUARDIAN(maxhp,200);BAD_GUARDIAN(hp,0);
    BAD_GUARDIAN(hp,151);BAD_GUARDIAN(armor,0);BAD_GUARDIAN(shield,1);BAD_GUARDIAN(alive,2);
    BAD_GUARDIAN(order,ORDER_GAS);BAD_GUARDIAN(target,MAX_UNITS+MAX_BUILDINGS);
#undef BAD_GUARDIAN
#endif
    q.alive=1;q.hp=37;q.work_timer=1;
#define BAD(f,v) do { Unit b=q;b.f=(v);assert(!sc64_guardian_wire_encode(&b,&wire,1)); } while(0)
    BAD(raw_type,44);BAD(target,43);BAD(work_timer,0);BAD(work_timer,751);
    BAD(build,0);BAD(carry,1);BAD(flags,1);BAD(order,1);BAD(maxhp,150);BAD(hp,0);BAD(hp,201);
#undef BAD
    return 0;
}
