# T7 and T9 from HOME on the normal Terran cartridge

The retained host tapes start after a private probe entry deploys the
mission. The physical replay starts from fresh HOME on the unchanged
KODAK Terran cartridge (`cbba1c5`), selects each mission in the ordinary
campaign menu, and reads its complete briefing. It uses the original
held-key transitions for gameplay and acknowledges authored dialogue
with a press and release while the simulation frame stays unchanged.
The observer reads guest state and does not write it.

The Trump Card (T7) matches all 15 host observations across 38 guest
fields. The controller's last tracked carrier slot is used to observe
that actor after it stops carrying the emitter; it is not itself a guest
field. The replay reads 17 briefing pages and acknowledges four in-game
pages. Kerrigan survives, and the Psi Emitter reaches the goal even
though its carrier dies. The game wins at physical call 4,274, frame
3,931, with its objective and completion bits set and no trigger faults.
Mean loop cost is 850,073 guest cycles (19.74 emulated updates per second),
with a peak of 20,836,897 cycles.

## T9: New Gettysburg

New Gettysburg (T9) wins at gameplay call 47,984 (physical call
48,051, frame 47,266). All 51 host observations match across 18 guest
fields. The replay reads nine briefing pages and acknowledges 13 in-game
pages. Kerrigan survives, Protoss Force 2 has no buildings left, and
Zerg Force 3 retains 21. The objective and completion bits are set with
no trigger faults. The host controller's private `failed_action` flag is
excluded. Mean loop cost is 1,075,139 guest cycles (15.60 emulated
updates per second), with a peak of 27,042,746 cycles.

The first T9 attempt matched 45 observations through gameplay call
42,000 (physical call 42,049, frame 41,326) before its free-space guard
stopped the run. The full retry started from fresh HOME with the same
ROM, ELF, observer, and tape. The retry checked free space every 500
calls and wrote 32,783 bytes of result JSON plus 1,788 bytes of progress
log; the guest log was empty.

These checks establish one winning route per mission from HOME. They do
not establish hardware speed or fresh SRAM continuation. No cartridge
was rebuilt.

| Pinned input or receipt | SHA-256 |
| --- | --- |
| KODAK Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| T7 host tape | `7971ec31a860a4441c83c85437d45bce46d513b3e909b65dd56be0ccfc3df1d7` |
| T7 harness | `79fce0e5e8dae5cdf35dcbdc1f90fa82b536cb8f9467c209bec8b6f958c26336` |
| T7 physical receipt | `918cb43c2f4b41db00465c856e4f9c2d172513fecea1beda5d51b42b604b87a1` |
| T9 host tape | `f0342ae15aaef00f8a55581e2758a365d73692ec5388920eea8bf754d0cbf82f` |
| T9 early prefix receipt (through call 4,000) | `bc49b057ca97b7902546c732fd290790a0db617190a00a0b86df83c4878e4867` |
| T9 extended prefix checkpoint log | `b0fdc863ba61a395f87adbb578efda58a852859f1f2e850b7facb559f41b4251` |
| T9 full replay harness | `4a7ec8a17faa99c571a79a4c1c0c3b14dd62cbb80862c3411e3d63b458fa90ff` |
| T9 full physical receipt | `529c1ea2b3086cce6ef83512cb4b3203fb4bb21883c7e0a1f0877580008343e8` |

### November 20 current Terran fresh SRAM and result

The normal Terran ROM from release `06353a5d` wins New Gettysburg from
HOME after a pause save and a fresh emulator load. The sanitized HOST
controller provided 47,984 gameplay calls. The normal cartridge
matched all 51 checkpoints across 18 fields, including 12 after the
fresh load. Physical inputs revealed all nine briefing pages and
acknowledged 13 dialogue pages while the simulation was paused.

At gameplay call 36,000, frame 35,363, the player has no selected unit
or building and no held buttons. START pauses and B saves a 32 KiB image
with one checksum-valid bank and one erased bank. Sequence one uses
outer tag `0x76435442`, inner tag `0x72435442` and a 15,846-byte payload.
A new emulator loads it through HOME L. The named world state, full
late-mission state block, cursor and checked control fields match the
pause cut. The cartridge derives camera position from the saved cursor.
Both camera coordinates match at this cut, (508, 307).

Kerrigan survives at frame 47,266. Protoss Force 2 has zero buildings,
Zerg Force 3 retains 21 and the mission completes with zero trigger
faults. The victory autosave preserves bank one byte for byte and writes
sequence two with a 15,832-byte payload. A third emulator loads the
victory through HOME L, reproduces the terminal world and mission state,
and stays frozen through eight neutral updates with an empty impact
queue. Its 32 KiB readback is byte-identical to the victory SRAM.

The combined run averages 1,111,168 guest cycles over 48,062 main-loop
calls, including the menus and pause/load. That is 15.10 calls per
emulated second. Physical GBA timing remains unmeasured.

With prepared Terran inputs, generate the HOST tape using
`build/test-sc64-t9-campaign-run --limit 65000 --trace "$trace"`. Run the
[controller replay](../../tools/sc64_t9_normal_replay.py) with the pinned
ROM, matching ELF and port-free physical observer. Use a new directory
whose parent exists.

```sh
python3 tools/sc64_t9_normal_replay.py \
    --trace "$trace" --rom "$rom" --elf "$elf" --runner "$runner" \
    --out "$out" --save-tick 36000
```

| Pinned input or receipt | SHA-256 |
| --- | --- |
| November 20 Terran ROM | `b2997fdb7366dc1e69b4b81e7b0676d53181fbb6d9e9ac108eebbd3b95ec88e4` |
| Matching ELF | `f8411fea46d2a588f2fcecfe9fc0d5fd7e1b889cf83c6d0b975bf86b00deb46a` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Sanitized HOST tape | `3add75a0a7c7865ebbcf36794a41e71eb168aad2ad15d1d4f070d6dd1fcea2a8` |
| Replay harness | `ae3f0b11cf6724418d964252a4f2be6a6cab179e456034812fdcf4c28fe56f05` |
| Pause SRAM | `3ed3d68cb4819a43352f78e8f53c39025889ee8c883c2edac8a1f7ce0c174151` |
| Victory SRAM and fresh readback | `9b1afb8f41ed464b45efb5df8662172447f150dfb3719bebd5f16d1b5667436c` |
| Full replay and both journal checks | `e7c3bb4a61f8a0a9b8c04bcf946ccbceb3f0eaeadb78c3170b9768494e90360d` |
