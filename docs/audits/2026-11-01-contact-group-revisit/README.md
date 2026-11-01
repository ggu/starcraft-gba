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
