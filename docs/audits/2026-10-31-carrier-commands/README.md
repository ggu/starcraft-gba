# Paid Carrier and contextual command stage

Runtime `79d93bf` builds on P8 `62a56ea`. This is a staged custom-game admission/UI fix, not complete cross-product Carrier support. P8 itself remains unpromoted and held for shared-service performance. Canonical campaign coverage stays T10/Z6/P7 (23/30); all 30 missions and near-30-fps play remain unfinished.

## Problem and behavior

The Carrier scenario rule deliberately uses producer255. Custom-game admission rejected that sentinel even in the P7/P8 profiles with training, flight and projectiles enabled. The paid queue independently rejected it as well. Both now resolve a shared effective producer: Stargate for raw72 only with the actual complete existing Carrier service and active custom binding. The scenario rule and campaign admission retain producer255. Battlecruiser's existing qualified producer override uses the same helper; unrelated sentinel products and Reaver raw83 remain refused. Catalog prerequisites, owner, power, wallet, queue, supply and pool checks remain authoritative.

The Interceptor contextual command was also unreachable: custom catalog dispatch returned before its legacy campaign row. It is now an appended custom-game command. A shared ordered row registry provides identical count/label/dispatch offsets for Scarab, Yamato, facility/research, Bunker, transport, Battery and Carrier contexts. This prevents mixed unit selections from assigning multiple actions to the same row. Campaign row bases and dispatch priority are retained. A selected building takes precedence over stale Carrier unit selection.

## Local evidence at runtime79d93bf

- A new actual-engine fixture fails at paid Carrier admission on the unchanged parent, before the runtime fix. The fixed P7 and P8 production vectors complete the ordinary Stargate queue with the 350/250 debit, spawn an empty Carrier, buy an Interceptor through real B/menu/A input for25 minerals, finish its timed training, and save/load the real child. Missing Fleet Beacon and wrong-owner cases reject.
- Mixed selected Carrier/Battlecruiser exposes separate Yamato and Interceptor labels and actions. The optional Dropship branch explicitly reports unavailable in actual P7/P8 raw maps; no test-only mapping or feature is injected. The initial attempted unmapped Dropship fixture failure is retained. Three-way transport coverage is not claimed.
- Actual Showcase flags reject Carrier production, even with a Stargate/Beacon and funds: its full training/flight backend is still absent. Its exposed detection/cloak/Battery fixtures pass. This refusal is a negative capability check, not a completed Showcase Carrier service.
- All four normal Make ARM products (Terran, Zerg, P7, Showcase) build within unchanged guards. BSS/IWRAM ends remain T020370c0/03004b40, Z02037134/03004c10, P7020382f4/030049e0, Showcase0203afa8/03004c78. Independent title/code/checksum/SRAM-marker, ELF-payload and FF-padding checks pass. The verifier's initial NUL-only title-padding assumption was corrected to accept Make's ASCII spaces; no ROM changed for that correction.
- Four real normal-ROM fresh-process skirmish save/continuations pass at9,000+900 updates with three mixed bots, active-queue fingerprints and unchanged SRAM. These do not specifically qualify a mid-flight Carrier ARM save.
- All12 actual product-vector Yamato UI, research and lift/land sanitizer regressions pass (three per product).
- A separate clean unprepared checkout passes boundary, doctor, build and full tests:883 Python cases/23 optional skips,5/8 ancillary cases and native fixtures. Existing P7/P8/Showcase shared-service sanitizer fixtures pass. All108 original/selected prepared-input hashes are unchanged after qualification; inputs were read-only aliases, not regenerated.

Private receipts are retained in the owned worktree under `.local/qualification`: `carrier-services-results.json`, `carrier-arm-results.json`, `carrier-rom-verification.json`, `context-regression-results.json`, `fresh-sram-carrier-controls/summary.json`, and input manifests. The clean checkout separately records each default command/result. Prepared service targets are `build/test-sc64-protoss-services-p7`, `-p8`, and `-showcase`; their production vectors must remain intact.

## Remaining gates

The base of this qualification is62a, before the separately qualified service optimization now public atc32893d. A combined current-parent build/qualification remains required before merge. The separate experimental T/Z/Show Carrier backend, owner-generic AI purchases, additional types and custom save overlay are not included here. Complete common flight/art/persistence and remaining roster services still require work. Existing Carrier capacity is four; this stage does not implement the capacity upgrade or native motion/RNG fidelity. No new hardware, full N64 parity, universal30fps or canonical campaign promotion is claimed.

Source only under the repository GPL-3.0-or-later terms. No proprietary payloads, generated game headers, ROMs, saves or captures are tracked. No Actions, public ROM upload or SD writes.
