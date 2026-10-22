#ifndef SC64_RESOURCE_ACTIONS_H
#define SC64_RESOURCE_ACTIONS_H

#include <stdint.h>

/*
 * Source-derived SetResources arithmetic only. This deliberately does not
 * interpret or expand the action's player/group selector.
 *
 * Evidence: the verified N64 USA ROM's action table entry 26 points to
 * 0x80082DCC (ROM offset 0xC81CC). That handler dispatches modifiers 7, 8,
 * and 9 to 0x800828B0 (set), 0x80082A48 (add), and 0x80082C0C (subtract).
 * The corresponding per-player callbacks touch the four u32 resource fields
 * at 0x800B1D44 + {0x00,0x30,0x90,0x60}, respectively. Their instructions use
 * 32-bit MIPS arithmetic; add therefore wraps naturally. Unknown resource
 * or modifier operands return successfully without changing state.
 */

typedef struct Sc64ResourceActionState {
    uint32_t current_minerals;
    uint32_t current_gas;
    uint32_t total_minerals_gathered;
    uint32_t total_gas_gathered;
} Sc64ResourceActionState;

enum {
    SC64_RESOURCE_MINERALS = 0,
    SC64_RESOURCE_GAS = 1,
    SC64_RESOURCE_BOTH = 2,
    SC64_RESOURCE_MOD_SET = 7,
    SC64_RESOURCE_MOD_ADD = 8,
    SC64_RESOURCE_MOD_SUBTRACT = 9
};

/* Apply opcode-26 arithmetic to one already-selected player. Returns zero
 * only for a null state pointer; unsupported operands are successful no-ops.
 */
static inline int sc64_resource_action_apply_one_player(
    Sc64ResourceActionState *state,
    uint16_t resource,
    uint8_t modifier,
    uint32_t amount) {
    if (state == 0) return 0;
    if (resource > SC64_RESOURCE_BOTH) return 1;
    if (modifier != SC64_RESOURCE_MOD_SET &&
        modifier != SC64_RESOURCE_MOD_ADD &&
        modifier != SC64_RESOURCE_MOD_SUBTRACT) return 1;

    if (resource == SC64_RESOURCE_MINERALS || resource == SC64_RESOURCE_BOTH) {
        if (modifier == SC64_RESOURCE_MOD_SET) {
            state->current_minerals = amount;
            state->total_minerals_gathered = amount;
        } else if (modifier == SC64_RESOURCE_MOD_ADD) {
            state->current_minerals += amount;
            state->total_minerals_gathered += amount;
        } else {
            state->current_minerals = amount < state->current_minerals
                ? state->current_minerals - amount
                : 0;
        }
    }

    if (resource == SC64_RESOURCE_GAS || resource == SC64_RESOURCE_BOTH) {
        if (modifier == SC64_RESOURCE_MOD_SET) {
            state->current_gas = amount;
            state->total_gas_gathered = amount;
        } else if (modifier == SC64_RESOURCE_MOD_ADD) {
            state->current_gas += amount;
            state->total_gas_gathered += amount;
        } else {
            state->current_gas = amount < state->current_gas
                ? state->current_gas - amount
                : 0;
        }
    }
    return 1;
}

#endif
