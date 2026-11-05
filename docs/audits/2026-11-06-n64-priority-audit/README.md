# Incremental Terra audit and maintainer priorities

Date: 2026-11-06. Three GPT-6.1 Terra subagents reviewed construction and
production, combat/skirmish and persistence, and campaigns/performance. Parent
reviewed their conclusions against current code and qualification receipts.
Canonical GitHub main remains `6b4a1932`; this document is based on draft
`19435672`, with experimental HOST work identified separately. This is a
source and existing owned-N64-reference audit: no new N64 gameplay session was
run for this round. Captures, extracted records and traces remain private.
PC reference implementations are proxies, not proof of N64 execution.

## Maintainer priority decisions

1. **Finish one coherent shared production/combat persistence contract.**
   Complete Egg rollout qualification and the Guardian prerequisites/morph;
   replace the campaign Reaver's generic attack with paid Scarab ammunition.
   These determine which tech can actually be used and change combat/economy.
   Preserve existing Egg/Shuttle identities before appending actors, use distinct
   authenticated format generations and genuine prior writers. One HOST codec
   proof is progress, not permission to enable the combined guest service.
   Existing-content Z8 qualification still holds common Egg draft #298.
2. **Optimize measured busy-loop work in parallel.** Current P7 baseline is
   2,200,226 cycles versus approximately 559,000 per 30 fps period. Start with
   repeated collision/steering work for average cost. An exact route-field
   kernel project can reduce stutters but cannot close the average gap alone.
   Require paired normal-vector state/pixel checks, multiple workloads and no
   regressions before promotion. Do not invent a uniform nine-tick actor skip.
3. **Qualify campaign progress and full custom games on the combined engine.**
   Preserve all 29 implemented missions, resolve Z8's ordinary-controller gate,
   and prove Z10's actual Temple/crystal win. Current candidate has 28 ordinary
   HOST controller wins, not 30 fully qualified campaigns. Continue real
   fresh-process saves in active play and both-map, mixed 1–3-AI progression;
   early neutral P1–P7 prefixes do not qualify arbitrary saves or whole missions.
4. **Fix shared projectile release/flight/impact as the next combat project.**
   Gather bounded N64 moving-target, bounce and melee/stationary release
   observations, then make depicted impacts agree with damage and handle
   separate shots, target death, pool pressure and save continuation. This is
   higher impact than isolated decorative frames, but native timings must be
   measured rather than inferred from extracted assets alone.
5. **Complete construction presentation, shield feedback and fog afterward.**
   Construction layers, damage fire, facing and cargo already exist; do not
   reopen old blanket absence reports. Target remaining lifecycle differences
   with paired reference phases and crowded-scene costs. No renderer change
   should worsen the already severe frame budget.

These are sequencing decisions, not waiver of memory, save or regression
checks. Services must reach every applicable campaign, skirmish and Showcase
product rather than become Showcase-only features.

## New evidence affecting the order

- Detector-first prototype `a5226e8d` regressed mean full-loop cost by 0.1074%
  despite matched state/pixel checkpoints. It is rejected, not a speed gain.
- Actual route-tail diagnostic `199a2912` matched 90 checkpoints, 958 advancing
  updates and three modal acknowledgments. At call 8347, one reverse-field
  build visited 12,001 cells and cost 2,264,933 cycles, with zero blocked-start
  gradient steps; the other eleven calls totaled 59,714 cycles. Diagnostic
  overhead was 0.1326%. This narrows the stutter cause but the event is rare in
  the sample, so its cost is not a recurring average saving.
- The current experimental Scarab reader preserved a genuine predecessor
  Terran custom paid-Egg SRAM with unchanged Egg identity 44. This is one HOST
  profile, not the combined Guardian/Scarab save matrix or actual GBA proof.
- Raw hangar fields in owned Z8/Z9/Z10 Reaver descriptors are zero. Their native
  initialization meaning remains unknown; do not manufacture starting ammo or
  treat a PC loader that discards that field as an N64 observation.
- Parent rejected a stale proposed opening discrepancy: current code already
  removed the extra Depot/Pylon and matches the observed supply caps. Native
  initial wallets/full inventories remain unverified. Audit historical source
  scopes before scheduling another fix.

The detailed agent reports below distinguish code facts, source data, PC
inference, actual N64 observations and remaining unknowns. This round changes
no engine, ROM, save format, SD contents or Actions settings.

- [Construction and Guardian](guardian-construction.md)
- [Scarab, skirmish and persistence](scarab-skirmish.md)
- [Campaign and performance](campaign-performance.md)

The [living backlog](../../PROJECT-TODO.md) keeps stable task IDs and the full
completion goal. The earlier [N64 audit](../2026-11-06-n64-priorities/README.md)
remains a historical snapshot; this update does not retroactively requalify it.
