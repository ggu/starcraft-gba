#include "../starcraft/sc64_p7_archon_rules.h"

#include <assert.h>

int main(void)
{
    Sc64P7ArchonPacket packet;
    assert(sc64_p7_archon_packet(0,0,&packet));
    assert(packet.tier==SC64_P7_ARCHON_SPLASH_INNER &&
           packet.packet_damage_q8==30u*256u);
    assert(sc64_p7_archon_packet(1,3,&packet));
    assert(packet.tier==SC64_P7_ARCHON_SPLASH_INNER &&
           packet.packet_damage_q8==33u*256u);
    assert(sc64_p7_archon_packet(0,4,&packet));
    assert(packet.tier==SC64_P7_ARCHON_SPLASH_MIDDLE &&
           packet.packet_damage_q8==15u*256u);
    assert(sc64_p7_archon_packet(0,15,&packet));
    assert(packet.tier==SC64_P7_ARCHON_SPLASH_MIDDLE &&
           packet.packet_damage_q8==15u*256u);
    assert(sc64_p7_archon_packet(0,16,&packet));
    assert(packet.tier==SC64_P7_ARCHON_SPLASH_OUTER &&
           packet.packet_damage_q8==7u*256u+128u &&
           sc64_p7_archon_packet_integer_damage(&packet)==7u);
    assert(sc64_p7_archon_packet(1,30,&packet));
    assert(packet.tier==SC64_P7_ARCHON_SPLASH_OUTER &&
           packet.packet_damage_q8==8u*256u+64u);
    assert(sc64_p7_archon_packet(0,31,&packet));
    assert(packet.tier==SC64_P7_ARCHON_SPLASH_NONE &&
           packet.packet_damage_q8==0);
    assert(!sc64_p7_archon_packet(11,0,&packet));
    assert(!sc64_p7_archon_packet(0,0,0));
    Sc64P7ArchonDamageTarget target={100u*256u,2u*256u,1u*256u,1,0};
    Sc64P7ArchonDamageResult result;
    assert(sc64_p7_archon_normal_damage(&target,33u*256u,&result));
    assert(result.shield_damage_q8==2u*256u &&
           result.hp_damage_q8==30u*256u);
    target.shields_q8=15u*256u;
    target.armor_q8=1u*256u;
    assert(sc64_p7_archon_normal_damage(&target,33u*256u/2u,&result));
    assert(result.shield_damage_q8==15u*256u &&
           result.hp_damage_q8==128u); /* 16.5 - 15 shields - 1 armor = 0.5. */
    target.shields_q8=0;
    target.has_shields=0;
    target.armor_q8=1u*256u;
    assert(sc64_p7_archon_normal_damage(&target,7u*256u+128u,&result));
    assert(result.hp_damage_q8==6u*256u+128u);
    target.invincible=1;
    assert(sc64_p7_archon_normal_damage(&target,30u*256u,&result));
    assert(result.hp_damage_q8==0 && result.shield_damage_q8==0);
    return 0;
}
