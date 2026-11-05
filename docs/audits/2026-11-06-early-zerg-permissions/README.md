# Authenticated early-Zerg permission inputs

Source checkpoint `a0c6387e` adds an independent private Z1–Z4 CHK/PUNI
extractor and immutable packed-bit accessor. It does not enable production or
change normal ROM flags. Runtime callers must separately validate source-slot
ownership; raw CHK SIDE values are not the engine's race enumeration.

Run from the repository root after preparing the pinned extractor:

```sh
python3 -m tools.sc64_early_zerg_permissions \
  --rom /path/to/owned/StarCraft64-USA.z64 \
  --extractor references/sc64-maps/extract_sc64_maps.py
```

Output defaults to the new ignored `.local/sc64/early-permissions` directory.
Existing output is rejected instead of overwritten; `--out` must resolve within
this checkout's ignored `.local`. Keep all CHKs, source records and generated
headers private. No game data is included in this source repository.

The tool checks normalized USA ROM SHA-256
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`,
extractor commit `09b57f787a247107d1de82b8c5eab2734667ea1d` and the extractor
file's exact pinned Git object before loading it. All four maps are decoded and
validated before writing outputs. The manifest hashes retained CHKs, PUNI,
packed effective bits, OWNR, SIDE and the generated header. Z4's CHK hash agrees
with the previously authenticated Z4 input; pinned existing headers are untouched.

## Source observations

For source slot 3, which has human controller kind 6 in all four extracted maps:

| Mission / stable ID | Mutalisk 43 | Guardian 44 | Lair 132 | Hive 133 | Greater Spire 137 | Queens Nest 138 |
| --- | --- | --- | --- | --- | --- | --- |
| Z1 / 0 | denied | denied | denied | denied | denied | denied |
| Z2 / 1 | allowed | denied | allowed | denied | denied | denied |
| Z3 / 14 | allowed | denied | allowed | denied | denied | denied |
| Z4 / 15 | allowed | denied | allowed | denied | denied | allowed |

These are effective source PUNI facts, not proof of N64 runtime timing, a
complete tech tree or permission to map source slot 3 blindly to any saved
controller. Production denial does not invalidate a legitimately source-placed
unit. Early-Zerg admission still needs the immutable per-mission owner mapping.

## Qualification and limits

Four asset-free tests pass, including the actual C accessor under ASan/UBSan,
player/default override semantics, final bit, malformed CHK rejection and stable
mission IDs. A separate private C program checked all **10,944 accessor results**
against direct original-PUNI interpretation across four maps, twelve slots and
228 raw identities. Actual generated header SHA-256 is
`c9ff17c083b5f9cb699aad753da5b496931dd818f5b50d315954926632457a03`.
All 1,959 existing game-input hashes in the qualification input root were
unchanged. Extraction and native tests used the serialized native lock/disk guard.

The first pin-verification attempt used the old primary checkout, which lacks
late input files. Its four unit tests passed, but that pin verification failed
on a missing file and was not counted. Verification against the actual pinned
qualification root subsequently passed; no source/input was altered to force it.

Runtime/candidate integration, real early-campaign Eggs, owner mappings, every
save codec, full campaign wins, GBA RAM/stack and native execution remain
separate gates. The accessor only reads permissions and fails closed on unknown
mission, source-slot or raw-unit bounds. It reads no live state or SRAM.
