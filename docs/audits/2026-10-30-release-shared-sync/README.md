# Release/shared integration and save fixes — 2026-10-30

## Source and behavior

Merge `beb411afe577b472c1b0378d5c13c97bb80a176d` combines shared
`4947d920a7a3c405df6de9c01884054df5ae7bb4` with release
`bb60315fb7dc25c05ffc5260f8eedd7ea9ef60a5`. Both histories and their source
changes are retained, including T9/Z7 provenance, the explicit optional Z8
candidate, shared Battlecruiser services, mine geometry and profile bounds.
The published T9/Z7 provenance and original qualification audits are
byte-identical to release; their historical runtime scope is unchanged.

Qualified runtime `6003e607ecad0d9783a00cecc16ec0564ba4cfde` also fixes two
preexisting save defects discovered while checking the combination:

- `02ecdbc` validates decoded Z8 actors before applying a loaded world. A
  structurally valid envelope with a human unit assigned enemy source slot 1
  previously returned failure after changing live state. Candidate preflight
  now checks the same owner/type rules as the retained post-load rebind, using
  bounded local row decodes and the existing scratch layout. It is selected
  by the candidate mission header, not the previous live mission.
- `6003e60` clears the whole geyser array before populating campaign and
  custom worlds, including the imported-map replacement of synthetic geysers.
  P5-to-P1 restart/re-entry previously left inactive slot 2 at `(1600, 976)`
  on P1's 1536-pixel-wide map. Strict save validation then rejected a valid
  active world. Validation remains strict; inactive entries are initialized
  instead. No save fields, namespace, schema, actor limits or reserves change.

The promoted-profile fixture now exercises feature-tagged production envelopes
for both legacy-container and wrong-payload-version rejection. The asset-free
tag fixture extracts the real generation-selection chain without assuming P7
is its first branch. Independent literal generation/namespace assertions remain,
with Z8 precedence and namespace-collision coverage. These repairs do not relax
expected saved state or damage behavior.

## Verification

- Clean indexed source without private inputs passes boundary, doctor, default
  build and default test on `6003e60`: 775 Python cases, 23 optional skips,
  separate eight-case suite and C checks. The default ROM remains SHA-256
  `6e5556d7fbbab348ab54bdc34ea8a228077b2981817f27100a1dd9f9f90aea0d`.
- Actual canonical Zerg and P7 promoted fixtures each pass all 234 combinations
  of two maps, three human races and all one-to-three-AI race lineups, plus
  their campaign save roundtrips and legacy-negative checks. These are bounded
  startup/save setups, not full-match victories. The P5-to-P1 reproduction uses
  fixture restart/re-entry; an input-only finished-result-to-Home path was not
  newly proven.
- Full Terran host qualification passes 22 recorded executable/group results:
  all 14 controller recipe commands with their exact arguments and all eight
  aggregate prerequisite groups. All ten mission controllers complete. Unified
  host qualification passes 39 direct commands and both original grouped
  audio/VFX comparisons, 41 records total. The unified ROM also verifies.
- Disk exhaustion interrupted the first aggregate attempt. Its failure log is
  retained. The successful retry compiles/runs the same frozen Make targets and
  original recipe arguments sequentially, with per-target exits and hashes;
  it is not described as a successful single aggregate Make invocation.
- Z8's existing explicit candidate passes its actual-codec ASan/UBSan fixture:
  four malformed unit/building owner/type cases reject without changing the
  sampled world/scheduler/AI digest; a valid pause save resumes for 3,000
  matching updates with both AI towns producing and both Nydus links present.
  This is focused rejection/continuation evidence, not every-byte atomicity
  proof. The explicit candidate ARM build verifies within existing guards,
  without BC gates. Its ROM SHA-256 is
  `3fb425bd3807b555631da9657d9e7e492d8f93ff679e15242ce595aff235844e`.
- Four actual normal Make ARM products pass header, ELF-payload, FF-padding,
  embedded-audio and unchanged memory guards. Ordered driver vectors retain
  all nine BC/CV5 gates and one body translation unit. All 104 inputs in their
  recorded baseline match before/after/current hashes. No generators run.
  Additional frozen-path hashes are retained with their narrower current-only
  scope; they are not presented as independent before/after proof.

| Product | BSS end | IWRAM end | Unpadded payload bytes |
| --- | --- | --- | ---: |
| Terran | `020370b8` | `03004ec0` | 26,816,164 |
| Zerg | `02037124` | `03004fd0` | 25,463,460 |
| Protoss P7 | `020382e4` | `03004c48` | 25,897,504 |
| Showcase | `0203df38` | `03004fc8` | 25,552,824 |

The renderer is byte-identical to shared parent `4947d92`. Focused merged-source
checks pass 1,410,048 raster cases, 3,526,824 wide arithmetic cases, 400,000
reverse-scan comparisons and authored metadata/fallback tests. This preserves
global-first profile bounds, reverse last-matching-event selection and the
sprite birth-frame LO correction; it is not a new speedup measurement.

Private commands, logs, executable hashes and cleanup inventories are retained
under `.local/build/sync-arm-v2/`, `.local/build/release-sync-host/` and
`.local/build/release-sync-root-v1/` in the integration worktree. Default receipts
are repository-local `.local/release-sync-default-qualification-v1/`, including
the original test failure and ENOSPC retry evidence. Completed test/debug files
were reclaimed with inventories; ROMs, saves, inputs and logs were preserved.

## Remaining scope

Canonical menus remain T1–T10, Z1–Z6 and P1–P7. Z8 stays an explicit unpromoted
candidate without a new ordinary-input win, ARM replay or physical-GBA claim.
The shared branch remains draft for common roster/services and remaining
campaign/performance work. The prior 23.36-call/s Zerg scene is historical,
instrumented evidence, not a current hardware/general FPS result. Near 30 fps
and all 30 original missions remain unfinished.

No proprietary assets, generated banks, ROMs, captures or private traces are
included in this audit. No Actions or SD writes were performed.
