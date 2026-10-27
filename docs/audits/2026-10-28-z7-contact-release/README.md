# Z7 CONTACT release candidate link (2026-10-28)

This audit records the standalone `zerg-z7` release candidate after selecting
the CONTACT movement implementation and removing debugger input storage from
the main loop. It is a link and host-replay qualification only. It does not
promote Z7 or establish a full ARM win for this exact flag vector.

## Selected release flags

The standalone `build/starcraft-zerg-z7.elf` rule starts with the existing
full-render Z7 feature set, including generation-36 saves, and replaces these
three movement flags:

- `SC64_COLLISION_CLASS_CACHE`
- `SC64_COLLISION_STEER_LIST`
- `SC64_COLLISION_PACKED_HIT`

with `SC64_CONTACT_CACHE`, `SC64_CONTACT_STEER_ROM` and
`SC64_CONTACT_STOCK_ROM`. It adds `GBA_NO_DEBUG_INPUTS` for this standalone
release only. The flag removes the debugger schedule/injection queue from
`common/main.c`; physical `gba_keys()` input, held/pressed edges, frame state
and cycle metrics remain. The historical full-render/steering probes and
their host fixtures retain their previous flags. The six-mission Z6 target,
save generation 30 and `zerg` alias remain unchanged.

## Evidence

The standalone ARM link used the existing prepared source and native inputs;
no assets were regenerated. Linker assertions passed. The map ends at
`0x0203d7e0` BSS and `0x03004d28` IWRAM. Against the limits
`0x0203e000` and `0x03005000`, that leaves 2,080 bytes of BSS margin and
728 bytes of IWRAM margin. The previous full-render link ended at
`0x0203dfe8` / `0x03004d10`; the combined CONTACT and physical-input-only
selection recovers 2,056 bytes of BSS while using 24 more IWRAM bytes.

The CONTACT host replay used the frozen ordinary-input winning tape and the
same `starcraft/game.c` source hash as this candidate. All 365 records matched
the reference byte for byte, including the terminal source win at host tick
209,485 / frame 205,764. The replay harness calls `game_update`, audio and
render directly; it does not exercise the standalone main loop or the new
debug-input compile gate.

A separate bounded late ARM comparison reported 5.7467% lower mean combined
cost for CONTACT than the prior movement path. That is a late-continuation
measurement, not a whole-mission average. The original full-render frequency
profile measured about 2.99 rendered updates per second; these results do not
meet the near-30-fps target.

The linked ELF SHA-256 is
`4f2849b0c1be37def053812b03226d4840d872bea22af9771ecd90b17a0672a3`.
The `common/main.c` and `starcraft/game.c` source hashes are respectively
`55330d9b9ccfb9a5687235a52a4e07d8d62e8c785f7a3e33394cd81338cf6938` and
`964726143e9d196dd2789b0369307a5c160230c19ae6ffc3277dd33ba7782e56`.
The matching CONTACT host replay log has SHA-256
`3446f601408b5ae361dfbdddbfebc4253062a9e5a2adb24bdddcf36995625ef2`.

## Limits

This exact release vector has not completed a full ARM replay or a fresh
process SRAM continuation. The older full ARM win and SRAM evidence used the
previous collision flags and does not qualify this link. No ROM was launched
in an emulator for this link check; the recorded artifact is an ELF, not a
fixed ROM. Z7 remains an unpromoted candidate; Z7
missions 8–10, broader enemy-AI fidelity and physical GBA performance remain
unqualified.

Private compiler argv, map, ELF, host output and source identities are retained
under `.local/build/z7-contact-release-20261028/` and
`.local/build/z7-contact-placement-20261028-v3/`.
