# T5 from HOME on the normal Terran cartridge

The retained T5 controller tape began after a compile-gated probe had already
started Revolution. The T10 test fixture can now record the ordinary HOME
route: open the mission menu, go right twice, select T5, and skip the briefing
with Start. It releases Start on the first gameplay update. The validator
checks the recorded menu inputs and accepts older probe tapes as well.

The controller's Supply Depot placement search asks the engine for reachable
sites. Those queries can warm route fields used by later game updates. The
fixture now restores the covered route state after scoring the candidate
sites. In the debug run, hashes also check that no observer changes that
state between game updates. Neither the mission scripts nor the runtime
were modified.

Two ASan/UBSan HOME runs built with the normal Terran T10 flag vector from
`6e843d814da131950c92da79901985fc2e05ff78` produced byte-identical winning
tapes. The run ends at game frame 122,281 after 123,792 calls, with Kerrigan
and Raynor alive, a player Command Center still standing, no enemy buildings,
the primary objective and completion bits set, and zero adapter faults.
The validator accepts its 7,208 held-key transitions and 150 observations.
The legacy probe fixture also still wins at its original frame 126,331.

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0 -DROUTE_PURITY_DEBUG' build/test-sc64-full-t5-home-run
./build/test-sc64-full-t5-home-run --limit 200000 --trace .local/sc64/t5-home.json
```

The unmodified normal Terran T10 cartridge won from a fresh HOME boot in
mGBA at frame 122,281. All 150 observation records matched at 139 distinct
calls, including the paid Starport, Control Tower and Wraith wing. The
human trigger outcome was victory, the primary and completion bits were
set, both heroes were alive, and the player kept a Command Center. No
enemy buildings or trigger faults remained.

Fourteen authored text pages took ordinary A presses and releases. Each
left the simulation frame unchanged and preserved the held gameplay keys.
The replay therefore used 123,820 physical updates for the 123,792-call
HOST tape. The emulator runner made no guest-memory writes or SRAM loads.

The full replay averaged 792,269 guest cycles per main loop (21.18 emulated
updates per second), including rendering, UI and presentation waits. It
does not meet the near-30-fps target or measure physical GBA frame rate.
This qualifies one route from HOME; it does not test SRAM reload.

- ROM SHA-256: `a5375ee144fc105c072410385b2b2ed0a6f31d5abe883f4bbad6b4ff54c7f337`.
- ELF SHA-256: `d17801881a4badb0152f9169f5c7c0cadebb79832365f77fbddf54431c1b5913`.
- Winning HOME tape SHA-256: `5547d53fc2fccf6fa802f4fff9165525f78cc5e80a72f39de2963db312ae70d9`.
- Full physical win receipt SHA-256: `345efc2a25d17ebfb53f473770b256b3b3eec441a0924afd18db3f6943c1df85`.

## Exact November 13 KODAK cartridge

The same winning HOME tape ran on the unmodified KODAK Terran ROM
(`cbba1c5`). All 150 observations matched, and the same 14 gameplay
pages were acknowledged with physical input while the simulation frame
held. It won at physical call 123,820, game frame 122,281, with Raynor
and Kerrigan alive, a surviving Command Center, 15 paid Wraiths,
no enemy buildings, and no trigger faults. The win's objective flag
matched the retained trace. No save was supplied or cartridge rebuilt.

The exact cartridge averaged 784,492 guest cycles per update (21.39
emulated updates/s), including rendering and display waits. This closes
one fresh HOME win on the exact KODAK ROM for each of the ten Terran
missions; see the [campaign matrix](2026-11-13-normal-campaign-matrix.md).
It does not measure real hardware or verify every campaign branch or save.

- KODAK ROM SHA-256: `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8`.
- Matching ELF SHA-256: `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8`.
- Physical observer SHA-256: `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549`.
- Exact physical replay receipt SHA-256: `25b010e84ebd8df7bc9c5b5e433dee5c67e3a8f3d79a139c478643690ffd5448`.

## November 20 fresh SRAM and result autosave

The normal Terran ROM built from `afffebec` replays the retained
paid-air probe tape with the earlier frame-126,331 win. Its gameplay
starts at the frame-zero boundary. The observer opens Revolution
from HOME, reveals all nine briefing pages and presses Start. It
then supplies the unchanged gameplay button transitions from the
tape and reads guest state using the matching ELF.

At gameplay call 73,391, frame 72,076, the first attack of the
12-Wraith wing has been issued. Raynor and Kerrigan are alive, the
Starport and Control Tower are complete, and all 12 Wraiths are
selected. Start pauses and B writes bank 0. A fresh emulator loads
that record at HOME with L. The observed world, campaign and
Revolution trigger state, control selection and cursor are the same
as at the save. The camera derived from the cursor also matches.

All 162 source observations match at 148 distinct gameplay calls.
The fresh process matches 94 observations at 81 calls after loading.
The observer acknowledges 14 authored gameplay pages with the
simulation frame held. At frame 126,331, Raynor and Kerrigan survive,
16 paid Wraiths remain, all hostile buildings are gone and the
recorded trigger outcome is victory with zero faults. Bank 1 records
the result and leaves bank 0 byte-identical. A third process loads
the frozen victory, restores selected unit slot 2 and exports
identical SRAM.

The measured 127,959 controller calls average 800,843 guest cycles
per main loop, or 20.95 emulated updates per second. The largest
update takes 34,735,863 cycles. These timings cover rendering, UI,
pause, save and load. Physical GBA performance remains unmeasured.

Use the pinned local inputs to repeat the check.

```sh
python3 tools/sc64_t9_normal_replay.py --chapter 5 --save-tick 73391 \
  --trace "$T5_TRACE" --rom "$TERRAN_ROM" --elf "$TERRAN_ELF" \
  --runner "$PHYSICAL_OBSERVER" --out "$NEW_RECEIPT_DIR"
```

| Input or receipt | SHA-256 |
| --- | --- |
| Normal Terran ROM | `01240c5e95c1f0e648028dad75005e3185904c02e6c9b0577aa368d14e7958f3` |
| Matching ELF | `85aa28c9206b11cd9baebdc9f33d8d9bf6de49fa624146480c1cf351d87dbc10` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Paid-air HOST tape | `1102fa2289606f3db298ed878a2ce4c2783a55f317d6f1062e41c5fc5b3cacc4` |
| Replay harness | `a478103d82f6983e8ec273c5426c8128a169154f31193882559cdee2fb4a07a2` |
| T5 checkpoint observer | `5ccb54216f058e356e2c4862cbf5cc50e38ba644b9ef57b1fdac3c8c6f6560c6` |
| Pause SRAM | `c05a204e4b6adff751c23e750f1324ebc9f8fe6d90a1315dea8e53b463fdf1eb` |
| Victory SRAM and fresh readback | `2ef8ee686161e3c8aa627ce3eb1162b3b1ea5f918fdd370033fe60b54a9306b6` |
| T5 replay and both journal checks | `2e92579a3f33d849900384133bd415ab926a3e292a30d82e4f3bba5fb6fcb22c` |
