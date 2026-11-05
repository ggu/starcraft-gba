# N64 parity audit and maintainer priorities — 2026-11-06

## Scope and evidence

Three GPT-6.1 Terra agents independently reviewed campaign/skirmish behavior,
visual/combat behavior, and performance/cadence. The maintainer checked the
current feature fences, impact scheduler, straight-flight implementation and
construction renderer before choosing the order below. This audit covers
merged #295, `34c1b38f94ce56934bdd317ca555f82622c8cb64`; its whole tree equals
reviewed `b57351af4578a1066d8a6d2432d88c8499b86dda`.

This round is a **source and existing-reference-evidence audit**. No new N64
or GBA emulator session was run. The agents used authenticated owned-ROM
content facts and recorded N64 observations, alongside labeled pinned PC
behavioral references. An extracted event is not proof of when the N64 runtime
executes it; a PC implementation is not N64 gameplay. Reports identify missing
observations rather than fill them with assumptions. Source-only reports:

- [Campaign, production and skirmish](campaign.md)
- [Visuals and combat events](visual-combat.md)
- [Performance and simulation cadence](performance.md)

Existing native observations include [three Blood Bath openings](../2026-11-05-n64-blood-bath-openings/README.md)
and the [bounded same-process order-timer trace](../2026-11-05-n64-order-cadence/README.md).
They establish neither complete combat behavior nor a general nine-update gate.

## Maintainer decision

Prioritize mechanics that unlock normal play and preserve progress. Run the
performance investigation alongside them. Do not spend this stage matching
isolated decorative frames while production, ammunition or saves are incomplete.

| Order | Work and reason | Next reviewable milestone |
| --- | --- | --- |
| 1 | S1: complete shared Zerg production. Custom Eggs have a real ARM/SRAM proof, but that opt-in service does not enable Z8–Z10 or Guardian production. | Carry the coherent Egg context into late campaigns with owner wallets, paid paired births and trigger-create reservations; ordinary-input campaign save/load and continuation. Then integrate Greater-Spire Guardian morph/cancel and Cocoon lifecycle. |
| 2 | C2: replace the Reaver substitute with paid Scarabs. The generic shot changes combat/economy and custom Reavers remain unavailable. | One shared ammunition/launch/flight/impact/save contract, including source-placed ammunition and AI stocking. Integrate namespaces rather than combine incompatible staged flags. |
| Parallel | P1: remeasure current-source busy frames, then optimize exact order/detection and movement work. This has more potential than an unmeasured cosmetic change, but no identified patch supplies the whole budget. | A current production-vector baseline and stage/tail attribution; one bounded optimization with paired per-update state, route/pixel checks and material full-loop improvement across campaign and custom windows. |
| 3 | S3/C3: verify combined campaign progress and finish Z10 normally. Preserve 29 existing missions; component fixtures are not an ordinary win. | Fresh-process SRAM continuation for early Protoss entries under P10, plus an ordinary Z10 Temple/crystal/result sequence after the relevant mechanics work. No injected progress or difficulty changes to force a pass. |
| 4 | B2: unify projectile and attack-event behavior. Fixed rendered endpoints can disagree with a moving live damage target; stationary/melee attacks still bypass the moving-event path. | Capture native follow-flight and release/impact cases, then implement bounded shared flight/event state and consistent impact/damage locations. Qualify follow, bounce and separate shots in stages, with persistence and crowded-scene checks. |
| Later | C4/U1/K1: correct verified AI/availability discrepancies and qualify complete custom matches; V1/V2: construction finish, shield effects, Egg motion and fog. | Establish a concrete current mismatch first. Use bounded native captures for AI launches/wallets, construction completion and shield effect mapping; finish both-map progression/wins and save/load for mixed 1–3 AIs. |

This ordering does not make saves, memory or reference checks optional. S1/C2
must not become separate Showcase-only fixes, and experimental prototypes must
remain explicitly gated until their combined persistence and product matrix pass.
Early campaign save qualification may run independently; it need not wait for a
Z10 win. Projectile research is also an independent handoff, provided it does not
simultaneously change shared save layouts or the scarce actor pool.

## Important corrections to expectations

- Coverage remains **29/30**, not a newly completed campaign.
- The normal product keeps shared custom Eggs off; the separately named preview
  enables them. The actual ARM proof covers one custom setup and static Egg/HUD
  art, not all campaigns, full matches, animation or hardware performance.
- Walking, damage fire, source construction layers, cargo, markers, death effects
  and moving-impact queues already exist. Their presence does not prove complete
  native behavior, but old absence reports should not be treated as new defects.
- A generic Reaver path caused many observed Z10 HOST losses; this does not prove
  that a faithful Scarab path will make the mission easier or produce a win.
- The sampled busy P7 full loop is about 2.20 million cycles versus a roughly
  559,000-cycle 30 fps period. A reduction around 75% is needed in that window;
  small gains or overlapping historical scope totals cannot establish it.
- Do not skip entire actor updates to imitate an inferred N64 cadence. Movement,
  cooldowns, pre-handlers and decisions have different obligations.

## Next native observations

Use unmodified owned-ROM play, private independent emulator settings/saves,
ordinary controller input, bounded captures and the existing serialized native
job/disk guards. Record ROM/emulator/source identities and timing units. Keep
reference screenshots, extracted art and traces private.

1. Follow projectile against a moving target, stationary/melee release and HP
   change, and Mutalisk chained targets. Separate displayed frames from game
   updates and verify identity in the same process.
2. Pylon and Assimilator completion: anchor, materialization, attachment/underlay,
   finish signal and build/power timing.
3. Reaver initial ammunition, paid replenishment, launch and target-loss cases;
   late-Zerg AI composition/readiness/wallets before changing scalar wave logic.
4. Guardian production/cancel and Egg birth timing/animation; full untouched
   Z10 Temple/crystal handoff. Do not reuse an altered reference run for balance.

These are the next measurements to collect, not tests claimed to have run.
The [living backlog](../../PROJECT-TODO.md) remains the task list.
