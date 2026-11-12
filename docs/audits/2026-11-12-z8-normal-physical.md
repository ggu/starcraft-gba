# Carried Z8 on the normal Z10 cartridge

The retained Z10-carried controller tape for Eye for an Eye still wins on
`6e843d814da131950c92da79901985fc2e05ff78`. A new ASan/UBSan HOST run used
the same Z10 flag vector as the normal cartridge, including packed building
rendering. A local test hook hashed the covered route state immediately after
each game update and again before the next. No observer changed that state.
The run produced the same 561,911-byte tape and reached frame 45,468 at
call 46,487. It destroyed both source Nexuses, retained Kerrigan at 400 HP,
and reported no scheduler faults. The controller and game source did not
need changes.

The normal Z10 cartridge reached the same WIN from a fresh HOME boot in
mGBA. The 46,487-call HOST tape contains 5,605 held-key transitions and an
ordinary pause-menu save; the cartridge followed those buttons without
loading a save. Seven additional A presses and releases dismissed authored
text at the same simulation frame, bringing the total to 46,501 physical
updates. The runner made no guest-memory writes.

All 19 HOST checkpoints matched across 16 named fields, including each
Nexus's health, Kerrigan, the economy and supply, and the three guard counts.
The southeast guard moves after a raid. The observer used the actual HOST
guard center at each checkpoint to count nearby ARM actors; a first pass
using the initial center understated that guard's late count. The center
only defines where to observe, and no controller logic runs in the ROM.

The result had both starting Nexuses gone, Kerrigan at 400 HP, the human
scheduler outcome set to victory, the primary objective and campaign
completion bits set, and zero scheduler faults. Other enemy buildings
remained; destroying the two Nexuses is the mission objective.

The mission uses the authenticated Z8 trigger program, which binds 39
trigger rows. The adapter treats each destroyed starting Nexus as a human
kill because only the human fights those two Nexus owners. This is the
documented stand-in for the original game's per-player kill census; the
route does not claim native kill attribution or AI parity.

The full replay averaged 1,242,715 guest cycles per main loop (13.50
emulated updates per second). That figure includes rendering, the UI and
presentation waits. It is below the near-30-fps target and is not a
physical GBA frame-rate measurement.

- ROM SHA-256: `6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
- ELF SHA-256: `9bbe176b7d2916f945aa75691feccc2d1743e8b9e18b6e40807c99e05883c307`.
- HOST tape SHA-256: `b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb`.
- Full physical win receipt SHA-256: `d5e20e7bdb6ece945732c92f535f99afe8f6c81a4e8ef901876483a2b80ebfc1`.

This qualifies one complete route from HOME. The replay did not test
loading a save on the Z10 cartridge.
