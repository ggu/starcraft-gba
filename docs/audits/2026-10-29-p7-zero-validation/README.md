# P7 zero-record validation optimization

The P7 cloak cache distinguishes unused Unit records from dead or marked
records by their complete object representation. Its old `memcmp` against a
static zero Unit repeatedly scanned unused records, both during cloak
refreshes and through the mission's live-world validator.

`sc64_bytes_are_zero` reads an aligned word at a time, with a byte prefix and
tail. It checks every byte, including padding, without reading past the
range. The P7 caller still checks exactly `sizeof(Unit)` bytes; no Unit,
save payload, cache field, cadence or gameplay rule changes. NULL remains
false. The original linked ARM zero Unit is 56 bytes and every byte is zero;
the host object representation is also checked by the actual-Unit oracle.

The GCC helper disables interprocedural specialization so typed callers
cannot cause the compiler to remove its alignment handling. Host Clang uses
the byte-oriented interface with `noinline`. Word loads use `may_alias`;
other compilers retain the byte loop. The new public helper has no imported
game dependencies.

## Local verification

- `make test-sc64-zero-bytes` passes 951,562 ASan/UBSan cases against `memcmp`,
  including eight pointer offsets, empty/NULL ranges, every nonzero byte
  position through 256-byte ranges, exact allocation ends and randomized
  ranges through 512 bytes. This target is included in the default tests.
- A separate sanitizer oracle against the actual 56-byte Unit representation
  passes 80,168 cases. The original ARM zero symbol and generated aligned
  `ldr` loop were inspected; there is no `memcmp` in the optimized helper.
- The exact tracked P7 ordinary-input host and ARM replays match all 20
  sampled state digests and the terminal state. They use the current START/L
  input tape and a witnessed Attack-Move, with no world-state writes.
- A separate pixel-instrumented copy of that current replay explicitly
  requires P7 campaign mode. All 21 world records and 21 RGB555 records
  (including the duplicate terminal frame) match fresh host goldens, with
  every ARM capture in Mode 4. Its timing is excluded from the comparison.
- Captured canonical generation-43 cache/aura fixtures pass after correcting
  only the expected generation in ignored test copies. Their original
  generation-40 assertion failures are retained; the unmodified immediate
  save/first-tick equivalence fixture passes. These are fixture results, not
  fresh-process SRAM or full-mission qualification.
- Default build and full tests pass: 649 Python tests, 20 optional skips,
  eight profile checks and the C suites, including the new byte oracle.

All four normal ARM products pass the unchanged linker assertions, ROM
headers and 32 MiB size limit. Their BSS and IWRAM endpoints do not grow:

| Product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `0x0203708c` | `0x03004db8` |
| Zerg Z6 | `0x020370f8` | `0x03004f58` |
| Protoss P7 | `0x020382b8` | `0x03004b70` |
| Showcase | `0x0203df38` | `0x03004fd0` |

Normal Terran and Zerg ROMs are byte-identical to the preceding main build.
The P7 replay retains 9,588 bytes of measured stack margin; this harness
measurement is not a normal-product stack qualification.

## Bounded campaign timing

The baseline is main runtime `1f875e1` with the replay repair from #135;
`e75036c` adds no runtime difference. Both runs use the captured normal P7
flags and existing immutable prepared inputs. Range work and contact
grouping are OFF. No other private optimization is combined with this one.

| Window | Frames | Baseline mean full cycles | New mean full cycles | Reduction |
| --- | ---: | ---: | ---: | ---: |
| Opening | 600 | 2,041,279 | 1,875,549 | 8.119% |
| Attack | 1,400 | 2,226,503 | 2,105,917 | 5.416% |

Full cycles include update, audio, rendering and presentation. These means
correspond to about 8.95 / 7.97 fps at the GBA clock, still far outside the
559,241-cycle budget for 30 fps. Opening peak is unchanged at 11,181,679
cycles; attack peak changes from 2,903,615 to 2,893,425. No full-campaign,
steady-30-fps or physical-device performance claim is made.

All owned emulator processes used isolated configuration/save directories
and were terminated and waited on.

## Evidence scope and retention

Ignored evidence is under `.local/build/p7-empty-unit-zero-v1/` (private
prototype), `p7-zero-adopted-qualification-v1/` (tracked-source replay and
oracles), `adopted-zero-default-checks/` and
`adopted-zero-normal-products/`. The first normal-product postprocessing
attempt used an incorrect baseline filename; all links had completed. The
correction reuses those exact ELF/raw ROM files and records the proper
`product.raw.gba` comparison without recompiling or overwriting them.

An earlier RGB smoke run used the previous skirmish harness through its
captured argv, despite a newly copied probe file. Its actual compiled source
and raw records are retained and its scope corrected to skirmish; it is not
P7 campaign-render evidence. Pixel-instrumented runs are separate from the
timing comparison above.

Source-only GPL-3.0-or-later; no game data, ROMs, saves, screenshots or private
traces are tracked. Checks ran locally with existing prepared inputs. No
Actions, imported Make recipes, asset generation, GDB or SD writes.
