#ifndef SC64_T5_AI_H
#define SC64_T5_AI_H

/* T5's CHK carries opaque BW/N64 AI script IDs.  This adapter validates and
 * dispatches those source requests through callbacks; it does not pretend to
 * execute the proprietary script bodies.  Any GBA economy/attack policy
 * supplied through backend_tick is explicitly an authored substitute.
 */
#if defined(SC64_T5_RUNTIME) && defined(SC64_T4_RUNTIME) && \
    defined(SC64_UNIFIED) && defined(SC64_T5_ENABLED)

#include "sc64_t4_triggers.h"

enum {
    /* The source bytes spell these IDs in ASCII, while CHK's little-endian
     * argument words (and the native MIPS compares) use the values below. */
    SC64_T5_SCRIPT_RSCU = 0x75637352u, /* "Rscu" */
    SC64_T5_SCRIPT_TER5 = 0x35726554u, /* "Ter5" */
    SC64_T5_SCRIPT_TE5H = 0x48356554u, /* "Te5H" */
    SC64_T5_SCRIPT_ENBK = 0x6b426e45u, /* "EnBk" */
    SC64_T5_SCRIPT_CLRC = 0x43726c43u  /* "ClrC" */
};

enum {
    SC64_T5_SCRIPT_BIT_RSCU = 1u << 0,
    SC64_T5_SCRIPT_BIT_TER5 = 1u << 1,
    SC64_T5_SCRIPT_BIT_TE5H = 1u << 2,
    SC64_T5_SCRIPT_BIT_ENBK = 1u << 3,
    SC64_T5_SCRIPT_BIT_CLRC = 1u << 4,
    SC64_T5_AI_PLAYERS = 8
};

typedef struct {
    /* Number of authored 30 Hz backend steps completed. */
    u32 elapsed_ticks;
    /* Successfully dispatched script IDs, one five-bit set per source slot.
     * This is provenance/diagnostic state, not a claim that their bodies ran. */
    u8 requested_scripts[SC64_T5_AI_PLAYERS];
    u8 reserved[4];
} Sc64T5AiState;
_Static_assert(sizeof(Sc64T5AiState) == 16, "T5 AI state budget");

typedef struct {
    /* Return 1 if an implementation accepted the source request atomically,
     * 0 to retry on a later trigger pass, or -1 if unsupported. */
    int (*script)(void *context, unsigned trigger_owner, u32 script_id,
                  const Sc64T4Location *area, int has_area);
    /* Optional authored substitute, called once per game tick. Return 1 on
     * success; a failure leaves elapsed_ticks unchanged. */
    int (*backend_tick)(void *context, u32 elapsed_ticks);
} Sc64T5AiOps;

static int
sc64_t5_ai_init(Sc64T5AiState *state)
{
    if (!state) return 0;
    u8 *p = (u8 *)state;
    for (unsigned i = 0; i < sizeof(*state); ++i) p[i] = 0;
    return 1;
}

static u8
sc64_t5_ai_script_bit(u32 id)
{
    switch (id) {
    case SC64_T5_SCRIPT_RSCU: return SC64_T5_SCRIPT_BIT_RSCU;
    case SC64_T5_SCRIPT_TER5: return SC64_T5_SCRIPT_BIT_TER5;
    case SC64_T5_SCRIPT_TE5H: return SC64_T5_SCRIPT_BIT_TE5H;
    case SC64_T5_SCRIPT_ENBK: return SC64_T5_SCRIPT_BIT_ENBK;
    case SC64_T5_SCRIPT_CLRC: return SC64_T5_SCRIPT_BIT_CLRC;
    default: return 0;
    }
}

static int
sc64_t5_ai_state_valid(const Sc64T5AiState *state)
{
    if (!state) return 0;
    for (unsigned i = 0; i < SC64_T5_AI_PLAYERS; ++i)
        if (state->requested_scripts[i] & ~0x1fu) return 0;
    for (unsigned i = 0; i < sizeof(state->reserved); ++i)
        if (state->reserved[i]) return 0;
    return 1;
}

/* Handles exactly the source AI Script (15) and AI Script At Location (16)
 * actions. `area` must be the already-resolved 1-based CHK location. Native
 * dispatch uses the current trigger source player; the T5 action's player
 * operand is unused (its retained value is 0 even for source players 3/6).
 * Other operand values fail closed rather than being treated as selectors.
 */
static int
sc64_t5_ai_action(Sc64T5AiState *state, const Sc64T4Action *action,
                  unsigned trigger_owner, const Sc64T4Location *area,
                  const Sc64T5AiOps *ops, void *context)
{
    if (!state || !action || !ops || !ops->script ||
        !sc64_t5_ai_state_valid(state) || trigger_owner >= SC64_T5_AI_PLAYERS)
        return -1;
    if (action->opcode != 15 && action->opcode != 16) return -1;
    if ((action->flags & ~0x0fu) || action->player != 0) return -1;
    u8 bit = sc64_t5_ai_script_bit(action->argument);
    if (!bit) return -1;

    int has_area = action->opcode == 16;
    if (!has_area) {
        if (action->location || area) return -1;
    } else {
        if (!action->location || action->location > 64 || !area ||
            area->left >= area->right || area->top >= area->bottom)
            return -1;
    }

    int result = ops->script(context, trigger_owner, action->argument,
                             area, has_area);
    if (result == 1) state->requested_scripts[trigger_owner] |= bit;
    return result == 1 || result == 0 ? result : -1;
}

/* The fallback callback is intentionally separate from source script
 * dispatch. A parent may use it for a documented GBA-authored paid AI policy;
 * no script ID is implied by this per-frame hook.
 */
static int
sc64_t5_ai_tick(Sc64T5AiState *state, const Sc64T5AiOps *ops, void *context)
{
    if (!state || !sc64_t5_ai_state_valid(state)) return 0;
    if (ops && ops->backend_tick &&
        ops->backend_tick(context, state->elapsed_ticks) != 1) return 0;
    ++state->elapsed_ticks;
    return 1;
}

#endif /* T5 unified runtime */
#endif
