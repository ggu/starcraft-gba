# Scarab flight and damage integration notes

These are authenticated USA N64 data facts interpreted with the pinned
OpenBW PC reference `4b046d5f65302b10cb0a745f0fecd37ec85b20a8`.
They are not a native N64 execution trace. The source files below are
upstream references; no extracted game data or upstream implementation is
vendored here.

## Actor and animation identity

Unit 85 resolves through flingy 48, sprite 200, image 147 and IScript 160.
Its source collision bounds are left/up/right/down 2 each, a 5×5 inclusive
box. Half-scale geometry must preserve the edges; do not round this box to
zero. The actor has 20 HP, 10 enabled shields and a source invincibility
flag. Pinned OpenBW UI explicitly excludes Scarabs from selection.

The flingy DAT defaults (top speed, acceleration and halt distance each
1 in their fixed-point fields, turn rate 27, movement type 2) are not a
working movement policy by themselves. Walking IScript sets flight speed
2048 Q8, executes move 16, emits sprite 373 and waits one source tick in a
loop. OpenBW interprets the move operand as a fixed-point speed command.
Do not choose a GBA speed solely from either the DAT default or the preceding
speed opcode; scheduling, clamping and turns still need a dedicated proof.

IScript 160 SpecialState1 emits weapon 82 and order signal 1 at its local
tick zero, then enters a long idle loop. That wait is not the impact delay.
The actor's Death animation emits image 444 and ends after a one-tick wait;
image 444 animates frames 0–9 at one tick each. This is distinct from weapon
82's impact image, so it must not substitute for the weapon effect.

Weapon 82 resolves through flingy 172, sprite 370, image 503, IScript 292.
The image's Init waits one tick and signals, then has a local damage opcode
at tick seven. Its Death branch instead damages at local tick five and ends
one tick later. OpenBW's type-5 bullet starts Init and can switch to Death
on the initial signal. The absolute damage update therefore requires the
actual bullet/animation scheduler; neither local script tick is sufficient
as an unconditional arrival-to-impact delay. Bullet creation can fail when
its pool is full and must not manufacture damage on failure.

## Orders, loss and expiry

Pinned `bwgame.h` order_ScarabAttack (around 7433) initializes a 90-source-
tick removal timer and seven initial no-collision ticks. This order runs
in the fast dispatch before normal order throttling. Global timer updates
precede orders. Reaver release sets a 60-tick timer and permits only one
outside child, but its own throttled order means that is not proof of an
attack exactly every 60 updates.

Arrival is within ten source pixels: half weapon 82's inner splash radius,
using inclusive unit rectangles and the source integer length approximation.
It starts SpecialState1 and waits for its signal before retiring the actor.
If the target dies or hides, reference cleanup clears the unit pointer but
retains the move-target position. An already-flying Scarab continues to that
position and must arrive exactly there or expire; it does not pick a new
target automatically. A null target before initial setup instead uses the
order's stored target position.

Parent destruction kills inside children and detaches outside children.
STOP sends outside children to SelfDestructing, whose handler kills the
actor. Lifetime expiry also kills the actor. These paths do not by
themselves emit the arrival weapon. A detached shot retains its player
owner but has no parent pointer for unit kill credit. Transfer cancels and
refunds the queue before changing ownership; preserve the actual source
upgrade-grant and child-owner policy when adding that hook.

## Splash packet

The authenticated weapon has ground target flags, enemy splash, NORMAL
damage, 100 base damage plus 25 per specific Scarab damage upgrade, and
inclusive source radii 20/40/60 for full/half/quarter damage. Distance is
from the impact point to the nearest point of a target's inclusive bounds,
then the source integer `xy_length` approximation, not Euclidean distance
between centers. Inner splash can include burrowed targets; the outer two
bands exclude them. Same-owner targets are excluded except the explicit
bullet target; do not replace that test with a blanket allied-player test.
Flying and invincible targets fail weapon eligibility. Hidden units are
removed from the spatial finder; player fog is not a splash immunity rule.

Source damage divides the tier before armor/shield processing and includes
fractional arithmetic and other effects. Existing siege distance helpers
are candidates for reuse after checking bounds, but its radii, explosive
size scaling and complete resolver are not Scarab rules. Mine splash and
the single-target impact queue are not substitutes for delayed ground AoE.
Any adapter must define bounded bullet capacity, lost references, event
ordering, rounding and save representation before claiming fidelity.

Primary reference entry points are
[`bwgame.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h)
orders, fighter destruction, target cleanup, create_bullet and
bullet_deal_splash_damage, plus the pinned
[`data_loading.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/data_loading.h)
schema and
[`ui/ui.h`](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/ui/ui.h).
The ignored numeric audit records the ROM/data/source hashes and exact
column/script observations; its receipt hash is
`f3cfbd2bf3640b4bf7217f3454805e1bc8668eaa1b4034108bfc6413d976fb63`.
