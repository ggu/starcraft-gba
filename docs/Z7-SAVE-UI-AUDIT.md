# Opt-in race save UI context audit

This audit covers source checkpoint `188ae9b88416cf36b362c51813b730c59a3f2641`.
The feature remains **off by default**: `SC64_SAVE_UI_CONTEXT` is used only
when explicitly supplied to the build. The experimental ROM below was built
at `276137a0dcd6b1ff819cd7da1f38e849cb7d7f9f`; the only later changes through
`188ae9b` are the fixture and validation documentation, not product sources.

The extension puts a tagged UI record in existing RaceSave word 31. Word 30
is the Protoss service clock. The loader validates the tag and bounds before
world commit, treats zero as legacy, and clears a selected building if its
slot is no longer live. The extension does not change RaceSave size, actor
capacities, RAM guards or default flags. It intentionally leaves the menu
closed and the mode normal on load.

## Z7 physical-input witness

The unmodified trace drove the experimental normal ROM through the ordinary
pause-save at host tick 30204. A separate process loaded the exported SRAM
with title L and resumed the remaining tape. The prefix checked 81 retained
host observations; the fresh process checked the remaining 222 and won at
host tick 179764, frame 177804, screen 3, won 1, lost 0, outcome 1 and fault
0. An independent audit compared the recorded checkpoints to the trace and
found all 303 unique observation ticks present, with no extra or missing
checks. The comparison covers the named Z7 model, action arrays, entity
counts, economy, frame, screen and outcome, rather than full world arrays.
The save, summary, trace, ROM, ELF and runner still match the hashes recorded
by both runs.

The SRAM journal contained one generation-36 Z7 record, sequence 1, payload
16296 bytes and frame 29480. Its UI word `0xa101013a` encoded selected
building 57, cursor 1 and last command 1. An independent fresh-process read
confirmed those UI values and the saved bounded world/model fingerprint at
completed update 1. The physical continuation's 149561 updates include that
one title load update. The measured emulator loop averaged 6.187 Hz including
UI and presentation; it does not establish physical hardware performance or a
30 FPS result.

## Compatibility and test scope

The experimental ROM loaded the original zero-word SRAM and retained legacy
UI reset behavior. Conversely, the original shipping ROM loaded the new
marked SRAM, matched its bounded world/model sample and retained its own UI
reset behavior. These two checks establish the tested Z7 reader compatibility,
not continuation equivalence on the older ROM.

In separate physical processes, checksummed SRAM mutations showed that a
dead selected building loaded as none; invalid marker, building index, cursor
and last-command values left the process at title/frame zero. These are
specific tested malformed records, not exhaustive fuzzing.

The original broad shared-mechanics fixture's candidate injection used a raw
RaceSave where the active Yamato/impact configuration requires its compact
impact envelope. The test-only fix writes the candidate using the production
impact envelope helper and retains the construction, burn and rejected-load
assertions. Full fixture runs passed with the UI feature both off and on for
Terran, Zerg and Protoss production flag vectors. The enabled fixture also
exercised all three human races per vector, including actual legacy-zero and
dead-selection loads and malformed candidate rejection. The 109 prepared
build inputs were checked at preflight and postflight while those runs held
the shared build lock; that historical result does not assert the original
input paths still exist. The final source checkpoint also passed 18 focused
Python replay tests and `git diff --check`. No fresh ARM build or all-mission
qualification is claimed by this audit.

To run the optional UI-context fixture on all six existing production
vectors when their prepared inputs are available:

```sh
make test-sc64-shared-mechanics-ui-context
```

This builds separate host binaries for Terran, Zerg, Protoss, Showcase, Races
and Unified, enabling only `SC64_SAVE_UI_CONTEXT` in each existing vector. It
runs the full shared-mechanics fixture, including its original assertions and
UI save tests. `make test-sc64-shared-mechanics` remains the default-off
matrix. This target has not itself been run as part of the historical witness
above; those recorded UI-on builds covered Terran, Zerg and Protoss.

## SHA-256 evidence

| Artifact | SHA-256 |
| --- | --- |
| Strict trace | `5121ed97ea0ea3d57697ce2c7fc57eac2082e3a457bb4ad98517f09ecf510489` |
| Experimental Z7 ROM | `64bea52aad8a8535256e2386339c2a3e888e05af6bbe73de9bd9342721068e9b` |
| Experimental ELF | `1c91e4a56aa524a4ede3a2e060b72ca169d14f37130140df76dc1d64954b0ddb` |
| Physical runner | `95b3a8ccf0d2a04b7820366ee484012912e2657acb5bddf940ce473c1f06a18b` |
| Pause-save SRAM | `edec6a6bd4e5dd933fbea0b1d46388962462db80f2f6db68b6915432e3ab8c79` |
| Saved-state summary | `a2fb3da0b3e81690718cc0b605d15c29d0ae552a5d5364ea4747d6c9fbea5140` |
| Prefix result JSON | `f777737689dd20b9d0ae3da6ae1954dced6ca452e6c6f965fe0ced0cacf2f891` |
| Fresh-resume result JSON | `758a7c6b6774bec50b5f569b28c9f78003919125e754b8dd4676738cfb44d112` |
| New-ROM UI restore result | `2c43d8dd0fe006b33288c06b51d3d1604c1a3e4fc2bd6d0333b1fe1a67fe3774` |
| New-ROM legacy reader result | `ae62bab6b8d78822437c7771f837c001348a1788c914a106b01e2a086fb14cb8` |
| Old-ROM new-record reader result | `cb194899c7130ef73fa8bf6d8c77804f6e41364056ce2b34219b04ba3a008342` |
| Mutated SRAM checks result | `dbb51d7e55486c436db6eab40fbc2453d0c3db36a26367e1fee43c2c2206b9e7` |
| Six-fixture test log | `b70de6692c47704e640943db37556b9b4d0cbba74bc5841c07ed31a81bd3f8d4` |
