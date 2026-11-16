# Packed source-sight rows in late Zerg battles

Z10's [cycle profile](2026-11-12-z10-battle-cycles.md) identified repeated
sight-span arithmetic as a simulation cost. The probe builds the exact
integer circle rows once into ROM. Each byte encodes both offsets from
the actor's center tile or an empty row. The runtime still clips to the map
and applies the same old/new row differences in the same order. Negative
centers and unsupported geometry retain the previous path.

| Normal Z10 campaign window | Mean loop cycles, stock → probe | Mean simulation cycles, stock → probe | p95 loop, stock → probe |
| --- | ---: | ---: | ---: |
| Calls 19,000–19,999 | 1,568,109 → 1,525,574 (−2.71%) | 791,327 → 749,235 (−5.32%) | 1,965,123 → 1,924,902 |
| Calls 35,000–35,999 | 1,995,868 → 1,939,680 (−2.82%) | 992,461 → 936,700 (−5.62%) | 2,375,134 → 2,314,352 |

The paired normal physical-input ROMs match all 34 gameplay checkpoints.
Twenty-four sampled drawn pages, palettes, camera positions, visibility
and exploration also match. The 51,200 generated rows match an independent
squared-distance scan; the asset-free sanitizer test runs the new variant
alongside the five existing full-grid oracle variants. Z10 verifies with
unchanged IWRAM (`0x03004ae0`) and EWRAM (`0x0203c9b8`).

The prepared Z10 fixture also compares the packed decoder with the scalar
row path for all phases and supported radii on both 64×64 and 128×128 map
geometry, including edge clipping. The pinned 1,000-call 64×64 skirmish
window matches its frame/screen and sampled actor counts. Mean full-loop
cycles fall from 1,368,916 to 1,356,372 (−0.92%); p95 falls from
1,726,162 to 1,717,394.

Probe ROM SHA-256:
`02fbc1319442a9ce82dc29e28f1435a3857a15ad1b99211e291991df973f3eee`.

With prepared inputs in a clean build tree:

```sh
python3 -m unittest discover -s tests -p test_sc64_z7_sight_spans.py
make LATE_ZERG_PACKED_SIGHT=1 verify-zerg-z10
```

## Current split-cache cartridge

With the packed walking and split terrain caches already active, the same
two 1,000-call windows average 1,532,236 → 1,491,881 (−2.63%) and
1,914,170 → 1,859,653 (−2.85%) full-loop cycles. Both runs match all 34
sampled gameplay checkpoints and all 24 sampled drawn pages, palettes,
camera positions, visibility and exploration.

The prepared phase/clipping fixture passes. All four Z7–Z10 opt-in
cartridge verifiers pass their original guards. The exact enabled Z10
ROM is `31d54a25fa9b45fd23d8e3b70fa03949f8d9548bf09aa20ef017d92584fbd47c`.
Switching back to 0 in the same build tree rebuilds the normal Z10 ROM
and reproduces its stock SHA-256
`b1a0831adbf2d1b15cf8ae669047be43a64547246815f7a0470eda955252fc9f`.

## Full Z10 win and current custom comparison

The same enabled cartridge loaded ordinary SRAM and won Full Circle with
physical buttons at game frame 58,795 (controller call 60,834). All 60
named gameplay checkpoints matched the qualified witness, with zero
mission faults and nine ordinary text dismissals. The first 34 also
match the stock raw unit, building and mineral snapshots. There were no
guest-memory writes.

The current split-cache stock and enabled ROMs also match every sampled
state in the pinned 64×64 custom window, calls 10,860–11,859. Mean
full-loop cycles fall from 1,338,144 to 1,325,662 (−0.93%); p95 falls
from 1,696,800 to 1,687,167. Mean simulation cycles fall from 788,304
to 776,058 (−1.55%).

## Per-race supply build and legacy continuation

The normal Z9 cartridge with packed rows loaded ordinary SRAM from the
earlier Z8 mission in a fresh process and won with physical buttons at
frame 45,468 (call 46,487). The 31 named observations, starting at saved
frame 16,660, matched the earlier qualified Z8 route. The cartridge was
`ec22e6ff01b2c0827833abd606f93160633a4ae143989cfbb069545c1915c90c`.
The default Z9 cartridge at `20469c2` repeats that win from the same
legacy SRAM, with all 31 raw and named observations identical, zero
mission faults, and four ordinary text acknowledgments. Its SHA-256 is
`93bafe49f7f0a22b713d90a2bface97070b2c19540d362958adf2448b92876e7`.

After the per-race supply changes (`20469c2`), the historical Z10 save and
button timings stopped reproducing the win with either sight path. A new
normal, flag-off Z10 run in mGBA made its save from the pause menu at
frame 2,527 (call 2,666). The exported SRAM was byte-identical to the new
host controller save. Both stock and packed normal ROMs loaded that SRAM
from the title screen in fresh emulator processes and won Full Circle at
frame 59,997 (call 61,938), with zero mission faults. The 61 sampled gameplay
states, including raw units, buildings and minerals, and the 24 sampled
rendered pages, palettes, camera positions, visibility and exploration
were identical. Each run acknowledged nine text boxes with physical buttons;
the replay did not write guest memory.

| Current Z10 campaign calls | Mean loop cycles, stock → packed | Mean update cycles, stock → packed | p95 loop, stock → packed |
| --- | ---: | ---: | ---: |
| 19,000–19,999 | 1,854,119 → 1,802,117 (−2.80%) | 972,899 → 920,970 (−5.34%) | 3,077,396 → 3,069,981 |
| 35,000–35,999 | 1,849,036 → 1,791,044 (−3.14%) | 1,018,671 → 960,807 (−5.68%) | 2,822,000 → 2,809,013 |

The maximum loop changed by +0.13% and −0.25% respectively; the large
stalls remain. These measurements still exceed the 30 fps budget and
do not establish physical hardware frame rate.
The packed Z10 link adds 51,616 ROM bytes before padding and keeps the
same IWRAM and EWRAM limits. Both cartridges are still 32 MiB.

- Flag-off ROM: `e6eb6eeba139332eaff64c2af54f6602e222d78763dbe57ad0beb4321b6e2a03`.
- Packed ROM: `0f144b2789f695658e09dd721d8b7349c29aacbde7edce93f476ce29b9c47608`.
- Ordinary physical SRAM: `740ffc1f24454f67bdf3ca954850016a47e672040fa33035bdef1412bedc1888`.
- Controller input trace: `bea076602f09b1c8496b8c3dfd5d49640be3a8352946763178f0ef40a8329310`.

Packed sight is on by default for the late Zerg layout in Z7–Z10,
including custom matches on those cartridges. Other layouts use a
different shared-span transition and need separate evaluation. To compare
against the scalar path, build with `LATE_ZERG_PACKED_SIGHT=0`.

All four default ROMs verify under the unchanged linker guards. The
default Z10 ROM is byte-identical to the packed ROM above.

| Default cartridge | IWRAM code end | EWRAM BSS end |
| --- | ---: | ---: |
| Z7 | `0x03004e68` | `0x02039a24` |
| Z8 | `0x03004ae0` | `0x02039ee4` |
| Z9 / Z10 | `0x03004ae0` | `0x02038ae8` |
