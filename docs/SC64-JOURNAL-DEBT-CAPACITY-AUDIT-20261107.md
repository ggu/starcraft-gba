# Existing journal capacity for an HP-debt tail — 2026-11-07

A universal, fixed 400-byte HP-debt tail cannot be appended to every legal
record in the existing two-slot 32 KiB SRAM journal while preserving the
current saved impact and Yamato combinations. Five of the six measured
production profiles have a counterexample. This is a capacity result for that
specific fixed append, not a proof that every variable-length or lossless
encoding is impossible, nor a hardware qualification. Runtime EWRAM room for
a 400-byte sidecar does not itself establish journal room.

## Measured layouts

The retained production-six qualification identifies source `10bda18e`, the
exact build vectors and input receipts in
`/Users/gabrieluribe/hacks/sc64-arbiter-perf-current-nov7/.local/perf-current/production-six/results.json`
and `margins.json`. Sizes below were read from each matching ARM ELF's DWARF
`RaceSave` and `RaceImpactRecord` typedefs and the
`RACE_IMPACT_SAVE_CAPACITY` enumerator. Each ELF SHA-256 was independently
checked against `margins.json`:

| Profile | Retained ELF path | SHA-256 | RaceSave | RaceImpactRecord | Saved impact capacity |
| --- | --- | --- | ---: | ---: | ---: |
| Terran | `/Users/gabrieluribe/hacks/sc64-arbiter-perf-current-nov7/build/starcraft-terran.elf` | `46e775edba39e313ef0d94992314f67d699b9211525094a9e0ded4422a8e733f` | 15,856 | 16,072 | 32 |
| Z7 | `/Users/gabrieluribe/hacks/sc64-arbiter-perf-current-nov7/.local/perf-current/production-six/z7.elf` | `0735929111f3ddcb9e928a919e79a9bfb0cea7405230186c6c4579d26bc953b9` | 16,296 | 16,368 | 8 |
| Z9 | `/Users/gabrieluribe/hacks/sc64-arbiter-perf-current-nov7/build/starcraft-zerg-z9.elf` | `f61b0890bb5ea51598def2a131397388ed2ce775a75be2c3c0e8e6d29339cac2` | 16,296 | 16,368 | 8 |
| P10 | `/Users/gabrieluribe/hacks/sc64-arbiter-perf-current-nov7/build/starcraft-protoss-p10.elf` | `4d91f9de1535dac8a51c9ec717ab4bff111829fed281ec901e0ae73e3d35b248` | 16,332 | 16,404 | 8 |
| Showcase | `/Users/gabrieluribe/hacks/sc64-arbiter-perf-current-nov7/build/starcraft-showcase.elf` | `dcefdbad3572c419617b2427d58d3c991b230448c8fd53e873b6105b37b42342` | 14,228 | 14,444 | 32 |
| Z10 | `/Users/gabrieluribe/hacks/sc64-arbiter-perf-current-nov7/.local/perf-current/production-six/z10.elf` | `31cb54b6c57e3f2043088e5dd0f36ed3dea8d302eadbca858b0b80fb113f66e0` | 16,296 | 16,368 | 8 |

The record is an in-memory scratch structure; its size is not always the
serialized length. P10's 16,404-byte scratch exceeds one payload because it
includes space for both full Yamato rows and the maximum no-shot impact block.
The wire writer omits inactive shots and verifies the resulting actual length.

For a retained ELF, inspect its layout with
`arm-none-eabi-readelf --debug-dump=info <elf>`: locate `RaceSave` and
`RaceImpactRecord` typedefs, follow each `DW_AT_type` reference to its
structure's `DW_AT_byte_size`, and inspect the
`RACE_IMPACT_SAVE_CAPACITY` enumerator's `DW_AT_const_value`. Verify the same
input with `shasum -a 256 <elf>`. Source definitions are in
`starcraft/sc64_race_save_types.h`, `starcraft/sc64_impact_save_types.h`,
`starcraft/sc64_impact_events.h`, and `starcraft/sc64_impact_adapter.h`.
No new compile or emulator run was used in this audit.

## Wire arithmetic

`common/gba.c` reserves 16 bytes of each 16,384-byte slot for its journal
header, leaving **16,368 payload bytes**. The shared transport trailer costs
16 bytes. A nonempty impact block costs an 8-byte header plus 6 bytes per row.
On Yamato-enabled profiles, `RaceSave` includes two 20-byte shot rows, but the
writer starts at `sizeof(RaceSave) - 40` and appends 20 bytes per active shot.
The table uses nonempty impact blocks and a maximum legal combination; it does
not imply every combination of all individual maxima fits P10.

| Profile | Maximum-state payload calculation | Payload | Slack | Fixed 400-byte tail |
| --- | --- | ---: | ---: | ---: |
| Terran | `(15856 − 40) + 2×20 + 8 + 32×6 + 16` | 16,072 | 296 | 104 bytes short |
| Z7 | `16296 + 8 + 8×6 + 16` | 16,368 | 0 | 400 bytes short |
| Z9 | `16296 + 8 + 8×6 + 16` | 16,368 | 0 | 400 bytes short |
| P10 | `(16332 − 40) + 2×20 + 8 + 2×6 + 16` | 16,368 | 0 | 400 bytes short |
| Showcase | `(14228 − 40) + 2×20 + 8 + 32×6 + 16` | 14,444 | 1,924 | Fits; 1,524 remain |
| Z10 | `16296 + 8 + 8×6 + 16` | 16,368 | 0 | 400 bytes short |

For comparison, P10 with no active Yamato shots and eight impacts uses 16,364
bytes, leaving four; with one shot and five impacts it uses 16,366, leaving
two. Terran with two shots and eight impacts leaves 440 bytes, but its existing
32-impact save capacity must also be preserved: that legal maximum leaves only
296. The live impact queue has 32 rows; the save capacity is profile-specific
and the existing writer refuses excess pending impacts rather than discarding
them.

The experimental typed-research wire stores 96 bits (12 bytes) in existing
validated building-row padding, not as an additional trailer. Those bits are
already used when that experimental format is active and cannot also be
counted as free HP-debt storage.

The separate Flash1M and larger-journal work remains an investigation: these
measurements do not switch the normal backend, establish save migration or
power-loss behavior, or prove flashcart compatibility. Any alternative compact
encoding must preserve all previously accepted actor, impact, transport, and
Yamato states, existing guards, and old-tag fallback, and requires independent
codec, resource, and hardware qualification.
