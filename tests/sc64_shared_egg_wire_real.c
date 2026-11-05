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
    assert(ENC(&row,&q,2048,2048));assert(DEC(&out,&row,2048,2048));
    assert(!memcmp(&q,&out,sizeof(q)));
    assert(sc64_shared_egg_wire_valid(&out,1,custom_owner,0));
#define BAD(field,value) do { Unit bad=q;bad.field=(value);assert(!sc64_shared_egg_wire_valid(&bad,1,custom_owner,0)); } while(0)
    BAD(raw_type,59);BAD(raw_owner,3);BAD(type,U_OVERLORD);BAD(target,U_MARINE);
    BAD(work_timer,0);BAD(work_timer,2048);BAD(hp,0);BAD(hp,201);BAD(armor,0);
    BAD(build,0);BAD(carry,1);BAD(flags,1);BAD(alive,0);BAD(selected,2);
#undef BAD
    printf("Egg HOST real codec roundtrip row=%zu passed\n",sizeof(row));return 0;
}
