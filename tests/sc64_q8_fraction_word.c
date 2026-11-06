/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../starcraft/sc64_q8_fraction_word.h"
int main(void)
{
    uint32_t packed=0;
    unsigned phase=0,hp=0,shield=0;
    /* Enumerating every encoded integer independently catches holes,
     * collisions, and channel reversal across the complete product domain. */
    for (uint32_t expected=0; expected<SC64_Q8_WORD_LIMIT; ++expected) {
        assert(sc64_q8_fraction_unpack(expected,&phase,&hp,&shield));
        assert(phase==expected/65536u && hp==(expected/256u)%256u && shield==expected%256u);
        assert(sc64_q8_fraction_pack(&packed,phase,hp,shield));
        assert(packed==expected);
    }
    packed=42; assert(!sc64_q8_fraction_pack(&packed,1280,0,0) && packed==42);
    assert(!sc64_q8_fraction_pack(&packed,0,256,0) && packed==42);
    assert(!sc64_q8_fraction_pack(&packed,0,0,256) && packed==42);
    phase=11;hp=22;shield=33;
    assert(!sc64_q8_fraction_unpack(SC64_Q8_WORD_LIMIT,&phase,&hp,&shield));
    assert(phase==11 && hp==22 && shield==33);
    assert(!sc64_q8_fraction_pack(0,0,0,0));
    assert(!sc64_q8_fraction_unpack(0,0,&hp,&shield));
    assert(!sc64_q8_fraction_unpack(0,&phase,0,&shield));
    assert(!sc64_q8_fraction_unpack(0,&phase,&hp,0));
    puts("83886080 Q8 phase/channel combinations passed");
    return 0;
}
