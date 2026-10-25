# Z7 defense target qualification

This adds an explicit experimental Z7 build for the existing
`SC64_ZP_DEFENSE_TARGETS` path. Canonical race builds are unchanged. The
reference is the sight-span probe; the candidate adds only fresh defense
owner/flight lists. Both include the stock and sprite fixes merged in #25
(`544f087`). No production combat implementation changes in this follow-up.

The lists rebuild at the defense phase, after mission and production changes.
Target selection still rechecks live actors, uses the strict reference radius,
and resolves equal distances to the lowest unit slot. An invalid owner falls
back before applying damage. The lists use bounded local storage rather than
new actor, BSS or save fields.

## Local semantic checks

```sh
make test-sc64-zp-defense-targets test-sc64-z7-defense-targets
python3 -m unittest tests.test_sc64_z7_trace_parity
make zerg-z7-defense-targets-probe
python3 tools/sc64_z7_trace_parity.py PRIVATE_TRACE.json --variant defense-targets --output-dir .local/build/defense-parity
```

The aggregate now includes the P5 shuttle profile. Fixtures cover 800 randomized
snapshots, target ties, exact boundaries, earlier kills, invalid-owner fallback,
Spore ground/air filtering, and completed morph/owner/death changes made before
the defense phase. Independent expected damage checks cover Sunken Explosive
damage against small, medium and large units, shields and armor. ASan/UBSan,
Z7 runtime/save checks and the five Python parity-tool tests passed.

Two fresh authenticated normal-input controller traces passed full per-update
host comparison: the baseline prefix (12051 updates) and early-gas prefix
(12000). Neither is a mission win. Their final shared-state streams match
exactly between reference and candidate.

## ARM measurement

A private harness includes the current `starcraft/game.c` directly. It replays
the first 12000 updates of the baseline input tape from the normal menu, with
input events converted from host one-based ticks to zero-based driver ticks.
It measures Timer 2/3 CPU cycles around update, then audio and render. Render
includes the existing two-frame page-flip gate and any wait it performs.
Separate reference/candidate ROMs, logs, saves and emulator
configurations are used; audio/video synchronization is disabled. No GDB
server or debug-state writes are used. Observer hashes run outside the timed
interval; that observer work can still affect cadence between samples.

| Measurement | Sight-span reference | Defense-list candidate |
| --- | ---: | ---: |
| Mean update cycles | 589058 | 491452 |
| Mean update/audio/render cycles | 1115920 | 1033218 |
| Cycle-equivalent frames/second | 15.03 | 16.24 |
| Updates above 561792-cycle two-VBlank budget | 11884/12000 | 11884/12000 |

The sampled mean update cost falls by 16.6%, and total timed cost by 7.4%.
All 25 ARM checkpoints, including startup, have identical common-state hashes
and state summaries between the two builds. Active checkpoints retain the Z7
map/context binding, prevalidated source tables and zero runtime fault. At
12000 updates both are at gameplay frame 11726 with started mask 42 and AI mask
15. Hashes cover selected shared fields at 500-update checkpoints, rather than
every transient or the entire runtime. They are pairwise ARM evidence, not a
claim of complete host/ARM state parity.

The production probe links with IWRAM end `0x03004bd0` and BSS end
`0x0203de00`. The save fixture retains a 16296-byte payload. The existing
IWRAM, BSS and 16368-byte save guards were not changed.

This is an improvement in one active campaign prefix, not near-30-fps,
mission-completion, full campaign, cold SRAM or physical hardware proof.
Rendering remains substantial. Private tapes, benchmark drivers, ROMs and
logs stay under `.local/build/z7-defense/`; none are published.

## Canonical race baseline

A separate current-source ARM sample used normal-menu map 1, human Terran,
and three AIs (Zerg, Protoss, Terran) in each canonical race build. The first
9000 updates remained nonterminal. Mean total cycle cost was 749011 for the
Terran build, 579439 for Zerg and 649314 for Protoss: respectively 22.40,
28.95 and 25.84 cycle-equivalent fps. Rendering was the largest measured
slice. These are one fixed setup and bounded prefixes, not a complete map,
race/opponent matrix or hardware frame-rate qualification. Their private
measurement report is under `.local/build/canonical-perf-audit/`.

All checks ran locally. GitHub Actions remain disabled/manual-only.
