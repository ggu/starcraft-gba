# Z7 normal release ROM: physical-input qualification

This checkpoint advances the staged 30-mission goal. It does not promote Z7,
change the canonical T10/Z6/P7 menus, establish all campaign compatibility,
or establish the near-30-fps target.

## Unmodified release product

The normal zerg-z7 product was built from source 55751df with literal
production flags and debugging information. Its shared engine is public P8
232bc0f; the intervening changes are the controller and validation workflow.
The 109 fixed input hashes stayed unchanged. The release defines
GBA_NO_DEBUG_INPUTS, so it has no debugger scheduler. The initial attempted
scheduler prefix failed before starting the emulator and was retained.

The physical backend uses mGBA setKeys for ordinary buttons, stopping before
gba_keys reads them. It denies every guest-memory write in physical mode.
Its bounded update batches count only consecutive increments of the normal
demo_frame counter at that input-sample breakpoint. It neither embeds a key
tape in the cartridge nor replaces common/main.c. The original runner and
the release ROM were not modified.

| Artifact | SHA-256 |
| --- | --- |
| Normal release ROM | 59bd05448a2a579e6addd54cd37809a5678371b38feee0413bf9d52fff052b29 |
| Normal release ELF | f8fe62b4006c47fe521fddab7fc43c63490e11a9783957de9f5a24c93168a281 |
| Physical runner | 95b3a8ccf0d2a04b7820366ee484012912e2657acb5bddf940ce473c1f06a18b |
| Strict controller trace | 5121ed97ea0ea3d57697ce2c7fc57eac2082e3a457bb4ad98517f09ecf510489 |

The raw ELF payload is 27,973,428 bytes. Its payload after the ROM header
matches, and all remaining bytes of the 32 MiB cartridge image are FF.
Header, checksum, SRAM signature and unchanged linker guards pass.
BSS ends at 0203d6f8 and IWRAM at 03004ab0.

## Complete ordinary-input ARM witness

Driver f83b876, using the unchanged release product and physical backend,
replayed all 14,344 held-key transitions from the ordinary title and mission
menus. It won at input tick 179764 / simulation frame 177804. Every one of
the 303 retained observations matches the named Z7 model and action arrays,
unit/building counts, workers and economy. The final observation has zero
source enemy buildings, 12 workers, a victory screen and no mission fault.

This is not complete actor-array or pixel equality at every update. The host
policy that generated the tape observes internal state and uses path and
placement queries that may inspect hidden obstacle occupancy. It is not a
pixel-only or general AI. The physical ARM replay executes only its buttons;
none of those controller probes executes in the ROM.

## Performance is still a material hold

The normal release's own shipped counters report:

| Window | Mean full-loop guest cycles | Effective emulated loops/second |
| --- | ---: | ---: |
| First 1,000 updates | 969,279 | 17.31 |
| Updates 5,001 through 5,900 | 1,039,544 | 16.14 |
| Entire 179,764-update witness | 2,472,137 | 6.79 |

The complete witness totals 444,401,255,622 guest cycles, with a maximum
49,718,805-cycle loop. These counters include presentation waiting and UI;
they are emulator main-loop evidence, not physical-device FPS. This workload
is far below the intended rate. The early 900-update profile attributes
500,546 mean cycles to drawing and 459,146 to update work. The nested map and
HUD measurements must not be added to drawing again. Route-processing
spikes and later army workloads need larger improvements than the small
owner-query experiments that were already held.

The separate, unpromoted cache experiment 1948f6f frees 28,128 EWRAM bytes
and reduces drawing by about 10% in the same early window, but full-loop
mean falls only about 3.5%. Nine displayed buffers and palettes match there.
That is a bounded experiment, not a new release or the 30-fps result.

## Save and load findings

The ordinary pause-menu save at input cut 30204 produced a validated fresh
sequence-1 journal: 16,296-byte payload, mission 18, simulation frame 29480.
The saved image SHA-256 is
675d1edd990ec633c19145b35f390d672ddf1f21956820b2d274d309cbe3ec84.

Actual physical replay revealed that L consumes one title update. Its
release is the first resumed gameplay update, rather than an extra neutral
load update. Driver 63f63ee corrects that physical-only timing; the separately
supported scheduler path is not relabeled as a release-product proof.

After this correction, a fresh process restores the bounded persisted-world
fingerprint and all saved Z7 model fields exactly. The original loader
clears selected_building and does not restore command preferences, however.
The uninterrupted cut has building 57 selected and menu cursor 1; the fresh
load has no selected building and cursor 0. A later ordinary A command then
differs. Strict continuation stops at original tick 32792 with one additional
enemy building; that failed attempt remains retained and is not a saved win.

A separate opt-in shared save-context prototype uses the previously unused
state word 31, leaving the Protoss service clock at word 30 unchanged. Its
actual new save restores the selection and command preferences, and both
old-to-new and new-to-old Z7 reader compatibility have been observed. Its
complete resumed ARM witness is still pending at this audit checkpoint.
The prototype is not included or enabled by this source checkpoint.

## Verification and remaining scope

The exact earlier 197677a unprepared default build and test suite pass.
The current physical replay and validator Python suite passes 89 tests,
including its real-ROM backend protocol tests. Default native checks are
not relabeled as a fresh run of every later tool-only source revision.

The prepared Z7 controller, save, profile, frame and Scourge fixtures pass;
234 startup/save configurations and 18 terminal skirmish cases pass. The
complete prepared target still fails in older carried controllers. Z1,
Z3, Z4 and Z6 failures are retained; controller-only repairs are in separate
branches, with their own source-scoped wins or remaining failures.
They are not grounds to claim all seven missions already carried cleanly.

Arguments, logs, frame observations, counters, journal images, failed
attempts and input manifests remain in ignored qualification directories.
Source-only publication includes no ROM, extracted art, save or proprietary
capture. No hosted Actions or SD writes were performed.
