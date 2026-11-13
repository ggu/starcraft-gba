/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RESULT_TRACE_H
#define SC64_RESULT_TRACE_H
#include <array>
#include <cstdint>

// Pre-instruction events. Each opcode is checked against the pinned USA ROM.
struct Sc64ResultPoint { std::uint32_t pc, opcode; };
inline constexpr Sc64ResultPoint sc64_result_points[] = {
  {0x8008db2c, 0x3c058011}, // countdown (old value)
  {0x8008db58, 0xa4a2dbd2}, // scan begins, before reload and pending clear
  {0x8008dbbc, 0x0c023490}, // owner trigger dispatch
  {0x8008d9a8, 0xa0640000}, // published result write
  {0x801357a4, 0xa233004c}, // actor owner write
  {0x8008dbec, 0x8fbf0024}, // result pass returned
  {0x8008ce0c, 0xa462dbd2}, // countdown reset to 1
};
inline constexpr unsigned sc64_result_max_steps = 128;
inline constexpr unsigned sc64_result_max_rows = 1024;
inline constexpr unsigned sc64_result_fields = 10;
using Sc64ResultRow = std::array<std::uint32_t, sc64_result_fields>;
struct Sc64ResultTrace {
  std::array<Sc64ResultRow, sc64_result_max_rows> rows{};
  unsigned count = 0;
  bool failed = false;

  template<class Reader>
  bool add(unsigned step, unsigned event, std::uint32_t opcode,
           std::uint32_t arg0, std::uint32_t arg1, Reader read) {
    if (failed || count == rows.size() || event >= std::size(sc64_result_points) ||
        opcode != sc64_result_points[event].opcode) { failed = true; return false; }
    Sc64ResultRow row{step, sc64_result_points[event].pc, arg0, arg1};
    // Countdown, two packed groups of four pending/published bytes, and
    // the four active Blood Bath owners' building counts (one byte each).
    const unsigned addrs[] = {0x10dbd2, 0x10dbe0, 0x10dbe4, 0xb936c, 0xb9370};
    for (unsigned i=0; i<std::size(addrs); ++i)
      if (!read(addrs[i], i ? 4 : 2, row[4+i])) { failed = true; return false; }
    for (unsigned owner=0; owner<4; ++owner) {
      std::uint32_t buildings = 0;
      if (!read(0x10ac80+4*owner, 4, buildings) || buildings > 255) {
        failed = true; return false;
      }
      row[9] = (row[9] << 8) | buildings;
    }
    rows[count++] = row;
    return true;
  }
};
#endif
