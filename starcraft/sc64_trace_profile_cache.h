/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Included after the source VFX profile and key types are defined. */
static inline int sc64_trace_profile_key_matches(const Sc64WeaponSourceAttackProfile *p, Sc64WeaponSourceVfxKey k)
{
    return p->actor_raw==k.actor_raw && p->weapon_id==k.weapon_id &&
        p->mode==k.target_air && p->variant==k.variant;
}
static inline unsigned sc64_trace_profile_slot_index(const Sc64WeaponSourceAttackProfile *p, Sc64WeaponSourceVfxKey k)
{
    if (!p || !sc64_trace_profile_key_matches(p,k)) return 0;
    unsigned index=(unsigned)(p-sc64_weapon_source_profiles);
    return index<255u && index<SC64_WEAPON_SOURCE_PROFILE_COUNT ? index+1u : 0;
}
static inline const Sc64WeaponSourceAttackProfile *sc64_trace_cached_profile(unsigned cached, Sc64WeaponSourceVfxKey k)
{
    if (!cached || cached>SC64_WEAPON_SOURCE_PROFILE_COUNT) return 0;
    const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[cached-1];
    return sc64_trace_profile_key_matches(p,k) ? p : 0;
}
