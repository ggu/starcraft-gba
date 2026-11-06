# Protoss campaign human research context

This unused helper separates a current human controller from an actor's retained
source origin. It is research integration groundwork, not enabled research or
active-job save qualification.

The actual P10-product P1–P10 initializers at `1286edbf` each map exactly one
source slot to compact SIDE (0): slot 7 in P1–P9, slot 1 in P10. The human
wallet accessors use the global minerals/gas pair; the independent AI bank array
at index zero is not that wallet. A rescued building may become SIDE while
retaining its old raw source owner for mission triggers.

`sc64_research_bind_protoss_human` consumes explicit mode, exact mission ID,
descriptor human slot and runtime role map. It returns compact human owner and
wallet slot separately from source controller/payer identity. It rejects custom
games, non-human owners, ambiguous role maps and unknown mission identities.
It does not authenticate arbitrary caller-supplied context. Live and detached
callers must authenticate their own canonical context first; a generic imported
OWNR descriptor is insufficient where runtime adapters override roles. Enemy
wallet mapping is outside this contract.

At `3f95ebbb`, ASan/UBSan pure and actual-engine fixtures passed in 19.37 seconds
under the shared native-build lock with 40 private dependency hashes unchanged.
The actual-engine fixture runs all ten initializers, distinguishes the human
wallet globals from the AI bank, and exercises P1 rescue of a raw-owner-1 Nexus:
current controller and payer remain human 7 after rescue, while raw origin stays
1. A captured explicit P1 context remains P1 when unrelated live menu globals
are changed. These are constructed HOST transitions, not ordinary mission wins,
GBA gameplay, or proof that a candidate decoder authenticates its own context.

Public commands are `make test-sc64-research-human-binding` (asset-free) and
`make test-sc64-research-human-binding-game` (prepared P10 inputs). The private
qualification used a local recipe supplying the already authenticated cover
header and current audio bank; it generated no new game data. No runtime or
journal namespace changes are made by this stage.
