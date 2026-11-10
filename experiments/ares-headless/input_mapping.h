/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ARES_INPUT_MAPPING_H
#define SC64_ARES_INPUT_MAPPING_H
/* Logged input IDs are independent of command bits; axes retain IDs 8/16. */
constexpr unsigned sc64_command_bit(unsigned id) {
    switch (id) {
    case 1: return 1; case 2: return 2; case 4: return 4;
    case 32: return 8; case 64: return 16; case 128: return 32;
    case 256: return 64; case 512: return 128; case 1024: return 256; case 2048: return 512;
    default: return 0;
    }
}
constexpr bool sc64_button_pressed(unsigned id, unsigned mask) {
    return (sc64_command_bit(id) & mask) != 0;
}
#endif
