# Contact bucket grouping revisit

`SC64_CONTACT_GROUP_INDEX` is a disabled source-only prototype for the Z7
legacy contact profile, not a normal product setting. It reuses the existing
per-cell bucket index and adds one group-next byte per unit slot. Representatives
link by bucket cell; members retain their slot links. All ordinary bucket
consumers still traverse every member, while the contact mask may collapse a
group only after verifying live type and coordinates for each member. Invalid
classes and stale/nonuniform groups use individual reference processing.

The opt-in is limited to the legacy Z7 contact profile without P6 combat or
steering lists; other profiles, including P6, retain their existing path. Group
maintenance helpers are placed in ROM; the candidate preserves existing memo
invalidation and filters loaded and noncolliding Scarabs per slot. The group
scan still visits every member to validate current keys and eligibility, so
this is not elimination of all bucket visits. Source edits, prior historical
measurements and this audit are not proof of present correctness, code/stack
fit, or a production speedup. No normal profile enables this flag.

Before any benchmark, review and exercise all bucket consumers, strict-step
and mask equivalence, dense groups, representative removal/promotion,
within-cell moves, stale callbacks, births/reuse, egg morphs, transport and
Scarab state changes, invalid facts, active-index overflow, and memo behavior.
Compare the current normal disabled ROM byte-for-byte, check unchanged ARM
memory guards and stack usage, and run source-matched physical inputs. No
historic prototype speedup is claimed for the current engine.

The source-only membership oracle calls the full-world race collision fallback
with the same boundary, gatherer, noncolliding and immovable-mover rules; it
does not traverse the candidate group links. The existing manual egg rekey
case is only a helper check and does not establish the real update-orders egg
dispatch; that lifecycle remains a qualification requirement. The optional
invalid-class case reports unavailable when the profile has no representable
invalid class, and is not evidence of coverage in that configuration.

A separate bounded host lifecycle fixture now prepares a real Z7 Egg whose
remaining timer is set to one and calls `update_orders()`, observing the real
Egg dispatch after the paired Zergling birth while the bucket is active. Its
host-only callback compares both the in-dispatch and rebuilt-bucket result to the
full-world oracle. The transport fixture requests real board and unload actions
and checks nonzero-speed masks before and after rebuilding. Fixture setup is
controlled host state; it is not an ordinary-input or paid-production proof.
Both tests are source-only and unrun until the frozen qualification is executed.

The host Egg witness is placed three pixels from the queued Egg before the
real `update_orders()` call. The read-only callback requires a nonzero
toward-newborn collision bit as well as full-world equality at speeds 1 and 2.
This demonstrates a contribution from the in-place newborn if the fixture
passes; it does not establish collision coverage of the second pair member.

## Focused host qualification

At `d1a3b7b`, the unchanged membership fixture passed with grouping disabled
and enabled. Both runs reported the invalid-class fixture unavailable in the
canonical Z7 profile, so that case is not claimed as covered. The initial
lifecycle OFF run stopped because its controlled Egg setup omitted the supply
provider needed by the real production admission check; that failure remains
separate from the later result.

At `f234987`, which adds the existing canonical fixture's Overlord supply
provider without changing runtime code, the controlled real Egg-dispatch and
board/unload lifecycle fixtures passed with grouping OFF and ON. The
near-newborn full-world witness required an actual nonzero collision; it does
not prove collision coverage for the second newborn. The canonical Z7
contact-cache differential also passed in both modes with identical reports:
4,141,228 step checks, 637,112 indexed and 130,670 inactive mask checks,
71,716 mutations, and zero failures; its memo oracle reported no unhooked
mutations. These are controlled host fixtures, not player-input proof. Normal
ARM fit, OFF ROM identity and current physical performance remain unmeasured.

## ARM placement and attempted replacement trace

At source `7d16c46c88053f268de93edae2f80fb192d93d00` on engine base
`30858e34246418c6847d8c7e11a779b3ef701b12`, normal Z7 OFF and opt-in ON
ARM links and the unchanged ROM verifier passed under the native lock. OFF ROM
SHA-256 `3251518bd82b07923fc9ded25c390c1b38354d4ca9305a13ed297508eab7f352`
matched the base. ON ROM SHA-256 was
`1132da285bc708063d4a08d04d1ce389cbeaec872751be46d2e3fae943a94b60`.
OFF/ON EWRAM BSS ends were `0203d718`/`0203d808` (+240 bytes), and
IWRAM ends were `03004f80`/`03004ad0`; stack use was not measured. The
receipt is `/tmp/sc64-contact-group-arm-7d16-20261101/result.json`. Earlier
statements that ARM fit was unmeasured describe the preceding stage.

The prior winning physical input trace `5121ed97ea0ea3d57697ce2c7fc57eac2082e3a457bb4ad98517f09ecf510489`
is unavailable; the retained `3251518b` physical WIN remains evidence for
that fixed input and binary. Two new ordinary-input host controller attempts
on the current 308 engine did not produce qualified replacements. The modern
controller, early-gas with a 200000-update bound, reached the bound with zero
workers, source fault zero and 54 enemy source buildings; its incomplete trace
SHA-256 was `1e5c477609db3211794aff731490e435d85f4cfa7bc763c6fdf44c460e8c33e5`.
The historical 232bc planning policy, with the current strict terminal checks
and the same policy and bound, lost at host 15290/frame 15035, source fault
zero and 55 enemy source buildings; its partial trace SHA-256 was
`056faa9c515609e8f33d0ded07e52b1218adca0f86fa11c40bcce15d889140f0`.
The logs are `/tmp/sc64-z7-strict-regenerate-308-output-20261101/run.log` and
`/tmp/sc64-z7-historical-policy-output-20261101/run.log`, respectively.
Neither partial trace is a winning witness or suitable for the proposed ROI
comparison. Current grouped-contact performance remains unmeasured; no
historical speedup, physical-hardware FPS or admission is claimed.

## Early-battle diagnostic comparison

The frozen `30858e3` normal OFF and grouped ON ROMs ran a fresh physical-button
prefix of the new host trace through update 9900. This trace ended at the host
bound without victory, so this is a diagnostic prefix, not a full mission
qualification or replacement for a winning trace. Both modes matched all 25
named prefix observations and nine full canonical entity scalar/array,
visibility/source-count/minimap, active frame and palette samples. At update
9000 both were PLAY without loss or fault with 23 friendly and 85 enemy units;
at 9900 they had 24 friendly and 87 enemy units.

Across updates 9001–9900, total cycles averaged 1,100,718 OFF and 1,113,187 ON
(+1.13%); p95 was 1,404,401 versus 1,404,531 and window maximum 3,443,432
versus 3,655,056. Mean update cycles were 448,301 versus 463,711, and mean draw
cycles 570,573 versus 570,834. These are emulator guest-cycle observations,
not physical-device FPS. Grouping remains OFF and is not promoted. The full
receipt and per-update samples are in
`/tmp/sc64-contact-group-early-battle-20261101/`; driver SHA-256 was
`482977693aab2dbbfd9e92682b8ee17e7482b24c7d46c29f5f738bc13a9f3b68`.
