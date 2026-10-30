# Canonical indexed weapon lookup

Runtime/build candidate `3ebbf3f94dbcb05c46602c8005a1d3d0ef71875a` enables the
qualified image/animation lookup index for Terran, Zerg Z6, Protoss P7 and
Showcase. The original prepared weapon bank remains read-only. Make generates
a separate managed header in ignored `build/generated/weapon-lookup` and
selects it with `SC64_WEAPON_SOURCE_BANK_HEADER`.

`CANONICAL_LOOKUP_INDEX=0` selects the original resolver; `1` is the default.
The configuration key includes mode, source path and source SHA-256. A
read-only comparison supplies a forced dependency when the key changes.
This matters on macOS Make 3.81, which missed a rapid mode change when only
stamp timestamps were used. Unchanged settings do not force recompilation.
The normal original-art target requires neither this header nor private data.
Different settings require separate build directories for concurrent Make
invocations, as with other shared build outputs.

The managed generator prepares a complete candidate in the destination
directory and replaces only its own marked output atomically. Invalid input,
an unmanaged existing file, aliases, or unsafe destinations fail without
overwriting the previous output. The original unmarked, create-only CLI
remains available.

## Qualification

- Nine focused tests pass. The production Make database audit checks 272
  canonical flag consumers for bank/config dependencies, including templates.
  Real Make rules with synthetic inputs exercise unchanged configuration,
  mode switches, an older alternate bank, and a same-path same-second edit.
- A clean checkout without prepared inputs passes boundary checking, doctor,
  default build and default tests: 791 Python cases with 23 optional skips,
  eight ancillary cases and the C fixtures. The default ROM remains SHA-256
  `e612df7f0bd1417754e8665ca293bc53eedb194d55f93e2b996ea54ede1bd69c`.
- Four actual normal ARM Make builds use the default indexed selection and
  match the previously qualified #197 indexed ROMs byte for byte. The managed
  header, excluding its ownership comment, is exactly that qualified derived
  bank. All 104 original prepared input hashes remain unchanged.
- Their strict BSS/IWRAM endpoints remain Terran `020370b8/03004c08`, Zerg
  `02037124/03004cd8`, P7 `020382e4/03004ab8` and Showcase
  `0203df38/03004e38`. No actor capacities, RAM reserves or save fields change.
- All four actual indexed canonical tracer fixtures pass ASan/UBSan, including
  61 profiles per product, variant fallback, slot lifecycle and long durations.
- All 96 same-process host save/reload continuations pass with the default
  indexed flags, both maps, all human races and 1–3 AI slots. These are not
  fresh-process SRAM or normal-product ARM play-through proofs.
- `make unified`, `make test-unified`, `make test-terran` and
  `make test-sc64-terran-full-campaign` pass with frozen existing prepared
  inputs. The Terran checks include 234 startup/save configurations and the
  ten host mission controllers. Their state-observing input policies retain
  the previously documented limits, including inherited air-route knowledge.

The first default-test and Terran-build attempts failed with ENOSPC. Logs
were preserved; unchanged-source retries passed after completed native test
executables/debug bundles were retired with hash and open-file receipts.
ARM ELF/ROM/map artifacts, saves, inputs, sources and evidence were preserved.
Actual build commands, dependency dry runs and private artifacts are retained
under `.local/activation` and the clean qualification worktree. No art
generators ran; only the derived lookup header was produced.

The private package `.local/releases/20261031-indexed-lookups-experimental`
contains the four ROMs, compressed ELF/maps, manifest, readback hashes and a
testing guide. It binds the compiled source above separately from later audit
or merge commits. Canonical campaigns remain T10/Z6/P7, or 23/30; Scarab
namespace experiments remain excluded from these products.

The same-code paired custom-main Zerg/three-AI result remains 2.2200% fewer
aggregate update/audio/render cycles with 90/90 sampled actor/fog/RGB/frame
checkpoints equal. Worst whole-loop time increased; a separate diagnostic
attributed most of its reproduced difference to presentation. See the #197
lookup-index audit for exact numbers and scope. This establishes neither
near-30-fps hardware operation nor full-state equality. Further route
experiments are separate and are not enabled here. No Actions, SD writes,
public binaries or proprietary source-tree payloads were used.
