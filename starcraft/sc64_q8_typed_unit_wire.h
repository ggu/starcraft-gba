/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Q8_TYPED_UNIT_WIRE_H
#define SC64_Q8_TYPED_UNIT_WIRE_H
/* Prototype only: call after the historical typed codec is declared. The new
 * tag must select these accessors; old tags must retain the old decoder. */
static int sc64_q8_typed_unit_transform(Sc64P7TypedUnitSave *out,
 const Sc64P7TypedUnitSave *input, unsigned *fraction, int encode,
 unsigned ww, unsigned wh)
{
    Sc64P7TypedUnitSave temp;
    Unit actor;
    unsigned bit=49, pos=22, hpcode, alive, debt;
    u32 v;
    if (!out || !input || !fraction || *fraction>255u) return 0;
    if (sc64_p7_typed_unit_zero(input)) {
        if (encode && *fraction) return 0;
        *out=*input; *fraction=0; return 1;
    }
    if (!sc64_p7_typed_unit_get(input,&bit,1,&v)) return 0;
    alive=v; bit=22;
    if (!sc64_p7_typed_unit_get(input,&bit,18,&v)) return 0;
    hpcode=v; temp=*input;
    if (encode) {
        if (!sc64_p7_typed_unit_decode(&actor,input,ww,wh)) return 0;
        if (alive) {
            if (actor.hp<=0 || actor.hp>actor.maxhp || actor.hp>1023) return 0;
            hpcode=((unsigned)actor.hp<<8)|*fraction;
        } else if (*fraction) return 0;
    } else if (alive) {
        debt=hpcode&255u; hpcode=(hpcode>>8)+131072u;
        if (hpcode>=262144u) return 0;
    } else debt=0;
    for(unsigned i=0;i<18;++i) {
        unsigned at=pos+i;
        temp.bytes[at>>3]=(u8)((temp.bytes[at>>3]&~(1u<<(at&7u))) |
                                   (((hpcode>>i)&1u)<<(at&7u)));
    }
    if (!encode) {
        if (!sc64_p7_typed_unit_decode(&actor,&temp,ww,wh)) return 0;
        if (alive && (actor.hp<=0 || actor.hp>actor.maxhp || actor.hp>1023)) return 0;
        *fraction=debt;
    }
    *out=temp; return 1;
}
#endif
