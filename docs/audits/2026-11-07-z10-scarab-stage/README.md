# Z10 Scarab integration boundary

## Later HOST custom-controller stage

The separate runtime `60c6c12c` connects the existing paid child queue to an
explicit, default-off `SC64_SCARAB_AI_RESTOCK_HOST_STAGE`. At most one request
is attempted on the existing source-update hook when no job is queued. This
is an authored automatic-purchase policy, not observed N64 AI timing. It binds
payments to active custom-game Protoss controllers and refuses campaign town
wallets. A transient payer witness prevents refunds being redirected after a
controller change. Capture remains refused even after the old queue is settled;
authenticated transfer and child settlement are still unfinished.

The actual-engine authored HOST fixture passes with ASan/UBSan and all 593
prepared inputs unchanged. It covers distinct human/AI wallets, a 15-mineral
debit, queue refusal, shortage, stale race mappings, transfer refusal and
cancellation, training, lethal parent damage and slot reuse under a different
payer. It checks that all four save/load entry points refuse this unsaved stage.
The source-boundary check also passes. The first compile correctly rejected the
incompatible worker-cargo/Scarab codec combination; only the isolated fixture
recipe now excludes the cargo flag, and the runtime guard is unchanged.

This is not an ordinary-input, ARM, campaign, complete flight/impact, save,
performance or hardware qualification. Normal product flags are unchanged;
the full default suite and composed release qualification remain outstanding.
Private receipt SHA-256:
`17035068e6fd7e292a0ac34de4a2c12d08ddca30b4e4e9fcbb6217527b7ae036`.

## Earlier source audit

Source-only review against main `df5c8ee8`. No generated inputs were copied,
native jobs run, or feature enabled. Z10 has no qualified ordinary-input win.
In an older paired, 31,000-call HOST trial using an experimental Egg service,
the generic Reaver path supplied 12 lethal Drone and 63 lethal Hydralisk hits;
the control and observer traces matched. This is a concrete port behavior, not
proof that paid ammunition alone wins the mission or matches N64 difficulty.
See [damage attribution](../2026-11-05-z10-damage-attribution/README.md).

The existing `SC64_SCARAB_AUTONOMOUS_PROBE` diverts Reaver attacks to a
child-launch path and prevents the generic instant-hit fallback. The existing
paid queue model charges 15 minerals, limits unupgraded inside-plus-queued
children to five, allocates real actor slots, and advances its 105-source-tick
training on explicit calls. These are separately gated components, not a
qualified complete service. The production tick is currently registered in
the human-control probe; that probe requires the experimental Scarab namespace
and weapon stack. Starting it for all actors without the matching lifecycle
and save semantics is not a safe ammunition-policy change.

The late-Zerg AI maintains three distinct town wallets. Its body-queue helper
temporarily maps the selected town's bank into the collapsed ENEMY wallet,
then commits it back. The existing Scarab adapter instead falls back to the
shared ENEMY bank for Z9/Z10. That would debit the wrong payer when called
outside the body-queue helper. `raw_owner` records actor origin, which is not
generally proof of the current controller after capture or transfer. Late-Zerg
source slots 2, 7 and 0 share an engine owner in Z10, so the engine owner alone
cannot distinguish their banks. No code in this stage guesses that binding.

Pinned OpenBW confirms Scarab capacity/admission and Reaver's actual child
launch, but the reviewed code does not establish a Z10 AI automatic-purchase
decision or its cadence. Native AI purchase timing is likewise unobserved.
Automatically filling ammunition on every tick would invent policy; disabling
the generic attack before adding a supported purchase policy would instead
leave AI Reavers unable to fire. Neither is a faithful bounded fix.

## Next concrete unit of work

1. Establish a source-backed or explicitly labeled, tested AI purchase policy
   and a current-controller binding for each actual Reaver. For Z10, audit
   initial CHK actors, all trigger actions and every ownership-transfer path;
   bind a town wallet only when its current controller is uniquely established.
   Reject unknown, duplicate, transferred or stale bindings without changing
   any wallet. The human SIDE wallet and other custom controller wallets retain
   their own existing bindings.
2. On a separate default-off stage, connect the existing paid enqueue and one
   source-cadence production tick to actual AI Reaver lifecycle calls. Prove
   with the real adapter that each successful request debits the right town
   exactly once; shortage, queue/capacity/full-slot failures preserve wallets,
   other towns and pending work. Cover transfer, death, stop, reset and slot
   reuse. Do not call the generic hit path when the paid child is unavailable.
3. Integrate child launch, actual movement/impact/splash and event cleanup, then
   define authenticated save/restore of queued, hidden and in-flight children
   and pending impacts. Current namespace rejects active flight/impact saves;
   shared Egg and Scarab candidate contexts are explicitly incompatible.
   Preserve old raw-36 Overlords, source ownership, actor capacities, Z8/Z9
   trigger reservations and atomic fallback while composing these namespaces.
4. Test current-source control and candidate through the same ordinary Z10
   inputs with per-town wallet/queue, projectile and mortality observations,
   preserve losses and limits, and independently run normal menu→Temple→Drone
   and crystal→delivery→result with genuine save/reload. Rerun affected modes,
   product guards and earlier missions; this is not a promise of a Z10 win.

This work can be developed independently of the Forge/Core and Flash branches.
Any eventual release must qualify the composed service and its save formats;
neither those upgrades nor a Flash migration substitutes for the missing
Scarab mechanics. Memory headroom and physical hardware performance are open.
