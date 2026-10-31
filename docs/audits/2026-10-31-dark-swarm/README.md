# Dark Swarm reference and first state stage

This is an inactive, asset-free state stage. It does not expose a cast, create
an effect in the game, change projectile damage, draw art, or serialize state.
Do not enable the shared world query until those services are integrated and
qualified across the intended products.

The pinned OpenBW `bwgame.h` copy used in this audit has SHA-256
`5ee316ca2c008c9252f7fde7444ca5dc7f35440549be2576dc37fa228fb31ae6`.
Its `Dark_Swarm` tech eligibility requires a Defiler that is not burrowed; the
position-target validation accepts a point. Its `dark_swarm` function clamps the
point to the effect unit's bounds, creates a non-colliding effect under neutral
owner 11, and assigns a 900-source-tick remove timer. Completed-unit processing
decrements a nonzero remove timer and removes the unit when it reaches zero.
The new isolated lifecycle primitive implements only that last bounded rule;
allocation, map clamping, save encoding and update scheduling remain the
caller’s responsibility.

The same reference excludes buildings from the overlapping swarm query and
returns zero dodge for flying targets. For a ground target under swarm the
projectile miss threshold is 255, compared to an 8-bit roll with `<=`; this is
not a blanket immunity rule for every damage path. Yamato's experimental
launcher already calls `sc64_world_dark_swarm_affects_target`, whose existing
stub intentionally prevents enabling the incomplete shared capability.

Still needed: authenticate the N64 source unit bounds, casting energy and
animation cadence and art; resolve exact target-box overlap and weapon classes,
including splash and direct-impact exceptions; implement ordinary human and AI
orders, allocation and ticking, shared combat queries, rendering and a
versioned save continuation for every supported mode. These items are not
implied by the lifecycle check or enabled in current products.

Run the asset-free lifetime and boundary check with `make test-dark-swarm-state`.

## Authenticated N64 facts and probe boundary

The retained authenticated N64 `units.dat` and `techdata.dat` hashes match
`tools/sc64_openbw_oracle_tables.py`. Parsing those tables with the pinned
OpenBW DAT loaders yields tech 14 energy 100 and unit 202 dimensions
left/top 80 and right/bottom 79 source pixels. Its art metadata follows unit
202 → flingy 125 → sprite 286 → image 337 → GRP index 369 (BOLT `00E/070`).
This identifies the source record; it does not provide or qualify rendered art.

OpenBW deducts energy when an in-range, ready caster enters its casting
animation; the effect is created later at the projectile hit, so failure to
allocate the effect cannot retroactively refund that energy. The opt-in
`SC64_DARK_SWARM_CAST_PROBE` models those two separate points using an actual
engine Defiler, caller-owned source fixed-point energy and allocated effect
slot. Caller-established ownership, range, cooldown, facing, animation and
visibility are prerequisites, not implemented by the probe. Its placement uses
the source bounds and map clamp including the excluded bottom 32 pixels.
No production path includes this probe or enables casting.

A sanitizer host check built with the current Showcase flags plus the test-only
skirmish roster gate exercises the real unit record, insufficient energy,
burrow rejection, debit before failed allocation, boundary placement and the
existing source cadence predicate. In the tested generic binding, 1,125 game
frames yielded 900 source ticks; only those ticks advanced the effect timer.
This does not qualify another mode's timing or save-resume behavior.

The pinned launch RNG returns a 15-bit LCG value, and the missile launch masks
it with 255 before its `<=` comparison; the swarm threshold therefore covers
all 256 possible samples. This applies to the referenced fly/follow/bounce and
appear-at-target launch paths when both source and target exist. It is not
evidence that unrelated self-destruct, source-position, direct or splash
resolution paths are immune; their exact behavior remains to be tested before
wiring ordinary combat.

## Shared identity investigation

The four canonical Terran, Zerg, P7 and Showcase flag vectors do not enable
`SC64_SKIRMISH_ROSTER`; the earlier extra-roster fixture therefore did not
prove that a normal product could represent raw 46. The opt-in
`SC64_SHARED_DEFILER_STAGE` appends one ordinary Defiler identity after the
existing identities when no roster or P9 identity is present; it aliases an
existing P9 Defiler or skirmish Defiler instead of adding a duplicate. It also
participates in the generated-fact feature fingerprint. It does not enable
production or any spell command. Four header-level tests with the actual
canonical flag vectors passed the raw-46 round trip. A sanitizer game probe
using Showcase flags plus only this stage gate passed; its typed compact row
also round-tripped a 50-energy Q8 value in the existing per-unit `carry` field.
That codec observation is not a save-format admission: caster energy semantic
validation, source regeneration, save versions, order and effect continuation
remain to be implemented. No mission payload is assigned for this stage.

The authenticated weapon 59 has source minimum range 0, maximum range 288,
cooldown 1, bullet type 5 and hit type 13. The reference also accepts the
Unclean One (raw 52) as a Defiler; the current probe covers ordinary raw 46
only and must not replace existing hero handling. The four-bit persisted order
field already reserves 14 for construction and 15 for the shared Battery, so
an order/save design must not silently allocate a conflicting value. Exact
casting animation release timing and production-safe UI, AI and effect storage
are still prerequisites for an integrated order.
