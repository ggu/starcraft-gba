#ifndef SC64_Z5_CONSUME_SAVE_H
#define SC64_Z5_CONSUME_SAVE_H

#ifdef SC64_Z5_CONSUME_ENABLED
/* Validate decoded order links without reading or mutating the live world;
 * HOME can load a different mission. The enclosing validator authenticates
 * the mission state, unique hero identity, energy and complete source world. */
static int race_z5_consume_save_candidate_valid(const RaceSave *candidate,
                                                 int width, int height)
{
    if (!candidate || width < 1 || height < 1) return 0;
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit caster, target;
        if (!race_save_decode_unit(candidate, i, (unsigned)width * 16u,
                                   (unsigned)height * 16u, &caster)) return 0;
        if (!caster.alive || caster.order != ORDER_Z5_CONSUME) continue;
        if (candidate->state[0] || candidate->state[2] != SC64_Z5_GLOBAL_ID ||
            caster.owner != SIDE || caster.type != U_ZP_INFESTED_KERRIGAN ||
            caster.raw_type != 51 || caster.raw_owner != 3 || caster.hp <= 0 ||
            (caster.flags & 8u) ||
            caster.carry < 0 || caster.carry > SC64_Z5_CONSUME_MAX_ENERGY_Q8 ||
            caster.target < 0 || caster.target >= MAX_UNITS ||
            caster.target == (int)i ||
            !race_save_decode_unit(candidate, (unsigned)caster.target,
                                   (unsigned)width * 16u, (unsigned)height * 16u,
                                   &target) ||
            !target.alive || target.hp <= 0 || target.owner != SIDE ||
            target.raw_type != 37 || target.raw_owner != 3 ||
            target.type != U_ZERGLING || (target.flags & 16u) ||
            race_save_unit_invincible(candidate, (unsigned)caster.target)) return 0;
    }
    return 1;
}
#endif
#endif
