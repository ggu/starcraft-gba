# Protoss opening Shield Battery on Blood Bath

Two unmodified USA StarCraft 64 Free for All runs on Blood Bath have a
passive Terran human. Run 5 seats one Protoss computer. Run 1 seats Zerg,
Terran and Protoss computers. A span in parentheses ends with the first
sample where the event is visible. [Native capture method](2026-11-19-n64-skirmish-ai.md)

| Observation | Native solo, game frame | Native mixed, game frame |
| --- | ---: | ---: |
| First Shield Battery foundation, raw 172 | (3,898, 4,047] | (4,028, 4,229] |
| First completed Shield Battery | (4,464, 4,639] | (4,643, 4,776] |
| First Assimilator sample | 6,407 | 6,197 |
| First Cybernetics Core sample | 9,317 | 12,550 |

Both owners place one Battery after reaching 15 completed Probes, two
completed Gateways and two completed Pylons. Neither owner has an
Assimilator, Forge or Cybernetics Core at placement. The two-Nexus
Forge and Cannon opening from the separate keepalive run is distinct.

## Shared planner

The control at `f6869829` starts gas and the Cybernetics Core before
the second Gateway. The shared GBA planner now sends the second Gateway
forward and requests the first Battery when the owner has 15 live Probes,
two completed Gateways and reaches update 5,058. The time floor comes from
the earliest first Battery sample at the existing 5/4 source clock. The
Probe queues the normal paid placement, with the regular construction
reservation and power check. A walking Probe keeps its reservation. A
placed foundation releases the opening plan while warp-in continues.

The opening applies only to Protoss AI owners on Blood Bath in Free for
All. The ordinary planner remains available if the owner already has
gas or Core construction, faces armed air, has a worker target below 15,
or lacks the Battery catalog record. The ordinary plan also resumes when
the window closes at update 7,746, based on the earliest native first
Assimilator sample. This lets late or interrupted openings continue into
the normal tech tree. Save files keep their existing record layout. The
current world and construction orders supply the planner's inputs.

## GBA qualification

The four prepared host profiles produce the same results while the human
stays idle. The audit observes one foundation purchase on a powered tile
with exactly 15 live Probes and two completed Gateways, reconciling the
bank and supply without any granted resources.

| First event, GBA updates | Native solo at 5/4 | Native mixed at 5/4 | GBA solo | GBA mixed |
| --- | ---: | ---: | ---: | ---: |
| Battery foundation | 5,058 | 5,286 | 5,167 | 5,152 |
| Completed Battery | 5,798 | 5,970 | 5,729 | 5,714 |
| Assimilator foundation | 8,008 | 7,746 | 5,204 | 5,192 |
| Cybernetics Core foundation | 11,646 | 15,687 | 5,314 | 5,291 |

Native columns convert the first sample using integer division, so they
are upper observations. The port observes the same order through the
Battery purchase. Gas still begins roughly 2,600 to 2,800 updates before
its first native sample, and the Core first appears roughly 6,300 to
10,400 updates earlier. The timing of subsequent tech, placement near
the base entrance, AI use of Battery recharge, and the separate Bunker
and Cannon openings remain open.

All four regular 32 MiB ARM ROMs retain the control's IWRAM and EWRAM
link endpoints. Ordinary physical-button replays on each ROM first see
the solo Battery at update 5,173 and the mixed Battery at 5,161, with
the mixed warp completed by 5,725. The normal Terran control already has
gas and Core at its first sample, 4,801, and has no Battery by 6,001.

For each ARM profile, the solo pause menu saves at the 5,173 sample
while there is exactly one paid Battery under construction. HOME+L in
a fresh emulator restores the same live structure and per-owner banks.
After 600 updates, both the uninterrupted and fresh routes reproduce
the same structure slots, bank values, queues, units and mineral fields.
There is no duplicate Battery or second charge on the resumed route.

| Profile | Solo ARM opening and fresh load receipt | Mixed ARM opening receipt |
| --- | --- | --- |
| Terran | `3e9283eb7ded2981206dd849942a0caf60acc63da59e8a7ca6f138b3f7756975` | `c936d13b8ad5f9665ef1271bdf5dc1138afb95de836807428316bf8825e039af` |
| Z10 | `6201ea5144e09b6410ba85d16a34e16b5ea5e4869a5bf1263246eb0ba64bc1a9` | `d52f2a09f0779a721f9fdf4c4d0bb2406c20cd704a221c61359a49ce074d6f64` |
| P10 | `23f62fd7b2441041e558e382cabfa472112d068b3e0b167c5e30b27fc9170a24` | `6d08c0a9bdee9c798705194908f22a7104a5e7a8350e051b711efbbfb014a501` |
| Showcase | `392d62ea1be37f947c5d4c0c05143e3d0e3162e22f44d63b5fc9d5c866561999` | `2bd6a3dccf8917a55a82c9fad1bc032642bdc064a89d8eeee6f4a3250561e1e5` |

### Current release composition

P10 and Showcase linked again with the typed-building decoder from
release `8318fa1d` and retained their previous RAM endpoints. The paired
ordinary solo and mixed observations, paused SRAM hashes and resumed
worlds all match the first Battery builds. Terran and Z10 use the other
building codecs, so their first Battery builds cover the combined source.

| Profile | Combined solo and fresh load receipt | Combined mixed receipt |
| --- | --- | --- |
| P10 | `5276ed939d8c61ab5bed2099ce11d36a14b5570302eb8f16fad83a56d8221787` | `91e7c177091af6383969dd263a4118af818ca05982fe8f137202e77e437d4ec0` |
| Showcase | `2a4e3552343ae144a7ba829b6eb9fa39bf6bf60030dafcc74b8c63bb8fbaa8e4` | `6de3f58440f936f92abece43e069f1b454f013d31ad21c27e1ab3accbd485909` |

## Regression cases

With the prepared inputs, build `build/test-sc64-skirmish-ai-progress-terran`
and its `zerg`, `protoss` and `showcase` counterparts. Run each with
`--map 1 --human t --bots p --opening-battery` and repeat with
`--bots ztp`. The same matrix passes `--opening-defense` with `--bots z`
and `--bots ztp`, `--first-wave` with `--bots p`, and
`--resume-at 5200 --resume-len 600` with `--bots p`.

| Captured evidence | SHA-256 |
| --- | --- |
| Unmodified native solo census, run 5 | `81247f8f94c472f2d70754a32b7d2634354420171bddf025b2146c11afc58c4c` |
| Unmodified native mixed census, run 1 | `9ab2e19766f935ad5c30c2c78cb51eda3967c0b6a2724f025ac63e092f5b4ca6` |
| Terran ARM solo control | `34d70205a94810f114b6f2a3c973e87483970dafd4cd7676e4b7785e1b38a105` |
