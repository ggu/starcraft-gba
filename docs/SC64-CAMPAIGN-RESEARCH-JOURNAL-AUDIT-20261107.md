# HOST campaign research journal experiment — 2026-11-07

This is an opt-in HOST experiment, not a shipped save format or an enablement
of campaign research. `SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL`
requires the existing campaign jobs and jobs-journal flags. Default builds
remain off, and this stage must not be used with an unqualified fractional
combat implementation.

The experiment reserves namespace byte `0x47` for campaign active jobs;
`0x43`, `0x44`, and custom active-job `0x45` retain their existing meanings.
The independent Flash prototype's `0x46` test literal is not treated as a
normal journal reservation. The namespace test exhausts current base and
generation families and every high-byte selector combination. A future tag
transformation must update this inventory.

The reader uses the detached candidate's mode and canonical mission index,
the immutable campaign map and P3–P6 role overrides, and the generated policy
for the current human controller. An actor's retained raw origin is not its
payer. Only the compact human `SIDE` payer is admitted; ambiguous enemy jobs,
duplicate family jobs, invalid rows, unsupported source costs, and levels
outside the source policy are rejected. The validated receipt uses the frozen
`0x45` quote and must also agree with the campaign source-default quote. Old
campaign records receive the scenario's source-initial research floor in the
candidate, while retaining any greater recorded level subject to policy. The
existing building and actor preflight still runs before publishing the result.
Checksum and tag validation detect malformed records; they are not
cryptographic proof that payment occurred.

At source commit `f5a5b7aba24098a2136e8f2bcac874ff512b7402`, the optional
`test-sc64-research-campaign-journal-p10` target passed with P10 HOST flags,
ASan and UBSan. Separate processes used actual game save and load functions:

- Authored P1 Forge ground weapons and Cybernetics Core Singularity jobs were
  paid, saved, loaded in fresh processes, and ticked through completion.
- Exact `847dbaaebf6454b0cb45577ce1e7ddd91d81e7d2` game/common sources
  produced genuine `0x43` and research-only `0x44` predecessor saves. Fresh
  experimental readers loaded them, then wrote new `0x47` records. A
  checksum-valid, deliberately invalid newer context fell back to each
  predecessor without changing SRAM.
- A sole checksum-valid invalid `0x47` record was rejected without changing
  SRAM, wallet, selected mission, research levels, unit array, or building
  array. A forged payer was refused on write without changing SRAM.
- The same combined experiment wrote and fresh-loaded a custom `0x45` job and
  ticked it through completion. This is a focused non-regression, not the full
  custom journal matrix.

The prior P1–P10 detached role comparison at `23ecd869` also matched each
live role/policy result while the currently selected live mission was changed.
The qualification used 593 hash-verified private inputs, unchanged before and
after; they and the generated saves remain outside version control. The
baseline producer used the exact 847d source checkout through the optional
`SC64_CAMPAIGN_BASELINE_ROOT` Make variable. Results and full command output are
retained locally in `.local/research-campaign-runtime/journal-v6.json` and
`journal-v6.log`.

These are constructed HOST states, not an ordinary-input campaign playthrough,
physical hardware test, GBA build or a claim that every historical or malformed
record has been qualified. Further independent review, broader campaign and
malformed-row coverage, actual ordinary-play continuation, combat and fractional
state persistence, and guarded GBA resource qualification remain necessary.
No actor or impact cap or existing memory guard is changed by this experiment.

An independent review found that the first version rejected unrelated Terran
and Zerg campaign saves. At runtime commit `2e4ce4e50bf65e699cd55039a8b3c9a87aa89fbb`,
those campaigns use a separate inactive-feature path: the candidate's T/Z
mission and descriptor human race must agree, every typed level must be zero,
and Forge/Core rows must not contain job-like state. The normal map and actor
preflight still applies. Old non-Protoss records do not receive a Protoss
scenario floor.

At test commit `db99319086ac2adbdb459a7391ab09d7289a4c96`, T1 under the
Terran HOST vector and Z1 under the Z9 HOST vector each passed a genuine 847d
`0x43` predecessor export, fresh experimental load, migrated save and fresh
reload, as well as a new inactive save/fresh load. Each rejected a nonzero
research level without mutating SRAM. These are authored HOST fixtures, not
coverage of every T/Z mission or a claim that every inert legacy producer row
has been tested. The v7 P10 and v8 Terran/Z9 receipts are retained locally in
`journal-v7.json`/`.log` and `journal-v8.json`/`.log`, respectively; the 593
private inputs and both source heads were unchanged in each run.
