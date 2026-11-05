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
