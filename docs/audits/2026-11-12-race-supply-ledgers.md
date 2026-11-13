# Independent race supply ledgers

The [200 supply gate](2026-11-12-stock-supply-cap.md) capped the sum of
all races owned by a player. In the owned N64 runtime, maximum, provided
and used supply are stored independently by player and race in doubled
units. A rescued Terran base under a Protoss player's control therefore
supplies that player's Terran products, and neither fleet should block
or subsidize the other. The observed Terran limit at `0x800B3B20` was
400; its player-one provider and usage rows at `0x800B3B50` and
`0x800B3B80` gave 40 and 40→36 as the HUD moved from 20/20 to 18/20.
The other race maximum bases at `0x800B3BB0` and `0x800B3C40` were also
400. This verifies independent native storage and the Terran HUD field;
it is not a native mixed-army or Mind Control replay.

The shared paid production gate now checks the ledger for the product's
race. Each ledger is derived from the current owning player and the race
of live units, completed providers and paid products. Loaded units and
active Egg reservations still consume supply; a paired Zergling order
counts both bodies. Provider loss never erases a live unit or paid
reservation, and a zero-supply Overlord can still be ordered while over
its ledger. The AI census follows the AI's own race when planning supply.
Wallets, producer compatibility and mission/custom availability remain
separate checks.

The resource bar follows the command race, showing the rescued Terran
economy while its producer or worker is selected in P10. This is a
presentation choice; it does not assert identical mixed-race HUD
selection on the N64. Legacy aggregate values and save rows remain
unchanged. The ledgers need no extra save fields or migration because
they are rebuilt from authoritative live and queued actor data.

`make test-sc64-race-supply-ledger` is asset-free. With the normal private
assets prepared, `make test-sc64-stock-supply` covers the old 200-point
boundary and tests mixed owner transfers in the same game queue used by
ordinary field and AI orders. There is no new support for changing a
unit's owner via a spell; the tests exercise the accounting consequences
of an existing transfer. The focused monorace and mixed cases passed on
both Race and Unified with sanitizers. The Unified HUD cache matched a full
redraw. The real P10 first trigger, paid SCV and Probe, and a fresh-process
10,000-update P10 save continuation passed on the host. The normal P10
ARM ROM passed its build, memory and ROM guards. The global GBA actor budget still constrains
large games separately from the supply ceiling. The staged Ghost missile
service needs to add its reservations to the Terran ledger when the two
changes are composed.
