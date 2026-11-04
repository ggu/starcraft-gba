# Prepared save selection

Cargo-aware loading previously prepared journal candidates, reloaded the chosen
raw bytes, and prepared the winner again. The explicit `save_select_prepared`
API retains the successful prepared scratch and load plan. Existing raw
selection and transactional APIs retain their contracts. All canonical
cargo-aware products use the shared path; no save format, namespace, writer,
unit layout or persistent allocation changes.

Eligible committed records are checksummed and snapshotted before selection.
Unambiguous candidates are tried newest first, with signed sequence wrap and
slot-0 ties. A semantically rejected newest candidate falls back. At exactly
half-range separation both candidates are validated: two accepted candidates
reject, while a sole accepted first candidate is prepared again after a rejected
second candidate may have destroyed its private plan. The selected original
record is rechecked before publishing its token. Callbacks must deterministically
rebuild scratch/private preparation state from the current payload without
mutating SRAM or live gameplay. The cargo callback's uncommitted transport
candidate and restored temporary Protoss binding are private preparation state.
This is not a claim of hardware atomicity against concurrent SRAM mutation.

## Sources and local checks

Runtime implementation `f4b140bffad3912f693b7e55560568bacb248c1a` was integrated
with default branch `9f596ad1` at tested commit
`a9a0ea76b01dd6964b0ebe13d9339449b1953bc0`. Subsequent changes in this PR only
clarify the header contract and add this audit; qualified ROMs were built at
`a9a0ea76`. Private inputs, journals, logs and generated data remain local.

- `make test-gba-save-prepared`: 9,956 paired authored HOST journal cases in
  each of four protocol configurations, plus focused negative/preservation
  cases, pass against the existing raw selector. Prepared scratch is retained,
  failure leaves the output token unchanged, and existing transaction
  regressions run unchanged. Invalid journals are HOST fixtures only.
- Five actual game save fixtures (Terran, Z7, P7, P10, Showcase) pass under
  AddressSanitizer and UndefinedBehaviorSanitizer.
- All six normal product builds/guards pass: Terran, Z7, P10, Showcase, Z9 and
  Z10. The late profiles retain their 808-byte EWRAM reserve. All 1,958 prepared
  input hashes remain unchanged.
- A separate clean checkout without imported inputs passes source-boundary,
  doctor, `./dev build`, and `./dev test`: 965 Python tests (26 skipped), separate
  5- and 8-test groups, and native fixtures. Prepared `make unified` and
  `make test-unified` pass. This does not claim every optional prepared-default
  renderer test passes. No GitHub Actions were dispatched.

## Genuine ARM continuation and migration

Read-only emulator backends used ordinary controller input and genuine SRAM.
These are continuations of previously qualified prefixes, not new from-start
campaign qualifications or physical GBA measurements.

| Runtime / mission | Genuine cut frame | Compared checkpoints | Win frame |
| --- | ---: | ---: | ---: |
| Z7 / Z7 | 136058 | 49 | 173807 |
| P10 / P8 | 31374 | 64 | 92377 |
| P10 / P10 | 31064 | 118 | 145833 |

All three pass with zero reported faults and complete terminal SRAM files
byte-identical to their previous qualified references. Z7 uses scalar
checkpoints; generic P8's digest omits flags/work timer/build; P10's strict
digest includes those cargo-sensitive fields. Terminal SRAM equality covers
serialized final state, not unobserved intermediate fields.

Terminal SRAM SHA-256:

- Z7: `de1d039ed52756d98114ea31a644693ed73e394ab212200f8f8b41b8674cb5ba`
- P8: `1f082fed3dc664257da39ecf008959b38651a202eb9d09dd867d9cf21dbd9232`
- P10: `8742e7f7761f00a1844a732d477bb21349044c3e0c9036357d8cf98bc24d94d0`

A genuine P10 save from the older `6417737c` release loads at the same frame
301 (9 units, 3 buildings), retains existing cargo bits, saves through the menu
while preserving the old journal slot, and matches a fresh-process continuation
for 60 updates. Its complete migrated SRAM equals the prior migration result:
`35129913cc2f8ace01b09baba5175e9fb2278084df330c0a0681a722d17ebff3`.
This is not P7-to-P10 migration support.

## Timing scope

The instrumented genuine Z7 load pair compares the earlier integrated baseline
`162bf1fc` with implementation `f4b140bf`. Loaded scalar fields match and SRAM
is unchanged. Two full preflights become one; whole measured load falls from
119,306,055 to 68,463,821 guest cycles (42.615%). Diagnostic placement affects
timings; this is load-only evidence, not mission qualification or steady FPS.

The separate normal combined Z7 continuation passes and reduces its maximum
measured update span from 158,692,721 to 107,288,788 cycles (about 9.46 to 6.40
emulated seconds). Its mean is still 2,750,128 cycles per update, about 6.10 fps
equivalent. The 30 fps goal remains unfinished; this change reduces blocking
save/load work rather than solving busy-gameplay performance.
