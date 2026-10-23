/* Asset-free checks for the bounded T5 opaque-script dispatch boundary. */
#define SC64_T4_RUNTIME 1
#define SC64_T5_RUNTIME 1
#define SC64_UNIFIED 1
#define SC64_T5_ENABLED 1
#include "../starcraft/sc64_t5_ai.h"

#include <assert.h>
#include <string.h>

_Static_assert(SC64_T5_SCRIPT_RSCU == 0x75637352u, "CHK/native Rscu word");
_Static_assert(SC64_T5_SCRIPT_TER5 == 0x35726554u, "CHK/native Ter5 word");
_Static_assert(SC64_T5_SCRIPT_TE5H == 0x48356554u, "CHK/native Te5H word");
_Static_assert(SC64_T5_SCRIPT_ENBK == 0x6b426e45u, "CHK/native EnBk word");
_Static_assert(SC64_T5_SCRIPT_CLRC == 0x43726c43u, "CHK/native ClrC word");

typedef struct {
    unsigned calls;
    unsigned owner;
    u32 id;
    int has_area;
    int result;
    Sc64T4Location copied;
} Context;

static int
script(void *opaque, unsigned owner, u32 id, const Sc64T4Location *area,
       int has_area)
{
    Context *c = opaque;
    ++c->calls;
    c->owner = owner;
    c->id = id;
    c->has_area = has_area;
    if (area) c->copied = *area;
    return c->result;
}

static int
backend_tick(void *opaque, u32 elapsed)
{
    Context *c = opaque;
    ++c->calls;
    c->id = elapsed;
    return c->result;
}

static void
test_script_ids_and_state(void)
{
    static const u32 ids[] = {
        SC64_T5_SCRIPT_RSCU, SC64_T5_SCRIPT_TER5, SC64_T5_SCRIPT_TE5H,
        SC64_T5_SCRIPT_ENBK, SC64_T5_SCRIPT_CLRC
    };
    static const u8 bits[] = {
        SC64_T5_SCRIPT_BIT_RSCU, SC64_T5_SCRIPT_BIT_TER5,
        SC64_T5_SCRIPT_BIT_TE5H, SC64_T5_SCRIPT_BIT_ENBK,
        SC64_T5_SCRIPT_BIT_CLRC
    };
    Sc64T5AiState s;
    Context c = {0};
    Sc64T5AiOps ops = { script, backend_tick };
    Sc64T4Location area = { .left = 10, .top = 20, .right = 30, .bottom = 40 };
    Sc64T4Action a;

    memset(&s, 0xa5, sizeof(s));
    assert(sc64_t5_ai_init(&s));
    assert(sc64_t5_ai_state_valid(&s));
    assert(s.elapsed_ticks == 0);
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        memset(&a, 0, sizeof(a));
        a.opcode = (i & 1) ? 16 : 15;
        a.player = 0;
        a.argument = ids[i];
        if (a.opcode == 16) a.location = 1;
        c.result = 1;
        assert(sc64_t5_ai_action(&s, &a, 0, a.opcode == 16 ? &area : 0,
                                 &ops, &c) == 1);
        assert(s.requested_scripts[0] & bits[i]);
        assert(c.id == ids[i] && c.owner == 0);
        assert(c.has_area == (a.opcode == 16));
        if (a.opcode == 16) assert(c.copied.right == 30);
    }
    assert(c.calls == 5);
    assert(s.requested_scripts[1] == 0);

    /* In the native handler these AI actions use the enabled trigger source
     * player, not their stale CHK player operand (zero on all T5 rows). */
    memset(&a, 0, sizeof(a));
    a.opcode = 15; a.player = 0; a.argument = SC64_T5_SCRIPT_RSCU;
    assert(sc64_t5_ai_action(&s, &a, 3, 0, &ops, &c) == 1);
    assert(c.owner == 3 && (s.requested_scripts[3] & SC64_T5_SCRIPT_BIT_RSCU));
    a.argument = SC64_T5_SCRIPT_TER5;
    assert(sc64_t5_ai_action(&s, &a, 6, 0, &ops, &c) == 1);
    assert(c.owner == 6 && (s.requested_scripts[6] & SC64_T5_SCRIPT_BIT_TER5));
}

static void
test_atomic_retry_and_validation(void)
{
    Sc64T5AiState s;
    Sc64T5AiOps ops = { script, backend_tick };
    Context c = {0};
    Sc64T4Action a;
    Sc64T4Location area = { .left = 1, .top = 2, .right = 3, .bottom = 4 };
    assert(sc64_t5_ai_init(&s));
    memset(&a, 0, sizeof(a));
    a.opcode = 15; a.player = 0; a.argument = SC64_T5_SCRIPT_RSCU;
    c.result = 0;
    assert(sc64_t5_ai_action(&s, &a, 0, 0, &ops, &c) == 0);
    assert(s.requested_scripts[0] == 0);
    c.result = -1;
    assert(sc64_t5_ai_action(&s, &a, 0, 0, &ops, &c) == -1);
    assert(s.requested_scripts[0] == 0);
    c.result = 1;
    assert(sc64_t5_ai_action(&s, &a, 0, 0, &ops, &c) == 1);

    a.opcode = 16; a.location = 1;
    assert(sc64_t5_ai_action(&s, &a, 0, 0, &ops, &c) == -1);
    assert(sc64_t5_ai_action(&s, &a, 0, &area, &ops, &c) == 1);
    a.location = 65;
    assert(sc64_t5_ai_action(&s, &a, 0, &area, &ops, &c) == -1);
    a.location = 1; area.right = area.left;
    assert(sc64_t5_ai_action(&s, &a, 0, &area, &ops, &c) == -1);
    area.right = 3;
    a.argument = 0x12345678u;
    assert(sc64_t5_ai_action(&s, &a, 0, &area, &ops, &c) == -1);
    a.argument = SC64_T5_SCRIPT_RSCU; a.player = 1;
    assert(sc64_t5_ai_action(&s, &a, 0, &area, &ops, &c) == -1);
    a.player = 0; a.opcode = 24;
    assert(sc64_t5_ai_action(&s, &a, 0, &area, &ops, &c) == -1);
    a.opcode = 16; a.location = 1; a.flags = 0x80;
    assert(sc64_t5_ai_action(&s, &a, 0, &area, &ops, &c) == -1);
    assert(s.requested_scripts[0] == (SC64_T5_SCRIPT_BIT_RSCU));
}

static void
test_backend_cadence_is_separate(void)
{
    Sc64T5AiState s;
    Sc64T5AiOps ops = { script, backend_tick };
    Context c = {0};
    assert(sc64_t5_ai_init(&s));
    c.result = 0;
    assert(!sc64_t5_ai_tick(&s, &ops, &c));
    assert(s.elapsed_ticks == 0);
    c.result = 1;
    assert(sc64_t5_ai_tick(&s, &ops, &c));
    assert(sc64_t5_ai_tick(&s, &ops, &c));
    assert(s.elapsed_ticks == 2);
    assert(c.id == 1); /* Callback receives previous completed-tick count. */
    assert(sc64_t5_ai_tick(&s, 0, 0));
    assert(s.elapsed_ticks == 3);
    s.reserved[2] = 1;
    assert(!sc64_t5_ai_state_valid(&s));
    assert(!sc64_t5_ai_tick(&s, 0, 0));
}

int
main(void)
{
    test_script_ids_and_state();
    test_atomic_retry_and_validation();
    test_backend_cadence_is_separate();
    return 0;
}
