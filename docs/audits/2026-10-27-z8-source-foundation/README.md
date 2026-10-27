# Z8 authenticated scenario foundation

This change records the source contract for Zerg mission 8 without enabling
it in a campaign ROM. It authenticates the existing USA ROM and retained
scenario, then preserves numeric scenario tables in a private descriptor.
It does not implement mission logic, import art, or change any ROM profile.

## Reproduce

Read `CONTRIBUTING.md` and the optional campaign setup in
[`docs/DEVELOPMENT.md`](../../DEVELOPMENT.md) first. The input directory must
already contain `z8/source.chk` and its matching `manifest.json`, retained by
the pinned `sc64_races_pack.py` catalog preparation. Its usual location is
`.local/sc64/zp-stage/generated/source_maps`. The following commands read
that directory; they never invoke an extractor or regenerate its banks:

```sh
python3 tools/sc64_z8_pack.py pack \
  --rom /path/to/StarCraft64-USA.z64 \
  --source-root .local/sc64/zp-stage/generated/source_maps \
  --out .local/build/z8-source-review/descriptor.json
python3 tools/sc64_z8_pack.py verify \
  --rom /path/to/StarCraft64-USA.z64 \
  --source-root .local/sc64/zp-stage/generated/source_maps \
  --descriptor .local/build/z8-source-review/descriptor.json
python3 -m unittest discover -s tests -p 'test_sc64_z8_pack.py' -v
```

Use a new output filename for another pack run. Output is restricted to an
ignored `.local/build/z8-*` directory and existing files are rejected.
The JSON contains extracted game data and must stay private, along with the
ROM, CHK, manifest, generated headers, and any later captures or saves.

The verifier rebuilds the expected descriptor from authenticated source and
compares every field. Recomputing a modified file's self-fingerprint cannot
make it pass. The C header is a freestanding identity and census check only;
it does not authenticate arbitrary runtime memory or execute triggers.

## Authenticated scope

- USA ROM SHA-1: `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`.
- Extractor revision: `09b57f787a247107d1de82b8c5eab2734667ea1d`.
- Catalog entry 19, map `008/01B`, dimensions 128 by 128.
- CHK SHA-256: `ff6d0177a6a76fc68ef98f0962661857853ea11c50de9060b90ff934826cfd8e`.
- Canonical descriptor fingerprint:
  `f80b5f9e9419c324a6b86b9a11bbe3dab0ec545d20817fb0f26509e82c85a986`.

The descriptor retains 39 triggers, 122 actions, 55 conditions, 64 locations,
198 complete UNIT records, ownership/force tables, unit availability, and
unit-property data. UNIT serial IDs and reciprocal links are retained,
including four Nydus records forming two pairs. Terrain, art and audio are
outside this descriptor.

There are 13 PreserveTrigger actions and 16 CreateUnitWithProperties actions.
These totals are source inventory, not a claim that every preserved trigger
spawns units or that the current engine supports their behavior. Source AI
actions distinguish `Zer8`, `Ze8T`, and `MvTe`; they must not be collapsed
into one generic AI routine.

## Remaining runtime work

Z8 needs its own scheduler covering all 39 triggers, preserved-trigger
lifecycle, Kill conditions, property-aware creation, mutable locations,
invincibility, and the source AI scripts. Linked Nydus transit and the
scenario's actor/detection requirements also need integration. Reusing a
one-shot Z7 trigger model or dropping unsupported actors would change the
mission rather than complete it.

No mission-win, ARM gameplay, SRAM continuation, sprite coverage, or
frame-rate qualification is established by this foundation. Canonical
campaign menus and the qualified mission count remain unchanged.

## Local verification

On 2026-10-27 the nine focused synthetic tests passed, including altered
rows/properties with recomputed fingerprints, invalid links and bounds,
malformed UNIT rows, duplicate JSON keys, output-path restrictions, and
exclusive non-overwrite behavior. A new private pack and independent verify
both produced the pinned descriptor fingerprint. A private C fixture accepted
the header identity and rejected an altered action count; its freestanding
ARM link had no undefined symbols and used no data or BSS.

`python3 tools/check_source_tree.py` accepted all 787 indexed text files.
`./dev doctor`, `./dev build`, and `./dev test` passed. The latter included
563 Python tests and the asset-free C checks, including route-policy,
collision, actor-vital-bar, and source-weapon checks. These checks do not
exercise Z8 gameplay. No GitHub Actions runner was used.
