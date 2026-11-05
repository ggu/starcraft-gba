# N64 Reaver, skirmish and save priorities

This is an incremental read-only review against normal main `6b4a1932` and
draft base `19435672eb922d932d0f2f13ae9858f4b16785e9`. Their only changed
`starcraft/` file is `sc64_engine_capabilities.h`; the skirmish admission,
combat, codec and save sources discussed here are identical between those
commits. The later uncommitted HOST candidate in this worktree is explicitly
separate. No new N64 session or native observation was made. This report adds
raw hangar-field and predecessor-writer evidence to the existing
[N64 priority audit](../2026-11-06-n64-priorities/README.md), rather than
requalifying its observations. Authenticated ROM content, PC proxy behavior, HOST
fixtures and observed N64 execution are different evidence classes; none alone
proves complete game parity. Prior skirmish AI documentation predates some
shared transport and Carrier probes, so the current product gates below take
precedence over its older blanket roster list.

## P0: Reaver ammunition and economy

Normal custom-game product admission explicitly rejects raw 83 in
[`sc64_skirmish_adapter.h`](../../../starcraft/sc64_skirmish_adapter.h), and
its Support Bay is hidden in
[`sc64_skirmish_menu.h`](../../../starcraft/sc64_skirmish_menu.h). Campaign
Reavers instead have a generic immediate 100-damage fallback, not a paid
Scarab service ([P8 readiness](../2026-11-01-p8-scarab-readiness/README.md)).
The paired Z10 HOST trial attributed 12 lethal Drone and 63 lethal Hydralisk
hits to that generic path; its sampled AI unit purchases were paid, and the
trial did not prove a native ammunition outcome or a mission win
([damage audit](../2026-11-05-z10-damage-attribution/README.md)). The impact is
both unavailable custom progression and potentially different combat and AI
spending. Hiding campaign Reavers or treating free shots as ammunition would
not meet this gap.

The authenticated ROM data identifies Reaver raw 83 and separate Scarab raw
85/weapon 82 ([weapon data](../../native-weapon-effects.md)). Pinned OpenBW
`4b046d5f65302b10cb0a745f0fecd37ec85b20a8` is a PC behavioral proxy; existing
component work models 15 minerals, capacity five/ten and a 105-source-tick
training phase, but is not a measured N64 service clock
([training](../2026-10-31-scarab-training-timing/README.md)). Its map loader
reads then discards the hangar field; it cannot establish native initial stock.
Owned decoded Z8/Z9/Z10 descriptors contain respectively 4/9/1 raw-83 UNIT
records, all with raw hangar 0 and validity 39. That observation is not proof
of the N64 initialization semantics. Existing N64 opening videos do not show
Scarab production, replenishment, launch, target loss, or impact.

Acceptance: observe unmodified N64 initial and trained stock, spending,
capacity/upgrades, launch cadence, terrain/target loss, splash and death; then
implement bounded shared paid human and owner-specific AI queues, real
source-placed semantics, flight/impact and cleanup with no generic free hit.
Test both maps and all applicable races, parent/child death, cancellation,
slot reuse, ownership and loss; qualify ordinary-input ARM and fresh-process
saves, actual limits and CPU. Do not change objective conditions, wallet or AI
settings to force a win.

## P0: roster, ownership and authenticated saves

Shared transport can select the Robotics Facility as the producer for raw 69
under its capability; the admission path validates owner, producer and
requirements. Carrier production is separately conditional on its complete
staged service; the custom Reaver remains explicitly blocked. The custom AI is
an authored approximation, not decoded N64 melee AI
([AI scope](../../SKIRMISH-AI.md)). Source-slot wallets cannot be replaced by
a generic combat-owner bank: P8 has independently bound source-slot 0 and 3
wallets despite shared combat ownership ([readiness](../2026-11-01-p8-scarab-readiness/README.md)).
Without exact ownership and refunds, ammunition can debit or credit the wrong
player, and AI composition or apparent balance may change.

Current code fences incompatible Egg, Guardian and Scarab draft namespaces;
old Scarab draft code mutates candidate representations and refuses active
outside children or pending Scarab impact packets. It is not a complete save
format ([packet audit](../2026-10-31-scarab-shared-packets/README.md)). The
private HOST checkpoint in this worktree uses detached candidate authentication
and an append-only Egg-first Scarab actor experiment; it is not a released
service. A separate-process prior-`19435672` Terran custom writer produced paid
Egg SRAM, and its current HOST reader loaded Egg, Reaver and Shuttle; both
reported legacy bound/Egg IDs 44/44. This one codec/profile is not a general
migration, guest, N64-save or Guardian proof. Historic HOST-only Guardian/Egg
and Scarab prototypes must not be silently assigned the same meaning as a new
format. Candidate data and the selected outer/inner tag pair must be bound
without mutating authenticated rows; preflight must precede publication.

Acceptance: preserve real prior Egg, ordinary and Shuttle identities in every
profile, place new Guardian/Cocoon and Scarab IDs after existing public IDs,
and choose a distinct collision-audited generation. Validate Egg, Guardian,
Scarab, Carrier, transport, worker cargo, trigger reservations and shared
impact tails in the same candidate-local context. Test genuine old-process
writers per codec, independent older-valid fallback, malformed newer records,
unchanged SRAM/live state on rejection, and real training/inside/flight/pending
impact save and fresh-process continuation. Preserve legacy tag denials and
memory guards. Current N64 save compatibility and original ownership semantics
for all these cases are untraced.

## P1: moving weapons and attack events

The current bounded visual tracer uses captured endpoints while pending damage
may follow a live moving target; generic bounce-class shots do not yet provide
full chained damage ([visual/combat audit](../2026-11-06-n64-priorities/visual-combat.md)).
An older weapon-effects page's statement that all hits are immediate is stale:
the current moving-impact queue already exists. The visible projectile and
actual damage can therefore diverge in busy fights; stationary/melee and
multi-shot timing also remain incomplete. Authenticated N64 content contains
weapon, flingy and image-script fields, but it does not by itself prove the
runtime update order. The OpenBW Scarab order states, 90-tick lifetime and
seven-tick collision grace are PC proxy observations, not N64 traces
([order audit](../2026-10-31-scarab-native-order/README.md)).

Acceptance: capture untouched N64 moving-target follow flight, bounce chains,
release-to-impact/HP timing and separately spaced shots, then compare exact
same actor and timing units. Implement bounded shared projectile/event state,
target loss, pool exhaustion, shields and save continuation; verify rendered
impact and damage location agree and measure crowded-scene resource use.

## Existing direct N64 observation

Three ordinary-input Blood Bath openings in ares observed Terran 4/10, Zerg
4/9 and Protoss 4/9 visible supply ([opening evidence](../2026-11-05-n64-blood-bath-openings/README.md)).
The earlier GBA comparison showing 4/18 and 4/17 for Terran and Protoss is
historical: merged `7fee3810` removed the extra starting Depot and Pylon, and
current `race_skirmish_init` at main `6b4a1932` retains that correction
([supply adjustment and qualification](../2026-11-05-skirmish-opening-supply/README.md)).
It spawns each town hall and four workers, plus a Zerg Overlord, and assigns
200 starting minerals; it does not add the former Depot or Pylon. The scoped
observed supply discrepancy is therefore already addressed, not a new fix to
prioritize. Full native starting inventories, wallets and AI openings remain
uncertain because the captured viewports and transition timing do not establish
them. No N64 Scarab, Reaver purchase, weapon flight or save observation follows
from those openings.

## Private evidence checked for this update

Read-only inventory of the primary checkout's private `.local/n64-ref` found
17, 10 and 5 PNGs in the three opening directories named in the prior audit,
and 63/16 images plus manifests in the `flyer-contact-8wjb1hj7` and
`flyer-paused-ghyoy55z` sets. Those manifests identify captured files; their
existence alone does not establish Scarab or combat outcomes. I inspected the
first Terran opening image (`openings-eQG7Q6`, 02:46:07), which is an
intermediate Blood Bath/Melee setup screen with Human Terran, Computer Zerg,
Computer Random and Computer Protoss displayed. It is not the final closed-slot
configuration and is not evidence of in-game production or starting supply.
The prior opening report's observations came from its later separately identified
captures and remain bounded to those sessions. The private `analysis/` directory
also contains a Z8 static AI report; decoded commands are content evidence,
not an observed N64 launch sequence. No private image, ROM or save was copied
into this report.
