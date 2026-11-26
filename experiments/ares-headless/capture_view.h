/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CAPTURE_VIEW_H
#define SC64_CAPTURE_VIEW_H
/* Pure helpers for the capture spike frontend: a line-wise cache-aware range
 * reader, the 14-bit pad mask, unit-table locate, link and record checks,
 * closed-loop tape conditions and run-directory file names.
 * Nothing here touches the emulator; every guest access goes through the
 * caller's read-only callbacks. */
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

constexpr std::size_t SC64_CAPTURE_RAM = 8u * 1024u * 1024u;
constexpr unsigned SC64_UNIT_ROWS = 900, SC64_UNIT_BYTES = 312;
constexpr std::uint32_t SC64_UNIT_TABLE_BYTES = SC64_UNIT_ROWS * SC64_UNIT_BYTES;
constexpr std::uint32_t SC64_HEAP_GUARD = 0xBABEFACEu;
static_assert(SC64_UNIT_TABLE_BYTES == 0x448E0u);

/* Existing command bits keep their values; Z, L, D-pad Up and Left are new. */
constexpr bool sc64_capture_name_is(const char *a, const char *b) {
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}
constexpr unsigned sc64_capture_pad_bit(const char *name) {
    struct Entry { const char *name; unsigned bit; };
    constexpr Entry table[] = {
        {"A", 1}, {"Start", 2}, {"Down", 4}, {"C-Left", 8}, {"Right", 16},
        {"R", 32}, {"C-Right", 64}, {"C-Up", 128}, {"C-Down", 256}, {"B", 512},
        {"Z", 1024}, {"L", 2048}, {"Up", 4096}, {"Left", 8192},
    };
    for (const auto &entry : table) if (sc64_capture_name_is(name, entry.name)) return entry.bit;
    return 0;
}
constexpr bool sc64_capture_mask_allowed(long mask) { return mask >= 0 && (mask & ~16383L) == 0; }

struct Sc64CaptureReadStats {
    std::size_t cached_lines = 0; /* 16-byte lines served from the data cache */
    std::size_t dual_hits = 0;    /* lines held by both alias lines */
    std::size_t dual_differ = 0;  /* dual hits whose two copies differ */
};

/* raw(address, length, out) copies RDRAM bytes in guest order.
 * line(line_address, alias, out16) reports whether the data-cache line indexed
 * by the physical line address (alias 0) or by that index with virtual bit 12
 * flipped (alias 1) holds it, and if so copies its 16 bytes in guest order.
 * On a dual hit the alias-0 copy is used and the hit is counted, so the caller
 * can mark the read as a conflict. */
template<class Raw, class Line>
bool sc64_capture_read(std::uint32_t address, std::size_t length, std::size_t ram_size,
                       std::uint8_t *out, Raw raw, Line line, Sc64CaptureReadStats *stats) {
    if (!out || ram_size == 0 || ram_size > SC64_CAPTURE_RAM ||
        address >= ram_size || length > ram_size - address) return false;
    if (!length) return true;
    raw(address, length, out);
    const std::uint64_t end = std::uint64_t(address) + length;
    for (std::uint64_t at = address & ~15u; at < end; at += 16) {
        std::uint8_t first[16], second[16];
        bool hit_first = line(std::uint32_t(at), 0, first);
        bool hit_second = line(std::uint32_t(at), 1, second);
        if (!hit_first && !hit_second) continue;
        if (stats) {
            ++stats->cached_lines;
            if (hit_first && hit_second) {
                ++stats->dual_hits;
                if (std::memcmp(first, second, 16) != 0) ++stats->dual_differ;
            }
        }
        const std::uint8_t *source = hit_first ? first : second;
        std::uint64_t from = at < address ? address : at;
        std::uint64_t to = at + 16 < end ? at + 16 : end;
        std::memcpy(out + (from - address), source + (from - at), std::size_t(to - from));
    }
    return true;
}

inline std::uint32_t sc64_capture_be32(const std::uint8_t *p) {
    return std::uint32_t(p[0]) << 24 | std::uint32_t(p[1]) << 16 | std::uint32_t(p[2]) << 8 | p[3];
}

/* The table is one heap block: size word, three guard words, then 900 rows.
 * Returns the number of matching headers; *offset is the first row of the
 * first match. Exactly one match is the only usable answer. */
inline unsigned sc64_capture_find_unit_table(const std::uint8_t *ram, std::size_t size,
                                             std::uint32_t *offset) {
    unsigned found = 0;
    if (!ram || size < 16u + SC64_UNIT_TABLE_BYTES) return 0;
    for (std::size_t at = 0; at + 16u + SC64_UNIT_TABLE_BYTES <= size; at += 4) {
        if (sc64_capture_be32(ram + at) != SC64_UNIT_TABLE_BYTES ||
            sc64_capture_be32(ram + at + 4) != SC64_HEAP_GUARD ||
            sc64_capture_be32(ram + at + 8) != SC64_HEAP_GUARD ||
            sc64_capture_be32(ram + at + 12) != SC64_HEAP_GUARD) continue;
        if (!found && offset) *offset = std::uint32_t(at + 16);
        ++found;
    }
    return found;
}
inline bool sc64_capture_unit_header_ok(const std::uint8_t *header16) {
    return sc64_capture_be32(header16) == SC64_UNIT_TABLE_BYTES &&
           sc64_capture_be32(header16 + 4) == SC64_HEAP_GUARD &&
           sc64_capture_be32(header16 + 8) == SC64_HEAP_GUARD &&
           sc64_capture_be32(header16 + 12) == SC64_HEAP_GUARD;
}

struct Sc64CaptureLinkStats {
    unsigned bad = 0;        /* link words that are not zero or a row start */
    unsigned asymmetric = 0; /* next(prev(r)) != r or prev(next(r)) != r */
};
/* Rows start with previous and next links (guest pointers, KSEG0). */
inline Sc64CaptureLinkStats sc64_capture_unit_links(const std::uint8_t *table,
                                                    std::uint32_t table_address) {
    Sc64CaptureLinkStats stats;
    auto row_of = [&](std::uint32_t pointer, unsigned *row) {
        std::uint32_t delta = pointer - table_address;
        if (pointer < table_address || delta >= SC64_UNIT_TABLE_BYTES || delta % SC64_UNIT_BYTES) return false;
        *row = delta / SC64_UNIT_BYTES;
        return true;
    };
    for (unsigned row = 0; row < SC64_UNIT_ROWS; ++row) {
        const std::uint8_t *p = table + std::size_t(row) * SC64_UNIT_BYTES;
        for (unsigned side = 0; side < 2; ++side) {
            std::uint32_t pointer = sc64_capture_be32(p + side * 4);
            unsigned other = 0;
            if (!pointer) continue;
            if (!row_of(pointer, &other)) { ++stats.bad; continue; }
            /* The neighbour's opposite link must point back at this row. */
            std::uint32_t back = sc64_capture_be32(table + std::size_t(other) * SC64_UNIT_BYTES + (side ? 0 : 4));
            if (back != table_address + row * SC64_UNIT_BYTES) ++stats.asymmetric;
        }
    }
    return stats;
}

/* A row is live only when its sprite pointer (+0x0C) is nonzero; free rows keep
 * stale bytes. Each live row is appended as a big-endian u16 index and its 312
 * bytes. Returns the live count. */
inline unsigned sc64_capture_live_rows(const std::uint8_t *table, std::vector<std::uint8_t> &out) {
    unsigned live = 0;
    for (unsigned row = 0; row < SC64_UNIT_ROWS; ++row) {
        const std::uint8_t *p = table + std::size_t(row) * SC64_UNIT_BYTES;
        if (!sc64_capture_be32(p + 0x0C)) continue;
        out.push_back(std::uint8_t(row >> 8));
        out.push_back(std::uint8_t(row));
        out.insert(out.end(), p, p + SC64_UNIT_BYTES);
        ++live;
    }
    return live;
}

/* Big-endian value of a 1, 2 or 4-byte guest field; other widths read 0. */
inline std::uint32_t sc64_capture_be(const std::uint8_t *p, unsigned bytes) {
    if (bytes == 1) return p[0];
    if (bytes == 2) return std::uint32_t(p[0]) << 8 | p[1];
    if (bytes == 4) return sc64_capture_be32(p);
    return 0;
}

/* Closed-loop tape conditions ("until"): compare a guest field read after a
 * step with a target. "changed" compares with the value read before the first
 * step; "set"/"clear" test that every target bit is set or clear. */
enum class Sc64UntilOp { invalid, eq, ne, ge, lt, changed, set, clear };
constexpr Sc64UntilOp sc64_capture_until_op(const char *name) {
    struct Entry { const char *name; Sc64UntilOp op; };
    constexpr Entry table[] = {
        {"eq", Sc64UntilOp::eq}, {"ne", Sc64UntilOp::ne}, {"ge", Sc64UntilOp::ge}, {"lt", Sc64UntilOp::lt},
        {"changed", Sc64UntilOp::changed}, {"set", Sc64UntilOp::set}, {"clear", Sc64UntilOp::clear},
    };
    for (const auto &entry : table) if (sc64_capture_name_is(name, entry.name)) return entry.op;
    return Sc64UntilOp::invalid;
}
constexpr bool sc64_capture_until_met(Sc64UntilOp op, std::uint32_t value, std::uint32_t target,
                                      std::uint32_t start) {
    switch (op) {
    case Sc64UntilOp::eq: return value == target;
    case Sc64UntilOp::ne: return value != target;
    case Sc64UntilOp::ge: return value >= target;
    case Sc64UntilOp::lt: return value < target;
    case Sc64UntilOp::changed: return value != start;
    case Sc64UntilOp::set: return (value & target) == target;
    case Sc64UntilOp::clear: return (value & target) == 0;
    default: return false;
    }
}

/* World point under the cursor on one axis ("untilw"): the camera word (s32,
 * world pixels of the screen's top-left corner) plus the cursor (u16, screen
 * units), as a signed value (S6). Only eq, ne, ge and lt apply, signed. */
constexpr std::int64_t sc64_capture_world(std::uint32_t camera, std::uint32_t cursor) {
    return std::int64_t(std::int32_t(camera)) + std::int64_t(cursor & 0xFFFFu);
}
constexpr bool sc64_capture_world_op_ok(Sc64UntilOp op) {
    return op == Sc64UntilOp::eq || op == Sc64UntilOp::ne || op == Sc64UntilOp::ge || op == Sc64UntilOp::lt;
}
constexpr bool sc64_capture_world_met(Sc64UntilOp op, std::int64_t value, std::int64_t target) {
    switch (op) {
    case Sc64UntilOp::eq: return value == target;
    case Sc64UntilOp::ne: return value != target;
    case Sc64UntilOp::ge: return value >= target;
    case Sc64UntilOp::lt: return value < target;
    default: return false;
    }
}

/* Hex text of 1 to 8 digits, with an optional 0x, as a 32-bit value. */
constexpr bool sc64_capture_hex32(const char *text, std::uint32_t *out) {
    if (!text || !out) return false;
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) text += 2;
    std::uint32_t value = 0;
    unsigned n = 0;
    for (; text[n]; ++n) {
        char c = text[n];
        unsigned digit = c >= '0' && c <= '9' ? unsigned(c - '0') : c >= 'a' && c <= 'f' ? unsigned(c - 'a' + 10)
                       : c >= 'A' && c <= 'F' ? unsigned(c - 'A' + 10) : 16u;
        if (digit > 15 || n >= 8) return false;
        value = value << 4 | digit;
    }
    if (!n) return false;
    *out = value;
    return true;
}
/* A guest data address: a physical RDRAM offset, or its KSEG0 (0x80…) or KSEG1 (0xA0…)
 * alias. Anything else is refused rather than folded into RDRAM. */
constexpr bool sc64_capture_guest_offset(std::uint32_t address, std::uint32_t *offset) {
    std::uint32_t physical = address;
    if ((address & 0xE0000000u) == 0x80000000u || (address & 0xE0000000u) == 0xA0000000u) physical = address & 0x1FFFFFFFu;
    else if (address & 0xE0000000u) return false;
    if (physical >= SC64_CAPTURE_RAM) return false;
    if (offset) *offset = physical;
    return true;
}

/* A plain decimal of 1 to 9 digits (tap's raw stride); signs, spaces and words are not. */
constexpr bool sc64_capture_decimal_ok(const char *text) {
    if (!text || !*text) return false;
    unsigned n = 0;
    for (; text[n]; ++n) if (text[n] < '0' || text[n] > '9' || n >= 9) return false;
    return true;
}

/* Names of files the frontend writes or loads in its run directory: one plain
 * component of at most 64 characters from [A-Za-z0-9._-], not starting with '.'. */
constexpr bool sc64_capture_file_name_ok(const char *name) {
    if (!name || !*name || *name == '.') return false;
    unsigned n = 0;
    for (; name[n]; ++n) {
        char c = name[n];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                  c == '.' || c == '_' || c == '-';
        if (!ok || n >= 64) return false;
    }
    return true;
}

/* "poke" payload: an even number of hex digits, 1 to SC64_POKE_MAX bytes, no prefix. */
constexpr unsigned SC64_POKE_MAX = 256;
constexpr bool sc64_capture_hex_bytes(const char *text, std::uint8_t *out, unsigned *count) {
    if (!text || !out || !count) return false;
    unsigned n = 0;
    for (; text[n]; ++n) {
        char c = text[n];
        unsigned digit = c >= '0' && c <= '9' ? unsigned(c - '0') : c >= 'a' && c <= 'f' ? unsigned(c - 'a' + 10)
                       : c >= 'A' && c <= 'F' ? unsigned(c - 'A' + 10) : 16u;
        if (digit > 15 || n >= 2 * SC64_POKE_MAX) return false;
        if (n % 2 == 0) out[n / 2] = std::uint8_t(digit << 4);
        else out[n / 2] = std::uint8_t(out[n / 2] | digit);
    }
    if (!n || n % 2) return false;
    *count = n / 2;
    return true;
}

/* Guest write ("poke"): raw(address, byte) stores one guest byte in RDRAM, and
 * line(address, alias, byte) stores it in the data-cache line that indexes the
 * address (alias 0) or that index with virtual bit 12 flipped (alias 1), only if
 * that line holds the address, and says whether it did. Both are written, so a
 * later guest read sees the new byte whether or not the line is cached, and a
 * dirty line's write-back keeps it. *cached gets the number of bytes a line held. */
template<class Raw, class Line>
bool sc64_capture_write(std::uint32_t address, const std::uint8_t *bytes, std::size_t length, std::size_t ram_size,
                        Raw raw, Line line, std::size_t *cached) {
    if (!bytes || ram_size == 0 || ram_size > SC64_CAPTURE_RAM ||
        address >= ram_size || length > ram_size - address) return false;
    std::size_t hits = 0;
    for (std::size_t i = 0; i < length; ++i) {
        std::uint32_t at = address + std::uint32_t(i);
        raw(at, bytes[i]);
        bool first = line(at, 0, bytes[i]);
        bool second = line(at, 1, bytes[i]);
        hits += first || second;
    }
    if (cached) *cached = hits;
    return true;
}

/* extra.bin ("region ADDR BYTES" with "tap tick rec"): an 8-byte magic
 * "SC64XTR\0", then little-endian u32 version and region count, then per region
 * u32 KSEG0 address and u32 byte count; then one record per recorded follow-up
 * read: u32 step, u32 tick, u32 tick-change ordinal, then each region's bytes in
 * guest order, in declaration order. Regions are fixed once the file is opened. */
constexpr unsigned SC64_EXTRA_VERSION = 1, SC64_EXTRA_MAX_REGIONS = 16;
constexpr std::uint32_t SC64_EXTRA_MAX_BYTES = 1u << 20;   /* all regions together */
inline bool sc64_capture_region_ok(std::uint32_t offset, std::uint32_t bytes, std::uint32_t used, unsigned count) {
    return count < SC64_EXTRA_MAX_REGIONS && bytes > 0 && bytes <= SC64_EXTRA_MAX_BYTES &&
           used <= SC64_EXTRA_MAX_BYTES - bytes && offset < SC64_CAPTURE_RAM && bytes <= SC64_CAPTURE_RAM - offset;
}

/* Output budget: the frontend counts every byte it writes to its run directory (tap.bin,
 * extra.bin, ticks.raw, dumps, states, frames) and stops with exit 10 before a write would
 * pass the budget, or leave less than the reserve free on the output volume (`available` is
 * the free space last measured, `since` the bytes written after that measurement). */
constexpr std::uint64_t SC64_OUTPUT_CHECK_BYTES = 16ull << 20;   /* re-measure free space this often */
/* A byte count from the environment: 1 to 20 decimal digits, no sign, no overflow. */
constexpr bool sc64_capture_u64(const char *text, std::uint64_t *out) {
    if (!text || !*text || !out) return false;
    std::uint64_t value = 0;
    for (unsigned n = 0; text[n]; ++n) {
        if (text[n] < '0' || text[n] > '9' || n >= 20) return false;
        std::uint64_t digit = std::uint64_t(text[n] - '0');
        if (value > (~std::uint64_t(0) - digit) / 10) return false;
        value = value * 10 + digit;
    }
    *out = value;
    return true;
}
constexpr bool sc64_capture_budget_ok(std::uint64_t written, std::uint64_t bytes, std::uint64_t budget,
                                      std::uint64_t available, std::uint64_t since, std::uint64_t reserve) {
    if (bytes > budget || written > budget - bytes) return false;
    const std::uint64_t used = since + bytes;
    return available >= used && available - used >= reserve;
}

/* FNV-1a, used only to localise a full-RAM hash difference to a 64 KiB region. */
inline std::uint64_t sc64_capture_fnv64(const std::uint8_t *p, std::size_t n) {
    std::uint64_t h = 1469598103934665603ull;
    for (std::size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}
/* tap.bin ("tap tick rec"): an 8-byte magic, then little-endian u32 version,
 * record-header bytes, globals bytes and row bytes. Each record is
 * SC64_TAP_HEADER_WORDS little-endian u32 (step, tick, phase, live, seconds,
 * pause halfword, selector, running byte, countdown, table offset, flags, rows
 * written), the globals in guest byte order (the four 12-player bank arrays at
 * 0x800B1D44, then the 12 RNG bytes at 0x800EFF20), then the rows written, each
 * a big-endian u16 index and 312 bytes as sc64_capture_live_rows gives them.
 * Flags: 1 rows written (the follow-up read one step after a tick change),
 * 2 table present, 4 a dual cache-alias read differed, 8 bad or asymmetric links.
 * tools/parity_decode.py reads it. */
constexpr unsigned SC64_TAP_HEADER_WORDS = 12, SC64_TAP_FILE_HEADER_BYTES = 24, SC64_TAP_VERSION = 1;
inline void sc64_capture_put_le32(std::uint8_t *p, std::uint32_t v) {
    p[0] = std::uint8_t(v); p[1] = std::uint8_t(v >> 8); p[2] = std::uint8_t(v >> 16); p[3] = std::uint8_t(v >> 24);
}
inline void sc64_capture_tap_file_header(std::uint8_t *out24, std::uint32_t globals_bytes) {
    std::memcpy(out24, "SC64TAP\0", 8);
    sc64_capture_put_le32(out24 + 8, SC64_TAP_VERSION);
    sc64_capture_put_le32(out24 + 12, SC64_TAP_HEADER_WORDS * 4);
    sc64_capture_put_le32(out24 + 16, globals_bytes);
    sc64_capture_put_le32(out24 + 20, 2 + SC64_UNIT_BYTES);
}
inline void sc64_capture_extra_file_header(std::uint8_t *out16, std::uint32_t regions) {
    std::memcpy(out16, "SC64XTR\0", 8);
    sc64_capture_put_le32(out16 + 8, SC64_EXTRA_VERSION);
    sc64_capture_put_le32(out16 + 12, regions);
}
#endif
