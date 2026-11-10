/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RESEARCH_HUMAN_BINDING_H
#define SC64_RESEARCH_HUMAN_BINDING_H
#include <stdint.h>

/* Context-only result: no pointer into the live game or a detached candidate.
 * The current engine's compact SIDE is 0; its human bank is the global
 * minerals/gas pair, not race_minerals[0]/race_gas[0]. */
enum { SC64_PR_BINDING_HUMAN_OWNER = 0, SC64_PR_BINDING_HUMAN_BANK = 0 };
typedef struct {
    uint8_t current_controller_key, payer_key, wallet_slot, chapter, compact_owner;
} Sc64ResearchHumanBinding;

/* The caller authenticates mode, mission identity and its RUNTIME role map
 * against the relevant live/detached canonical context before this call.
 * This function does not authenticate arbitrary supplied arrays or source
 * bytes. In particular, a generic imported OWNR descriptor is insufficient
 * where a campaign adapter overrides its roles (for example P6).
 *
 * Only the uniquely human-controlled side of P1..P10 is supported. Raw actor
 * origin is deliberately absent: rescued actors may retain it for triggers.
 * Enemy/source-wallet binding is a separate, unproved contract. */
static inline int sc64_research_bind_protoss_human(int custom_mode,
    const char *mission_id, unsigned descriptor_human, const int8_t roles[8],
    int current_owner, Sc64ResearchHumanBinding *out)
{
    unsigned chapter, count = 0, human = 8;
    Sc64ResearchHumanBinding result;
    if (custom_mode != 0 || !mission_id || !roles || !out ||
        current_owner != SC64_PR_BINDING_HUMAN_OWNER || mission_id[0] != 'P')
        return 0;
    if (mission_id[1] >= '1' && mission_id[1] <= '9' && mission_id[2] == 0)
        chapter = (unsigned)(mission_id[1] - '0');
    else if (mission_id[1] == '1' && mission_id[2] == '0' && mission_id[3] == 0)
        chapter = 10;
    else return 0;
    if (descriptor_human != (chapter == 10 ? 1u : 7u)) return 0;
    for (unsigned source = 0; source < 8; ++source) {
        if (roles[source] < 0 || roles[source] > 4) return 0;
        if (roles[source] == SC64_PR_BINDING_HUMAN_OWNER) {
            ++count;
            human = source;
        }
    }
    if (count != 1 || human != descriptor_human) return 0;
    result.current_controller_key = result.payer_key = (uint8_t)human;
    result.wallet_slot = SC64_PR_BINDING_HUMAN_BANK;
    result.chapter = (uint8_t)chapter;
    result.compact_owner = SC64_PR_BINDING_HUMAN_OWNER;
    *out = result;
    return 1;
}
#endif
