# Protoss10 custom-game hardware feedback

User report: November 6 KODAK Protoss10, custom game, real GBA. Shipped source
is e9838086; current canonical code after UI #304 is 31a5ddf8. A report is not
closed merely because a source path exists. Every fix must reach applicable
campaign, custom and Showcase products and retain save/memory qualification.

| Priority | Report | Current evidence / required work |
| --- | --- | --- |
| P0 | Buildings outside Pylon power | Request and arrival checks exist, but use radius-144 circle instead of the pinned shaped psi mask. Test boundaries/ownership/completion, enforce actual geometry, then verify placement-field feedback. |
| P0 | Cannot cancel producer queues | L-release is Barracks-only despite shared ordinary queues. A narrow all-friendly-producer fix with refund/ring/no-repeat tests is staged; Egg/Carrier/Scarab lifecycles remain separate. Not yet passing/merged. |
| P1 | Forge / Cybernetics Core upgrades absent | Forge ground weapons/armor exist in fixed B rows, not selected-Forge actions. Cyber Core paid upgrades are missing from that path. Add discoverable building-context research with typed payment/level/completion/save state, not free stat changes. |
| P1 | Workers stop on depleted minerals | Shared worker loop explicitly idles. Retarget an established gathering trip to a reachable explored nearby field while preserving final cargo. Keep STOP/idle workers idle and preserve explicit-order/unknown-cargo semantics. |
| P1 | Mineral collection feels too fast / slow controls | The 45-tick timer is set after collection, bypassed during loaded return, then replaced by 20 at deposit: it is not a proven gather delay. A collection-cycle diagnostic is authored but not yet run. Open menu pauses economy. Measure actual deposit cycles, contention and wall time before changing rates. |
| P1 | Deaths all large explosions / wrong sounds | Source death families and imported-art path exist, with authored primitive fallback for missing art. Audit actual P10 bank mappings, extent/phase and per-unit audio against native deaths; do not label the complaint fixed based on flags. |
| P1 | Long slow B lists | Latest highlight alignment is merged, but menu hierarchy is unchanged. Prototype horizontal Orders / Build / Research or building-context categories with stable command IDs, clear controls and all tech options reachable. Verify native command-grid behavior. |
| P1 | Missing Protoss warp placement/building audio | Current N64 harness discards audio. Capture bounded PCM and compare event timing plus source sound IDs before adding sound calls. Keep sounds shared across modes. |
| P2 | Pylon boundaries not shown | Separate field overlay and correct placement footprint from enforcement. Native custom Protoss comparison pending. |
| P2 | Hidden minerals / hard black fog | Map objects currently require current visibility. Compare native unexplored terrain, previously explored fog, remembered resources and minimap separately; avoid revealing enemy state or all resources without reference evidence. |
| P2 | AI difficulty options | Needs real behavior presets, visible setup selection and persistence; not just renaming identical AI. Preserve independent bot race/slot choices and 1-3 opponents. |
| Answered for current port | Base vs building defeat | `race_skirmish_outcome` counts all live owned buildings; zero loses, zero enemy-owned buildings wins. Not a final-Nexus rule. Native custom defeat semantics remain to verify. |

## Worker retarget reference and limits

Read the pinned OpenBW `order_MoveToMinerals` implementation at
[4b046d5f](https://github.com/OpenBW/openbw/blob/4b046d5f65302b10cb0a745f0fecd37ec85b20a8/bwgame.h#L4278).
It bounds replacement searches and checks exploration/reachability, including
field occupancy choices. This is a PC reference implementation, not proof of
N64 timing or exact search rules. The GBA explicit pending-order test currently
requires cancellation if its exact target disappears before arrival; distinguish
that from ongoing gather/return depletion. Do not auto-assign manually stopped
workers or drop a final carried load. Reuse existing fields without a save ABI
change where possible, and reject unreachable/unseen replacement fields.

## Qualification queue

Terra: power/cancellation/research source slice and native Protoss power/warp
comparison. Parent: worker retarget, resource/death source audit and integration.
Performance guest/canonical checks continue independently under the same native
lock. No newly reported item is marked fixed or deployed by this audit.

Pinned OpenBW `order_MiningMinerals` waits on its source timer before giving
cargo. That supports investigating the ordering discrepancy, but does not by
itself set the GBA duration or prove N64 timing. The new standalone diagnostic
is on `codex/mineral-retarget-20261106` at `1211e73`; it is explicitly unwired,
not yet compiled, and not a gameplay fix.

## Additional confirmed findings

Fresh native Melee / Challenger (human Terran versus computer Zerg) shows
50 minerals at the earliest sampled map and the next sample, before in-map
commands. Current GBA custom setup gives 200 to every active player. The
native human starting bank is directly observed; the native bot bank is not.
This starting-rule discrepancy is separate from mining-cycle throughput.
Do not infer new timer constants from the starting bank.

Ordinary queue cancellation also needs a full refund rather than preserving
the current 75-percent refund: pinned OpenBW `cancel_build_queue` uses the
full-unit-cost refund, separately from partial construction refunds. Terra's
candidate now reflects that distinction, but focused tests/native confirmation
and merge are still pending.


## Mining follow-up: measured defect and safe timing plan

The P10 production-vector synthetic adjacent Probe/resource/depot diagnostic
collects on handler call 1, deposits on 2, collects again on 23 and deposits
on 24. Its intended post-pickup timer 45 is bypassed while loaded and replaced
by 20 at deposit. This establishes an ordering defect, not wall-clock mineral
income or a native N64 measurement. The 50-mineral opening bank is separately
merged in #306; that change does not repair steady-state income.

A no-new-field timing design is available for review: counter 0 arms the gather
wait at the resource, values above 1 decrement, and 1 performs pickup/reset.
Deposit resets to 0. That covers initial, repeated and implicitly admitted
orders without repurposing cargo bits or adding save bytes. OpenBW pinned
revision `4b046d5f65302b10cb0a745f0fecd37ec85b20a8`, `bwgame.h`
`order_MiningMinerals` sets 75 and `order_HarvestGas` sets 37. The existing
project 5:4 conversion would yield 94/47 GBA update intervals, counting pickup
D intervals after the arming call. These are source-derived proxies, not a
calibrated N64 clock. Native per-patch queues and refinery hide/exit behavior
are still absent; individual worker delays alone cannot establish native
multi-worker mineral rates.

Before promotion: cover both worker loops, loaded/empty/in-transit states,
explicit cross-kind/unknown cargo, depletion, no depot, and real save/reload
at counters 0/1/D; state the one-cycle interpretation of legacy nonzero
20/45/60 counters. Run normal product ARM guards and real campaign/custom
pacing witnesses. No timing behavior change is currently enabled.
