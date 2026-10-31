# Shared custom-game transport (staged)

`SC64_SHARED_TRANSPORT` is an experimental, default-off capability. It extends
custom-game transport to ordinary Shuttle and researched Overlord identities
while retaining the existing Dropship and mission-specific transport paths.
Four owners have independent Ventral Sacs research jobs. The human command
menu can start or cancel research at a completed Lair or Hive; this stage does
not add an AI transport strategy or claim original-game AI parity.

The feature uses its own save tag and a 16-byte research trailer. It does not
import saves from older tags. Feature-on ROMs must have a distinct, versioned
artifact and save basename; reusing another version's SRAM is unsupported.
With impact events, the trailer reduces the maximum number of pending impacts
that fit in a save. A save exceeding the remaining space is refused rather than
truncating effects. The existing two-slot save journal remains in use.

This is staging documentation, not release or performance qualification.

Custom games use the broader pinned cargo-capacity facts. Campaigns retain
their previously qualified cargo whitelist for now. Qualifying those facts
against each campaign's available actors, save paths and source permissions
is still required before unifying the rule across modes; this staging boundary
is not a claim that the original game had different capacity rules.
