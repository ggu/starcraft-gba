# Experimental ares v148 callback frontend

This source-integrated prototype is not a release tool, a UI control mechanism or a game-parity claim. It uses the official v148 internal C++ platform/node interfaces; see the source audit in `docs/audits/2026-11-06-ares-headless-feasibility`. It has no guest-memory inspection or write, debugger command, savestate injection, host-key event or screenshot API. A successful link alone is insufficient for runtime qualification.

The CMake injection is opt-in and requires an isolated clean official source checkout at commit `0aafd85789215e84e1e43415c07d4c88461b7899`. Configure upstream itself as the top-level project with `ARES_SKIP_DEPS=ON`, `ARES_CORES=n64`, optional targets and CHD disabled, `CMAKE_PROJECT_INCLUDE` pointing to `inject.cmake`, `SC64_PROBE_SOURCE` to `main.cpp`, and an explicitly identified `MoltenVK_LIBRARY`. CMake's upstream resource generator writes four ignored generated `resource.cpp`/`resource.hpp` files into that isolated source checkout; it is not an immutable-source build. Retain their hashes. Do not configure against a protected shared source checkout.

The original neutral attempt timed out at its first run step. The N64 VI inactive path sets its `refreshed` flag without calling `screen->frame()`; the desktop worker repeatedly calls `root->run()`, and screen delivery is threaded. The revised probe counts steps and callbacks separately. After initial 1×1 blanks it captured the Expansion Pak and Blizzard startup screens and then the main menu. An ordinary Start, Down and A sequence reached the Wasteland briefing; further Start pulses reached its map. The earliest sampled map at step 5851 shows Terran units and 10/18 supply; a sample at step 7051 shows 40 minerals. These are visual observations, not calibrated display aspect, timing equivalence, or full N64 parity. Local manifests, input events, callback metadata and sampled frames preserve the runs.

Upstream macOS configuration disables build RPATHs, so the executable weakly links `@rpath/libMoltenVK.dylib` without an embedded path. The wrapper checks the exact installed application library path and SHA-256 `432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99` before launch, and supplies its parent via a child-only loader environment. It records the hash again after execution. No global settings or libraries are changed.

`run.py` checks the exact upstream commit, clean tracked source, owned ROM SHA-256, new output directory and 1–9000 run-step limit; it copies the ROM into a new isolated input directory (so nearby patches are not applied), uses isolated MIA save and system directories, sets a 180-second process limit, and records ROM, binary, source, generated-resource, library and frame hashes. `neutral` sends no emulated button; `tap`, `start`, `mission` and `advance` use explicit bounded schedules in `main.cpp`, with releases between pulses. This is a **boot probe**, not a gameplay or mission replay. It records input polls, audio callbacks, status and run steps; copies video while each callback buffer is valid; and retains metadata for all callbacks, samples every 360th non-1×1 callback plus selected calibration steps, and limits captured images to 32 frames and 64 MiB. Input events record polled A/Start/Down/C-Left and analog axis values and observed steps. The run loop stops once it observes the image cap. Raw `.argb` files are packed native-endian `u32` values; `frames.csv` records dimensions and the current step observed asynchronously, which is not a precise frame-to-step correspondence. A successful process and callback do not guarantee a nonblank game image.

N64 option return values are checked, but upstream v148 returns true even for unknown option names; only its exact source-recognized names are used, and GPU initialization must also explicitly report success. There is no validation that output matches installed ares, an N64 console, or a StarCraft game update. Lock, free-space/output budgets, timeout, renderer configuration and detailed outcome logs are required for each later run. No ROM, frame, save, upstream source, generated resource or executable belongs in source control.

A later ordinary-input calibration selected a Marine, whose panel displayed 40 HP, and an SCV, whose panel displayed 60 HP with Build, Repair and Move controls. The X and Y values are signed axis values delivered through the pinned core’s input API; input logs record polls. One sampled frame in an unsuccessful intermediate selection attempt showed transient pixel corruption, with later frames recovered; this remains a renderer qualification limitation.

The SCV Build control opened a mission-specific grid and highlighted Supply Depot at 100 minerals. A subsequent Down press left the highlighted building unchanged and displayed a map-location message; this does not establish refinery availability. A separate ordinary-input route selected Scenario and Setup Custom, displaying Melee on Challenger with Human Terran and Computer Zerg. The custom-start run was queued at the time this note was written. Audio is drained and not recorded, so these runs support no sound-effect conclusions.

Fresh native custom observations: Setup Custom → Melee → Challenger, Human Terran vs Computer Zerg, showed 50 minerals and 4/10 supply in the earliest sampled map frame, before in-map inputs. Changing the human slot to Protoss through Down, Down, Right and launching fresh showed 50 minerals and 4/9 supply. These observations are for the human slot and exact setup only; they do not establish bot banks or game-tick timing. The first longer Terran launch exceeded the input metadata cap and failed closed; the shorter repeat succeeded, and the limit was subsequently raised for longer bounded runs.

In the fresh Protoss custom run, the on-screen mineral counter rose after launch without an in-map mining command; the later small-font exact reading has not been independently verified. An attempted Probe selection instead selected a Mineral Field, and a subsequent cursor adjustment missed a moving Probe. Neither attempt establishes Pylon power or warp behavior.

## Blood Bath result timing probe

This optional extension records the pre-instruction events in the [result
cadence audit](../../docs/audits/2026-11-13-n64-blood-bath-result-cadence.md).
The upstream instruction debugger formats every instruction before its log
callback. For this probe, `result_trace_source.py` instead derives a small
filter in the interpreter of a **disposable** checkout at the pinned ares
revision. Run `python3 result_trace_source.py /path/to/disposable-ares`
from this directory, then build the opt-in frontend with the normal pinned
CMake recipe, `SC64_PROBE_SOURCE` pointing to `actor_observer.cpp`, and
`-DSC64_RESULT_TRACE=ON`. Keep the default static core link for the probe
callback. Do not edit or build in the pinned reference checkout. The
normal frontend build and normal snapshot wrapper remain disabled for
result tracing by default.

In a local copy of the wrapper and observer source companions, pin the
new executable SHA-256, the exact controller command SHA-256 and sample
schedule, `HYBRID_STEP`, and `RESULT_TRACE_WINDOW=(begin,end)`. The
normal snapshot build still transitions at step 4200. For the trace build,
pin the hybrid transition to the end of a controller command at step 4200
or later, before the final attack. The half-open trace window starts at or
after the transition, spans at most 256 observer steps, and must end within
the recorded tape. The normal wrapper invocation and file/disk/time guards still
apply; the wrapper requires the source to differ from pinned ares only
by the exact derived hook, and hashes the modified source and decoder.
For a two-object relink, keep upstream clean and set
`RESULT_TRACE_CPU_SOURCE` to the detached CPU translation unit; the
wrapper compares it byte for byte with the exact derived hook.
A known opening tape can check the decrement/scan path. Qualifying defeat
requires a separately recorded ordinary controller combat tape.

`result-trace.bin` is at most 40 KiB. Rows are ten big-endian words:
observer step, instruction address, two event arguments, old countdown,
two words of pending bytes, two words of published bytes, and one word
of building count bytes for slots 0–3. Each event precedes the named
instruction; the file order breaks ties within a step. An `owner` event
carries the current owner in argument 0. A `publish` event carries the
owner and the about-to-be-written result. A `relink` event carries the
actor pointer and the about-to-be-written owner; the immediately
preceding full capture is the place to verify actor identity and the
post-scan capture verifies its links and orders. The `published` event
samples the result tables after the pass returns. The `reset` event
identifies the reset helper's write of 1. The countdown samples may
bound the census change without identifying the precise instruction
that wrote the building counter. Decode with
`python3 result_trace_decode.py /path/to/result-trace.bin BEGIN END`. Add
`--cadence` to report underflow anchors, dispatched owners, result-pass
ordering, reset intervals, and complete reset-free `30 … 0` intervals.
A trace without two anchored scans cannot verify the 31-decrement cadence.
A relink in the result pass still requires survivor identity and ownership
confirmation from the full snapshots.
Trace overflow, opcode mismatch, incomplete window, or an ambiguous
cache read rejects the witness; the output is never truncated to a
passing trace. The extension is source-only until a pinned build and
runtime witness are verified.

For a simultaneous-loss parity check, first reproduce the authenticated
opening with the observer attached. Then use a new ordinary-input match
to stage attacks on the last registered grounded building of each owner.
Retain the two building identities and census before the volley; record
the first two result scans across the losses and the resulting state.
Two census losses between adjacent scans can establish what takes
precedence at the next scan. To prove an identical gameplay update or the
order of damage, also capture a verified update boundary and the two
census stores. The current trace alone cannot make either stronger claim.
For a projectile that survives owner transfer, qualify a normal impact
control first, then observe its target, impact and HP separately.

A guarded opening control on 2026-11-14 linked the pinned two-object
observer successfully. The child stopped on `SIGXFSZ` before the first
gameplay command under the existing 8 MiB soft/hard per-file limit. An
open read/write Metal cache data descriptor referred to an 83,722,240-byte
file; the stopped-descriptor diagnostic does not identify the interrupted
write. No RAM snapshots, frames or trace rows were captured. The next
qualification needs a disposable Metal cache that meets the existing
process and owned-output budgets. Preserve the existing shared cache.

| Local artifact | SHA-256 |
| --- | --- |
| Linked two-object observer | `41112c2d8c83731faaf18e5efb690e1c365f29fed7e3327d9eb239288b266a82` |
| Detached CPU source | `084e5b438d9e7d94bdc8e29bcedf4adf9d096aee518f20b3dead2fab3fee9d1b` |
| Link receipt | `c2c0a80fd4a8112c06ec79db00638616ba9f20bb5c4332e49fc3b3c37d29d393` |
| Guarded capture manifest | `33a36401c474ecde3bd67208b1f2f4d6a090726f926b9f2949ed9b1fd6b2d7e6` |

## Guarded opening control passed

On macOS, the identical observer completed the authenticated opening
under its original 8 MiB soft and hard file limits when Metal cache
writes were denied for the wrapper and its child. The additional
sandbox rule leaves cache reads available; it is not a general-purpose
containment profile. It changes no cache paths, shared files, guest
inputs or output limits. With a new output directory and the pinned
opening wrapper described above, prepend this fixed command:

```sh
/usr/bin/sandbox-exec -p '(version 1) (allow default) (deny file-write* (regex #"/com[.]apple[.]metal(/|$)"))' \
  /usr/bin/env PYTHONDONTWRITEBYTECODE=1 python3 hybrid_snapshot.py \
  --binary "$observer" --source "$ares_source" --moltenvk "$moltenvk" \
  --rom "$owned_rom" --command-file "$opening_tape" --output "$new_output" \
  --file-limit-mode metal-compatible
```

The archived 3,694 and 4,204 images and two retained RAM observations
at steps 4,201 and 4,215 match byte for byte. The hybrid transition
preserves the cache-aware view. The result file contains 95 rows
(3,800 bytes). Scans at steps 4,223, 4,328 and 4,431 each dispatch
owners 0–3 before returning; both complete intervals contain exactly
31 decrements with no reset. Pending and published result bytes stay
zero throughout. This proves the opening cadence and read-only observer
plumbing. It does not qualify live defeat, projectile ownership or result
precedence during combat.

| Local artifact | SHA-256 |
| --- | --- |
| Successful opening manifest | `5f9491117b00fc57faefc8b90d5653ae1d84e152cf1aa55e41d6645a94d8ffb0` |
| Pinned opening tape | `da668b3e6826b61974364c671afd7f218e6288ef9e216bfabbdbfc83f761436b` |
| Opening result trace | `8eb9aed95f0b4a3b86216d0f4e89eaf75b161b15c2bcd322c3643c9425bb1635` |
