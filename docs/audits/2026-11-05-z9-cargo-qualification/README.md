# Z9 ordinary-input candidate on the cargo integration

At `e8b836f5`, the separate Z9 candidate won through ordinary menu and gameplay
inputs, with the cargo integration enabled. It is not promoted as the default
Zerg ROM. The authored town AI and trigger cadence remain approximations; this
result does not establish N64 combat equivalence.

The ASan/UBSan host run won at call 82,066, frame 79,409, with zero mission
faults. The Z9 game fixture and `make -k test-sc64-zerg-z9` passed. The host
full and fresh-process save continuation matched at 77 checkpoints. A tentative
controller change to assign loaded idle Drones failed this route at the existing
200,000-call limit; it was reverted. The original controller at the same
integrated cargo source and inputs reproduced the win.

The input-only ARM replay used mGBA 0.10.5 in separate processes, with no
state injection or debugger. Full replay matched host at 83 of 83 observations;
the prefix matched 7 of 7, and the fresh-process SRAM continuation matched 77
of 77 against the host full replay, reaching the same win. All reported
checkpoint digests were present. The pause save was taken at call 6,515, frame
6,292 with construction active. Its committed SRAM record matched the host
record byte-for-byte; the resumed SRAM preserved that record and added a second
record at victory. The replay digest covers selected world fields but does not
include worker flags, work timers or build fields, so it does not establish
full cargo-state equality at checkpoints.

Whole-tape timing was 178,930,138,262 cycles over 82,066 frames, or about 7.69
fps equivalent at 16,777,216 cycles/s; peak was 93,538,228 cycles. This is
well below 30 fps and does not measure physical hardware. The production ROM
and three replay ROMs linked within unchanged RAM guards. The production EWRAM
end `0x0203dff8` leaves eight bytes below `0x0203e000`; replay EWRAM ends at
`0x0203dfe0`. IWRAM ended at `0x03004bf0`, below `0x03005000`.

Inputs came from the local prepared receipt `.local/qualification/prepared-inputs.json`;
all 1,794 listed hashes matched before and after the runs. Private output logs,
tapes and SRAM remain in `.local/qualification/z9-arm-e8`; they are not source
release artifacts. The candidate still needs the separately owned late-Zerg art
work, carried-mission qualification on the final integration, and broader native
reference evidence before any promotion. Z10 has no qualified ordinary-input win.
