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
| P1 | Mineral collection feels too fast / slow controls | Gather/return use simulation updates plus travel. Open command menu pauses economy. Measure deposited amounts per simulation time and wall time/frame drops before changing rates. Contention and saturation remain to audit. |
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
