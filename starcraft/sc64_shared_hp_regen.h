#ifndef SC64_SHARED_HP_REGEN_H
#define SC64_SHARED_HP_REGEN_H

/* Integer-HP adaptation of the source 4/256 HP per 24 Hz update at the
 * project's 30 Hz engine cadence. The caller owns typed phase storage and
 * source identity; incomplete actors retain the existing fraction. */
enum { SC64_SHARED_HP_REGEN_PHASES = 80 };
static inline int sc64_shared_hp_regen_tick(int *hp, int maxhp,
                                             unsigned *phase, int complete)
{
    if (!hp || !phase || *phase >= SC64_SHARED_HP_REGEN_PHASES ||
        maxhp <= 0 || *hp > maxhp || (complete != 0 && complete != 1)) return 0;
    if (!complete || *hp <= 0) return 1;
    if (*hp == maxhp) { *phase = 0; return 1; }
    if (++*phase == SC64_SHARED_HP_REGEN_PHASES) { *phase = 0; ++*hp; }
    return 1;
}
#endif
