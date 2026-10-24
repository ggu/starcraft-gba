#ifndef SC64_CONSTRUCTION_HP_H
#define SC64_CONSTRUCTION_HP_H

/* Integer 30 Hz construction progression. The source game starts a new
 * building at one tenth HP and adds the remaining nine tenths over its build
 * clock. Deltas use adjacent cumulative floors so the final tick reaches the
 * authored maximum without overwriting damage already taken. */
#ifdef SC64_CONSTRUCTION_HP
#ifndef SC64_RACES
#error "SC64_CONSTRUCTION_HP requires SC64_RACES"
#endif

static inline int sc64_construction_hp_start(int maximum)
{
    int start = maximum / 10;
    return start > 0 ? start : maximum > 0 ? 1 : 0;
}

static inline int sc64_construction_hp_gain(int maximum, int progress,
                                             int total)
{
    int start = sc64_construction_hp_start(maximum);
    u32 remaining, at, before;
    if (maximum <= start || total <= 0 || progress <= 0) return 0;
    if (progress > total) progress = total;
    remaining = (u32)(maximum - start);
    at = remaining * (u32)progress / (u32)total;
    before = remaining * (u32)(progress - 1) / (u32)total;
    return (int)(at - before);
}

static inline int sc64_construction_shield_start(int maximum)
{
    return sc64_construction_hp_start(maximum);
}

static inline int sc64_construction_shield_gain(int maximum, int progress,
                                                 int total)
{
    return sc64_construction_hp_gain(maximum, progress, total);
}

/* BW morph completion changes absolute hit points by the max-HP difference.
 * Clamp to the surviving minimum and the new type's cap. */
static inline int sc64_construction_morph_hp(int current, int old_maximum,
                                              int new_maximum)
{
    int hp = current + new_maximum - old_maximum;
    if (hp < 1) hp = 1;
    if (hp > new_maximum) hp = new_maximum;
    return hp;
}
#endif

/* Shared temporary markers let the construction ticker distinguish the two
 * in-place Z7 Creep Colony morphs before their adapter header is included. */
#if defined(SC64_Z7_COLONY_DEFENSE_ENABLED)
#ifndef SC64_Z7_SUNKEN_MORPH_FLAG
#define SC64_Z7_SUNKEN_MORPH_FLAG 0x8000u
#endif
#ifndef SC64_Z7_SPORE_MORPH_FLAG
#define SC64_Z7_SPORE_MORPH_FLAG 0x4000u
#endif
#endif

#endif
