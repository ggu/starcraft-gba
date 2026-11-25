/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free checks for the capture spike's pure helpers: pad bits, the
 * line-wise cache-aware reader, and the unit-table locate/link/live rules.
 * Synthetic buffers only; no ROM, emulator or guest data. */
#include "../../experiments/ares-headless/capture_view.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

static void put32(std::uint8_t *p, std::uint32_t v) {
    p[0] = std::uint8_t(v >> 24); p[1] = std::uint8_t(v >> 16); p[2] = std::uint8_t(v >> 8); p[3] = std::uint8_t(v);
}

static_assert(sc64_capture_pad_bit("A") == 1 && sc64_capture_pad_bit("B") == 512);
static_assert(sc64_capture_pad_bit("Right") == 16 && sc64_capture_pad_bit("C-Right") == 64);
static_assert(sc64_capture_pad_bit("Z") == 1024 && sc64_capture_pad_bit("Left") == 8192);
static_assert(sc64_capture_pad_bit("Axis") == 0 && sc64_capture_pad_bit("") == 0 && sc64_capture_pad_bit("Startx") == 0);
static_assert(sc64_capture_mask_allowed(16383) && !sc64_capture_mask_allowed(16384) && !sc64_capture_mask_allowed(-1));

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

int main() {
    pad_bits_are_distinct();
    reader_overlays_cache_lines();
    locates_one_table();
    checks_links_and_liveness();
    std::uint8_t sample[3] = {1, 2, 3};
    assert(sc64_capture_fnv64(sample, 3) != sc64_capture_fnv64(sample, 2));
    return 0;
}
