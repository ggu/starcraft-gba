# Economy planner on the current shared engine

Runtime merge `4b719f9f6dd647705266b2dc818f89f6126fedff` combines
AI stage 1 `e0992e5401a917fe1f501ff1e46e3b5537a5cd4c` with shared
`16f55474f484cc0ef7b6b47d3ea626928e984cfd`, preserving both histories.
This includes the current route defaults and seven-ID packed walk/hover poses.
The subsequent `d12442b` and `7b70672` commits change verification only.

All work ran locally, without Actions, a network debugger, or SD writes.
Private inputs, artifacts, SRAM, transcripts and result JSON stay ignored.

## Checks

- Clean, unprepared `./dev build` and `./dev test` passed at runtime merge
  `4b719f9`. The Python suite ran 824 tests with 23 optional skips; the native
  checks and additional 5- and 8-test suites passed. The original-art ROM SHA-256
  remains `e612df7f0bd1417754e8665ca293bc53eedb194d55f93e2b996ea54ede1bd69c`.
- A separate checkout of the identical commit used the retained 104 prepared
  input records. Every source input matched its previous SHA-256, was frozen
  against regeneration, and matched again after the ARM builds and host matrix.
  Current authored producers generated the derived lookup/word sidecars locally.
- All four normal ARM products built and verified as 32 MiB ROMs with unchanged
  reserve guards. Their BSS/IWRAM endpoints equal shared `16f5547`:

| Product | BSS end | IWRAM end | ROM SHA-256 |
| --- | --- | --- | --- |
| Terran | `020370c0` | `03004b70` | `fa0995dd51bb22ec69903fc3af0a2555150b8c4f9eb2042a3c50e331bfc9665d` |
| Zerg | `02037134` | `03004c40` | `7bbe854b3740aacda98d9ce54d96f3906a73baf6863531848c088daf624fe8d0` |
| Protoss | `020382ec` | `03004a18` | `a6a44cf032b45918d860037786aefac5bd2b788b410dd9d0470aa70816f125d5` |
| Showcase | `0203df40` | `03004e08` | `df379bb1ffc5f483154c83d79008e9fef56dcf98bd89524c0cd7442b9d29d555` |

- `tools/sc64_skirmish_ai_matrix.py --criteria economy --jobs 3` reproduced
  **212/216**, including both maps, all human races and 1–3 mixed opponents.
  This is not a full pass. The same four exceptions remain: three small-map
  passive-human matches end before their requested save window, and one Zerg
  opponent reaches six rather than eight army supply before the human loses.
  Walking-builder, worker and recovery checks retain their reported passes.
- The three missing save cases were additionally run at frame 4,000, with 1,800
  continued host updates: Terran/Zerg-human/ttt, Zerg/Protoss-human/zzz and
  Protoss/Terran-human/ppp, all on map 0. All three actual-codec continuations
  passed. This adds earlier save evidence; it does not turn the original late
  save requests into passes.
- Fresh-process ARM SRAM continuation passed in all four products on map 1,
  against three opponents (Terran/Zerg/Protoss). Each saved through the pause
  menu at 9,000 play updates, then compared 900 uninterrupted and freshly loaded
  updates. Saved/restored and continued fingerprints matched, and loading left
  the 32 KiB SRAM file unchanged.
- Those four ARM cases passed again using the strengthened verifier from
  `7b70672`, against the same frozen runtime ROMs. It adds active production
  queue contents in logical ring order and selected additional persisted actor
  fields. Actual DWARF layouts cover both five 32-bit queue entries and five
  compact byte entries. Focused parser/verdict suites pass (9 + 13 tests).
  Negative tests distinguish active queued-type mutations from stale inactive
  ring cells and reject malformed DWARF arrays.
- An actual Terran SRAM-corruption control with the strengthened tool exited
  nonzero, rejected the load and failed both parity checks, as expected. It is
  retained as a successful negative control, not a continuation pass.
- Independent source review found no concrete blocker to stage-1 integration
  into shared. It checked dispatch isolation, builder selection and validation,
  arrival payment, reservations, stalled-order recovery, and loaded-worker
  exclusions. Whitespace and source-boundary checks passed.

## Limits and remaining work

The fingerprint explicitly compares selected persisted gameplay fields; it is
not a complete save-state, framebuffer, audio or path-cache comparison. This
qualification does not prove universal mission continuity, native AI parity or
near-30-fps hardware performance. The planner remains authored and omniscient,
and expansions still use the engine's first-depot cargo-return behavior.

This qualifies the scoped economy change for the shared integration branch,
not the release of #167, military stages #181/#182, Scarab admission, or campaign
promotion. Canonical coverage stays T10/Z6/P7 (23/30). The exposed unsupported
Reaver chain remains a separate shared-release hold.
