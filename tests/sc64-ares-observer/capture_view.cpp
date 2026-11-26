/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free checks for the capture spike's pure helpers: pad bits, the
 * line-wise cache-aware reader, the unit-table locate/link/live rules, the
 * closed-loop tape conditions and run-directory file names.
 * Synthetic buffers only; no ROM, emulator or guest data. */
#include "../../experiments/ares-headless/capture_view.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

static void put32(std::uint8_t *p, std::uint32_t v) {
    p[0] = std::uint8_t(v >> 24); p[1] = std::uint8_t(v >> 16); p[2] = std::uint8_t(v >> 8); p[3] = std::uint8_t(v);
}

static_assert(sc64_capture_pad_bit("A") == 1 && sc64_capture_pad_bit("B") == 512);
static_assert(sc64_capture_pad_bit("Right") == 16 && sc64_capture_pad_bit("C-Right") == 64);
static_assert(sc64_capture_pad_bit("Z") == 1024 && sc64_capture_pad_bit("Left") == 8192);
static_assert(sc64_capture_pad_bit("Axis") == 0 && sc64_capture_pad_bit("") == 0 && sc64_capture_pad_bit("Startx") == 0);
static_assert(sc64_capture_mask_allowed(16383) && !sc64_capture_mask_allowed(16384) && !sc64_capture_mask_allowed(-1));

static_assert(sc64_capture_until_op("eq") == Sc64UntilOp::eq && sc64_capture_until_op("changed") == Sc64UntilOp::changed);
static_assert(sc64_capture_until_op("clear") == Sc64UntilOp::clear && sc64_capture_until_op("eqx") == Sc64UntilOp::invalid);
static_assert(sc64_capture_until_op("") == Sc64UntilOp::invalid);
static_assert(sc64_capture_until_met(Sc64UntilOp::eq, 5, 5, 0) && !sc64_capture_until_met(Sc64UntilOp::eq, 4, 5, 0));
static_assert(sc64_capture_until_met(Sc64UntilOp::ne, 4, 5, 0) && !sc64_capture_until_met(Sc64UntilOp::ne, 5, 5, 0));
static_assert(sc64_capture_until_met(Sc64UntilOp::ge, 6, 5, 0) && !sc64_capture_until_met(Sc64UntilOp::ge, 4, 5, 0));
static_assert(sc64_capture_until_met(Sc64UntilOp::lt, 4, 5, 0) && !sc64_capture_until_met(Sc64UntilOp::lt, 5, 5, 0));
static_assert(sc64_capture_until_met(Sc64UntilOp::changed, 7, 0, 6) && !sc64_capture_until_met(Sc64UntilOp::changed, 6, 0, 6));
static_assert(sc64_capture_until_met(Sc64UntilOp::set, 0x13, 0x03, 0) && !sc64_capture_until_met(Sc64UntilOp::set, 0x11, 0x03, 0));
static_assert(sc64_capture_until_met(Sc64UntilOp::clear, 0x10, 0x03, 0) && !sc64_capture_until_met(Sc64UntilOp::clear, 0x12, 0x03, 0));
static_assert(!sc64_capture_until_met(Sc64UntilOp::invalid, 0, 0, 0));
static void parses_addresses() {
    std::uint32_t v = 0;
    assert(sc64_capture_hex32("800AFDCC", &v) && v == 0x800AFDCCu);
    assert(sc64_capture_hex32("0xff", &v) && v == 0xFFu && sc64_capture_hex32("FFFFFFFF", &v) && v == 0xFFFFFFFFu);
    assert(!sc64_capture_hex32("100000000", &v) && !sc64_capture_hex32("", &v) && !sc64_capture_hex32("0x", &v));
    assert(!sc64_capture_hex32("12g4", &v) && !sc64_capture_hex32("-1", &v) && !sc64_capture_hex32(nullptr, &v));
    assert(sc64_capture_guest_offset(0x800AFDCCu, &v) && v == 0x0AFDCCu);
    assert(sc64_capture_guest_offset(0xA0000010u, &v) && v == 0x10u && sc64_capture_guest_offset(0x7FFFFCu, &v) && v == 0x7FFFFCu);
    assert(!sc64_capture_guest_offset(0x01000000u, &v) && !sc64_capture_guest_offset(0x00800000u, &v));
    assert(!sc64_capture_guest_offset(0x80800000u, &v) && !sc64_capture_guest_offset(0xC0000000u, &v));
    assert(!sc64_capture_guest_offset(0x40000000u, &v));
}

static_assert(sc64_capture_decimal_ok("25") && sc64_capture_decimal_ok("1") && sc64_capture_decimal_ok("999999999"));
static_assert(!sc64_capture_decimal_ok("") && !sc64_capture_decimal_ok("rec") && !sc64_capture_decimal_ok("-1"));
static_assert(!sc64_capture_decimal_ok("+5") && !sc64_capture_decimal_ok("2x") && !sc64_capture_decimal_ok("1000000000"));
static_assert(!sc64_capture_decimal_ok(nullptr));
static_assert(sc64_capture_file_name_ok("lobby-1.state") && sc64_capture_file_name_ok("a_B.9"));
static_assert(!sc64_capture_file_name_ok("") && !sc64_capture_file_name_ok(".hidden") && !sc64_capture_file_name_ok(".."));
static_assert(!sc64_capture_file_name_ok("a/b") && !sc64_capture_file_name_ok("a b") && !sc64_capture_file_name_ok(nullptr));
static_assert(sc64_capture_file_name_ok("0123456789012345678901234567890123456789012345678901234567890123"));
static_assert(!sc64_capture_file_name_ok("01234567890123456789012345678901234567890123456789012345678901234"));

static void reads_field_widths() {
    const std::uint8_t p[4] = {0x12, 0x34, 0x56, 0x78};
    assert(sc64_capture_be(p, 1) == 0x12 && sc64_capture_be(p, 2) == 0x1234 && sc64_capture_be(p, 4) == 0x12345678u);
    assert(sc64_capture_be(p, 3) == 0 && sc64_capture_be(p, 0) == 0);
}

static void pad_bits_are_distinct() {
    const char *names[] = {"A", "Start", "Down", "C-Left", "Right", "R", "C-Right", "C-Up",
                           "C-Down", "B", "Z", "L", "Up", "Left"};
    unsigned all = 0;
    for (const char *name : names) {
        unsigned bit = sc64_capture_pad_bit(name);
        assert(bit && (bit & (bit - 1)) == 0 && (all & bit) == 0);
        all |= bit;
    }
    assert(all == 16383u);
}

static void reader_overlays_cache_lines() {
    std::vector<std::uint8_t> ram(256);
    for (unsigned i = 0; i < ram.size(); ++i) ram[i] = std::uint8_t(i);
    auto raw = [&](std::uint32_t a, std::size_t n, std::uint8_t *out) { std::memcpy(out, ram.data() + a, n); };
    /* Line 0x10 is held by alias 0, line 0x20 by alias 1, line 0x30 by both (different copies). */
    auto line = [](std::uint32_t at, int alias, std::uint8_t *out) {
        bool hit = (at == 0x10 && alias == 0) || (at == 0x20 && alias == 1) || at == 0x30;
        if (hit) std::memset(out, 0xA0 + int(at >> 4) + alias * 8, 16);
        return hit;
    };
    std::uint8_t out[0x30];
    Sc64CaptureReadStats stats;
    assert(sc64_capture_read(0x0C, sizeof out, ram.size(), out, raw, line, &stats));
    assert(out[0] == 0x0C && out[3] == 0x0F);                 /* uncached bytes before line 0x10 */
    assert(out[4] == 0xA1 && out[0x13] == 0xA1);              /* line 0x10 from alias 0 */
    assert(out[0x14] == 0xAA && out[0x23] == 0xAA);           /* line 0x20 from alias 1 */
    assert(out[0x24] == 0xA3 && out[0x2F] == 0xA3);           /* dual hit uses alias 0 */
    assert(stats.cached_lines == 3 && stats.dual_hits == 1 && stats.dual_differ == 1);
    /* Out of range and invalid arguments never call back. */
    unsigned calls = 0;
    auto counted = [&](std::uint32_t, int, std::uint8_t *) { ++calls; return false; };
    assert(!sc64_capture_read(250, 7, ram.size(), out, raw, counted, nullptr));
    assert(!sc64_capture_read(0, 1, ram.size(), nullptr, raw, counted, nullptr));
    assert(!sc64_capture_read(0, 1, SC64_CAPTURE_RAM + 1, out, raw, counted, nullptr));
    assert(calls == 0);
    assert(sc64_capture_read(5, 0, ram.size(), out, raw, counted, nullptr) && calls == 0);
}

static std::vector<std::uint8_t> heap_with_table(std::uint32_t at) {
    std::vector<std::uint8_t> ram(at + 16 + SC64_UNIT_TABLE_BYTES + 64);
    put32(ram.data() + at, SC64_UNIT_TABLE_BYTES);
    for (unsigned i = 1; i < 4; ++i) put32(ram.data() + at + 4 * i, SC64_HEAP_GUARD);
    return ram;
}

static void locates_one_table() {
    auto ram = heap_with_table(0x40);
    std::uint32_t offset = 0;
    assert(sc64_capture_find_unit_table(ram.data(), ram.size(), &offset) == 1 && offset == 0x50);
    assert(sc64_capture_unit_header_ok(ram.data() + 0x40));
    put32(ram.data() + 0x4C, 0);                              /* two guards are not a header */
    assert(sc64_capture_find_unit_table(ram.data(), ram.size(), &offset) == 0);
    assert(!sc64_capture_unit_header_ok(ram.data() + 0x40));
    std::vector<std::uint8_t> two(32 + 2 * (16 + SC64_UNIT_TABLE_BYTES));
    for (std::uint32_t at : {0u, 16u + SC64_UNIT_TABLE_BYTES}) {
        put32(two.data() + at, SC64_UNIT_TABLE_BYTES);
        for (unsigned i = 1; i < 4; ++i) put32(two.data() + at + 4 * i, SC64_HEAP_GUARD);
    }
    assert(sc64_capture_find_unit_table(two.data(), two.size(), &offset) == 2 && offset == 16);
}

static void checks_links_and_liveness() {
    const std::uint32_t base = 0x80300000u;
    std::vector<std::uint8_t> table(SC64_UNIT_TABLE_BYTES);
    auto row = [&](unsigned r) { return table.data() + std::size_t(r) * SC64_UNIT_BYTES; };
    auto addr = [&](unsigned r) { return base + r * SC64_UNIT_BYTES; };
    /* List 3 <-> 7 <-> 899, with sprites on 3 and 899 only. */
    put32(row(3) + 4, addr(7)); put32(row(7) + 0, addr(3)); put32(row(7) + 4, addr(899)); put32(row(899) + 0, addr(7));
    put32(row(3) + 0x0C, 0x80207c70u); put32(row(899) + 0x0C, 0x80207c94u);
    row(899)[0x10] = 0x5A;
    auto links = sc64_capture_unit_links(table.data(), base);
    assert(links.bad == 0 && links.asymmetric == 0);
    std::vector<std::uint8_t> out;
    assert(sc64_capture_live_rows(table.data(), out) == 2);
    assert(out.size() == 2 * (2 + SC64_UNIT_BYTES));
    assert(out[0] == 0 && out[1] == 3 && out[2 + 2 + SC64_UNIT_BYTES - 2] == 0x03 && out[2 + SC64_UNIT_BYTES + 1] == 0x83);
    assert(out[2 + SC64_UNIT_BYTES + 2 + 0x10] == 0x5A);
    put32(row(7) + 4, addr(899) + 4);                         /* not a row start */
    put32(row(3) + 0, base - SC64_UNIT_BYTES);                /* outside the table */
    links = sc64_capture_unit_links(table.data(), base);
    assert(links.bad == 2 && links.asymmetric == 1);          /* 899's prev no longer answered */
}

/* tap.bin file header: magic, then little-endian version and sizes. */
static void tap_header_is_little_endian() {
    std::uint8_t header[SC64_TAP_FILE_HEADER_BYTES];
    sc64_capture_tap_file_header(header, 0xCC);
    assert(std::memcmp(header, "SC64TAP\0", 8) == 0);
    assert(header[8] == SC64_TAP_VERSION && header[9] == 0 && header[10] == 0 && header[11] == 0);
    assert(header[12] == SC64_TAP_HEADER_WORDS * 4 && header[16] == 0xCC && header[17] == 0);
    assert(header[20] == ((2 + SC64_UNIT_BYTES) & 0xFF) && header[21] == ((2 + SC64_UNIT_BYTES) >> 8));
    std::uint8_t word[4];
    sc64_capture_put_le32(word, 0x11223344u);
    assert(word[0] == 0x44 && word[1] == 0x33 && word[2] == 0x22 && word[3] == 0x11);
}

/* poke payloads and the coherent guest write. */
static void writes_guest_bytes() {
    std::uint8_t out[SC64_POKE_MAX + 1]{};
    unsigned n = 0;
    assert(sc64_capture_hex_bytes("0c0A0a", out, &n) && n == 3 && out[0] == 0x0C && out[1] == 0x0A && out[2] == 0x0A);
    assert(!sc64_capture_hex_bytes("", out, &n) && !sc64_capture_hex_bytes("abc", out, &n));
    assert(!sc64_capture_hex_bytes("0x0c", out, &n) && !sc64_capture_hex_bytes("0g", out, &n));
    std::string longest(2 * SC64_POKE_MAX, 'f');
    assert(sc64_capture_hex_bytes(longest.c_str(), out, &n) && n == SC64_POKE_MAX);
    assert(!sc64_capture_hex_bytes((longest + "ff").c_str(), out, &n));
    std::vector<std::uint8_t> ram(64, 0);
    std::uint8_t line0[16]{}, line1[16]{};
    bool hold0 = true, hold1 = false;   /* line 0 caches bytes 16..31; alias 1 holds nothing */
    auto raw = [&](std::uint32_t a, std::uint8_t v) { ram[a] = v; };
    auto line = [&](std::uint32_t a, int alias, std::uint8_t v) {
        if (a < 16 || a >= 32) return false;
        if (alias == 0 && hold0) { line0[a - 16] = v; return true; }
        if (alias == 1 && hold1) { line1[a - 16] = v; return true; }
        return false;
    };
    const std::uint8_t bytes[4] = {1, 2, 3, 4};
    std::size_t cached = 99;
    assert(sc64_capture_write(14, bytes, 4, ram.size(), raw, line, &cached) && cached == 2);
    assert(ram[14] == 1 && ram[17] == 4 && line0[0] == 3 && line0[1] == 4 && line1[0] == 0);
    assert(!sc64_capture_write(62, bytes, 4, ram.size(), raw, line, &cached));   /* past the end */
    assert(ram[62] == 0);
    assert(sc64_capture_region_ok(0x1000, 4096, 0, 0));
    assert(!sc64_capture_region_ok(0x1000, 0, 0, 0) && !sc64_capture_region_ok(0x1000, 16, 0, SC64_EXTRA_MAX_REGIONS));
    assert(!sc64_capture_region_ok(0x1000, 16, SC64_EXTRA_MAX_BYTES - 8, 1));
    assert(!sc64_capture_region_ok(SC64_CAPTURE_RAM - 8, 16, 0, 0));
    std::uint8_t header[16];
    sc64_capture_extra_file_header(header, 3);
    assert(std::memcmp(header, "SC64XTR\0", 8) == 0 && header[8] == SC64_EXTRA_VERSION && header[12] == 3);
}

int main() {
    pad_bits_are_distinct();
    tap_header_is_little_endian();
    writes_guest_bytes();
    reader_overlays_cache_lines();
    locates_one_table();
    checks_links_and_liveness();
    reads_field_widths();
    parses_addresses();
    std::uint8_t sample[3] = {1, 2, 3};
    assert(sc64_capture_fnv64(sample, 3) != sc64_capture_fnv64(sample, 2));
    return 0;
}
