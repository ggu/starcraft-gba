# Current late-Zerg qualification candidate

Integration `062ddfe5` combines default branch `f9dc1179` (including prepared
save selection, late-objective source art and target-list memory recovery) with
the historical late-Zerg controller branch `3a2765bf`. The Makefile merge keeps
both the current source-art coverage fixture and the late-mission replay rules.
This branch does not change canonical campaign selection or promote Z8–Z10.

The historical Z9 win/save evidence is described in the adjacent
[cargo qualification audit](../2026-11-05-z9-cargo-qualification/README.md).
It does not qualify this integration. New HOST, ARM and genuine save tests
must use the current inputs and runtime before making a new playability claim.
The 1,958 prepared inputs copied from the qualified `a9a0ea76` workspace were
individually checked against its SHA-256 receipt; no importer was rerun.

The merged candidate retains the historical authored town-wave approximation,
including its provisional 5/4 wait conversion and averaged group sizes. Static
script reports are source evidence, not observed N64 attack execution. The
[late-Zerg integration audit](../2026-11-05-late-zerg-integration/README.md)
records those limitations. Ordered source-script execution and Guardian work
remain separate candidates; this integration does not establish combat parity.
The Z10 controller is preserved as an unqualified draft, not a demonstrated win.

Initial gates are source-boundary and script-report tests, Z9 actual game
fixtures, an ordinary-input HOST controller win and a normal Z9 ROM guard.
If they pass, generate a fresh input tape and run full/prefix/fresh-process
continuation on HOST and ARM. Compare complete journals as well as the existing
scalar/digest checkpoints, whose historical digest omits some cargo fields.
Carried-mission validation and unchanged canonical product qualification are
required before any default promotion. Timing remains a separate gate; a
successful mission does not imply the unfinished 30 fps target is met.

All native work uses the shared local flock and bounded storage. ROMs, generated
data, saves and logs remain ignored/private. No Actions or SD writes are part
of this qualification.

## Initial current-source result

At `474a0c9e`, the source-boundary check, script-report tests, Z9 game fixture,
source-art coverage fixture and normal Z9 ROM/guard pass. The ordinary-input
HOST controller wins at frame 79,409 / call 82,066 with zero mission faults,
matching its historical route's endpoint. All 1,958 input hashes remain unchanged.
ARM and fresh-process save qualification are pending.

The next replay source enables `PX_CARGO_DIGEST` in late-campaign replay wrappers
only, adding unit flags, work timer and build to the existing digest. Production
mechanics and older P8/P9 digest schemas are unchanged. An optional second HOST
argument exports complete final SRAM separately; it never overwrites a resume
seed implicitly. This permits comparison of genuine full/prefix/resumed journals
alongside sampled checkpoints, rather than treating digest equality as complete
state equality.

## Strict replay and journal results

At `0e6d3ba4c1468a73da70ca2ea70a16004d2c50af`, the new late-campaign replay
wrappers pass the ordinary-input HOST full/prefix/fresh-process continuation.
The continuation matches all 77 expected checkpoints, including the added
unit flags/work timer/build digest fields, and its complete 32 KiB terminal
SRAM equals HOST full replay. The corresponding input-only ARM runs pass
83 full, 7 prefix and 77 continuation checkpoints. Both ARM wins reach the
same frame 79,409 / call 82,066. The pause-menu prefix is at call 6,515 /
frame 6,292, with construction active; the continuation uses that genuine
unmodified ARM SRAM in a fresh isolated emulator process. Replay `faults` is
the generic unsupported-event counter; the HOST controller separately reports
zero late-mission scheduler faults. These are bounded emulator observations,
not exhaustive state or physical-hardware proofs.

The first whole-file cross-backend assertion failed. Investigation found an
incorrect harness assumption: HOST blank SRAM is `00`, mGBA blank SRAM is `FF`.
No guest data was patched and no guest replay was repeated to obtain a pass.
Every committed header and its entire 16,312-byte checksummed payload is
byte-identical across HOST and ARM, with sequence 1 at the prefix and a second
sequence-2 record at victory. The original slot, including its padding, is
preserved within each backend. Cross-backend prefix files differ only in
unused bytes 16,328–32,767; terminal files differ only in the 56 unused bytes
at each slot end (16,328–16,383 and 32,712–32,767), always `00` versus `FF`.
These zero-based raw-file comparisons remain unequal and are not described
as passing.

Within each backend, full and resumed terminal files are completely identical:

- ARM full/resume: `f5a48c511ba4da1e25366d1f7e391a3ccf0a6c448594fc3d7f46e318c0f2f7be`
- HOST full/resume: `5699ee8ace742989bb792d43b64cfdc54178aebf1c9311dc1099867e02aefa71`
- Genuine ARM prefix: `bcab7464ef58fbaffa102a6de599c00409719d73db2671e34b5558aed8ab1d22`

The ARM replay wrapper measures 180,000,538,291 total guest cycles over
82,066 calls, about 7.65 fps equivalent, with an 85,115,128-cycle maximum span.
This is one full tape using the current source-art bank and strict diagnostic
wrapper, not a performance improvement claim against the historical build.
All 1,958 prepared-input hashes remain unchanged; later APFS clone replacement
of duplicate input storage preserves those bytes and metadata. Source, logs,
journals and the failed assertion's classification remain private locally.

`160aafac` subsequently merges qualified default branch `1ffa7aad`, adding the
new-custom-game supply opening adjustment and its fixtures. No fresh combined
qualification is claimed here: final combined startup/carried-mission and
clean default/unified checks are pending. The default still exposes 27/30
missions. Z9 playability is demonstrated on the frozen source above; default
promotion remains a separate decision after the applicable carried checks.

The separate carried fixture at `daaf5be1` (latest-main opening adjustment
included) subsequently passes all nine campaign normal-menu startup/save
roundtrips and 234 skirmish configurations with generation 48. Its production
sources, Makefiles and promoted-profile fixture match `455e54bf` byte-for-byte;
only audit history differs. These are bounded startup/save checks, not carried
mission wins. Ordinary-input Z1–Z8 controllers remain in progress separately.
