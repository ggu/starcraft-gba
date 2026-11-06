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

A regenerated ordinary-input HOST prefix reached tick 136,058 through the
recorded tape and the START/B button sequence. Baseline and candidate journals
were byte-identical and both passed strict `cargo-transport1` validation with
258/258 tape observations; no timing was claimed for either HOST run.

The first fresh ARM baseline loaded that journal with the HOME L input and
reached the recorded win. It matched 49/49 resumed tape observations and all
50 HOST/ARM cargo-digest records; its two journal records passed the strict
checks. Its first evaluation was unqualified solely because the validator had
assumed that a timed guest resume always has a timed prefix. The explicit
`--prefix-untimed` option now validates the prefix independently as untimed,
while preserving the timing requirement for the ARM resume and all seed,
loaded-state, timeline and journal checks. The retained baseline was rechecked
with this option and passed. Tests also reject timing records in an explicitly
untimed prefix and reject a corrupt seed. The original rejected result remains
in the isolated local evidence directory.

With the repaired backend, both ordinary-input HOST prefixes strictly passed
all 258 expected tape observations and produced byte-identical, valid 32-KiB
journals, SHA-256
`182407d91e782c4e879e65047223a70a0ec5f83bbec0131d4e205debddbd0d39`.
The cut was tick 136,058 and the saved frame was 134,787. Both independent
HOST HOME-L continuations passed 49/49 resumed observations, with identical
logs and continuation journals.

Fresh mGBA ARM baseline and cache-candidate replay ROMs then loaded those
unaltered journals using the guest HOME-L input and continued the original
tape to its win. Each passed the strict resume and journal validator, 49/49
expected resumed observations, and all 50 corresponding HOST/ARM cargo-digest
records. The baseline's originally rejected validator result is retained; it
was revalidated with the explicit independently untimed prefix option. The
candidate ran with that option from the start. Both ARM links retained the
normal reserve guards; their BSS/IWRAM ends were `0203d554`/`03004f88` and
`02036774`/`03004f50` respectively. Each guest run was capped at 360 seconds;
the baseline took 265.3 seconds and candidate 261.4 seconds wall time.
Relevant source, generated-input, tape, seed and ROM hashes were recorded and
unchanged during each paired run. Local logs and validators are under
`build/z7-host-sram-contract/`.

This qualifies the button-produced seed and sampled continuation for the
specified baseline/candidate ROMs. The 32-bit cargo digest covers selected
live-unit fields, and the controller records are sampled: neither is a
full-state or framebuffer/palette comparison. The reported guest timing covers
the resume only and is not a modal-separated tail analysis or proof of
30-fps behavior. Busy-scene pixel, memory and performance qualification remains
open; no cache flag was promoted.
