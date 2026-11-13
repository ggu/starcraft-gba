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
