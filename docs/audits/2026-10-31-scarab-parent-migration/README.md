# Reaver parent field conversion prototype

The historical engine initializes generic `carry` and `work_timer` to zero
when spawning a Reaver (`spawn_unit` in `starcraft/game.c`). The experimental
parent initializer instead packs an empty queue, no active child, zero cooldown
and zero progress into those same fields. Those facts do not establish that
every historical saved Reaver still has zero in both fields, nor do they prove
the entire saved actor graph is migratable.

`sc64_scarab_parent_convert` is an unregistered, field-only conversion. Its
caller must determine the record format from an authenticated, collision-safe
namespace, not from the fields. It accepts a historical raw-83 row only when
both fields are zero, or a raw-83 row explicitly identified as packed after
validating that packed representation. Raw 85, other identities, unknown
formats and unsupported historical values are rejected without changing the
outputs. No actor or record is modified by the helper itself.

The focused sanitizer fixture tests empty conversion, packed round-trip,
cross-format rejection, raw-85 rejection and unchanged outputs on failure.
It is not an actual historical envelope, whole-save migration, transactional
commit, or evidence of N64 behavior. Before use, the caller must validate all
identities and child links, preserve every other field and tail, convert all
parents in candidate scratch, and preflight the complete candidate before
mutating live state. Namespace registration, historical save fixtures, and
fresh-process continuation remain open.

## Gate-off engine envelope

The separate-process `sc64_reaver_historical_save_fresh.c` fixture was built
serially using the retained Terran full-profile host vector from
`.local/decoder-proof/sc64_bc_impact_legacy_fresh-argv.json`, once without
`SC64_SCARAB_RUNTIME` for the writer and old reader and once with it for the
candidate reader. This is contemporaneous `25a890e` engine code with the
experimental gate off, not a binary from a past release. The synthetic host
setup clears units and buildings, calls `spawn_unit` for a Reaver and assigns
its raw owner in the fixture; it does not purchase a Reaver through gameplay.
The gate-off writer produces a current BC envelope: journal and embedded version
`0x73435442`, pre-BC version `0x53435442`. The spawned row had `raw_type=-1`
and mapped unit identity 83, with carry and work timer both zero. A fresh
gate-off process restored it; a fresh gated process rejected the unconverted
parent while preserving sampled units, buildings, minerals, gas, frame and RNG.
Its in-memory SRAM was written back and matched the input hash. This is not a
whole-world atomicity proof. This is a gate-off writer, not a hand-edited pre-BC record;
it exercises one initial parent in one profile, not all historical states.
It does not test conversion into a live world or validate a new save tag.

Compile and run argv, exit status and output, selected source and binary hashes,
and per-process SRAM hashes are retained in `.local/decoder-proof/reaver-*-receipt.json`.
The compile used source head `c8baa3f` with the subsequently modified fixture;
the recorded source hashes identify those files but do not cover every compiler
dependency. The SRAM SHA-256 was `80249bd89536725c84ca2ab6efe16921ab4cf4226762b223016341baf79071f2`
after writing and unchanged after both readers.

## Isolated namespace stage

`SC64_SCARAB_NAMESPACE_STAGE` is an additional, normally disabled experiment
restricted to the Terran compact/Yamato/impact profile and supported skirmish
maps. It uses bit `0x08000000` with compile-time collision checks,
selects semantically valid records among pre-BC, BC and the staged tag, and
rebuilds the winner's plan from reloaded bytes before the existing load
commit. Old empty Reaver parents are converted only after tag authentication;
raw-85 actors, other geometries and nonempty old parent fields are rejected.
The writer selects a predecessor before serializing and writes the staged tag
only after checking the candidate; fresh storage is protected by the journal
writer's unknown-record guard. Admission and Scarab children remain disabled.

The retained Terran ASan/UBSan host vector, with this extra macro, loaded the
gate-off Reaver fixture, wrote the new namespace and reloaded it in another
process. SRAM headers showed old tag `0x73435442` sequence 1 and staged tag
`0x7b435442` sequence 2. This is a single synthetic initial-parent case, not
campaign or other profile qualification. Existing post-commit restore failures remain possible; no
whole-load atomicity or production-readiness claim is made.

The staged reader was also exercised with a checksum-correct newer staged
record whose Reaver row was changed to invalid packed parent fields. It
selected and converted the older BC record, then wrote a replacement staged
record with sequence 2 while preserving every byte of the older slot; a fresh
reader selected the replacement. With the older tag removed, the bad record
was rejected while sampled units, buildings, resources, frame and RNG and
the SRAM image remained unchanged. The fixture deliberately changes the row
and checksum; this is a semantic-corruption test, not a naturally generated
save. Compile and seven process invocations with exits, outputs and SRAM hashes
are retained in `.local/decoder-proof/reaver-stage-edge-{compile,runs}.json`.
An actual pre-BC writer was not available for this run.

The stage now obtains dimensions from the saved skirmish map metadata after
checking its bounds and geometry, and checks the active map dimensions before
writing. A synthetic host fixture clears actors/buildings and spawns one empty
Reaver directly: all 18 combinations of the two maps, three human races and
one to three AI slots passed gate-off BC write, staged migration and fresh
staged read. These are not paid production or gameplay matches. Compile and
54 process receipts are in `.local/decoder-proof/reaver-matrix-*.json`.

The existing seeded `sc64_bc_impact_fresh.c` host fixture ran with two active
Yamato shots and a pending impact. Its write and fresh read continuation logs
matched byte-for-byte for a gate-off current BC writer read by the stage and
for staged write/read; observations include spell RNG, caster/target references,
projectile rows, and pending impact bytes. The schema-generated pre-BC fixture
also migrated a pending impact and retained it on fresh read. Its old envelope
is constructed by editing a current writer's version and markers, not produced
by a historical binary. Compile and run receipts are retained in
`.local/decoder-proof/{tail,prebc}-stage-*.json`. These fixtures do not prove
all tail combinations, campaign behavior, or hardware behavior.

The six retained compile/run receipt files were re-read after the stage run:
all six compile invocations and all 61 process invocations report exit zero.
The matrix run receipt SHA-256 is
`c2ebae3b416c8e34bd848ad5f0e539280eb6585c006111c56eb870ba7d2b0063`,
the tail run receipt is
`d16d3b7d3aad8c632c242b9ec730e8871e76d468d67c981f6d1f3cbba8b71a54`,
and the pre-BC run receipt is
`9e7b33cada0967775ef92b38dede532ab91a8fb8b84efd7284d296323d59e250`.
These receipt hashes do not authenticate every compiler input. The stage
compile vectors included `SC64_IMPORTED_SKIRMISH`; the explicit compile guard
requiring that flag was added after the test builds.
