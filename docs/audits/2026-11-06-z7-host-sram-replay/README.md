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

## Same-source coarse attribution of that window

An additional opt-in `Z7_BUSY_ATTRIBUTION` probe and its busy-only control were
built from the same source, recovered inputs and release vectors, differing in
the attribution macro. The four linked diagnostic ROM SHA-256 values were:

| Variant | Control | Attribution |
| --- | --- | --- |
| Baseline | `284f951c953a17de11a7633a5e6105fd850fc533b3c11cd4e5bf65b4d7c3872e` | `f410341ff2083f95adffc131a39337be5e62dc1dfca0da0ffdf2cf937d6a3d11` |
| Cache candidate | `b8a617f0f6a5d27e933dcae556d0974f1a342827aba04b1a78be223d8ece8978` | `ea0475038b614cdd5eaa7ba0a4034c525c5f2aa87820b863821503a4b4ce530c` |

All four runs used the validated seed above and the identical 1,000-input
window. Both pairs passed strict seed/load, unchanged-SRAM, recorded-observation
and cargo-digest checks. Their counts, workload endpoints and minima, and ten
sampled selected-state and indexed-frame/full-palette hashes matched across
control and attribution. Source/input manifests were unchanged during each
run. The extra collection uses ROM-placed, noinline diagnostic functions and
existing engine counters; the link reserve guards stayed enabled. Attribution
BSS/IWRAM ends were `0203d92c`/`03004f88` baseline and `02036b4c`/`03004f50`
candidate. The same-source controls matched the earlier busy-probe ends.

Observed advancing-call means were 2,512,225 cycles for baseline control and
2,513,001 for baseline attribution (+0.03%); the candidate figures were
2,446,291 and 2,446,201 (approximately -0.004%). These are one paired sample,
not a general bound on instrumentation overhead. In the baseline attribution
run, advancing-call means were 1,812,688 cycles for update and 700,185 for
render including presentation; nested update orders averaged 1,520,699, while
nested map and HUD averaged 434,913 and 152,747. These scopes overlap and must
not be added together.

The actual Z7 vector selects 14 maximum route slots; range-work may reserve two
of those planes. Existing engine counters reported 3,576 hits and 392 completed
route-field builds across the 1,000 calls. Among 975 advancing calls, 299 had at
least one build; the maximum was seven in one call. Update cycles averaged
3,827,509 on build calls versus 921,517 on the 676 calls without a build. The
slowest call, tick 136,592, took 10,106,676 cycles including 9,429,660 update
cycles and seven field builds. The cache candidate had the same route counts.
This is correlation, not isolated build cost: the shared build counter includes
ordinary goal and depot fields, and source-reachability work can call the same
ordinary builder. These data do not establish miss, eviction, invalidation or
caller proportions; the window immediately follows a fresh load and may include
cold-cache effects.

An existing bounded monotone shortest-step shortcut is an exact candidate for
further study, not a promoted change: the audited Terran/Zerg/P7 implementation
has independent FIFO-oracle tests, and this Z7 vector does not enable it. First
classify actual build sources, destinations and topology invalidations, then
measure any opt-in shortcut against the same seed and input with equivalent
state, pixels, memory and tails. Do not infer that enlarging route capacity is
correct or useful from this aggregate alone. Detailed pair and attribution logs
remain under `build/z7-host-sram-contract/attribution*-run/` in the isolated
workspace; private ROMs, saves and captures are not committed.

## Route-build source classification

A second diagnostic-only classification used the identical seed, tape window,
source and attribution control. Both normal/cache variants passed the same
strict load, observation, cargo, SRAM and ten sampled state/framebuffer-palette
hash checks. The classifier added 312 bytes of EWRAM and no IWRAM growth;
link guards remained enabled. The same-source control/classification ROM
SHA-256 values were:

| Variant | Control | Classification |
| --- | --- | --- |
| Baseline | `8233ec55f073abe0072e878d98a0dbe0b088b29ca82d2491caf8b64ab449c4d7` | `42f39935c17abd838b4f61538c522ef0bfde2899097e29432daf9ca770d00c47` |
| Cache candidate | `32bef07159369b63d1e694cabebcbcc29a8354e4ee2e32314b52e281bf9543eb` | `99373e79057afcc2fa29a303a5c08d648b0d77f07ef8e3200a675f686776f2b2` |

Both variants classified the 392 completed builds as 385 movement-goal,
zero component-reachability and seven depot builds. Twenty distinct
route/depot keys were observed, followed by 372 repeats; the bounded 128-key
tracker did not overflow. A build selected an invalid/unused slot 27 times and
replaced a valid slot 365 times. There was one explicit route reset and no
route-age wrap. All pre-call usable-slot observations were 14; this does not
establish that the range-work reservation never changed *within* a call.
The classification overhead was approximately -0.004% in each one-pair
advancing-call comparison, an observed difference rather than a general bound.

Movement misses dominate this window, so the pre-existing exact monotone probe
is a concrete opt-in candidate. Before considering any promotion it requires
independent blocked/partial/target-goal path tests, Z7's actual 14-slot vector,
and a measured same-source paired replay with state/pixel, memory and tail
checks. Valid-slot replacement and repeated keys also merit cache-policy
investigation, but do not prove that changing capacity is safe or beneficial.
The immediate-post-load window may contain cold-cache effects. Classification
artifacts remain local under `build/z7-host-sram-contract/route-*-run/`.

## Opt-in exact monotone route shortcut

The existing `SC64_ROUTE_MONOTONE_PROBE` is absent from the measured Z7 release
vector; this trial adds it only to private diagnostic builds. It uses the
existing bounded exact-shortest-step witness and falls back to the original
field builder when it cannot prove the same FIFO step. No route-slot, AI,
cadence or default-flag changes were made.

The independent FIFO dispatch tests passed in the existing regular, active
stride and partial-field configurations and in a new packed 128×128 Z7
configuration with 14 route planes and 12 usable during range reservation.
Explicit blocked-origin, blocked-target, enclosed-target and off-center/same-cell
target cases run against the independent FIFO oracle; the partial-field variant
confirms the monotone witness is excluded. These tests are not a proof of every
campaign or scene.

Four same-source diagnostic ROMs replayed the qualified seed and identical
1,000-call post-cut window. For each render-cache configuration the sole route
feature difference was the monotone flag. All four runs retained the strict
seed/load, unchanged-SRAM, two tape observations and two HOST cargo-digest
checks, with 975 advancing PLAY calls, 25 nonadvancing calls and zero text
modals. Workload, modes and all ten sampled selected-state and indexed-frame/
full-palette hashes matched across control and shortcut. Source/input manifests
were unchanged. Those sampled hashes do not establish complete state or pixel
identity. ROM SHA-256 values were:

| Render configuration | Control | Monotone shortcut |
| --- | --- | --- |
| Baseline | `87074fb09faf88e4cf666300359cc488fb1c12538808d99e10fc2577adb9ac3e` | `1b6f8b0c772d0c28db9695dc7d074c7ba56def70a19d1e523dbdc12f0b56144d` |
| Five-cache candidate | `5cc8bd0e08f146ab7495dc21ca0a57ea767ce91ff0e29caafbf3eca0bfa1bce3` | `62a02b649c89848aa9c8c515064e64e42b4ffd7d4bd30cca852bca351c3abfa3` |

Baseline advancing-call mean cycles fell from 2,512,864 to 1,679,145 (33.18%);
the five-cache figures fell from 2,446,063 to 1,612,046 (34.10%). Route
builds fell 392→26: movement builds 385→20 and depot builds 7→6. Valid-slot
replacements fell 365→5. For baseline, the 50th/10th-highest of all 1,000 calls
fell from 6,128,377/6,445,229 to 1,986,551/3,924,456 cycles, and the advancing
peak fell from 10,109,657 to 8,119,711. The HOME-L call was 68,458,162 versus
68,456,892 cycles; this is one load call, not complete boot timing. First-100
call totals were 280,519,785 versus 185,020,094 cycles. Diagnostics and
post-call state/pixel hashing may affect pacing; these are emulated diagnostic
measurements, not physical hardware performance.

All reserve guards stayed enabled. BSS ends were unchanged by the shortcut;
IWRAM ends changed from `03004f88` to `03004fe8` in the baseline and from
`03004f50` to `03004fb0` in the five-cache variant. The baseline is only 24
bytes below the existing IWRAM guard. Stack painting found 7,160 untouched of
11,416 painted bytes in each of these bounded runs; this is not a worst-case
stack proof. The ~1.68-million-cycle baseline mean remains about three times
the nominal 30-fps budget, and the immediate-post-load window can be cold.
Do not promote the shortcut or cache flags from this experiment alone; broader
uninterrupted gameplay, full-state/pixel, startup, memory and multi-mode proof
remain open. Artifacts are retained locally under
`build/z7-host-sram-contract/monotone*-run/` and `monotone/`.

### Fresh-start replay rejects the shortcut

The bounded post-load observation above does not establish equivalence. A later
same-source HOST replay started from erased SRAM and applied the complete retained
ordinary Z7 input tape. The control reached the recorded win at call 175,455 and
passed all 307 recorded observations. With `SC64_ROUTE_MONOTONE_PROBE` enabled,
the run diverged and ended in a loss at call 16,030; its strict comparison failed
with only 41 of 307 observations reached. Per-call diagnostic fingerprints first
differed in the actor arrays at call 817 (game frame 765), while the selected
world and displayed indexed-pixel/palette fingerprints still matched at that
call. Fingerprints are diagnostic hashes, not complete state proof. The first
mismatching route query and underlying cause have not yet been established.

This uninterrupted result supersedes the positive bounded timing as a release
candidate: **do not enable or ship this shortcut**. The original successful
control and failed shortcut outputs are retained under
`build/z7-host-sram-contract/host-full-traces/`; these private artifacts are not
part of the source tree. Normal uninstrumented ARM links for both variants of
Z7, Z8, Z9 and Z10 passed their existing memory guards, but linking does not
establish gameplay equivalence; Z7 shortcut IWRAM headroom was only 24 bytes.
The default remains unchanged.
