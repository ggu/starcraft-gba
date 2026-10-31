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
bullet is created, and retain it through the post-impact source tick or until queue reset. A gated 32-entry
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


### Experimental individual ordinary shots and retirement

The disabled `SC64_SHARED_ORDINARY_STAGE` adds an individual-bullet slice for
initially Marine raw 0 / weapon 0 and Scout raw 70 / air weapon 74; later
validated pairs are listed below. Pending windups do
not reserve capacity; release attempts independently acquire the typed 100-slot
arena. The source-world occupancy runs observed Marine allocation 1/1/0 and
Scout allocation 2/1/0 at 2/1/0 free slots, respectively, with cooldown spent
even on failure. The corresponding actual-engine sanitized fixtures exercise
those cases, partial Scout damage, STOP, death and actor-slot reuse, and an
actual Z6 hallucination-flagged actor. This is a controlled synthetic setup.
Other weapon families still follow their previous path and do not contend here;
the profile-derived GBA schedule is not a proof of exact source timing for
arbitrary positions, phases, motion, or animation. The stage adds sidecar RAM
and has not passed production memory or save-format qualification. Active
staged bullets and windups reject save/load.

The Scarab reservation now survives the damage update through its next source
update. The pinned controlled trace shows weapon 82 present at damage frame
129 and first absent at frame 130. The actual mixed-family fixture confirms
that another allocation fails during that tail. Yamato similarly retains its
reservation separately from the reusable shot row: source image 541's Death
script creates image 544 and executes `domissiledmg`; image 544's 14 frames
wait two source ticks each, and OpenBW retires the bullet only when its sprite
images end. One controlled source-world cast dealt damage at frame 48 and was
first absent at frame 76. A sanitized real Yamato/Interceptor fixture confirms
the capacity remains full for the post-impact tail and clears on tick 28. This
is evidence for that script and test setup, not hardware or full allocator
qualification. Interceptor retirement remains tied to its existing DYING to
FREE transition. All experimental flags remain off in normal products.


### Disabled stationary Defiler executor

`SC64_SHARED_DEFILER_EXECUTOR` adds a deliberately limited actual-game consumer
for the existing field-menu point intent. It requires the staged Defiler,
shared projectile arena and existing source clock. In the tested stationary
raw-46 controlled interior, the actor turns toward the point, pays energy,
attempts a typed projectile allocation at its later script event, and creates
a separately timed field on projectile impact. A full projectile arena still
spends energy; a full ordinary unit pool prevents field creation. Field slots
are non-colliding sidecars reserved from the same unit budget; unit reuse and
the existing free-slot preflights account for them. New releases are attempted
before bullet retirements. Projectile and field lifetime survive caster loss.
Pending casts, projectiles and fields refuse save/load.

On a synthetic actual-engine Showcase setup, ordinary B/menu/A input and five
source headings at each of five GBA/source clock phases matched the retained
controlled OpenBW samples: debit at source ticks 2, 2, 4, 5 and 1 for east,
south, west, north and southeast, then projectile at debit+3, field at
projectile+5, retirement one tick later. ASan/UBSan fixtures also cover full
typed/projectile and actor pools, a seeded simultaneous release/retirement
ordering counterexample, reserved slot expiry, and save refusal. The source
world oracle used authenticated private data and a synthetic flat map; it is
not native N64 or hardware proof.

This remains off in all normal products. It is not complete Dark Swarm: the
controlled interior is narrower than the source's unit-rectangle range and
halt calculation; visible-target checks, movement/approach, interruption and
full slot-retirement semantics, raw-52 hero and energy upgrades, the field
rendering, weapon launch miss protection, all-family shared bullet contention,
and persistent save format remain unqualified. A separate controlled eastward
source-world expiry probe across initial phases 0–4 observed the active field
leave at timer zero and its dead unit retain its slot for four more source
samples; the stage now models that inactive slot tail separately from active
field lifetime. Other expiry conditions remain unqualified. No production-
admission claim follows from these fixtures.


### Experimental field image

The disabled `SC64_SHARED_DEFILER_ART_PROBE` renders source image 337 using
the existing modifier-9 projected light renderer. The public generator
`tools/sc64_dark_swarm_assets.py` extracts its ten frames from the authenticated
owned ROM, palette remap and palette JSON into an ignored directory; no raw art
is stored in this repository. The tested generated header SHA-256 is
`7f137d60b696231cec4cb844b4f49a086d7fcfa09527c0c1fc1fc9c66a9f1946`.
Image 337 uses IScript 299: Init advances frames 0–4 at one tick each, then
loops frames 5,6,7,8,9,8,7,6 at three ticks each. The inactive unit tail
draws death frames 4–1; the last death image after unit-slot retirement is not
yet represented. The sanitized Showcase-host render fixture follows a real
menu cast to a field and checks visible pixels at interior, horizontal and
vertical scene edges while preserving the HUD bands. It does not establish
source-exact blending, final death-frame lifetime, all profile palettes, ARM
fit, or native hardware appearance. Normal products remain unchanged.


### Disabled cardinal clear-path approach

The additional `SC64_SHARED_DEFILER_APPROACH_PROBE` drives a stationary-start
point order through actual `game_update` using source-Q8 position, Defiler
IScript's four-source-pixel walking step and the source order's nine-update
range-check cadence. It requires a clear direct GBA terrain path and checks
ordinary actor collision at each step; blocked routes stay pending without
spending energy. This is an explicitly restricted GBA collision adaptation,
not OpenBW pathfinding. Non-cardinal paths fail closed. STOP clears the pending
order.

A synthetic prepared Showcase host fixture used ordinary menu input on clear
terrain, then compared its output to retained pinned OpenBW flat-world oracle
logs from an isolated caster at source phase zero. All 60 source samples of
position and heading matched for each of four cardinal routes, including debit at source samples 46, 46, 19 and 19; the
blocked-corridor case stayed in place without spending energy. The opt-in
`test-dark-swarm-approach` target requires those retained oracle logs through
`DARK_SWARM_ORACLE_DIR`. Source diagonal tracing showed an independent velocity
direction that turns toward intermediate 64-source-pixel waypoints while
visible heading stays fixed. It is intentionally not treated as cardinal
movement and remains unsupported, as do obstacle pathfinding, source
collision/range-edge semantics, moving starts, later order-timer phases and
multi-actor scheduling, raw-52 energy and full runtime integration. The
stationary center-distance band above 256 and at most 288 source pixels remains
pending rather than casting or approaching. Normal flags remain off.


### Disabled Unclean One identity

`SC64_SHARED_DEFILER_HERO_PROBE` appends a distinct raw-52 identity after the
existing unit IDs. The source profile supplies 250 maximum HP and three base
armor; the hero starts with 62.5 energy and regenerates eight Q8 units per
source tick to a 250-energy maximum. An actual sanitized Showcase-host fixture
uses the real raw-to-runtime mapping and normal `spawn_unit`, waits through
normal `game_update` regeneration without assigning energy or raw owner, then
uses B/menu/A to create a bullet and field. Live hero saves remain refused.
This synthetic-spawn test alone does not establish imported campaign setup,
P9 integration, hero body art, all energy effects or hardware behavior; the
separate bounded loader check below covers a synthetic authored record. Raw 46 and 52 now
have authenticated 320-source-pixel sight and respective source bounds
13/12/13/12 and 14/14/14/14; an actual generated fact-table fixture checks
these values and rejects pre-change geometry headers. Ordinary Defiler Metasynaptic Node research and hallucinated-energy rules
remain open. The fact generator records the new identity gate and its consumer
rejects older generated tables missing the hero marker.

A separate ASan/UBSan actual-menu approach fixture checks STOP during motion,
a later renewed cast, MOVE replacement, refusal while already moving, and a
subsequent STOP/recast. This is the controlled clear-route input path only.
All of these gates stay disabled in normal products.


### Disabled source-rectangle launch query

`SC64_SHARED_DARK_SWARM_PROBE` binds active staged fields to the experimental
Yamato world launcher and the separately listed normal-hit ordinary releases.
In the source-rectangle profile it intersects each
ground actor's source-pixel bounds with each active field's 160×160 source
rectangle; buildings, flying actors and expired death tails do not receive
the ground-target protection. When a field exists and target bounds are
unknown, the staged launch fails closed. A pinned OpenBW world oracle tested
adjacent source-coordinate edges on both axes for raw 0, 46 and 52. A separate
sanitized actual-game fixture created a field by real UI input, checked those
actors at the adjacent positions representable by GBA coordinates, and issued
a real Yamato order: the actual released shot took its source miss path and
the ground target remained undamaged. Its BC energy and heading were seeded
for the controlled fixture. A separate synthetic attack-order fixture advanced
actual `game_update`: a Marine bullet allocated against a Defiler under the
field but damaged neither it nor a bystander at the visual miss offset, while
an otherwise comparable unprotected target lost five HP after armor. An unknown
rectangle case confirms fail-closed release. This models the guaranteed field threshold at bullet
release; it does not reproduce the global source miss RNG stream.

This is not complete shared Dark Swarm damage behavior: only that experimental
Yamato path and the individually validated normal-hit ordinary releases are
bound; other ordinary weapons and projectile types, all-profile source
geometry, the zero-threshold miss and global source RNG stream, launch ordering
on the exact field-expiry tick, save/load and hardware remain open. The gate is off in normal products.


### Campaign loader hero check

An additional sanitized host fixture passes a synthetic authored raw-52 unit
record through the actual `init_state` and campaign unit loader. The loader
assigns the distinct hero identity, human source owner, HP and armor, and the
spawn path initializes heading and 62.5 energy; the fixture does not patch
the resulting live raw identity, owner or energy. Its compile-only map override
selects that synthetic record and is disabled in products. This qualifies the
bounded loader path, not the authored P9 map or a P9 continuation.


### Additional normal-hit weapon families

The disabled individual-projectile stage additionally handles Vulture raw 2 /
weapon 4, Wraith raw 8 / ground weapon 16, Hydralisk raw 38 / ground weapon 38
and Dragoon raw 66 / ground weapon 66. Each has one source projectile and a
normal direct hit. Sixteen pinned OpenBW flat-world runs covered these four
actors with one or zero free bullet slots, with and without a synthetic field:
one slot produced one bullet, zero slots produced none, cooldown was spent in
both cases, and only the unprotected one-slot case caused damage. A sanitized
actual-game fixture separately checked one/free and full arena behavior and
then drove real update dispatch with field-protected and unprotected targets.

Goliath raw 3 uses its raw-4 turret's air weapon 8. Three separate source-world
runs observed two, one and zero independent projectiles with two, one and zero
free slots. The staged actual-engine fixture verifies those counts and that the
one-slot hit deals half the two-slot result. These are specific validated
weapon/mode pairs, not a rule that equates every damage factor with bullet
count. `tests/sc64_openbw_ordinary_field_world.cpp` and
`tests/sc64_openbw_turret_world.cpp` retain the source-oracle harnesses; they
require separately supplied pinned OpenBW and authenticated private tables.

The field protection here suppresses damage for these normal-hit weapons at
release. The shared source miss RNG draw, zero-threshold miss, exact flight
geometry and timers, other actor/weapon pairs, Firebat splash, Zergling melee,
Scourge, native obstacle interactions, saved active shots, all-product fit and
production admission remain open. These gates are all off in normal builds.


### Optional source rectangle table across products

`tools/sc64_dark_swarm_rects.py` accepts the authenticated private `units.dat`
SHA-256 `54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df`
and emits an ignored 228-row compact source-rectangle header. The authenticated
DAT can be extracted from the owned ROM with the existing
`tools/sc64_openbw_oracle_tables.py` using its required extractor argument and
a new ignored output directory. The rectangle generator rejects other input
and does not overwrite an existing header. An independently executed pinned
OpenBW loader agreed with all 228 generated rows; the retained generated header
hash is `38d9daa88b97a6b86670180b6bb49bb44923ec8423dd240b2952d0c2d99e67c1`.
`tests/sc64_openbw_unit_rect_world.cpp` and
`tests/sc64_compare_source_rects.py` retain that source comparison. A sanitizer check separately rejects negative, out-of-world
and `INT_MAX` noncompact coordinates before doubling.

The separate, disabled `SC64_SHARED_DARK_SWARM_SOURCE_RECTS` option lets the
staged field query use this table without P7 actor facts. Sanitized host
fixtures using Terran, Zerg, P7 and Showcase product flag vectors each reached
a field through the actual menu and checked the rectangle query. The P7 run
used a newly generated isolated fact table matching the staged Defiler gate;
no previously prepared fact header was overwritten. The Showcase run also
checked source boundaries, the staged ordinary release subset and Yamato.
These are experimental host checks with additional disabled-stage flags, not
four normal product builds or production admission. Full actor sight for
profiles lacking Z/P facts, source RNG, remaining attacks, effect save state,
rendering qualification, and ARM/hardware fit remain open.


### Natural ordinary Defiler energy check

A sanitized synthetic Showcase-host scenario used the normal raw-46 spawn
energy of 50 and advanced the real game update loop without assigning live
energy or owner. After 1,999 game updates the actor had enough energy; ordinary
B/menu/point/A input then produced the staged projectile and field. The
fixture begins with a test-selected spawned unit. This supports the bounded
unupgraded stationary path, not the unimplemented Metasynaptic research,
hallucination rules, native source scheduling outside the verified slices,
or production admission.


### Optional restricted southeast approach

`SC64_SHARED_DEFILER_SE_APPROACH_PROBE` adds a further disabled slice: a
stationary raw-46 caster with initial source heading 104, beginning on a
64-source-pixel grid point and moving southeast along equal source-axis offsets from 280 through 448 pixels on an
unobstructed path. It stores source-Q8 position and keeps source facing and
velocity direction separate while walking toward successive 64-pixel
waypoints. The actor stops on the source order-check cadence and turns toward
the spell target while waiting for the later debit. Existing game terrain and
actor collision checks can stop this slice; they are not a claim of OpenBW
pathfinding equivalence. Other actor identities, diagonal directions, initial headings, unaligned
starts, distances outside this slice and blocked corridors fail closed.

At initial source phase zero, actual-game menu fixtures matched 120 source
positions, headings and debit timing at each of five target offsets: 280, 300,
320, 384 and 448 source pixels. The 280 case also matched 40 exact Q8 positions
from a separate instrumented pinned OpenBW run. A sanitized guard fixture
checks unsupported diagonals and unaligned starts, and actual B/menu STOP
interrupts a supported moving cast without debit. The synthetic flat-world
oracles and private source inputs do not establish other source order-timer
phases, many-actor scheduling, arbitrary pathfinding, art, hardware or
production performance. An actual noncompact-Unit executor fixture rejects
`INT_MIN`/`INT_MAX` caster coordinates on both axes and an invalid target
before coordinate arithmetic, under ASan/UBSan. Explicit source raw identities
that disagree with their runtime type are also rejected before regeneration or
casting; the negative fixture exercises both stationary and moving requests.
All normal flags remain off.

### Restricted Unclean One southeast experiment

The separate `SC64_SHARED_DEFILER_SE_HERO_PROBE` admits raw 52 only when both the existing hero and southeast probes are enabled. Five pinned OpenBW raw-52 source worlds at source-pixel offsets 280, 300, 320, 384, and 448 were compared with actual host `game_update` execution: all 600 position/heading samples and each debit matched, with 40 exact Q8 samples at offset 280. Both synthetic fixtures seed 100 energy for this motion comparison; the separate natural-regeneration and campaign-import fixtures cover their stated narrower behavior. An actual menu STOP test passed, and the existing hero gate still refuses the approach when the new gate is absent. The retained source inputs and results are under `.local/defiler-se-hero-set` and `.local/defiler-se-hero-verification`; the opt-in Make targets require those private oracle logs.

This is a stationary, grid-aligned, clear southeast slice with initial heading 104 and source order phase zero. Other headings, diagonals, moving starts, terrain/pathfinding equivalence, ARM fit, and normal or P9 admission remain unqualified; normal flags remain off.

### Experimental ARM feasibility, pre-current shared source

Four actual ARM product compiler vectors at `d693a9c` were linked with the staged ordinary projectiles, Defiler executor, hero, restricted approach, source rectangles and field art enabled, using newly generated hero-matched P7 facts. The original memory guards and actor limits remained in place. Terran, Zerg and P7 linked; Showcase failed because `.bss` ended at `0x02040454`, 1,108 bytes beyond physical EWRAM and 9,300 bytes beyond the `0x0203e000` working-RAM safety limit. Its IWRAM ended at `0x03004c80`, within the `0x03005000` guard. This is a diagnostic on the earlier shared base, not qualification of the current shared tree.

The map identifies per-unit Defiler state (8,640 bytes), per-unit pending ordinary attacks (5,760), per-unit field state (2,880), 100 ordinary bullets (2,000), 100 Defiler projectile records (1,200), and the shared 100-slot reservation arena (402); these are concurrent current allocations, not a production-ready common pool. The ARM `-fstack-usage` output is a per-function static-frame report, not a proven maximum call-chain bound: on the Terran build `race_load_game` reports 3,600 bytes and `sc64_defiler_clear_approach` 72 bytes. Field/projectile/actor lifetimes and slot reuse must be preserved in any compaction. Normal flags remain off. Compiler vectors, generated P7 header, maps, logs, exit codes, and stack-usage files are retained in `.local/defiler-arm-diagnostic-d693-rerun`; the prior failed four-profile attempt is also retained.

The initial ARM attempt exposed an unused `<string.h>` include in the reservation header, unavailable in the freestanding toolchain. It was removed before the retained links; no behavior changed.

An additional Showcase-only core diagnostic omitted the ordinary projectile stage and approach probes while retaining the caster, hero, field, rectangle and art gates. It linked with `__bss_end` at `0x0203cf84` (4,220 bytes below the working-RAM limit) and `__iwram_end` at `0x03004c80`. Its output is retained in `.local/defiler-arm-core-showcase-3a6`. This establishes only that bounded core's link feasibility; it does not establish room for common projectile contention, general movement, or the current shared product.

### Shared typed payload experiment

Ordinary and Dark Swarm projectile payloads now occupy a union indexed by the existing 100-slot typed arena. Accessors validate the family, slot, generation, and owner before scans, target cancellation, or resets touch a payload. Mixed-slot tests exercise family isolation, stale generation, reuse, ordinary reset while a swarm remains live, and full game reset; actual game-update field/ordinary and Defiler guard fixtures and separate family-only sanitizer fixtures passed. These tests do not establish all source projectile families or full save support.

The full-stage Showcase ARM relink moved `__bss_end` from `0x02040454` to `0x0203ffa4`, saving 1,200 bytes with no reduced arena capacity. It still exceeds the unchanged `0x0203e000` working-RAM bound by 8,100 bytes; IWRAM is unchanged at `0x03004c80`. This remains an unqualified experimental stage, on the earlier shared base. Arguments, logs, map and static stack reports are retained in `.local/shared-payload-union-v1`.

### Experimental field lifetime in unit rows

`SC64_SHARED_DEFILER_UNIT_FIELD_PROBE` stores a reserved field's raw 202 identity, source position and 900-tick plus four-tick death tail in its existing dead `Unit` row. The row remains noncolliding and untargetable through its `alive == 0` state, and the common free-slot predicate reserves it until the tail expires. The gate defaults off; the prior sidecar remains the default experiment. An actual menu-cast/game-update/expiry fixture, field raster fixture and all four product field-query fixtures passed under ASan/UBSan. A separate synthetic saturation case reserved every unit row, verified no allocator slot was available, held the play screen active (there were no living players), and verified expiry and subsequent reuse. The failed initial saturation receipt, in which the game correctly left play mode and stopped ticking, is preserved. A former-Carrier-slot and current shared cache integration case remains to be tested.

With the existing typed payload union and this unit-row gate, actual experimental ARM links passed for Terran, Zerg and P7. Showcase still failed its unchanged RAM guard, with `__bss_end` `0x0203f464`, saving a further 2,880 bytes versus the sidecar stage but remaining 5,220 bytes above `0x0203e000`. IWRAM ended at `0x03004c80`. These links are feasibility diagnostics on the earlier shared base, not production qualification. Focused host receipts are in `.local/defiler-unit-field-v2`; art and ARM arguments, logs, maps and static-frame reports are in `.local/defiler-unit-field-arm-v1`.

### Bounded actor flag packing

The experimental Defiler actor state packs Boolean flags, phase (0–2) and source-script age (0–4) into a 16-bit field; motion coordinates, heading and source cadence are unchanged. Actual cast/expiry and four profile field-query sanitizer fixtures passed in both unit-row and original-sidecar configurations. Five raw-46 and five raw-52 pinned-source southeast trajectories each matched all 120 integer position/heading samples and debit, plus 40 Q8 samples for each raw at offset 280. Inputs use controlled synthetic starting conditions, not a full movement claim.

Full-stage Showcase ARM `__bss_end` moved from `0x0203f464` to `0x0203ece4`, saving 1,920 bytes with all actor slots retained. It remains 3,300 bytes above the unchanged working-RAM bound; IWRAM remains `0x03004c80`. Host logs are in `.local/defiler-unit-field-compact-v1`, source comparisons in `.local/defiler-compact-base-verify` and `.local/defiler-compact-hero-verify`, and the ARM map and arguments in `.local/defiler-compact-trajectory-v1`. This does not qualify the current shared tree or enable the stage.

### Experimental per-actor state union

`SC64_SHARED_ACTOR_STATE_UNION_PROBE` stores mutually exclusive Defiler state and ordinary attack windup in the same per-unit row with an explicit discriminator. Initialization, ordinary scans/cancellation, Defiler scans and STOP, and slot reuse respect that discriminator. An independent review identified a malformed ordinary-to-Defiler selection path and a former-caster field-slot discriminator that needed explicit clearing; both were fixed and re-reviewed. Tests cover those cases, a real released spell using its dead caster's full-pool slot for a field and eventual reuse, actual ordinary lifecycle, mixed projectile families, synthetic saturated field expiry, old-sidecar control and four profile field queries. Raw 46 and raw 52 each matched five pinned-source southeast trajectories, including 40 exact Q8 samples per raw. The gates remain off by default.

The four full-stage ARM links on the earlier shared base pass their existing guards. Showcase ends at `__bss_end = 0x0203d754`, 2,220 bytes below the `0x0203e000` guard; IWRAM ends at `0x03004c80`. All 240 unit slots and 100 projectile reservations remain available. The result does not include the later shared/Carrier changes, and neither combined fit nor production activation is qualified. Final vectors, maps, stack reports and logs are in `.local/defiler-actor-union-final`; the four profile host fixture receipts are in `.local/defiler-actor-union-v2`; source comparison outputs are `.local/defiler-actor-union-base-verify` and `.local/defiler-actor-union-hero-verify`.

### Current-parent integration diagnostic

Merge `ca69081` combines the compact staged service with `ef34c9c` and preserves both contextual Carrier and Defiler commands and both P8 validation and staged save refusal. With the full Defiler/ordinary experiment gated on, the four actual product host field-query fixtures and all four ARM links passed. Terran, Zerg and P7 `__bss_end` values were `0x0203931c`, `0x020398f0` and `0x0203ac40`; Showcase ended at `0x0203d764`, leaving 2,204 bytes below the unchanged `0x0203e000` guard. Showcase IWRAM ended at `0x03004c80`. The newly generated P7 fact header SHA-256 is `0516cabbe0937837a84867d41aaec8cea721822f681a2a0d2e058f19ec178a67`; all 109 retained input hashes matched. Logs, compiler arguments, maps and stack reports are in `.local/current-integration-ca69081`.

These results exclude the separately developed later Carrier backend, and they do not qualify full common projectile contention, complete Dark Swarm behavior, performance or production activation. A synthetic P7 integration fixture passed actual Carrier and Defiler command dispatch, a released spell using a former Carrier slot, field position and expiry, save refusal, and post-expiry reuse. Its setup seeds spell energy and fills the actor pool. The receipt is `.local/current-carrier-defiler-v2`; the earlier failed fixture build is retained in `.local/current-carrier-defiler-v1`.

### Combined Carrier backend and Defiler experiment

Private merge `e2c0d37` combines `8c813bd` and the separately gated Carrier backend `12a42c2`. Both services remain disabled in normal builds. Conflict resolution preserves their contextual commands, staged save/load refusal, distinct added identities and the common unit-slot reservation predicate. An independent read-only review found no concrete conflict-resolution blocker.

With both experimental services and the compact actor/payload and unit-row field gates enabled, actual Carrier and synthetic cross-service fixtures passed under ASan/UBSan for all four product vectors. The cross-service fixture drives Carrier and Defiler commands and a released spell into a former Carrier slot, then checks field expiry, save refusal and slot reuse; it seeds spell energy and fills the actor pool. The Carrier fixture also exercises its render-arena canaries. All four actual ARM links passed unchanged guards: Terran, Zerg, P7 and Showcase EWRAM ends were `0x0203a77c`, `0x0203ad48`, `0x0203ac40`, `0x0203d8f4`; Showcase IWRAM ended at `0x03004d18`. Showcase has 1,804 bytes of working-RAM headroom in this vector. The original retained input hashes were unchanged; the combined source-derived P7 fact SHA-256 was `0516cabbe0937837a84867d41aaec8cea721822f681a2a0d2e058f19ec178a67`. Logs and arguments for P7/Showcase and all ARM links are in `.local/combined-initial-e2c0d37-v2`, and Terran/Zerg host receipts are in `.local/combined-tz-e2c0d37-v1`. The initial fixture-only guard failure is preserved.

The Carrier and experimental shared projectile storage are still distinct; these checks do not prove a common source-equivalent 100-bullet pool, Carrier save migration, arbitrary Defiler pathing, source-wide hit/RNG behavior, CPU performance, or normal product admission.

### Interceptor launch under a field

Under `SC64_SHARED_DARK_SWARM_PROBE`, the Interceptor projectile adapter queries the target at launch and sets the existing `hit_flags_low2` bit 0 for the guaranteed field miss. Unknown target geometry fails closed before either projectile allocation. The existing movement kernel consumes that bit to suppress damage; the packet layout is unchanged. Pinned OpenBW header SHA-256 `5ee316ca2c008c9252f7fde7444ca5dc7f35440549be2576dc37fa228fb31ae6` performs the source dodge decision during bullet initialization. Its Interceptor bullet owner is the fighter itself, including a detached live fighter; the separate Scarab case uses its parent.

A synthetic launched-child fixture uses a field created through the actual pad/game-update path and drives the real Interceptor projectile adapter through impact. The initial fixture and Carrier regressions passed for all four product vectors; the extended P7 and Showcase fixture additionally passed target crossings after launch, detached children and unknown geometry with unchanged arena/pool occupancy. Four actual combined ARM links passed unchanged guards; Showcase EWRAM/IWRAM ended at `0x0203d8f4`/`0x03004d18`. These ARM arguments contain `-g0` followed by `-g`, so the retained ELFs include debug information. Receipts are `.local/interceptor-swarm-v1`, `.local/interceptor-swarm-v2` and `.local/interceptor-swarm-final`; the final ELF compression receipt verifies unchanged bytes, modes and mtimes.

The stage does not model source miss displacement or general dodge RNG. Source hit bit 1 represents hallucination; a separate proposed Carrier save candidate validator that rejects all nonzero hit flags is not compatible with these semantics without further qualification. Carrier/Defiler persistence and production activation remain closed.

### Generic Carrier and Wraith air contention

Private merge `67a24d1` combines the compact shared-projectile/Defiler work with Carrier checkpoint `94e3928`. The generic Carrier backend uses the existing typed 100-slot reservation arena on actual Interceptor release and returns its reservation at projectile retirement; its projectile records remain in their existing separate arena. The staged ordinary path now also admits source-profile Wraith raw 8 air weapon 15, one shot. No canonical gate, actor cap, projectile cap or memory guard changes.

Pinned OpenBW flat-world runs for Wraith air weapon 15 recorded one own bullet at 98 and 99 occupied slots, and none at 100 while weapon cooldown was spent. Retained command arguments and logs are `.local/wraith-air-source-oracle-v1`. The same oracle's Zealot raw 65 ground weapon 64 produced direct melee damage even when the bullet pool was full; it remains outside the projectile quota stage. Those source worlds are controlled synthetic maps, not native N64 execution.

Four source-frozen ASan/UBSan product-vector fixtures pass actual Interceptor versus Marine, two-shot Scout and Wraith air contention with 98/99/100 occupied reservations, including no windup allocation, partial two-shot release, spent cooldown at capacity, and retention through post-hit lifetime. A Wraith full-capacity case advances the real `game_update` dispatcher. These tests seed a launched fighter and positions; they do not claim paid training. Separate four-vector fixtures drive paid Stargate production, menu purchase, Interceptor training, launch and damage through `game_update`, and observe both an actual typed reservation and its later retirement. Exact source, flags and outputs are retained under `.local/carrier-ordinary-paid-v1`; `-g0` is the final debug option.

This is a private, default-off stage. Source-exact timing for arbitrary motion/phase, other ordinary weapons and all remaining bullet families, source-global capacity parity, complete hit/RNG behavior, current combined ARM/memory qualification, save migration and production admission remain open.

### Battlecruiser stock attacks in shared quota

The default-off ordinary stage additionally handles Battlecruiser raw 12 stock ground weapon 19 and stock air weapon 20, each a source-profile one-shot attack. Yamato retains its separate path. Six pinned OpenBW synthetic flat-world capacity runs show one source bullet allocated at 98/99 occupied and none at 100 with cooldown spent; three additional ground-under-field runs confirm the source field miss. Exact arguments and outputs are in `.local/bc-stock-source-oracle-v1`.

Four actual product host fixtures pass 98/99/100 Interceptor versus BC stock ground/air contention, cooldown, damage and retirement alongside Marine/Scout/Wraith regressions. Four field-query fixtures also pass BC ground attacks inside and outside a real pad-created field, plus the existing ordinary and Yamato controls. These use synthetic positions and target health, not a full battle. Logs and compiler arguments are `.local/carrier-ordinary-bc-v1` and `.local/bc-field-regression-v1`. An independent source review found no concrete issue in this bounded stage.

All four full-stage ARM links passed unchanged memory guards on this private combined source. Terran/Zerg/P7/Showcase `__bss_end` values were `0x0203a77c`, `0x0203ad48`, `0x0203ac40`, `0x0203d8f4`; Showcase IWRAM ended `0x03004d18`. The sole debug option in each recorded argument vector is `-g0`. Maps, arguments, output and exit statuses are retained under `.local/bc-stock-arm-v1`. Full source-global timing and capacity, other weapon families, current P8 cache integration, persistence and production activation remain open.

### Combined P8 terrain-cache parent

Private merge `477b507` combines `3278955` with public P8 parent `85c9a47`. Carrier/Interceptor validation retains the public early identity rejection through the generalized shared aliases, and all fixture modules remain available. The shared Carrier experiment reserves 4,816 bytes at the start of OBJ tile VRAM; the terrain cache uses 48 rows (11,520 bytes) after that reservation. Normal Carrier, Defiler and projectile stages remain off.

On this exact runtime source, the paid Carrier/Interceptor lifecycle, ordinary quota fixture including Wraith and BC stock shots, and real field-query fixture each passed under ASan/UBSan for all four product vectors. The paid Carrier fixture observes an actual typed reservation and retirement and exercises arena/cache canaries. Four full-stage ARM links passed unchanged guards with `-g0` as the only debug option. Terran/Zerg/P7/Showcase EWRAM ends were `0x0203a77c`, `0x0203ad48`, `0x0203ac40`, `0x0203abf4`; Showcase IWRAM ended `0x03004d18`, leaving 13,324 bytes below the working-RAM guard. Compared with the otherwise corresponding older combined Showcase link, EWRAM fell exactly 11,520 bytes. The 109 retained input hashes were unchanged; new source-matched P7 facts hash to `0516cabbe0937837a84867d41aaec8cea721822f681a2a0d2e058f19ec178a67`. Commands, logs, maps and `.su` reports are in `.local/p8-combined-477b507`; its host compression receipt verifies identical bytes, mode and mtime.

These are gated synthetic and qualified component cases, not full shared 100-bullet source equivalence, source-wide hit/RNG behavior, general Defiler approach, Carrier/Defiler save migration, current-product performance qualification or normal admission. The later private Carrier save branch and unrelated performance branch are not included.

### Ghost air attack quota slice

The default-off ordinary stage admits raw 1 air weapon 2, one shot, using the existing source-profile windup, shared reservation and retirement path. Three pinned OpenBW synthetic flat-world air runs at 98/99/100 occupied bullets exited successfully: the first two allocated one shot, the full pool allocated none, and cooldown was spent. Ground Ghost is deliberately not admitted: its no-field source probe later exited when its STOP harness was rejected, so those logs are retained only as incomplete observations. Source commands and logs are in `.local/ghost-stock-source-oracle-v1`.

All four actual current-P8 product host vectors passed Ghost air 98/99/100 contention with the existing Marine/Scout/Wraith/BC controls; setup uses synthetic launched fighters and positions. An independent source review found no concrete issue in the narrow gate. All four full-stage ARM links passed unchanged guards, with only `-g0` as debug option: Showcase EWRAM/IWRAM ended at `0x0203abf4`/`0x03004d18`, unchanged from the combined parent. Arguments, host outputs, maps and exits are retained under `.local/ghost-air-quad-v1` and `.local/ghost-air-arm-v1`. This does not qualify Ghost ground attacks, arbitrary source timing, full shared projectile capacity or production admission.

### Draft source-boundary and exact-source checks

Source `495d698` fixes the original-art menu-count fallback, moves experimental fixture includes after their prerequisites, and adds the shared Carrier and derived Fleet Beacon fact gates to generated fact fingerprints. The latter means earlier experimental fact headers must not be reused: the uniquely regenerated P7 fact header for this source hashes to `e1fbb549ac5d9cf886762e04c150657d2031187e0912510967bc3d491a19b014`. The source-boundary checker passed 1,458 indexed text files; no private generated data is included. A fresh unprepared detached worktree passed `./dev build` and `./dev test`: main Python discovery ran 890 tests with 23 skipped, with native sanitizer fixtures and auxiliary checks also passing. Receipts are in the detached `defiler-draft-cleancheck-20261031/.local/draft-default-495d698`.

With the experimental gates explicitly enabled and the new facts, quota, paid Carrier and field-query host fixtures passed for all four product vectors. Four experimental ARM links passed unchanged guards with `-g0` as the only debug option: Terran/Zerg/P7/Showcase EWRAM ends `0x0203a77c`/`0x0203ad48`/`0x0203ac40`/`0x0203abf4`; Showcase IWRAM ended `0x03004d18`. Logs, actual arguments, maps, exits and verified same-byte compression receipts are under `.local/draft-gated-495d698`. The initial full test and link attempts and the 2-GiB floor stop were retained rather than overwritten.

These checks support a default-off draft checkpoint only. General Defiler movement, global projectile-family quota and source RNG/damage fidelity, full save migration, native N64/hardware behavior and current-product performance remain unqualified.
