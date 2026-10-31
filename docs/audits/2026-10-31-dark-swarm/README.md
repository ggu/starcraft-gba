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

### Source cast animation evidence

The authenticated N64 `iscript.bin` SHA-256 is
`f8e5f3d0fcca644470c37de65486e44b09eb30141eedccea3445afe7bc0f65bc`.
The source DAT chain resolves ordinary Defiler raw 46 and Unclean One raw 52
to the same image 13 and IScript 9. The `CastSpell` path performs four
one-source-tick waits, then emits `castspell` at source tick 4. In pinned
OpenBW this opcode validates the active spell order and fires its configured
weapon. Dark Swarm is allocated later by that projectile's hit handler. This
is evidence for a distinct animation/release stage, not for creating the
swarm immediately when energy is debited. Projectile flight/hit scheduling and
interruptions still require actual-world verification.

The authenticated weapon 59 projectile chain is flingy 172, sprite 370,
image 503 and IScript 292. Its Init path waits one source tick and signals
order 1. Pinned OpenBW then sends an appear-at-source projectile to its Death
animation; that path waits five ticks and emits `domissiledmg`. Exact
same-frame scheduling across the caster and projectile remains to be checked
with an executing world oracle. In particular, tick-4 `castspell` is not a
justification for allocating the effect at the start of the cast.

Until the caster and field state have an actual save schema, the dedicated
experimental identity gate makes `race_save_game()` reject a live raw-46
Defiler. This conservative guard is not a permanent save solution and does not
change normal products where the gate is off. The actual-engine sanitizer
fixture verifies this rejection. The normal UI/order, source-accurate facing,
projectile scheduling and end-to-end continuation remain unimplemented.

Source spell readiness also checks weapon range, movement and simulation
heading against the weapon attack angle before the debit. The existing
`sc64_facing` state is explicitly presentation-only, reconstructed after load,
and cannot be used as proof of that simulation heading. A correct shared order
needs a source-backed heading and interruption model, along with ownership of
its bounded pending state; the four-bit save-order slots are already occupied.
The guarded caster-save refusal above must also cover any active field when
field storage is introduced. These are unresolved integration requirements,
not completed casting support.

### Executing OpenBW world oracle

A controlled synthetic 32×32 walkable world was run with the pinned OpenBW
reference and authenticated N64 DAT, image metadata and IScript inputs. It is
not native N64 execution. The harness creates a raw-46 or raw-52 caster at
(320,320), explicitly sets 100 energy for the experiment, and issues the real
position-target Dark Swarm action. For a target 160 source pixels east, both
casters and initial scheduler phases 0–4 produced the same observations
relative to action: energy debit at sample 2, projectile present at sample 5,
field present at sample 10 with timer 900, and timer 899 at sample 11. Source
heading samples 1–3 were 104, 77, 64. With a target 400 pixels east the caster
moved to (456,320) before debit at sample 46, projectile at 49 and field at
54. These traces are specific to the stated controlled geometry and initial
state; they are not a general timing formula.

With the bullet pool exhausted before the action, both casters still paid the
energy but no Dark Swarm projectile or field appeared. Separately exhausting
the unit pool (1,699 additional units alongside the caster) still allowed the
projectile but prevented field allocation; energy remained spent. These
observations support keeping debit, projectile allocation and field allocation
as distinct steps. The source harness is
`tests/sc64_openbw_dark_swarm_world.cpp`; it reads private inputs by path and
does not contain those inputs. Actual UI/order, movement/turn state, visual,
damage and continuation integration remain open.

### Proposed gated order-state ownership

A subsequent integration can maintain a dedicated bounded simulation sidecar
indexed by live unit slot: actor generation/identity, source heading, cast
phase, target coordinates and elapsed source ticks. It must initialize from the
actual source spawn/heading rules, update during ordinary movement and turning,
validate the actor generation on every use, and clear on death, replacement,
new game and load. The visual `sc64_facing` byte cannot supply this state.
Active field records need separate bounded ownership and generation-aware
allocation. Use the native player input and shared AI request paths to create
orders; any temporary use of an existing order code must be conditioned on the
raw actor identity and reconciled with STOP, selection, interrupts and the
four-bit codec. Reject saves while a caster or field requires this unencoded
state; later replace the guard with a versioned transactional codec. Debit,
release, projectile and field allocation should be separate transitions driven
by source ticks and the actual-world observations above. This is a design to
implement and test, not an assertion that the current game has those paths.

The executing oracle also reports the completed ordinary caster's creation
values: N64 `unit_direction=13`, source simulation heading 104 and initial
energy 50×256. The experimental gate now initializes a dedicated per-slot
simulation sidecar with these values and clears it on slot reuse and new-game
initialization. It does not use visual facing or a mission payload. The
actual-engine host fixture verifies initial values and save refusal. This is
not the requested field-menu/order integration: updates, death/load handling,
interruptions, effect allocation, AI, visual and save continuation are still
outstanding, and imported actor initialization has not been qualified.

### Experimental field input checkpoint

With `SC64_SHARED_DEFILER_STAGE` only, a single selected owned unloaded
supported caster gets a Dark Swarm row appended after the existing contextual
rows. Ordinary B/menu/A input enters a point-target mode; A stores a bounded
point intent in the caster's stage sidecar. A caster below 100 energy is
rejected, and B cancels targeting. Enqueuing does not debit energy or create an
effect. The actual-engine Showcase-vector sanitizer fixture exercises the pad
sequence, target, cancellation, insufficient energy, mixed/enemy/loaded
selection checks and the temporary save refusal. It uses a test-created
ordinary raw-46 actor; imported raw-52 hero and other menu vectors remain to be
qualified. The intent is not yet consumed by the simulation: source heading,
approach, animation, projectile and field transitions remain required. This
checkpoint is therefore not playable Dark Swarm support.

### Typed projectile reservation stage

`SC64_SHARED_PROJECTILE_STAGE` adds a 100-entry typed reservation arena and
routes actual P7 Interceptor projectile creation and retirement through it;
existing Interceptor motion, drawing and damage remain in their original arena.
A failed typed reservation rolls back the newly acquired Interceptor slot;
shooter death invalidates references without retiring an in-flight shot.
Reset invalidates outstanding reservation handles. A validated older P7
projectile packet can reconstruct Interceptor reservations into an empty stage
arena. Saving or loading is refused while the experimental typed arena is
active, pending a supported transactional codec.

The sanitizer fixture fills 99 reservations as a synthetic second type, launches
one real Interceptor projectile, verifies that a second real launch fails at
100, frees a reservation and successfully launches again. It also checks
wrong-type and stale-handle rejection, shooter death, eventual retirement,
reset and old Interceptor packet reconstruction. The second type in this test
is a reservation, not a real Yamato launch. The normal flags remain off.

This is not yet the source-wide allocator: real Yamato, ordinary pending
impacts and Dark Swarm are not routed through it, so there is not yet honest
competition among all live projectile families. The 16-bit generation may
wrap under repeated slot reuse and must be addressed before activation; save
migration, all four profile bounds and source allocation failure semantics also
remain unqualified. The earlier menu-count wrapper was fixed to retain its
actual function symbol with the stage gate off; the exact gate-off P7 delayed
impact/save and 18 skirmish setups pass.

### Real Yamato contention stage

Under `SC64_SHARED_PROJECTILE_STAGE`, Yamato approach and charge retain their
existing two-row cap and do not reserve a typed projectile slot. At actual
release, a free typed slot is required before the launch miss roll; successful
flight retains that reservation through caster death, target loss and slot
reuse, and releases it when the projectile retires. If no slot is available,
the source-style release path retains the already-spent energy and existing
recovery/cooldown state. A preflight rejects legacy in-flight Yamato saves
under this experimental gate until a shared reservation codec is supported;
normal products do not enable this gate.

An actual-engine ASan/UBSan test uses 99 real Interceptor shots plus a real
Yamato, then 100 real Interceptor shots plus a real Yamato charge. It checks
contention in both directions, no pre-release reservation, full-pool refusal,
no extra launch miss RNG on a full pool, caster and target death, target-slot
reuse, projectile retirement and reset. The test uses P7 and BC logic with the
Yamato art flag omitted; it does not validate rendering, the complete shared
RNG stream, a combined save codec or native hardware. Gate-off resolved
Yamato flight/save/miss/removal tests also passed. The earlier gate-off fixture
was first run with autolaunch enabled even though that fixture expects manual
release and failed its age assertion; with its required manual-release vector
it passed.

Ordinary impact packets, Scarabs and Dark Swarm remain outside this typed
reservation. The 16-bit handle generation wrap and all-profile/link/save
qualification remain open, so this stage is not ready to activate.

### Real Scarab reservation checkpoint

On shared merge `e7df0d1`, `SC64_SHARED_PROJECTILE_STAGE` and the existing
Scarab weapon probe together reserve one typed slot when the actual Scarab
bullet is created, and release it on impact or queue reset. A gated 32-entry
handle sidecar follows the existing compacted impact rows. Target loss keeps
the positional Scarab packet and its reservation; ordinary impact rows remain
unreserved in this stage. A full typed arena leaves the child consumed and
produces no bullet, matching the referenced Scarab allocation-failure flow.
Normal products keep both stages off; the added sidecar and arena are not
qualified for normal memory admission.

A synthetic-host P7+BC fixture launched 99 real Interceptor projectiles and a
real Scarab bullet, observed both families refuse the hundred-and-first
allocation, then verified Scarab retirement frees a slot for a real
Interceptor. A separate Terran-vector actual-game-update fixture covered a
paid Reaver child with 100 synthetic occupants, target loss, row compaction and
reset. The existing five-paid-child/mixed-impact/save-rejection fixture passed
with the stage gates. Synthetic pool occupants are not evidence of real
ordinary weapon contention. The first real-family fixture had only 40-HP target
health, so its Scarab impact killed that target and invalidated the subsequent
Interceptor launch; a 500-HP synthetic target corrected that test assumption.

This checkpoint does not implement the source-wide allocator. Ordinary impact
rows may represent two source bullets and include attack animation time before
release; reserving one slot at queue creation would count the wrong objects at
the wrong time. Full-queue cooldown behavior, partial two-shot allocation,
source RNG timing, remaining families, generation wrap, save migration and all
profile memory bounds remain open. Neither Defiler input consumption nor
production admission follows from these tests.

### Pending-order replacement and ordinary allocation reference

Under the still-disabled Defiler stage, STOP and a successfully issued ordinary
move/attack now clear the selected caster's pending point intent without
spending energy. The Showcase-vector ASan/UBSan fixture enters the point order
through normal pad input, navigates the STOP row through pad input, verifies
cancellation, and separately checks replacement by a move. This prevents stale
intent from firing after a later command; it is not an implementation of
casting, movement, a projectile or a field.

An isolated executing OpenBW reference experiment used authenticated tables,
real action selection/attack and frame updates, with a stationary attacker
and target. With 98, 99 and 100 occupied source bullet slots at first release,
Marine raw0's one-bullet weapon allocated 1, 1 and 0 bullets respectively;
Scout raw70's two-bullet weapon allocated 2, 1 and 0. Cooldown was spent in
all three occupancy cases for each weapon. Fillers were replenished before
each frame and may expire later, so these observations qualify first release
only. The reference sets cooldown at attack decision and IScript later attempts
each bullet separately. The current GBA impact row combines hits and includes
wind-up before release: it cannot be treated as one native projectile. A
correct ordinary integration must represent individual release times and
partial allocation, preserve cooldown/RNG ordering, and cover interruptions
and local queue limits. The reference observations do not establish a general
GBA tick formula, and no ordinary allocation behavior has been changed here.

### Controlled direction-dependent casting observations

The executing OpenBW world probe accepts an optional y displacement as well as
x displacement. With a stationary caster at source `(320,320)`, 100 energy
injected for the experiment, initial phase 1, no pool exhaustion and an
in-range point, both raw46 and raw52 produced these first observed source
samples (energy debit / projectile present / field present): east `(160,0)`
`2/5/10`, south `(0,160)` `2/5/10`, west `(-160,0)` `4/7/12`, north
`(0,-160)` `5/8/13`, and southeast `(112,112)` `1/4/9`. All ten processes
exited zero. The source weapon's authenticated attack-angle value is 16;
turning affects readiness, so a fixed delay from the input action is incorrect.
These are synthetic OpenBW-world observations using the pinned source header and
owned input tables, not native N64 execution or proof for moving casters,
obstacles, interruption or other scheduler phases. The added harness input does
not implement the staged Defiler order, projectile, field, or save state.

`tests/sc64_openbw_ordinary_projectile_world.cpp` retains the reproducible
ordinary allocation probe. It validates the six controlled occupancy cases,
including spent cooldown, expected first-release bullet count and no first
impact when full, using private authenticated OpenBW input tables and the
synthetic walkable map. All six assertion runs exited zero. Its first-release
sample numbers are fixture-specific and are not used as a general-purpose
GBA scheduler or a claim of complete ordinary-projectile integration.
