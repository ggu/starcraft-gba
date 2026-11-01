# Shared HP regeneration candidate

This is an unrun, default-off candidate. `SC64_SHARED_HP_REGEN` is not enabled in any normal profile. It uses the separately authenticated source-regeneration flags for unshielded source identities, a per-actor phase in the existing remainder, and the existing Z5 80-engine-update integer-HP adaptation. The live-world pass occurs before orders/combat and after menu and authored-pause early returns. Completed loaded and burrowed actors are not excluded; incomplete buildings retain their phase without advancing. Existing Z5-specific ticking is disabled when the shared gate is on. The code uses typed local phase values across both compact and legacy layouts and introduces no save bytes or BSS state. This approximation does not establish N64 timing or fractional-damage ordering.

The Creep Colony morph validator accepts a bounded phase under the gate rather than discarding it. Egg 36 is not a source regenerator; its existing birth reset remains unchanged. Shielded actors, buildings with shields, nonmatching raw/type identities and non-regenerators do not acquire this phase. Terran burn and unrelated special fields remain outside the service. Source helper and prepared-profile fixture candidates are included but have not been compiled or run.

**Open compatibility limitation:** existing generic saves permit remainder values through 1279 even on unshielded actors, whereas a regeneration phase is bounded to 0–79. A previously valid save with a larger remainder is preserved, never silently reinterpreted or wiped, but this candidate leaves that actor's regeneration inactive. An explicit backwards-compatible discriminator or justified candidate normalization policy, and full candidate validation for all save variants, is required before activation. This branch does not claim the feature is ready to enable. Legacy Z5 phase, raw aliases and owner provenance, completed/unfinished morphs, loaded and burrowed actors, pause, save/load, death and row reuse need qualified tests across campaign, custom and Showcase; actual memory, performance and ARM budgets are unmeasured.

## Proposed save and product boundary (not implemented)

The next implementation should use a distinct versioned product and save namespace, for example a `Transport1-HPRegen1` ROM/save basename, with no automatic predecessor import. The existing payload layout and actor-field offsets can stay unchanged, but a new outer journal tag must identify the newly typed phase and the inner payload version must unambiguously agree. One candidate tag transform is the current complete outer tag XOR `0x02000000`: every current base tag in `game.c` has that bit set, while the existing roster, BC, Transport1 and Scarab combinations use separate bits. This is a proposal, not an assigned permanent tag: enumerate every supported build/tag combination, assert disjointness at compile time and verify no other product claims the resulting values before selecting it. The new version must compose with the actual selected transport/impact/BC/Scarab envelope and preserve its length, trailer and internal checks; simply changing `SAVE_VERSION` in one caller is insufficient.

Selection should accept only the new exact tag, complete checksum/length/version and full candidate preflight including every eligible live actor's phase in 0–79 before committing any world state. Non-regenerating actors retain their existing type-specific meanings and validators. The same predicate and raw/type resolution must be shared between live ticking and candidate checks, including an explicit valid predecessor identity for in-place colony morphs; a mismatch or invalid phase fails closed rather than freezing a silently admitted regenerator. Do not infer legacy phase from bytes or auto-import old saves. The product's separate save basename is the supported isolation boundary; manual cross-version SRAM reuse is unsupported and must be documented. Prefer the existing candidate-validated journal pattern (`save_select_transaction`, `save_write_selected`, `save_write_first`) made available for this feature rather than a checksum-only newest-record read: validate before live commit, write the opposite slot from the selected valid candidate, and leave a valid older new-version candidate intact on interruption. First write must not destroy an unrelated valid record. These APIs are presently compiled under the Scarab capability and some normal Transport1 paths use `save_read_parts`/`save_write_parts`, so their safe reuse or a reviewed equivalent across all profile envelopes is required. No new tag, writer, reader, namespace, migration or activation is implemented here.

## Unit completion scope (source review)

The current `Unit` records have no general completed/under-construction field. Scenario `race_campaign_adapter.h` creates imported actors through `spawn_unit`, while normal building production creates the completed product via `spawn_unit`; the modeled Z7 intermediate is distinct `U_Z7_EGG`/raw 36, whose source flag is non-regenerating. Its in-place completion changes the row to the product and resets the remainder. Raw 35 Larva is a source regenerator but is represented by a Hatchery larva count in this runtime, not a generally spawned Unit mapping. The shared skirmish catalog lists Guardian 44, Devourer 62 and Lurker 103 as unit morphs, but the normal queue admission explicitly accepts `SC64_SK_TRAIN` rather than those `SC64_SK_UNIT_MORPH` entries; the catalog alone is not an implemented incomplete cocoon lifecycle. On these inspected current paths, eligible live Unit rows are modeled as complete after spawn/birth, while the distinct Egg row does not regenerate. This is a bounded current-model conclusion, not a claim that all 50 source identities or future morph implementations are modeled. Before activation, enumerate every compiled profile's source/raw alias, custom and Showcase actor creation, special spawn/hidden states and any additional lifecycle path; reject or explicitly model any eligible incomplete actor rather than assuming its completion. Buildings have an explicit `under_construction` field, and valid in-place morphs need their predecessor's phase preserved and validated while incomplete.

Qualification after design review must exercise every supported codec and mode with ordinary valid fresh-save/load continuations at partial phases; pause/result, full HP, loaded and burrowed actors, ownership changes, death/row reuse and source-eligible aliases; interrupted new-namespace writes and candidate rollback; and colony morph start, save, load and completion. Existing legacy tags/readers remain unmodified and do not automatically import. ARM memory/CPU budgets and default-off byte identity must be measured before any normal-profile activation.

## Integration staging and hard boundary

The source audit enumerated all 38 `SC64_RACE_SAVE_GENERATION` values in `game.c`, the literal base tags and all combinations of roster, BC, transport and Scarab feature bits. Their 2,000 distinct conservative combinations all retain bit `0x02000000`; toggling it yields 2,000 distinct proposed values with no intersection. Historical test tags formed by the documented adjacent generation and roster variants retain the same bit. This verifies only the indexed source's known namespace, including conservative unsupported combinations; a permanent tag still requires review against external/private products and any future namespaces. The proposed inner and outer constants are now staged behind the feature gate with reserved-bit assertions; no production tag or reader/writer has been changed.

The source now stages strict eligible-actor phase checks before live save construction and inside load candidate preflight, using decoded rows across existing compact and legacy profiles. Matching source raw/type facts, no shields, and phase below 80 are required for admitted regenerators; an explicitly recognized custom in-place Creep Colony to Spore/Sunken morph preserves and checks its predecessor phase. A modeled unit-completion predicate excludes Larva, Egg, Cocoon and Lurker Egg unit rows unless an explicit lifecycle is implemented. Unsupported eligible aliases fail closed rather than silently freezing. The helper and tests remain uncompiled and unrun.

This does **not** complete the namespace integration. Normal Transport1 `save_read_parts` and impact `save_read_latest` select a checksum-valid newest record before `race_load_preflight`; they cannot fall back to an older semantically valid record after phase rejection. The candidate preflight itself currently works through `race_data` and shared scratch and can bind temporary service context. `save_select_transaction` and the selected-slot writer are compiled only for the Scarab stage; safely factoring them to all envelope variants while retaining trailer, projectile, BC and namespace validation is required. A checksum-only reader plus a later validator would violate the desired candidate rollback contract. Consequently this branch fails compilation when the feature gate is enabled in a product build; only an explicit host test-only define can stage fixtures. There is no product basename or live journal/tag change, no automatic legacy import, and no activation until that transactional component receives a separate implementation and review.

## Candidate-aware journal factoring proposal (review required)

The feature remains product-build blocked. The indexed source's candidate decoder helpers were additionally made available to the gated legacy-profile fixture; this does not enable a wire format. The proposed selected inner tag now composes with the Scarab inner namespace when that stage is selected. No native compilation or test was run.

The minimum proposed API boundary is `race_hp_regen_prepare(raw, length, outer_tag, context)` plus a selected journal writer for contiguous or two-span records. The prepare callback would accept exactly `SC64_HP_REGEN_OUTER_TAG`, check the envelope's precise permitted length and checksum-selected record, verify the new `SC64_HP_REGEN_INNER_TAG` before any scratch-only version normalization, and never inspect predecessor tags. For an impact envelope it must decode impact rows into `RaceLoadPlan` before other validators reuse the scratch tail; transport must decode its exact trailer into candidate-only state, and BC shot/mask and Scarab identities retain their existing per-envelope checks. It then runs the common decoded-row phase and complete semantic candidate preflight. Any normalization is confined to the copied candidate under the already-verified new tag; no old payload is reinterpreted. A non-impact Transport1 writer needs a selected-slot two-span equivalent of `save_write_selected`, or an existing bounded contiguous scratch representation, so checksum and tag still cover both payload and trailer.

`race_data` and `race_impact_record` alias route/save scratch. In ZP builds the Carrier decoded rows and expanded Z8 scheduler also occupy its tail, so prepare must treat that tail as destructive temporary workspace and must not retain pointers to it as immutable input. `RaceLoadPlan` must own any impact rows and other values required at commit, and the chosen physical candidate must be re-read and prepared again after selection, as `save_select_transaction` already does. The selector should compare both same-new-tag records using complete candidate preflight and sequence ambiguity rules, falling back to the older fully valid candidate when appropriate. Before saving, select and revalidate the previous fully valid new-tag record *before* constructing the current world in that same scratch; then `save_write_selected` rechecks it and writes the other slot. With no predecessor, `save_write_first` refuses to destroy any unrelated valid record. No old-tag import is introduced.

Candidate scratch is not entirely local today. Transport decoding writes global `race_skirmish_transport_candidate`; this is separate from live `race_skirmish_transport_tech`, but its value must be reset for each candidate and the selected value retained in the plan, with no rejected candidate committed. Protoss preflight binds `race_protoss_binding_override` and `race_protoss_override_mission`; the current wrapper resets only the override to -1. A reusable prepare boundary must snapshot and restore both exact prior values on every success and failure, including early returns. Scarab-stage `race_scarab_stage_wire` is another candidate selector when that envelope is supported. The shield-phase migration mutates only the copied `race_data` candidate; the new namespace should require its current marker rather than perform a predecessor import. `race_z8_save_unpack(..., apply=0)` uses candidate/route scratch and validates without assigning the live Z8 scheduler; its `apply=1` form does assign live state. Every other called candidate validator and its scratch aliases still needs a function-by-function side-effect audit before claiming the prepare callback is pure.

Selection alone is not atomic load. Existing `race_load_game` begins assigning live mode, wallets, unit/building arrays and mission data after preflight, but can still return failure during row decode or P5/P6/P7/P8/Z5/Z6/Z7/Z8 live rebind, Battery validation, Nydus rebind, or P7 projectile restoration. `save_select_transaction` currently takes a `void` commit callback and cannot roll those changes back. Before using it for HPRegen1, each fallible post-preflight step must either become a candidate-derived, fully validated bounded plan with an infallible commit or be proven equivalent for every supported profile and refactored so its failure happens before the first live write. Snapshotting the whole world would violate the stated memory constraints and is not assumed. Until that proof and refactor exist, strict atomicity is not established and the product compile guard must remain. The existing Scarab selector and all feature-off readers/writers remain unchanged by this proposal.

Qualification must use normal valid saves in each supported envelope plus existing authorized mock-SRAM checks: partial-phase fresh loads, older-valid/newer-semantically-rejected fallback, checksum failure, interrupted writes, unrelated valid slot preservation, exact new-tag/inner-tag provenance, no predecessor import, and snapshots of live units/buildings/wallets, services, bindings, projectile state and route-visible behavior after rejection. Test cases must also cover the plain legacy-row decoder availability, compact/derived/P7 typed rows, completed loaded and burrowed units, incomplete colony morph with retained phase, full HP, row reuse and all source-alias rules. Optional forged guest-SRAM predecessor experiments remain out of scope.

## Host qualification preparation (unrun)

The isolated qualification composition is based on experimental `4712e3a`,
with journal component `35f5ed4`, property metadata `0928e41` and the
`d84ce21` service/strict-preflight stack cherry-picked. Product builds remain
blocked by the existing compile guard; fixture builds may use only the
explicit `GBA_HOST` and `SC64_SHARED_HP_REGEN_TEST_ONLY` escape. This does
not integrate the HPRegen1 journal or qualify a new save envelope.

The compact stock profile's 50 eligibility bits come from the authenticated
N64/PC agreement documented in the sibling facts audit. The test closure must
use this source table, not substitute fabricated eligibility. Prepared-profile
fact tables must be generated from the exact target feature flags and this
source into separate ignored directories, with inputs, compiler command and
hashes recorded; pinned source inputs and earlier generated fact headers are
never overwritten. No regeneration has yet been run in this worktree.

Planned ASan/UBSan host checks include: helper boundaries and 80 live updates;
all represented owner slots and canonical campaign, skirmish and Showcase
bindings; loaded and actually burrowed actor state; completed and incomplete
colony morph, both phase bounds and relevant raw aliases; death and slot reuse;
actual update and pause paths; and Z5 with the old mission-specific tick
suppressed under the shared gate. Host-controlled actors and direct state
checks are fixture setup, not ordinary-input or game-level save/load proof.
Unsupported completion identities and any profile or feature whose evidence
is missing must be reported rather than inferred. The service stores phase per
actor; the expected 80-update interval is not a global clock phase. No native
compile or fixture result has been obtained for this composition.

## Focused host results on the staged composition

At frozen test head `22d88b9d873e597a7dd0b0fafde553fd016fd525`, the
ASan/UBSan helper, Z5, canonical Z7, canonical P7 and Showcase host fixtures
passed under the native lock. These use the host-only escape and do not enable
any product. The controlled tests cover per-actor 80-update cadence and five
owner slots, death/slot reuse, complete and incomplete colony phases, a
nonregenerator, controlled campaign/skirmish bindings, a pause/live update,
and the Z5 shared/legacy tick interaction. P7 and Showcase include the P6
combat implementation and passed an actual P6 burrow-state check. Showcase
also starts its real scene 5, unpauses and witnesses one live update. Shared
transport fixtures establish a real Dropship/Zergling boarding order and valid
carrier link, then retain it for 80 direct service ticks. These controlled
fixture states are not natural-input campaign play or guest save/load proof.

Exact standalone canonical P6 failed to compile at the existing minimum
impact-save capacity assertion. A separate HP-off canonical P6 host compile
failed the same assertion, so this is not introduced by the HP gate. The P6
profile itself remains unqualified; the P7/Showcase P6 burrow checks do not
prove its complete storage layout or budget. No assertion or capacity limit
was relaxed. No post-result fault, ordinary HPRegen1 save/load, performance,
ARM link, native hardware timing or full 50-identity coverage is claimed.
Larva (raw 35) remains explicitly unsupported by the staged completion
validator despite carrying the authenticated source property.

The 109 pinned inputs and the walk, contact, weapon and two marker sidecars
were checked unchanged. The P7 fact generator was built from this worktree's
`tools/sc64_fact_tables_gen.c` and wrote a new ignored fact directory; its
SHA-256 was `a9a01d7d70085ab0cbb4ea58a44d5781f980a6dc1c07c77393050a586cb8c441`,
identical to the prior normal P7 fact table. That generator emits actor,
sight and rectangle facts, not the stock regeneration bit. The staged service
reads the tracked `sc64_stock_profiles.h` directly; its 234 rows contain the
same 50 addressable eligible IDs as the authenticated comparison, with
228-byte eligibility-vector SHA-256
`29741f6688fb9ae03b6df2cc3eab0ccb5b308e7965307c521ea241e225691125`.
Raw 37 and 51 have flag 2, raw 131 has flag 3, and raw 0 and 36 have flag 0.
The separate property JSON was used only to verify the set, not as generator
input. No private table or vector is committed.

## Candidate-row preparation slice

At source head `dd25ac6198d80f3a3fbc69f71f987f5cbe7cfcac`, an HP-only
preparation token records a candidate pointer, geometry and (when compiled)
Scarab wire context. It decodes each unit, building and packed mineral after
candidate normalization and checks the shared regeneration phase predicates.
Accessors refuse a changed Scarab context. The caller must keep the plan and
outputs outside route/save scratch and preserve the candidate bytes; the token
does not enforce those alias and lifetime requirements. The existing live
loader does not consume this token and remains fallible after its first live
write. No tag, save envelope or product gate has been changed.

With the host-only escape, the helper and Z5, Z7, P7 and Showcase profile
fixtures passed under ASan/UBSan. A controlled ordinary host save supplies the
candidate, whose decoded unit/building/mineral fields are compared to live
snapshots. Invalid geometry and a separately encoded phase 80 are rejected
without changing the token or live arrays. The four profiles did not define
`SC64_SCARAB_NAMESPACE_STAGE`, so its context check has not been exercised.
The known standalone P6 capacity failure was not rerun. The fresh P7
feature-fact table and its target flags were checked separately; SHA-256
`a9a01d7d70085ab0cbb4ea58a44d5781f980a6dc1c07c77393050a586cb8c441`.
Source and pinned inputs were unchanged. No guest SRAM injection, full loader
atomicity, HPRegen1 save/load, native hardware or performance result is
claimed.

Remaining fallible post-preflight work includes row decode during live commit,
mission rebinds, battery live validation, Z8 apply-unpack and rebind, Nydus
rebind and projectile arena restore. In particular, P6's candidate validator
and later world-context/doodad binding are distinct operations; Z7 rebind
mutates cache and bindings and reconciles products before its live check.
Projectile packet validation shares packet preflight with the decoder, but the
latter also checks arena validity and overlap, writes the arena, and returns a
pool validation result. Nydus candidate resolution can use a currently bound
source for the selected mission, so the eventual transaction plan must bind
that decoding context as well. The route BFS, impact record, Carrier decoded
rows and Z8 scheduler candidate reuse the route/save scratch union; pending
impact data must be captured before its tail is reused, and no route reuse may
clobber candidate bytes before commit. These are outstanding proof and
implementation steps, not evidence of rollback safety.
