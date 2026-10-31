# Reuse owner lists for target-index setup — 2026-10-30

## Change

Shared parent `af77d81bf1f0fa3080d144054ad07222d732d81c` already builds
validated ascending per-owner unit and building lists each update. The target
index then rescanned both complete actor pools. Qualified runtime
`d570c6d8437d6f89cbbda120e960cd77384445ef` reuses those fresh lists instead.

Production calls the new builder only after the existing list-validity gate.
Loaded units are excluded by the same predicate. Insertion remains ascending
within each owner, with units before buildings, preserving each owner/cell
chain and target ties. Invalid lists bypass the index; failed insertion leaves
it inactive so queries use the existing fallback. Move, invalidation and
dirty-overflow behavior remain. The standalone full-scan entry point remains
available for callers without fresh lists.

No runtime storage, actor limit, targeting rule, cadence or reserve is added or
reduced. An initial instrumented Zerg prototype exceeded the existing IWRAM
guard; its failed link is retained. The qualified helper is explicitly placed
in ROM on GBA, rather than relaxing the guard. Later test-only commits
`bb0ecc4` and `272edfe` add the optional real-engine query workflow and exact
actor-byte comparisons; they do not change this runtime.

## Paired ARM measurement

Both sides use the current parent runtime `6003e60`/shared `af77d81`, the same
canonical Zerg feature vector, v6 bank, audio, input-only probe and ordinary-menu
three-AI tape. Timing covers calls 8021–9020 inclusive, 1,000 calls. Ninety
periodic selected-state/fog and displayed RGB555 hash/status checkpoints across
the complete 9,020-call replay match, including ten in the timed window.
These samples are not comparisons of every live byte.

| Measurement | Parent | Owner-list builder |
| --- | ---: | ---: |
| Total guest cycles | 718,197,178 | 715,369,996 |
| Orders-stage guest cycles | 171,861,981 | 169,104,540 |
| Equivalent calls/s | 23.36018 | 23.45250 |

Total cycles decrease by 2,827,182, or 0.39365%. This is one instrumented
emulator scene with a replacement input-only main and presentation wait.
It is not a general/all-profile FPS measurement, physical GBA result or
near-30-fps completion claim. Probe SHA-256:
`e81a735586cf310f1641cea1c149b349863eb25505333b7a52c66d278e5f8ca8`;
tape SHA-256:
`0d79706afb77e58c958a6dd622a56843f611f4400bfa014bdbeb7db751f6ef2b`.
The original failed candidate and both successful run logs remain private.

## Correctness and build qualification

- The asset-free ASan/UBSan differential passes 10,000 builds. It compares
  exact owner/cell heads, reachable next/previous/cell entries and occupancy,
  with dense ties, loaded actors, movement, forced dirty-overflow rebuild and
  invalid-coordinate recovery. Different dirty-cell recording order is not
  presented as identical cache state.
- `make test-target-owner-query` is an optional prepared-input workflow. All
  four actual canonical vectors pass 500 production update frames and 4,000
  old/new nearest-target comparisons each, including 1,186 nonempty results
  each. Exact Unit/Building snapshots and RNG remain unchanged by the queries.
  This is a seeded custom-game host workload, not campaign or malformed-world
  replay proof.
- Clean indexed source `272edfe` without private inputs passes boundary,
  doctor, default build and default test: 776 Python cases, 23 optional skips,
  separate eight-case suite and C checks. The default original-art ROM remains
  SHA-256 `6e5556d7fbbab348ab54bdc34ea8a228077b2981817f27100a1dd9f9f90aea0d`.
- Unified build/test pass at test head `bb0ecc4`; the later optional fixture
  hardening changes no runtime or Make recipe. Full Terran original recipe
  coverage passes as 22 bounded sequential records, including all ten mission
  controllers. All nine emitted controller traces match the retained exact
  parent-runtime `6003e60` traces byte-for-byte; T3 emits no trace.
- Four products pass 72 ordinary-input host matches and 96 save continuations:
  the three standard profiles cover 54/72 and Showcase adds 18/24 using the
  same Make templates with its actual flags. The 54 standard match traces
  match retained historical BC `dc286dd` traces. That historical comparator
  is not the exact `af77d81` parent, and no such comparison is claimed.
- Four normal Make ROMs pass header, ELF-payload, FF-padding, exact embedded
  audio and existing memory guards. Ordered vectors retain all nine BC/CV5
  gates and one body translation unit; the 104 prepared inputs match before
  and after the builds. No private generators run. BSS ends are unchanged.

| Product | BSS end | Parent IWRAM end | Candidate IWRAM end |
| --- | --- | --- | --- |
| Terran | `020370b8` | `03004ec0` | `03004d60` |
| Zerg | `02037124` | `03004fd0` | `03004e78` |
| Protoss P7 | `020382e4` | `03004c48` | `03004c48` |
| Showcase | `0203df38` | `03004fc8` | `03004fc8` |

Commands, source/input hashes, raw logs, executable inventories and comparison
receipts are retained under the worktree's ignored
`.local/bc-owner-index-perf-v1/`, `.local/build/owner-arm-v1/`,
`.local/owner-target-proof/`, `.local/target-qualification-v1/` and
`.local/owner-index-unified-v1/`. Clean source receipts are repository-local
`.local/owner-index-default-qualification-v1/`. No game data is in this audit.

The shared branch remains draft. Common roster services, all 30 campaign
missions, broader fidelity and near-30-fps/hardware qualification remain
unfinished. No Actions or SD writes were performed.
