#ifndef SC64_T4_ACTIONS_H
#define SC64_T4_ACTIONS_H

#include "sc64_trigger_types.h"

#ifdef SC64_T4_RUNTIME
/* Authored rules from the verified USA ROM. The research recipe is
 * StarCraft64-recomp @ 425f409e02edacf897354cd0872c61ce67a9067a.
 * Converter 0x8002C3EC preserves the modifier byte; native handlers
 * 0x800819EC (11) and 0x80084AB4 (42) establish the values below.
 * See docs/REFERENCES.md. No native implementation is included here. */
enum {
    SC64_T4_THG2_SPRITE_FLAG = 0x1000,
    SC64_T4_THG2_DISABLED_FLAG = 0x8000
};

/* Native opcode 11 uses zero as a one-unit default. */
static int
sc64_t4_creation_count(unsigned raw_count, unsigned *count)
{
    if (!count || raw_count > 255) return 0;
    *count = raw_count ? raw_count : 1;
    return 1;
}

/* Initial THG2 sprites and records carrying the unit-disabled bit do not
 * begin as enabled unit-state actors. Other flag bits remain uninterpreted. */
static int
sc64_t4_doodad_initial_enabled(u16 raw_flags, unsigned *enabled)
{
    if (!enabled || (raw_flags & SC64_T4_THG2_SPRITE_FLAG)) return 0;
    *enabled = !(raw_flags & SC64_T4_THG2_DISABLED_FLAG);
    return 1;
}

/* N64 opcode 42 accepts raw modifiers 0/6 as state-aware toggles, 4 as
 * enable, and 5 as disable. Sprite THG2 records are not unit-state targets.
 * On rejection, `next_enabled` is left untouched. */
static int
sc64_t4_doodad_set_state(u16 raw_flags, unsigned current_enabled,
                         unsigned raw_modifier, unsigned *next_enabled)
{
    if (!next_enabled || current_enabled > 1 ||
        (raw_flags & SC64_T4_THG2_SPRITE_FLAG)) return 0;
    unsigned next;
    switch (raw_modifier) {
    case 0: case 6: next = !current_enabled; break;
    case 4: next = 1; break;
    case 5: next = 0; break;
    default: return 0;
    }
    *next_enabled = next;
    return 1;
}
#endif /* SC64_T4_RUNTIME */

#endif
