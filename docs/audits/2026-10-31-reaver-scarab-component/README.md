# Reaver Scarab lifecycle component

This source-only component is not wired into gameplay. Reaver admission remains
blocked in ordinary skirmishes until actual Scarab actors, combat, art, saves,
and their shared-product qualifications are complete. The helper introduces
no live game state or save fields by itself.

`sc64_scarab_lifecycle.h` models payment and a bounded five-entry production
queue, not ammunition stored only on the parent. A successful order pays 15
minerals immediately. The current owner's capacity is five, or ten with the
capacity upgrade; completed inside children plus the paid queue count against
that limit, while a child already outside does not. A queued entry does not
reserve an actor slot: the caller allocates the actual active child when
production starts and retries if the actor pool is full. The helper uses 105
source ticks for construction. A newly launched child is an existing inside
actor, only one can be outside, and the model exposes 60 source ticks of parent
cooldown, 90 of child life, and seven initial no-collision ticks. The caller
supplies source ticks; these are not GBA frame counts.

The pinned OpenBW reference supports the queue capacity and payment behavior
(`bwgame.h` around 2293 and 17330), inside-plus-queue capacity rule (1805–1819
and 17505–17508), creation/retry/completion flow (8330–8369), one-outside
release and cooldown (7396–7410), and Scarab life/no-collision state
(7433–7469). The pinned build-time datum and helper's scheduler still require
an integration-level test of allocation, first progress and completion tick
ordering and of each timer relative to the engine's source clock. This model
is not a cycle-exact OpenBW implementation.

## Integration contract

- Callbacks must perform atomic allocation, completion and launch, validate
  actual child slot, parent, owner and phase, and report failure without side
  effects. A callback that reports a successful allocation with a negative or
  reused slot violates the contract and cannot be rolled back by this helper.
- Inventory can remain above the current owner's normal capacity after a
  transfer or upgrade change. Existing paid work can continue, but further
  orders must obey the new owner's current capacity. Inside plus queued still
  cannot exceed the absolute supported maximum of ten in this component.
- STOP must order an outside Scarab to self-destruct while preserving inside
  children and paid queue state. Both ordinary STOP entry points route through
  `stop_selected` in `game.c`; integrate that behavior there rather than
  treating every incidental order change as STOP. Parent death, child death, transfer, target
  loss, movement, collision, splash, and impact also require actual actor
  lifecycle handling; none is implemented by this helper.
- Before a load mutates live state, validate the candidate world's child slot
  bounds, uniqueness, owner/parent links, phases, paid queue, active build,
  clocks and lifetime, with atomic rejection on failure. Define a versioned
  encoding, legacy migration and capacity behavior for every active codec.
  `Unit.carry` is not an isolated ammo field (STOP and other code can clear it),
  `target` is used by transport, and the P7 typed codec restricts `work_timer`
  to 11 bits and `flags` to 14 bits. No field in this component is permission
  to reuse those fields without a complete ownership and persistence design.
- Qualify normal paid production, queue saturation and delayed allocation,
  owner transfer, STOP/death, attacks and saves across Terran, Zerg, P7 and
  Showcase builds. The real child must occupy a real actor slot; do not use a
  parent-only ammo count or reserve slots for future queued entries.

## Local test

`python3 -m unittest tests.test_sc64_scarab_lifecycle` compiles the isolated
component fixture with ASan/UBSan. It covers payment, queue/capacity limits,
full-pool retry, active-child completion, wrong-owner rejection, owner-capacity
change, one-outside rule and exposed timers. The fixture uses a small fake actor
pool to test the contract; it is not evidence of ordinary-game acquisition,
real Scarab combat, SRAM persistence, hardware timing, or product admission.
