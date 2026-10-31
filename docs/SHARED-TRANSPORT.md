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

## Qualification snapshot

At source `8e09a26a3cda146432060e60c39b8929667c82ee`, host
ASan/UBSan checks exercised T10, legacy Z6, promoted Z7, P7, the P8
candidate and Showcase with both maps, all human races and one to three
opponents, including mixed and all-Zerg setups. Structures, resources and
actor proximity were seeded. The checks include paid carrier production,
human Ventral Sacs research, boarding, unloading, ordinary save/load, carrier
death and slot reuse. Default-off custom Dropship checks cover T10, Z6 and Z7;
positive P5 and Z6 mission checks include the authored Z6 Overlord kill action.
They are not exhaustive controller playthroughs.

Feature-on ARM links passed the existing memory guards on the same runtime
source. The table records BSS and IWRAM end addresses; the IWRAM guard is
strictly below `03005000`.

| Profile | BSS end | IWRAM end |
| --- | --- | --- |
| T10 | `020370e0` | `03004b40` |
| legacy Z6 | `02037154` | `03004c48` |
| promoted Z7 | `0203d718` | `03004f80` |
| P7 | `02038314` | `03004a28` |
| P8 candidate | `02038cdc` | `03004a28` |
| Showcase | `02037008` | `03004cc0` |

The feature-gated P7 and P8 fact tables were regenerated from the pinned
inputs, with SHA-256 respectively
`a9a01d7d70085ab0cbb4ea58a44d5781f980a6dc1c07c77393050a586cb8c441` and
`f5920e90eeec1ba0372e174294ffe9e0f1d74303f5e7c14d6a008dea960e0b92`.
The clean, unprepared source check, doctor, default build and test passed:
926 Python tests with 26 skips, plus the separate five- and eight-test suites.
Pinned inputs were unchanged.

The initial Z6 mission fixture used an unavailable old probe bank and then
lacked a standard declaration and production art object; its recipe was
corrected and its positive test passed. An initial clean source check found
the main Makefile over its size limit, so transport recipes moved to an include.
A subsequent default test found the missing `SC64_SHARED_TRANSPORT` fact-table
gate; the generator and Z6 reference comparison were corrected before the
matching fact tables and affected checks were rerun.

This evidence does not establish AI transport or research tactics, native AI
parity, near-30-fps performance, or release admission. Shared transport combined
with the experimental Scarab save namespace has not been runtime or save
qualified. Campaign cargo permissions remain a separate qualification gap.

## Activation candidate

The activation candidate routes the normal Terran, Z7, P7 and Showcase builds
through the shared transport capability. The explicit Z8 and P8 candidates
inherit it. ROM filenames include `Transport1`, and `dev play` uses separate
versioned save directories for the normal race aliases. The explicit legacy
Z6 product keeps its earlier flags, filename and save directory. P7 and P8
use separately generated fact tables with the transport actor gate enabled.

These routes are candidates until their exact-source runtime, memory, save and
performance checks pass. A `Transport1` save is a separate namespace: the new
reader does not import a predecessor save. Reusing older SRAM manually is
unsupported; the new journal makes no claim to fence an older reader.
