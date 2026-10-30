# Baseline load preflight extraction

Runtime `059dda5` carries the same extraction as independently reviewed
`a0e7b4c`. Existing record selection, version checks, validator order and
post-commit restore order remain. Candidate-only outputs reside in an
automatic `RaceLoadPlan`, separate from route/save scratch. Pending impacts
are decoded before validators reuse that scratch. Expanded T4, T5, T6 and
late-Terran candidates share a union: the existing mission if/else chain
selects at most one, and the original validity/mission guards protect reads.

The initial all-members struct increased the measured Terran load frame;
it was rejected. The revised union reduces that frame from 6,288 to 3,600
bytes under the captured GCC vector. These numbers are not a complete
whole-call-chain stack bound. External library calls and interrupt margin
still need accounting before making such a claim.

## Qualification

- Four actual Terran, Zerg, P7 and Showcase gated Scarab production-envelope
  fixtures pass ASan/UBSan, including positive worlds and 13 checksum-valid
  malformed graphs. Their output logs match the baselines. Rejection checks
  cover actors, buildings, resources, frame and RNG, not every live global.
- The historical signed-16 RACES fixture passes both versions, including
  four map save/load round trips and corrupt rejection.
- T4 and T5 mission saves pass; T6 and late-Terran baseline/candidate fixtures
  pass under their matching generation-specific vectors. An earlier T6 run
  with the wrong profile asserted; its log is retained as a setup failure.
- Separate-process legacy-BC pending-impact and two-active-Yamato save,
  reload and continuation fixtures pass both versions, with identical SRAM
  and output hashes. These are seeded semantic tests, not ordinary-input
  mission or hardware proof.
- Four actual normal Make links with the explicit Scarab gate retain existing
  memory guards: IWRAM ends T `03004db8`, Z6 `03004e38`, P7 `03004c80`,
  Showcase `03004fc8`; BSS ends `020370b8`, `02037124`, `020382e4`,
  `0203df38`. Private inputs are frozen; no art generators run. This is the
  gated foundation, not a new normal-production or gameplay qualification.
- A clean indexed worktree at exact combined runtime `059dda5` passes
  boundary, doctor, default build and test: 786 Python tests with 23 optional
  skips, eight ancillary tests and C fixtures. The six source-boundary tests
  pass separately; a fixture-only child renames them into the existing
  default `test_sc64*.py` discovery pattern. No runtime changes in that child.

Private receipts remain in the `scarab-load-preflight-20261031` worktree's
`.local/preflight-*` directories and the `scarab-preflight-default-20261031`
worktree's `.local/qualification`. No ROM, save, trace or extracted payload
is tracked.

This is not Scarab namespace/migration registration or proof of atomicity
for existing fallible post-commit rebinds. Ordinary Reaver admission remains
blocked. Normal gate-off profile matrices and campaign qualification remain
necessary before promoting shared runtime changes. No new FPS or native-N64
claim, Actions dispatch or SD write is made.

## Combined oracle and bridge child

Clean indexed source `d5d03e3` also passes boundary, doctor and default build,
then the complete default test workflow: 793 Python cases (23 optional skips),
eight ancillary cases and C fixtures. It includes the discovered boundary
regressions, unregistered flight bridge, and source-only OpenBW world probe
helpers. The initial concurrent test attempt failed with ENOSPC; its log is
retained. A serial retry after byte-verified transparent compression passes.
All numeric exits and exact source/tree identities are under
`.local/qualification-oracle-bridge` in the clean preflight worktree. This
supersedes no optional-profile or runtime scope above. Following audit-only
changes do not change the tested implementation.
