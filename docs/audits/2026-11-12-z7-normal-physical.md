# Carried Z7 on the normal Z10 cartridge

The earlier winning HOST tape for The Culling does not reproduce on the
normal Z10 cartridge from `6e843d814da131950c92da79901985fc2e05ff78`.
From a fresh HOME boot, the physical replay matched the first 133 observations
through call 49,034, game frame 47,760. At call 49,397, game frame 48,123,
the cartridge had 57 player units and the old tape expected 58. The other
named fields, including economy, enemy actor counts, trigger state and action
timers, still matched. The runner used ordinary buttons and read-only guest
observations. No authored text box interrupted this prefix.

Two fresh ASan/UBSan HOST runs from `6127370`, with route purity checks and
the same controller arguments as the old tape, both reported 57 player units
at that checkpoint. Packed building rendering on and off produced identical
tapes. Both runs reached the controller's limit at frame 200,283 with 14
enemy buildings, no adapter fault, and a full 240-slot live unit bank: 50
player units and 190 enemy units. The player had 3,970 minerals, 4,839 gas,
and only two Mutalisks. The first mismatching observation therefore also
reproduces in HOST; the older tape is stale for this runtime.

The old policy activated its paid air plan at frame 127,146, too late for
the controller to build the desired air group on this route. The carried
Z10 test now asks the controller to start that plan after frame 85,000.
It made that decision at frame 86,706, bought eight Mutalisks by frame
112,141, and won at frame 194,998 after 197,455 calls and 16,075 held-key
transitions. The threshold only changes which buttons the controller
chooses: Lair, Spire, Mutalisks and orders still go through the game.
Three ASan/UBSan HOST runs, including packed and unpacked building flags,
produced the same winning tape. The carried Z9 and standalone Z7 defaults
are unchanged.

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0 -DROUTE_PURITY_DEBUG' build/test-sc64-z10-carried-z7
./build/test-sc64-z10-carried-z7 --policy early-gas --policy-workers 6 \
  --policy-reserve 50 --policy-hydras 8 --policy-second-first 0 --policy-retreat 0 \
  --policy-home-radius 320 --home-army-r --start-delay 33 \
  --force-late-air-at-frame 85000 --limit 200000 --trace .local/sc64/z7-current.json
python3 tools/sc64_z7_normal_replay.py --physical --trace .local/sc64/z7-current.json \
  --rom build/StarCraft-Zerg-Z10-Transport1-GBA.gba --elf build/starcraft-zerg-z10.elf \
  --runner "$SC64_HEADLESS_REPLAY" --output .local/sc64/z7-current-physical
```

The unmodified normal cartridge replayed the replacement tape from fresh
HOME and reached WIN at frame 194,998 after 197,455 ordinary button updates.
All 253 unique checkpoints matched across 20 named fields; the last periodic
checkpoint and the terminal observation are at the same call. At victory,
zero hostile buildings and 13 hostile units remained. The source model had
started the victory trigger, all action cursors and timers were clear, the
campaign objective and completion bits were set, and no adapter fault was
reported. The runner made no guest-memory writes and used no SRAM input.
No authored text box appeared. This proves one complete route on this
cartridge; it does not cover save and resume or every actor field.

The full route averaged 1,794,976 guest cycles per main loop (9.35 emulated
updates per second), with a peak of 58,707,335 cycles in one loop. The
mean includes the renderer, UI and presentation waits. It is emulator
evidence, not physical GBA frame rate.

On this exact cartridge, a separate 16,226-call prefix averaged 1,180,361
guest cycles per main loop, or 14.21 emulated updates per second. Its last
3,993 calls averaged 1,330,628 cycles (12.61 updates per second). All 40
observations matched. These counters include the renderer and presentation
waits; they are not physical GBA frame rates. They do not meet the 30 fps
target.

Two 1,000-call samples on the replacement tape separate simulation from
the full main loop. Calls 9,000–9,999 averaged 470,898 simulation cycles
and 1,235,452 full-loop cycles (13.58 updates per second). Calls
15,000–15,999 averaged 481,929 simulation cycles and 1,278,526 full-loop
cycles (13.12 updates per second). Simulation already consumes 84–86% of
the 561,792-cycle budget for two GBA video frames.

- ROM SHA-256: `6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
- ELF SHA-256: `9bbe176b7d2916f945aa75691feccc2d1743e8b9e18b6e40807c99e05883c307`.
- Earlier winning HOST tape SHA-256: `5dbc798d0146349978c1e0ce7ced1fb8605c709a293713699c5e6957717452f7`.
- Current runtime with old HOST policy SHA-256: `33ad17ca8f227cf292e3b51ff2d01712f643fc916a4b97651b58ebb69a6cadb2`.
- Current winning HOST tape SHA-256: `e1c7693d633f1ecd486036e797baea555dcaddf1e0a409d4da397f3b2f3fbe17`.
- Full physical win receipt SHA-256: `b1c314a50b932778ca9af415d49a242117a80216eafb48b054a8755243e2a058`.
- Physical divergence receipt SHA-256: `bf8851593eaa11eedb6fce0198abb8d5f42997140a5245e5177bf71da09c3058`.
- Timing prefix receipt SHA-256: `85dcee94d5a8078795ce485bf2522cf8e0bdc4414416fd2462b303e38a00e376`.
- Simulation timing receipt SHA-256: `1f0504f50e6df2f0486829f4ed0c6131fad6f67a7ee11c4715400d3ef45d8794`.

## Fresh SRAM on the November 13 Z10 KODAK cartridge

The normal Z10 ROM from `cbba1c5` replayed the same winning tape through
call 188,324, game frame 185,926, with 248 matching named observations.
Ordinary Start then B wrote the first SRAM journal. The tool verifies the
32 KiB image, authenticated Z10 transport tag `0x56435a61`, payload version
`0x52435a61`, 16,312-byte payload, sequence 1, checksum, zero transport
trailer, erased alternate slot, mission 18 and exact game frame.

In a fresh emulator process, L restored the saved frame and the bounded
world fingerprint, model and economy exactly. The unchanged cartridge
does not restore the selected building. Without intervention, Select on
the second gameplay update opened Pause, and the R in the next movement
chord restarted the mission. The first retained post-load observation
therefore saw frame 1,911 instead of 187,840. This was a control-state
mismatch after a successful load.

The cursor still pointed at the same Hatchery. Replacing the first neutral
update with ordinary A reselected building slot 1 and advanced exactly
one game frame. The tape then cleared the selection as intended. The
remaining 9,131 gameplay calls and all five observations matched, and the
fresh process won at frame 194,998. The SRAM seed stayed byte-identical.
This proves a continuation with one explicit button repair; it does not
show that the cartridge restores selection or every transient UI state.

For the tape and matching build, add `--export-save --cut 188324
--save-format z10-transport1` to the physical command above. In a new
output directory, use `--cut 188324 --save-format z10-transport1
--save-input PATH/saved.sav --saved-state PATH/saved-state.json
--reselect-building 1` instead. The reselection option requires a neutral
first gameplay update followed immediately by Select, and verifies the
actual slot and frame before continuing.

- KODAK ROM SHA-256: `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907`.
- KODAK ELF SHA-256: `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1`.
- Observer SHA-256: `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549`.
- SRAM SHA-256: `632a64001689bbbc35ae7a2ca4cea20b93c94a1187c5ee7cdb95ffe88b05662f`.
- Save export receipt SHA-256: `bd83af7627678c2448007fc1cc095c20f40403659a69fd339032df06f8478eb6`.
- Verified reselection and victory receipt SHA-256: `9acb1daa2cfb68a9d458cb9d66b1fc078cb3e10220c45a2a9b2114b146bb86ab`.

## Uninterrupted HOME on the exact KODAK cartridge

The same November 13 KODAK Z10 ROM and held-key tape now also win without
loading a save. The physical-button observer starts at fresh HOME and
checks 253 named observations through call 197,455 and frame 194,998.
All five simulation-frame observations shared with the fresh SRAM route
match in the named state, excluding the different input counters. The
objective completed with zero hostile buildings, 13 hostile units, and
no observed trigger fault. No authored text box interrupted the run.

Use the physical command above without save or cut options, with the
KODAK ROM and ELF hashes listed in the preceding section. The full run
averages 1,787,077 guest cycles per update (9.39 emulated main-loop
updates/s), including UI and presentation waits. Its peak is 58,629,570
cycles. This qualifies the route in mGBA; the 30 fps hardware goal remains
open.

- Uninterrupted KODAK HOME receipt SHA-256: `8767402601941f660ce5cdfa31fd62f4d03c82f5fa31990d56fc945af7e45c57`.

The timing counters also separate sustained load from a single stall.
Across simulation frames 50,015–75,289 the mean is 2,385,709 cycles
(7.03 updates/s); across 75,289–101,083 it is 2,465,591 cycles (6.80
updates/s). The interval at frames 46,499–46,976 averages 3,753,052
cycles across 480 updates, with 236–237 live units at its endpoints.
That interval is exactly calls 47,758–48,237. The clock advances from
69,130,754,997 to 70,932,219,943 cycles, and 479 of the 480 calls land
in the highest recorded presentation bucket.
No interval between the 253 sampled checkpoints averages under the
561,792-cycle budget. These are whole-loop measurements, including
menu and presentation time. Profile the 480-update interval by stage
before attributing its cost to actor count or a specific subsystem.

`tools/sc64_z7_stage_window.py --first 47758 --last 48237` can replay
the pinned inputs above and sample every completed call in that window.
It takes `--trace`, `--rom`, `--elf`, and `--runner` with their corresponding
`--*-sha256` values. Its `--plan` check verified the actual ROM, ELF, tape
and observer, all 14 counter symbols, and the 131 checkpoints through the
window. For a replay, use `--output` with a fresh directory and omit
`--plan`. The tool checks the prefix and endpoints and reconciles the
sampled loop cycles with the independent accumulated counter.

The stock ARM run matched all 131 prefix checkpoints and captured all
480 calls. The 477 updates that advanced simulation averaged 3,766,477
cycles per loop. Simulation used 3,160,442 cycles, including 2,609,769
in movement orders (69% of the whole loop), 312,487 in other update
work, and 228,600 in sight. Map drawing averaged 417,654 cycles.

Excluding the first sample, 357 active calls with a changed route-build
counter averaged 3,142,755 cycles in movement orders. The 119 active
calls without a changed counter still averaged 1,025,223 in orders.
The counter increased by 614 between the first and last samples. The
largest loop was call 47,823 at 9,087,886 cycles, including 7,895,389 in
orders. These counters identify where to profile next; they do not say
which fills were avoidable or measure hardware FPS. Preserve route-cache
semantics and compare the same dense window before promoting a change.

- Dense stage receipt SHA-256: `93809b1993233b716cdc09c9b132a69a775c0e4747989045413d9d00402e72cf`.


### November 22 route observer correction

The newer rendered HOST controller needed a route-observer snapshot correction.
Its corrected 24,000-call tape matches 62 named ARM observations on #708;
a full HOST win is recorded separately. The exact full replay on #711 remains
in progress. See the [route observer audit](2026-11-22-z7-route-observer-purity.md)
for the evidence and limits.

### November 25 rendered-controller recovery

The prepared current HOST controller with ASan, UBSan and route-purity checks
regenerates the corrected 270,585-call tape. Normal inputs win at frame
267,585 with terminal consistency, zero adapter faults and zero enemy
buildings. The tape SHA-256 is
`022d0f08d4020ab86b3ac69fcb8dfe430b7837c3817d37ae785e241a62cd3ad1`,
matching the November 22 corrected tape. The padded-journal Z10 ARM ROM
replays it from empty HOME using ordinary buttons. All 206 named model,
entity-count and economy checkpoints match. It wins at the same frame
with seven enemy units, zero enemy buildings and zero adapter faults.

The full replay averages 1,313,688 guest cycles per update, about 12.77
emulated updates per second including rendering and pacing. This run
verifies the entire ordinary route to victory on this exact ARM build.
Full actor arrays and a fresh reload of the victory are outside this run.

| Input or receipt | SHA-256 |
| --- | --- |
| Padded-journal Z10 ROM | `72a3fcea4044358b2380795ded70d4cc4dd78119c4041e2c055bd1189f59ee01` |
| Matching ELF | `da20e5abc0457b5d60b6476fbdb4d9a06bf2f9756cb271963de63c908b1f14f9` |
| Full physical-button win receipt | `115a0db4473784c8b10927c20acd0f58dda1dae1972d365c9f04135b03f7a33f` |

### November 25 vitals release replay

The exact Z10 image from [#853](https://github.com/ggu/starcraft-gba/pull/853)
replays the same tape with physical buttons from a fresh HOME. All 206
named mission, economy and entity-count checkpoints match. It wins at
game frame 267,585 after 270,585 guest update calls. The result screen
reports victory, seven enemy units, zero enemy buildings and zero
adapter faults. This run observes counts and mission fields. A full
actor array comparison and a fresh reload of the victory remain separate.

Mean loop cost is 1,312,171 guest cycles, or 12.79 emulated updates/s
including UI and presentation waits. This rate covers the complete tape
on the vitals build. Hardware and the [subsequent renderer build](https://github.com/ggu/starcraft-gba/pull/854)
still need their own end-to-end observation.

| Input or receipt | SHA-256 |
| --- | --- |
| Z10 ROM | `08675d363c0f832d55f319d8ad7f8d9669f7dbcdaba7a3a57bd1673f78bb2c16` |
| Matching ELF | `fdfcfa7a83fb7eb3e47153dcfc229d37e9cc67df184d5f78119c2a106fce0b56` |
| Full physical-button win receipt | `441e02d324ebe378571134408e89bc7269279719614069ec361e7b918d1bd3d3` |

### November 25 selection-ring release replay

The Z10 ROM from [#854](https://github.com/ggu/starcraft-gba/pull/854)
replays the same normal-button tape from fresh HOME and wins at
game frame 267,585. All 206 named checkpoints match. The terminal
screen reports victory, seven enemy units, zero enemy buildings and
zero adapter faults. Over 270,585 guest calls the mean cost is
1,311,899 cycles, about 12.79 emulated main-loop updates/s with UI
and presentation waits. Exact latest-composition replays and
physical GBA SP timing remain open.

| Input or receipt | SHA-256 |
| --- | --- |
| Z10 ROM | `a061a9bd503522b79019f71bc4d766a5985ef4d9455afc9792e6d1d246a57980` |
| Matching ELF | `49980741d66e993a7732864cf5f7c3ea17825463a05baf1420bc2a773849b10e` |
| Full physical-button win receipt | `f1743c62926265ace2eb9c7dfc4a36b4c7a61c5f2c040dadc924a77c8c802ca0` |

### November 25 combined supply and outline release replay

The packaged Z10 ROM at `5f758095` repeats the full physical-button
replay from HOME. All 206 named checkpoints match and the mission
wins at frame 267,585 with zero adapter faults. Across 270,585 guest
calls, the mean corresponds to 12.83 emulated main-loop updates per
second, including UI and presentation waits. The test covers an
uninterrupted win. A full-tape pause and fresh victory readback on
this exact release remain open.

| Input or receipt | SHA-256 |
| --- | --- |
| Packaged Z10 ROM | `b954fde5512b949b9afeab93fc947cc56931b1be17f9c433f793dafd93fd56d5` |
| Full ordinary-button victory receipt | `554e608cb4039a38ddd8e6a1fb7af39b40cd510df8c7e395224997b26954c555` |
