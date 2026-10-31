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
owner 11, and assigns a 900-update remove timer. Completed-unit processing
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
