# Worker cargo integration on the shared release engine

This is an unqualified integration checkpoint for PR #111, based on release
`63665045c84e5374d84109fd7357f07dc63bb941` and the original cargo draft
`b050bab17ac289ad5a52d4e2bb92a57f754c2c5a`. The cargo gate remains disabled in
production. The historical October 28 results do not qualify this combined
implementation, and this checkpoint does not complete the skirmish/performance
goal.

## Integrated behavior

- Gather commands use the current interaction priority and the same visible
  mineral target as the HUD. Selected workers can receive a new resource trip
  while retaining cargo until an actual pickup replaces it.
- Construction retains the release travel, arrival validation, payment,
  foundation and cancellation behavior. A Drone loses its cargo only when
  construction consumes it. Bot worker recovery, gas balancing, construction,
  scouting and emergency orders retain cargo identity.
- Cargo validation ignores dead actor contents; ordinary codec checks still
  apply. Repair remainder state remains separate from resources.
- Migration uses the release impact envelope and Transport1 trailer. It
  validates candidate records before committing live state and publishes saves
  relative to the selected valid predecessor. Save scratch uses the transient
  flood queue; failed selection/publication does not invalidate route caches.
  Successful saves retain PR #134's cache canonicalization.
- New cargo tags toggle `0x01000000` from the previous scenario version. Adding
  or OR-ing that bit would not distinguish these current scenario tags, which
  already have it set. The existing Transport1 bit remains separate. The
  impact-enabled, non-transport BC path also retains pre-energy migration.

The gated save implementation currently requires `SC64_IMPACT_EVENTS` and
excludes the separate Scarab namespace experiment. These restrictions are
explicit diagnostics, not claims of support for those combinations. All four
current production vectors use the impact engine; they still need actual
compilation and execution with cargo enabled.

## Verification status

Only source review, whitespace checks and the existing shared range flag
expansion check have run at this checkpoint. Native compilation and gameplay
qualification were initially stopped by the local disk-space guard. No hosted
Actions were enabled or dispatched. No ROM, save, imported asset, capture or
generated bank is part of this change.

The prepared-input fixture commands are:

```sh
make test-sc64-worker-gather-terran
make test-sc64-worker-gather-z7
make test-sc64-worker-gather-p7
make test-sc64-worker-gather-showcase
```

Each runs interaction/tick, AI and actual-codec save fixtures. `z6` remains an
optional historical-profile check. These authored fixtures are not ordinary
campaign controller, physical SRAM or ARM timing results. New fixture cases
cover construction interruption and rejected arrival, stopped human cargo,
dead stale worker rows and the actual impact/transport save envelope. Active
Yamato/impact payloads and broader campaign/controller continuity still need
qualification.

Before enabling or merging the completed feature, run the applicable default,
unified and full Terran checks, current ARM memory guards, normal-menu
race/map/AI coverage, fresh-process SRAM continuation and representative
render/performance comparisons. Preserve existing limits and namespaces.
