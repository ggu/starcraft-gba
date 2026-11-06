/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Hand-calculated vectors from the pinned source ordering. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_shield_damage_q8.h"
#define Q(x) ((x)*256u)
static Sc64ShieldDamageInput base(void) {
    Sc64ShieldDamageInput x={0}; x.divisor=1;x.hp_q8=Q(100);
    x.size=SC64_DAMAGE_SIZE_LARGE;x.damage_type=SC64_DAMAGE_NORMAL;return x;
}
static Sc64ShieldDamageResult run(Sc64ShieldDamageInput x) {
    Sc64ShieldDamageResult r; assert(sc64_shield_damage_q8(&x,&r));return r;
}
int main(void) {
    Sc64ShieldDamageInput x=base(); Sc64ShieldDamageResult r;
    x.packet_q8=Q(10);x.has_shields=1;x.shield_q8=Q(5);x.shield_armor_q8=Q(2);x.hp_armor_q8=Q(1);
    r=run(x);assert(r.shield_damage_q8==Q(5) && r.hp_damage_q8==Q(2));
    x=base();x.packet_q8=Q(2);x.has_shields=1;x.shield_q8=Q(3);x.shield_armor_q8=Q(3);
    r=run(x);assert(r.shield_damage_q8==128 && !r.hp_damage_q8);
    x=base();x.packet_q8=Q(2);x.has_shields=1;x.shield_q8=255;x.hp_armor_q8=Q(3);
    r=run(x);assert(!r.shield_damage_q8 && r.shield_after_q8==255 && r.hp_damage_q8==128);
    x=base();x.packet_q8=Q(10);x.matrix_q8=Q(10);x.has_shields=1;x.shield_q8=Q(2);
    r=run(x);assert(r.matrix_damage_q8==Q(10) && r.shield_damage_q8==128 && !r.hp_damage_q8);
    x=base();x.packet_q8=Q(10);x.has_shields=1;x.shield_q8=Q(3);x.shield_armor_q8=Q(1);x.hp_armor_q8=Q(2);x.damage_type=SC64_DAMAGE_EXPLOSIVE;x.size=SC64_DAMAGE_SIZE_SMALL;
    r=run(x);assert(r.shield_damage_q8==Q(3) && r.hp_damage_q8==Q(2));
    x=base();x.packet_q8=Q(10);x.has_shields=1;x.shield_q8=Q(3);x.shield_armor_q8=Q(9);x.hp_armor_q8=Q(9);x.damage_type=SC64_DAMAGE_IGNORE_ARMOR;
    r=run(x);assert(r.shield_damage_q8==Q(3) && r.hp_damage_q8==Q(7));
    x=base();x.packet_q8=Q(4);x.hallucinated=1;x.divisor=4;x.acid_bonus_q8=Q(1);
    r=run(x);assert(r.hp_damage_q8==Q(3));
    x=base();x.packet_q8=Q(3);x.hp_armor_q8=Q(1);x.damage_type=SC64_DAMAGE_CONCUSSIVE;x.size=SC64_DAMAGE_SIZE_LARGE;
    r=run(x);assert(r.hp_damage_q8==128);
    x=base();x.packet_q8=0;x.hp_armor_q8=Q(99);
    r=run(x);assert(r.hp_damage_q8==128);
    x=base();x.packet_q8=Q(5);x.has_shields=1;x.shield_q8=Q(8);x.damage_type=SC64_DAMAGE_NONE;
    r=run(x);assert(r.shield_damage_q8==Q(5) && !r.hp_damage_q8);
    /* Two independent 10-point impacts each pay two shield armor; one
     * pre-aggregated 20-point impact would incorrectly remove 18. */
    x=base();x.packet_q8=Q(10);x.has_shields=1;x.shield_q8=Q(30);x.shield_armor_q8=Q(2);
    r=run(x);assert(r.shield_after_q8==Q(22));x.shield_q8=r.shield_after_q8;
    r=run(x);assert(r.shield_after_q8==Q(14));
    x=base();x.packet_q8=UINT_MAX;x.hallucinated=1;memset(&r,0xa5,sizeof(r));Sc64ShieldDamageResult before=r;
    assert(!sc64_shield_damage_q8(&x,&r) && !memcmp(&r,&before,sizeof(r)));
    puts("pinned shield/armor Q8 per-hit vectors: PASS");return 0;
}
