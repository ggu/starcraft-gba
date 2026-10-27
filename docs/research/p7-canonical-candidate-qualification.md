# Canonical Protoss P7 candidate qualification

This note records qualification of the canonical-render P7 campaign candidate.
The default Protoss profile now exposes P1–P7; explicit `protoss-p6` remains a
separate P1–P6 product with its SCP6 identity and generation-30 save format.
Together with Terran 10 and Zerg 6, canonical campaign coverage is 23/30.
This does not complete the Protoss campaign: P8–P10 and the near-30-fps goal
remain open.

## Canonical host profile

The host tests used the P7 candidate composition from `PROTOSS_P7_CANONICAL_FLAGS`
with the prepared P1–P7 sprite bank, full canonical drawing/minimap/facing,
weapon and death art, format-2 audio, P7 runtime and typed generation-43 save
codec, and the P7 FACT V2/contact profile. The sprite bank was generated from
the pinned local USA ROM and palette remap into an ignored directory; none of
those inputs or generated assets are tracked. The ROM remains identified as
`SCP7`, save generation 43. Explicit `protoss-p6` remains `SCP6`, generation 30.

The following checks passed on the host using the same canonical vector:

- P7 campaign startup and save fixtures, including rejection of a generation-30
  P6 save without mutating the live P7 state.
- Normal-input campaign controller runs: P1, P2, P4, P5, P6 and P7 reached
  their strict source win predicates. P7 won at frame 39,738 after 39,976 input
  calls with all three heroes alive. P3 reached its 180,000-call test limit
  without winning.
- The P7 fixed-input replay independently reached the strict source win on the
  1,667-transition tape. Its host trace and 67-record expected-output file are
  retained under `.local/build/p7-canonical-qualification-20261028-v2/`.
- The 18-match skirmish matrix (two maps, each human race, one to three AI
  opponents) completed without fixture failures. Outcomes included one win and
  17 valid losses. The menu-driven save/reload continuation fixture also
  passed.

### P3 baseline comparison

The P3 run under the canonical P7 feature vector did not win by its 180,000
call limit: at frame 180,612 it retained 38 enemy buildings, 12 side units and
6 side buildings. To distinguish a profile regression from an existing
controller limitation, the same P3 host fixture and input policy were compiled
under the retained canonical P6 flags. The full trace files are both 461,901
bytes and have the same SHA-256:

`ac20bd5117b5a7f388d52b8d1ff69a0adb495dcec9dd081f7fb47421d48120e0`

The comparison was repeated after rebasing onto the current source: both
current P3 traces are 461,901 bytes and have the same SHA-256 shown above. The
observed P3 test-limit result is therefore baseline-identical under the tested
P6 and P7 vectors. This candidate does not change P3 behavior to force a pass;
the result is recorded as a current controller limitation, not a P7 runtime
regression.

## Canonical ARM and SRAM qualification

The standalone canonical candidate and replay ROMs linked with the repository's
unchanged RAM guard assertions. The standalone playable ROM has
`__bss_end=0x0203de70` (400 bytes below the limit) and
`__iwram_end=0x03004558` (2,728 bytes below the limit). The fixed-input ARM
qualification used the same canonical P7 source and presentation profile. Its
two-process SRAM replay saved during the ordinary pause-menu flow at frame
28,000, loaded from a fresh title screen, and continued to the strict source
win. The prefix matched 1,321 host records; the resumed segment matched 591.
The saved payload was 16,292 bytes. The continuation retained sequence 1 and
wrote a valid sequence-2 slot.

Both emulator processes passed stack canaries: prefix used 5,036 bytes with a
6,740-byte margin; resume used 4,128 bytes with a 7,648-byte margin. Across
the prefix and resumed tail, the replay reached the strict source win at frame
39,738 after 39,976 calls, with three heroes alive and all 1,667 input events
consumed. The ARM SRAM result is recorded in the qualification bundle
manifest. A separate linked full-tape replay ROM's host build matches all 67
reference snapshots. The paired prefix/resume ARM run supplies whole-tape ARM
coverage; the standalone full-tape replay ROM itself was not launched.

The canonical standalone output is `StarCraft-Protoss-P7-GBA.gba`; explicit
`protoss-p6` remains `StarCraft-Protoss-P6-GBA.gba` with its prior identity.
Both products reject each other's incompatible save generation rather than
silently migrating or replacing saves.

The frozen 237 authored `common/` and `starcraft/` files match the candidate
source. The 32 MiB standalone playable ROM has SHA-256
`a65b295b5c9e46ba0e29615cea1d9c61076e22cc4a16b252f3fbc86bfe770206`.
The paired ARM SRAM result record has SHA-256
`224bcd3b8034e51dd3fe9e4a3c26e6342b65ded1189436479b647b464846426f`.
These identities describe private local artifacts; no ROM or imported asset
is distributed with the source.

## Bounded performance sample

A separate canonical ARM probe checked 600 opening updates and 1,400 attack
updates against 21 state records. Mean full update-plus-draw costs were
2,155,200 cycles in the opening sample and 2,426,615 cycles in the attack
sample. These bounded results are well below the 30-fps target; they are not a
mission-wide average or a physical GBA measurement.

## Limits

This evidence does not qualify native enemy-AI equivalence, arbitrary save
compatibility, physical GBA behavior or 30-fps performance. The skirmish matrix
checks completion and save behavior; its losses are valid terminal outcomes,
not proof that the AI is balanced. The recorded-input ARM replay and save
continuation do not establish arbitrary player strategies, native AI behavior,
physical GBA behavior or mission-wide 30-fps performance. The campaign status
is 23/30, with P8–P10 and the performance target unfinished.
