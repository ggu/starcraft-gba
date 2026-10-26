# Z7 full-render probe audit

This audit records the optional generation-36 Z7 full-render candidate. The
candidate uses the normal Z7 menu/title integration and keeps all 231
canonical scenario rows. It is an integration and rendering probe; it is not
a completed-mission or hardware qualification. The source base is commit
`9bd1ce6`; changes for this candidate are build/profile-only.

## Reproduction

With the local Z6 and Z7 inputs already prepared, run:

```sh
make zerg-z7-full-render-probe
make test-sc64-z7-full-render
```

The ROM target is `build/StarCraft-Zerg-Z7-Full-Render-Probe-GBA.gba` (SCZR).
Generated inputs and run artifacts stay in `.local/` and `build/`.
The verified padded ROM is 33,554,432 bytes, SHA-256
`c25e2163d15db9d77747c7702cb6022fc1c0747d2d9123d5cf2d788f04dc2ec7`.

## Evidence

- Host full-render suite passed: startup/profile coverage, save rejection,
  frame coverage, and Scourge production/combat fixtures. The framebuffer
  comparisons matched the reference across camera positions, fog, overlays,
  transitions, mutation, save/load, and restart.
- The normal-menu save fixture reports a 16,296-byte `RaceSave`. In the
  two-process ARM restart, both 16,296-byte SRAM journal payloads were
  byte-identical (SHA-256
  `ec28b80e8065f6de50869ae35780e944779b736754abbdda29222e638485bc73`, tag
  `0x53435a54`): phase one wrote slot 0, sequence 1; phase two wrote slot 1,
  sequence 2. Six positive worker depot caches were present before save and
  immediately after reload. The resumed run reached frame 11,726 with no
  fault, win, or loss.
- Paired ARM presentation replay covered 12,000 input ticks and 34 projection
  checks, each projecting 18 state fields, eight cursors, and eight timers.
  Both builds completed all checks with zero mismatches. This does not compare
  the full world state or pixels, and the run did not finish the mission.
- Mean update cycles were 494,739 candidate / 490,366 reference; mean
  audio/draw cycles were 574,894 / 536,484; mean full-frame cycles were
  1,069,633 / 1,026,851 (+4.17%). The emulator-equivalent rates are 15.69 fps
  and 16.34 fps, respectively. These measurements do not establish hardware
  performance or 30 fps.
- Recorded memory guards: IWRAM 19,624 bytes with 856 bytes margin; BSS
  253,200 bytes with 256 bytes margin; ROM load margin 1,388,104 bytes.
- The source-only host verification ran `./dev test` (554 Python tests) and
  four full-render fixtures; all passed. No generated art or private harness
  code is part of this audit.
- `./dev doctor`, `./dev build`, and the indexed source-boundary check passed.
  The default original-art ROM remains byte-identical at SHA-256
  `2b89b77d3ab284d42bdf862d55ed85e37ceae2c70e785634055f9f3231cec19a`.

The depot-cache load fix keeps a valid worker's committed mid-haul destination,
resets route scratch, then validates positive cache entries once against the
restored world and clears stale entries. The feature remains behind its opt-in
compile flag; standard products and profiles without that flag are unchanged.

The Z6 canonical profile remains unchanged. Staged campaign qualification
remains 22/30. This audit does not claim a Z7 win, hardware qualification, or
30-fps qualification.
