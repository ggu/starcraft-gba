# Native Blood Bath counts the airborne last Command Center

An ordinary controller replay of single-player Blood Bath Melee
(Terran human, one Terran CPU) lifted the human's sole Command Center.
The native building count remained one while it was visibly airborne,
both result bytes remained zero, and the other owner stayed active.
This resolves one transitional state in the [native end-rule
audit](2026-11-13-n64-blood-bath-end-rules.md).

The headless ares replay used the authenticated one-CPU opening
with fresh isolated saves. It selected the Command Center with ordinary
analog movement and A, then pressed the displayed Liftoff command
(C-Down). In the qualified two-frame replay, the same raw-106 record
`0x802d6ca0` remained linked to human owner 2 before and after takeoff.
Its status at actor offset `0xd8` changed from `0xc0002800` to
`0xa000e800`; the first retained image showed a shadow and the Land
command. The broad building count at `0x8010ac80 + 4 * owner` stayed
one for owners 1 and 2. The pending result at `0x8010dbe0 + owner` and
published result at `0x800b936c + owner` both stayed zero for those
owners in all three samples (steps 4201, 4571 and 5004). The final image
showed the same building airborne after a rejected landing order.
The samples and images measure the observed states, not exact game
update boundaries.

A second passing replay chose a different landing point. At step 5121
the same actor had moved to (249, 265), held order byte `0x47`, and
still showed the airborne status and Land command. Both counts stayed
one and both result bytes stayed zero. This qualifies survival through
the landing request and movement, but the run ended before a grounded
post-landing state was observed. The second controller plan hash is
`9716619867927662ffd002bb7fb89c2adb314c2198f4a8ddddd5709eb60bf699`,
manifest `ce856a36246c020db6f9c209ac2902451862c8b7ab6c278f11b6b6fe3510df06`,
final RAM `985e613b950d661265e8a56bcd9871347801f3c6b70c2e0686ad4a4b3edcdff4`,
and final image `f171c7060b88a4eefac46d11d8f2db406077fa9a9da9d34b92956640e51f1919`.

The pinned observer used its authenticated same-process hybrid transition
at step 4200 and read-only cache-aware snapshots afterward; no guest
memory edits, cheats, prior save states or GUI controls were used. The
process exited 0 after 5012 steps. Its wrapper validated the exact input
and capture schedules, unchanged inputs and all owned output guards
(60,781,039 bytes of run output). The earlier three-image exploration
was rejected by the wrapper's fixed two-image witness validation and
is not needed for this result.

| Retained artifact | SHA-256 |
| --- | --- |
| Owned USA ROM | `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020` |
| Hybrid observer executable | `54d100a9b56d342b07c9a0fdf8fbffedeab0134010359eaa57952d0be0b0b75c` |
| Controller plan | `2b6c76d66b7f93cfe0a544ba203f4adf130bf712577059c36893c33a3fd0faef` |
| Passing observer manifest | `6a0d4f599269c0b05a7323931c1cd71f3260e580535bebe5f79e11e98c9de826` |
| Grounded RAM | `b485d972e7db7afe6530c8b858f4e4a5764f27f691fc928c003573995d8499b0` |
| Airborne RAM | `31a7bc2726853071bbfde7763e8556da8ab123d56cbcecbb6ee8543921a7a3ab` |
| Post-rejection RAM | `e2c3eef41da1827646f8c5542122387769d0f3a599551f7fe634cfb08be1e55b` |
| Airborne image | `ecaf8f670128379649b07a8351c956edb6e2ad6013520b05e7898660740eaa73` |
| Rejected landing image | `f694d59ae64c5223d3ebd75fcc0eaa43e3cc393fa7464889c0c9ef0e9ecbda56` |

GBA skirmish outcome counts live buildings without filtering the flight
flag. Its shared Science Facility flight does not allow Command Center
liftoff, and the late Terran campaign's Command Center flight is inactive
in skirmish. This comparison supports keeping the result count independent
of flight; it does not qualify a GBA skirmish flight feature. CPU survivor
transfer, interrupted foundations, morphs, simultaneous end conditions
and projectile behavior remain separate from this test. Nothing here
changes the runtime.

## GBA implementation boundary

Use one skirmish flight predicate for Command Center, Barracks, Factory,
Starport and the existing Science Facility; native runtime evidence here
covers only Command Center. Keep the separate late campaign save codec
working. Its `race_late.flight_goals[]` packs two 11-bit coordinates and
active/land flags, but its runtime is campaign-only. Shared Science
Facility flight uses `Building.flags` bit 2 for airborne, `level` for
pending landing, and `build_progress`/`queue_progress` for the landing
coordinates. Both compact building codecs already store these row fields.
Before extending the shared encoding, check each supported type's uses of
those same fields, including production and add-ons. Reserve a validated
marker if older readers would interpret a new airborne row as grounded;
validate supported type, owner, queue and construction state, coordinate
bounds, and load-time occupancy before world publication. Keep older
grounded and Facility-only records readable. Do not reuse the late mission
sidecar or silently reinterpret a producer's queue or progress.

A common predicate needs to drive menu/icons, movement and landing
reservation, path grid and placement, air-target classification, and
selection/rendering. The airborne producer must not train or supply an
attached add-on service; a lifted Command Center must cease to accept
worker deposits and invalidate cached routes. Detach/relink add-ons
through the existing service paths rather than clearing their slots.
Keep building identity, ownership and defeat census independent of
ground collision and depot eligibility. Before promotion, test that a
sole lifted Command Center can land after SRAM reload, that air/ground
targeting differs appropriately, and that both the normal and marked
save readers apply their documented admission policy.
