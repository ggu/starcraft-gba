/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WEAPON_VFX_PROFILES_H
#define SC64_WEAPON_VFX_PROFILES_H

/* Authored visual families for the existing instantaneous attack paths.
 * Profiles select presentation only; they do not change range, damage,
 * cadence, ownership, or native weapon behavior. Include after race_rules.h,
 * its optional actor enums, and the shared weapon VFX/color definitions. */
static int __attribute__((noinline))
sc64_weapon_vfx_unit_profile(int type, int target_air)
{
    int style = SC64_WEAPON_VFX_STYLE_NONE;
    int color = C_WHITE;
    switch (type) {
    case U_MARINE: color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MUZZLE; break;
    case U_FIREBAT: color=C_ORANGE; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_FLAME; break;
#ifndef SC64_RACES
    case U_SCV: color=C_SILVER; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SLASH; break;
    case U_RAYNOR: color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MUZZLE; break;
#else
    case U_RAYNOR:
        color=C_ORANGE;
#ifdef SC64_T4_RUNTIME
        style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SHELL;
#else
        style=SC64_WEAPON_VFX_STYLE_MUZZLE;
#endif
        break;
    case U_SCV: case U_DRONE: case U_PROBE:
        color=C_SILVER; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SLASH; break;
#endif
    case U_ZERGLING: color=C_GREEN; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SLASH; break;
    case U_HYDRALISK:
        color=C_GREEN; style=SC64_WEAPON_VFX_STYLE_NEEDLE; break;
#ifdef SC64_RACES
    case U_HUNTER_KILLER:
        color=C_GREEN; style=SC64_WEAPON_VFX_STYLE_NEEDLE; break;
    case U_VULTURE:
        color=C_ORANGE; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SHELL; break;
    case U_ZEALOT: case U_FENIX:
        color=C_CYAN; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SLASH; break;
    case U_DRAGOON: case U_FENIX_DRAGOON:
        color=C_CYAN; style=SC64_WEAPON_VFX_STYLE_PLASMA; break;
    case U_SCOUT:
        color=C_CYAN; style=target_air ? SC64_WEAPON_VFX_STYLE_MISSILE : SC64_WEAPON_VFX_STYLE_BEAM; break;
    case U_TASSADAR: color=C_CYAN; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_ARC; break;
    case U_GOLIATH:
        color=C_ORANGE; style=target_air ? SC64_WEAPON_VFX_STYLE_MISSILE : SC64_WEAPON_VFX_STYLE_MUZZLE; break;
    case U_TANK: color=C_ORANGE; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SHELL; break;
    case U_SIEGE_TANK: color=C_RED; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SHELL; break;
    case U_WRAITH:
        color=C_CYAN; style=target_air ? SC64_WEAPON_VFX_STYLE_MISSILE : SC64_WEAPON_VFX_STYLE_BEAM; break;
    case U_MUTALISK: color=C_GREEN; style=SC64_WEAPON_VFX_STYLE_ACID; break;
    case U_REAVER: color=C_ORANGE; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SHELL; break;
#ifdef SC64_T4_RUNTIME
    case U_GHOST: case U_RAYNOR_MARINE:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MUZZLE; break;
#endif
#ifdef SC64_Z5_ACTORS
    case U_Z5_GHOST:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MUZZLE; break;
#endif
#ifdef SC64_T5_RUNTIME
    case U_KERRIGAN:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MUZZLE; break;
    case U_RAYNOR_VULTURE:
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    case U_ZP_RAYNOR_VULTURE:
#endif
#if defined(SC64_T5_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        color=C_ORANGE; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SHELL; break;
#endif
#if (defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)) || \
    defined(SC64_Z7_SCOURGE_COMBAT) || defined(SC64_SHARED_SCOURGE_COMBAT)
    case U_SCOURGE:
        color=C_GREEN; style=target_air ? SC64_WEAPON_VFX_STYLE_BURST : SC64_WEAPON_VFX_STYLE_NONE; break;
#endif
#ifdef SC64_Z6_ACTORS
    case U_Z6_GUARDIAN:
        color=C_GREEN; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_ACID; break;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    case U_ZP_INFESTED_KERRIGAN:
        color=C_GREEN; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SLASH; break;
    case U_ZP_ULTRALISK:
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    case U_ULTRALISK:
#endif
#if defined(SC64_TERRAN_LATE_ACTORS) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        color=C_GREEN; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SLASH; break;
#endif
#ifdef SC64_P6_ACTORS
    case U_P6_INFESTED_TERRAN:
        color=C_RED; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_BURST; break;
    case U_P6_HERO_DARK_TEMPLAR: case U_P6_ZERATUL:
        color=C_CYAN; style=target_air ? SC64_WEAPON_VFX_STYLE_NONE : SC64_WEAPON_VFX_STYLE_SLASH; break;
#endif
#ifdef SC64_TERRAN_LATE_ACTORS
    case U_BATTLECRUISER: case U_NORAD_II:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_BEAM; break;
#endif
#ifdef SC64_P5_ACTORS
    case U_P5_BATTLECRUISER:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_BEAM; break;
#endif
#ifdef SC64_P7_ACTORS
    case U_P7_ARCHON:
        color=C_CYAN; style=SC64_WEAPON_VFX_STYLE_ARC; break;
    case U_P7_ARBITER:
        color=C_CYAN; style=SC64_WEAPON_VFX_STYLE_PLASMA; break;
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    case U_P7_INTERCEPTOR:
        color=C_CYAN; style=SC64_WEAPON_VFX_STYLE_PLASMA; break;
#endif
#endif /* SC64_RACES */
    default: return 0;
    }
    return style ? sc64_weapon_vfx_pack(style,color) : 0;
}

static int __attribute__((noinline))
sc64_weapon_vfx_building_profile(int type)
{
    int style=SC64_WEAPON_VFX_STYLE_NONE;
    int color=C_WHITE;
    switch (type) {
    case B_BUNKER: color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MUZZLE; break;
#ifdef SC64_RACES
    case B_PHOTON_CANNON: color=C_CYAN; style=SC64_WEAPON_VFX_STYLE_PLASMA; break;
    case B_SUNKEN_COLONY: color=C_GREEN; style=SC64_WEAPON_VFX_STYLE_SPINES; break;
    case B_SPORE_COLONY: color=C_GREEN; style=SC64_WEAPON_VFX_STYLE_ACID; break;
    case B_MISSILE_TURRET: color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MISSILE; break;
#ifdef SC64_T4_RUNTIME
    case B_FLOOR_MISSILE_TRAP: case B_RIGHT_WALL_MISSILE_TRAP:
        color=C_RED; style=SC64_WEAPON_VFX_STYLE_MISSILE; break;
    case B_FLOOR_GUN_TRAP:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MUZZLE; break;
    case B_WALL_FLAME_TRAP:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_FLAME; break;
#endif
#ifdef SC64_Z5_DOODADS
    case B_Z5_FLOOR_MISSILE_TRAP:
        color=C_RED; style=SC64_WEAPON_VFX_STYLE_MISSILE; break;
    case B_Z5_FLOOR_GUN_TRAP:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_MUZZLE; break;
    case B_Z5_WALL_FLAME_TRAP: case B_Z5_RIGHT_WALL_FLAME_TRAP:
        color=C_ORANGE; style=SC64_WEAPON_VFX_STYLE_FLAME; break;
#endif
#ifdef SC64_P6_ACTORS
    case B_P6_WALL_MISSILE_TRAP:
        color=C_RED; style=SC64_WEAPON_VFX_STYLE_MISSILE; break;
#endif
#endif /* SC64_RACES */
    default: return 0;
    }
    return style ? sc64_weapon_vfx_pack(style,color) : 0;
}

#endif
