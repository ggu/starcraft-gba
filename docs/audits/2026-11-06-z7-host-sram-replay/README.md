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

## Bounded post-cut busy diagnostic

An opt-in ARM diagnostic (`Z7_BUSY_DIAGNOSTIC` on the resume replay fixture)
loaded the unchanged, qualified seed with HOME L and replayed exactly the next
1,000 recorded calls, ticks 136,059–137,058. It reads game state, the indexed
front framebuffer and the full background palette after the timed call; it
does not write game state or SRAM. Those reads and diagnostic bookkeeping
can affect subsequent frame pacing. Both diagnostic ELFs linked with the normal
reserve guards: baseline BSS/IWRAM ends `0203d78c`/`03004f88`, candidate
`020369ac`/`03004f50`. Each run used the native mGBA backend, a 120-second
cap and the shared native lock. Wall times were 11.4 and 10.8 seconds.

Each variant matched both recorded tape observations and both HOST cargo-digest
checkpoints in this window, and the loaded SRAM image remained byte-identical
to the qualified seed. Ten sampled 32-bit hashes matched between variants for
selected unit/building/fog state and for the full indexed frame plus 256-entry
background palette. These hashes are sampled evidence, not a byte-for-byte
capture or proof of all state and pixels. Source/input manifests were stable.
The opt-in replay and `tools/sc64_z7_busy_compare.py` retain strict parsing,
seed and timeline checks; private ROMs, tapes, saves and logs are not committed.

The window contained 975 advancing PLAY and 25 nonadvancing PLAY calls; no
text-box/modal call occurred. Before and after, screen was PLAY throughout;
mode was normal for 968 calls and build for 32. Minimum counts were 48 friendly
and 169 non-side units, seven friendly and three non-side buildings. Resources
changed from 1,322 minerals/15,990 gas after the first call to 871/15,925 after
the last. The gameplay frame advanced from 134,787 before the first call to
135,762 after the last.

Advancing-call mean/peak cycles were 2,512,225/10,106,794 baseline and
2,446,291/10,087,988 candidate. Nonadvancing-call mean/peak cycles were
2,771,470/3,076,203 baseline and 2,660,158/3,059,425 candidate. Across all
1,000 calls, the 50th- and 10th-highest cycle counts were 6,132,061/6,446,624
baseline and 5,921,123/6,446,764 candidate. The advancing mean in this one
instrumented workload is well above the nominal roughly 559,241 cycles per
30-fps frame; this is not a physical-hardware result or an all-mode benchmark.
The result does not establish all-state/pixel equivalence or justify promoting
the cache flags. Detailed local artifacts are under
`build/z7-host-sram-contract/busy/`.
