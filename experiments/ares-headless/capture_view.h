/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CAPTURE_VIEW_H
#define SC64_CAPTURE_VIEW_H
/* Pure helpers for the capture spike frontend: a line-wise cache-aware range
 * reader, the 14-bit pad mask, and unit-table locate, link and record checks.
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

/* FNV-1a, used only to localise a full-RAM hash difference to a 64 KiB region. */
inline std::uint64_t sc64_capture_fnv64(const std::uint8_t *p, std::size_t n) {
    std::uint64_t h = 1469598103934665603ull;
    for (std::size_t i = 0; i < n; ++i) { h ^= p[i]; h *= 1099511628211ull; }
    return h;
}
#endif
