# Z7 host SRAM and untimed replay repair

The HOST backend previously started its static 32-KiB save array at zero. A
real ordinary-input Z7 prefix in the retained mGBA run started without a seed
and, after its first save, left the unused 16-KiB slot entirely `0xff`. The
strict Z7 journal verifier likewise treats a slot as unused only when every
byte is `0xff`. This evidence supports an erased initial HOST cartridge for
this emulator comparison; it does not establish the power-on contents of every
physical cartridge.

The host array now initializes once at process startup, rather than at every
`gba_init`. Thus a save staged before initialization and one retained across a
subsequent game boot both survive. The ARM save backend and serialization are
unchanged. The HOST Z7 replay also stops emitting zero-valued pseudo-cycle
records; its logs remain untimed. The comparator still rejects any timing
records in an untimed log and retains its strict journal checks.

The earlier button-save attempt on the zero-filled HOST backend remains
unqualified: its second slot was all zero, and the strict comparator rejected
it. No save bytes were modified after the fact to pass validation.

Targeted checks: `test-gba-host-save-boot`, `test-scarab-save-selection`, and
`test_sc64_z7_arm_replay.py` (16 tests) passed with the native build lock; the
source-boundary checker passed. Actual fresh guest load and continuation must
still be checked against the authenticated tape before any busy-window use.
