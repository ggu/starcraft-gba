# Shared Scourge campaign product picker

At base `b2d18ece24aa723fe95cc9cb8e974c2e95067706`, the shared Scourge backend
accepts non-Z7 production, but the campaign picker still unconditionally skips
Scourge outside Z7. Custom games return earlier through their catalog.
The fix makes the picker's capability guards agree with the backend: shared
combat includes Scourge, while the legacy Z7-only vector retains its restriction.
No unit identity, cost, combat, production timer or save format changes.

The authenticated retained Z8, Z9 and Z10 descriptors permit raw unit 47 for
source player 3. The new HOST fixture initializes each actual campaign state,
checks the producer's source-owner permission and calls the actual picker
helper. It verifies browsing spends nothing and selected Scourge survives the
real journal save/load path. This is a constructed HOST fixture, not an
ordinary-input campaign win or an N64 visual measurement.

The test-only baseline `72485a58` compiled successfully, then failed the picker
assertion. Runtime fix `37a2db17` passed the two ordinary Z9/Z10 production
vectors under ASan/UBSan; test successor `69cd24fb` also passed the journal
roundtrip. The runs used `-g0` to bound debug-output storage and checked clean
source and all 1,959 pinned inputs before and after under the exclusive native
lock. An initial roundtrip attempt stopped at the disk guard before compilation;
the later guarded retry passed. Local logs are `/tmp/sc64-scourge-picker-baseline.log`,
`/tmp/sc64-scourge-picker-patched.log`, `/tmp/sc64-scourge-picker-save.log` and
`/tmp/sc64-scourge-picker-save-retry.log`.

Reproduce with `make test-sc64-campaign-scourge` after preparing the private
Z9/Z10 inputs. The six release ROM/RAM builds, default checks, existing shared
pair/combat matrix, earlier campaign permission coverage and ordinary-input
ARM/SRAM checks remain pending. This change is not yet a release qualification.

## Source restrictions and current qualification

The subsequent read-only, pinned-extractor ROM audit checked all ten Zerg maps:
human source slot 3 cannot produce raw 47 (Scourge) or raw 141 (Spire) in Z1
or Z5. Runtime successor `0d6d4d7a` applies that rule consistently to browsing,
training and building admission. Captured actors use their current controller's
permission, while legacy save validation remains unchanged. Custom games retain
shared support. Test-only successor `b3d6f8ef793a244fb75f4f859abc3c7b42ae97b9`
adds viable captured-provenance producer and Drone/site controls: denied cases
spend nothing, and the identical setup is accepted in permitted Z2.

Both Z9/Z10 ASan/UBSan fixtures pass. All six ordinary ROM verifiers and RAM
guards pass, with reserve endpoints equal to c96 and all 1,959 inputs verified
before/after. The full clean-Git default suite also passes (989 Python tests,
26 skipped, plus compiled suites). Five optional-fixture failures in a prepared
checkout reproduce on the unchanged prior source; missing-Git errors in a source
export were resolved by checks in a real clean worktree. The failed attempts
remain recorded, rather than being counted as passing invocations.

A separate prior-code writer serialized an actual pre-worker-cargo HOST journal;
a fresh candidate process loaded its unmodified bytes and restored the Z1
Scourge selection. This is a constructed HOST compatibility case, not a guest
playthrough or proof of every arbitrary save. No format, stat or timing changes
were made. Native N64 controls and physical-hardware play remain separate checks.
Local receipts: `/tmp/sc64-scourge-release-qualification.json`,
`/tmp/sc64-scourge-clean-full.log`, and `/tmp/sc64-scourge-oldsave-b3/run.log`.
