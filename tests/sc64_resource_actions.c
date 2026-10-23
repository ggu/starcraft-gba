#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/sc64_resource_actions.h"

static void assert_state(Sc64ResourceActionState got,
                         uint32_t minerals, uint32_t gas,
                         uint32_t total_minerals, uint32_t total_gas) {
    assert(got.current_minerals == minerals);
    assert(got.current_gas == gas);
    assert(got.total_minerals_gathered == total_minerals);
    assert(got.total_gas_gathered == total_gas);
}

static void test_set_and_resource_isolation(void) {
    Sc64ResourceActionState s = {11, 22, 33, 44};
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_MINERALS,
        SC64_RESOURCE_MOD_SET, 900) == 1);
    assert_state(s, 900, 22, 900, 44);
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_GAS,
        SC64_RESOURCE_MOD_SET, 700) == 1);
    assert_state(s, 900, 700, 900, 700);
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_BOTH,
        SC64_RESOURCE_MOD_SET, 1234) == 1);
    assert_state(s, 1234, 1234, 1234, 1234);
}

static void test_add_wraps_wallet_and_totals(void) {
    Sc64ResourceActionState s = {UINT32_MAX - 3u, 4, UINT32_MAX - 1u, 6};
    const uint64_t m_wallet = (uint64_t)s.current_minerals + 9u;
    const uint64_t m_total = (uint64_t)s.total_minerals_gathered + 9u;
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_MINERALS,
        SC64_RESOURCE_MOD_ADD, 9) == 1);
    assert_state(s, (uint32_t)m_wallet, 4, (uint32_t)m_total, 6);

    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_BOTH,
        SC64_RESOURCE_MOD_ADD, UINT32_MAX) == 1);
    assert_state(s, (uint32_t)((uint64_t)(uint32_t)m_wallet + UINT32_MAX),
        (uint32_t)((uint64_t)4 + UINT32_MAX),
        (uint32_t)((uint64_t)(uint32_t)m_total + UINT32_MAX),
        (uint32_t)((uint64_t)6 + UINT32_MAX));
}

static void test_subtract_wallet_only_and_floor(void) {
    Sc64ResourceActionState s = {100, 7, 500, 900};
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_MINERALS,
        SC64_RESOURCE_MOD_SUBTRACT, 35) == 1);
    assert_state(s, 65, 7, 500, 900);
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_GAS,
        SC64_RESOURCE_MOD_SUBTRACT, 7) == 1);
    assert_state(s, 65, 0, 500, 900);
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_BOTH,
        SC64_RESOURCE_MOD_SUBTRACT, UINT32_MAX) == 1);
    assert_state(s, 0, 0, 500, 900);

    s.current_minerals = 100;
    s.current_gas = 200;
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_BOTH,
        SC64_RESOURCE_MOD_SUBTRACT, 100) == 1);
    assert_state(s, 0, 100, 500, 900);
}

static void test_unknown_operands_are_noops(void) {
    Sc64ResourceActionState s = {1, 2, 3, 4};
    Sc64ResourceActionState before = s;
    assert(sc64_resource_action_apply_one_player(&s, 3,
        SC64_RESOURCE_MOD_SET, 99) == 1);
    assert(memcmp(&s, &before, sizeof(s)) == 0);
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_MINERALS,
        6, 99) == 1);
    assert(memcmp(&s, &before, sizeof(s)) == 0);
    assert(sc64_resource_action_apply_one_player(&s, SC64_RESOURCE_BOTH,
        10, UINT32_MAX) == 1);
    assert(memcmp(&s, &before, sizeof(s)) == 0);
}

static void test_null_is_rejected_atomically(void) {
    assert(sc64_resource_action_apply_one_player(0, SC64_RESOURCE_BOTH,
        SC64_RESOURCE_MOD_SET, 100) == 0);
}

int main(void) {
    test_set_and_resource_isolation();
    test_add_wraps_wallet_and_totals();
    test_subtract_wallet_only_and_floor();
    test_unknown_operands_are_noops();
    test_null_is_rejected_atomically();
    puts("sc64 resource action tests: ok");
    return 0;
}
