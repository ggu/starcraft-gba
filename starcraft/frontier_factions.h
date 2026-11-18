/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FRONTIER_FACTIONS_H
#define FRONTIER_FACTIONS_H

/* Original Frontier roster and compact, cartridge-bound match setup. */
enum {
    FRONTIER_FOUNDRY = 0,
    FRONTIER_ROVERS,
    FRONTIER_BULWARK,
    FRONTIER_FACTION_COUNT
};

typedef struct {
    const char *name, *fighter;
    unsigned char hp, range, damage, speed, cooldown;
    unsigned char desired_workers, attack_threshold;
} FrontierFactionProfile;

static const FrontierFactionProfile frontier_faction_profiles[FRONTIER_FACTION_COUNT] = {
    {"FOUNDRY", "RIFLER", 40, 64, 6, 1, 19, 8, 8},
    {"ROVERS", "SKIMMER", 32, 48, 5, 2, 12, 6, 6},
    {"BULWARK", "WARDEN", 85, 32, 11, 1, 25, 9, 5}
};

/* Return -1 for an invalid setup. Bits 0, 1-6, 7-8 and 9-10 hold
 * the map, CPU factions, human faction and cartridge ID plus one. */
static inline int frontier_pack_setup(int map, int cartridge, int human,
                                      const unsigned char cpu[3])
{
    int packed, i;
    if ((unsigned)map > 1u || (unsigned)cartridge >= FRONTIER_FACTION_COUNT ||
        (unsigned)human >= FRONTIER_FACTION_COUNT || !cpu)
        return -1;
    packed = map | (human << 7) | ((cartridge + 1) << 9);
    for (i = 0; i < 3; ++i) {
        if (cpu[i] >= FRONTIER_FACTION_COUNT) return -1;
        packed |= (int)cpu[i] << (1 + i * 2);
    }
    return packed;
}

/* Validate everything before touching outputs. A zero cartridge marker denotes
 * the prior format, authenticated by its human faction. Map-only saves thus
 * remain valid only on the Foundry cartridge. */
static inline int frontier_unpack_setup(int packed, int cartridge, int *map,
                                        int *human, unsigned char cpu[3])
{
    unsigned char decoded[3];
    int i, decoded_human, marker;
    if ((unsigned)packed > 0x7ffu ||
        (unsigned)cartridge >= FRONTIER_FACTION_COUNT || !map || !human || !cpu)
        return 0;
    decoded_human = (packed >> 7) & 3;
    marker = (packed >> 9) & 3;
    if (decoded_human >= FRONTIER_FACTION_COUNT ||
        (marker ? marker != cartridge + 1 : decoded_human != cartridge))
        return 0;
    for (i = 0; i < 3; ++i) {
        decoded[i] = (unsigned char)((packed >> (1 + i * 2)) & 3);
        if (decoded[i] >= FRONTIER_FACTION_COUNT) return 0;
    }
    *map = packed & 1;
    *human = decoded_human;
    for (i = 0; i < 3; ++i) cpu[i] = decoded[i];
    return 1;
}

#endif
