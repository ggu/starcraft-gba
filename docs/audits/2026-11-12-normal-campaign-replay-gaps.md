# Normal cartridge campaign replay gaps

This inventory is pinned to the production source tree at `2c412e8`. A
current full result means an unmodified normal race ROM booted in mGBA,
ordinary button input from HOME or a genuine cartridge-format SRAM
resume, and a checked terminal win. A HOST controller win, a replacement
ARM loop, or a save that resumes for a bounded opening does not count
as a current full result. None of these are physical GBA tests.

| Missions | Best retained evidence | Full normal replay on this tree |
| --- | --- | --- |
| T1–T10 | [Normal-menu ARM wins across frozen Terran builds](../VALIDATION.md#canonical-terran-t1t10-profile-2026-10-23); the later canonical T9 replay uses the scheduler. | No exact current full physical result found for any of the ten. |
| Z1 | Carried Z9 [ARM test-entry win](2026-11-12-dense-point-flood.md#campaign-and-save-checks). | Open. |
| Z2 | [Carried Z9 HOST wins](2026-11-05-z9-carried-host/README.md). | Open. |
| Z3 | [Older normal Z7 physical win](2026-10-31-z3-normal-physical/README.md) and current Z10 replay below. | Full win from fresh HOME at frame 136,588. |
| Z4–Z6 | Carried HOST wins; earlier Z6 ARM and Z5/Z6 save coverage in [campaign notes](../RACE-CAMPAIGNS.md). | Open for all three. |
| Z7 | [Older normal Z7 physical win](2026-10-31-z7-normal-physical/README.md) and [carried Z9 test-entry replay and SRAM continuation](2026-11-05-z9-carried-arm/README.md). | Open on current Z10. |
| Z8 | [Normal Z9 physical win after loading legacy SRAM](2026-11-12-packed-sight-rows.md#per-race-supply-build-and-legacy-continuation). | Open on current Z10. |
| Z9 | Earlier [ARM and save continuation](../RACE-CAMPAIGNS.md) on the frozen Z9 integration. | Open on current Z10. |
| Z10 | [Normal composed Z10 physical win from genuine SRAM](2026-11-12-defense-combined.md#normal-z10-promotion-after-the-renderer-and-lobby-merges). | Full win from SRAM at frame 59,997; the production tree matches the qualified artifact. |
| P1–P6 | [P10-flag replacement-loop ARM wins](2026-11-03-protoss-p1-p10-evidence/README.md) and [older normal P10 physical save points](2026-11-06-early-protoss-save/README.md). | Open for all six; the older P1 tape did not win on the packaged ROM. |
| P7 | [Older normal P7 physical win](2026-11-01-p7-physical-prep/README.md); bounded current-family P10 replay and older early SRAM. | Open on current P10. |
| P8 | [Older normal P8 full physical win and SRAM resume](2026-10-31-protoss-cannon-lazy/README.md); P10-flag ARM replay with a replacement main. | Open on current P10. |
| P9 | [P10-flag replacement-loop ARM win and fresh SRAM continuation](2026-11-03-protoss-p1-p10-evidence/README.md#save-continuation). | Open on current P10. |
| P10 | [Earlier ARM replacement-loop win and SRAM](2026-11-03-p10-ordinary-input/README.md), plus an earlier normal physical continuation in the [project notes](../PROJECT-TODO.md). | No exact current full physical result found. |

This is a conservative audit of retained evidence: two exact current full
results (Z3 from HOME and Z10 from SRAM) and no exact current full result
found for the other 28 entries. It does not downgrade older wins or imply
that the remaining missions are unplayable.

## Carried Z3: controller reads no longer alter the route cache

The Z3 miner picks among visible or approachable mineral patches by
asking the real path engine. These are not read-only queries: a probe
can warm a route field that later affects ordinary combat or movement.
The controller now batches the mineral-ranking reads in the existing
host-only route snapshot and restores all covered route state before
sending buttons. It asserts that no game input occurs inside the
observer scope. Production code and mission data are unchanged.

With prepared Z10 inputs, two ASan/UBSan host runs under the normal Z10
flags both won at game frame 136,588, input call 139,009. The Chrysalis
kept 250 HP and five enemy actors remained, satisfying the source
presence condition. Of 115 observer scopes, one changed the cache and
was restored. `ROUTE_PURITY_DEBUG` checked the pre/post scope hashes.
The full traces are byte-identical, SHA-256
`bba07b5bef6a1c13ee1b39f01213f67d759002a7b4f47fd70f6bc11b1ff0644c`.

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0 -DROUTE_PURITY_DEBUG' build/test-sc64-z10-carried-z3
./build/test-sc64-z10-carried-z3 --policy hybrid --trace /tmp/z3-current-pure.json
```

The same tape's first 3,200 ordinary physical button updates ran on the
existing normal Z10 ROM `296bb98e174e079be581ecef2abe2fc96c2762c168c4cf21a8934735998a4a1e`,
whose runtime tree is identical to `2c412e8`. All 12 named mission and
world observations matched, through frame 3,000, including after the
frame-202 mineral query. The paired ELF hash is
`0daa2d6b8403b86bbb9094e53d0e3ce3b2e8e08d4ec6455cef669729f595cb00`.
This prefix made no guest-memory writes. It is not a full ARM win, a
full actor-state comparison, or a save/resume test.

### Full physical-button replay

A fresh emulator boot of the same ROM won through the normal menu at
frame 136,588, with the Chrysalis at 250 HP and five hostile actors left.
All 465 named observations matched the HOST tape. The recorded timeline
used 139,009 tape calls and 139,015 physical updates. The six extra
updates were ordinary A presses and releases for the three authored
victory transmissions. The replay checked that none advanced the game
frame and preserved the previously held gameplay buttons. It made no
guest-memory writes. The historical driver without acknowledgments
matched 463 observations, then fell behind when the text blocked the
simulation. Full replay receipt SHA-256:
`9e845def47d8b7b2b2cbf9261feef7d69263fcceff83465ee6ea2bf6776d3955`.
This proves one exact normal-ROM route in mGBA. It does not cover saves,
all actor fields, or the frame rate on hardware.

The remaining known cache-mutating controllers are Z4, Z5, Z6, T5 and
P3–P6. Z4's live mineral selection uses the same path helper as Z3;
its direct `sc64_route_next` calls are in failure diagnostics. P4–P6
call it directly during decisions, and Z5 explicitly rebuilds the path
grid. Snapshot the actual observer scope, including direct calls, then
require a controller-free tape match before treating a HOST win as
proof of the normal ROM.
