# Fresh Z9 setup qualification

Production is unchanged. This adds explicit `dev sc64` preparation, readiness,
build and test support for the existing nine-mission candidate; the canonical
Zerg alias and its save directory remain on Z7.

The initial run at 8afd6b33 moved this isolated worktree's cloned `.local/sc64`, `.local/build`,
`.local/p8-availability`, `.local/walk-words-managed` and `build/generated` caches
out of the preparation graph. No retained bank or program was copied back.
The first clean preparation exposed an invalid base-only/P6 generator pairing;
that failed attempt is retained. Retrying with the inherited T4 props enabled
passes preparation, normal ARM build/verification and status at 96508a34.

The resulting entire ROM SHA-256 is
`385c472b95d9656fd215239e376a70520839e37ed648da57af409f9febc276d1`,
byte-identical to the current-engine reference built at 6f5e006d. Production
sources and the complete Z9 compiler command match that reference. This is
fresh preparation equivalence, not a new gameplay, N64 parity or FPS result.
All 1,958 original qualification inputs remain unchanged.

The Z9 carried-profile fixture passes all nine campaign normal-menu startup/save
roundtrips and all 234 skirmish startup/save configurations under ASan/UBSan.
Its new art fixture initially tried to instantiate the Z10-only Temple building
identity under Z9; 683b889a corrects the fixture scope without changing the
production engine or the existing Z10 coverage. The final Z9 art fixture passes at 683b889a under ASan/UBSan. A separate
unprepared checkout at 96508a34 passes source-boundary, doctor, `./dev build`
and `./dev test`: 974 Python tests (26 skipped), the separate 5- and 8-test
groups, and native fixtures. The subsequent change is confined to the optional
Z9/Z10 art fixture. A merge of replay-only PR #282 preserves production and
adds no gameplay changes; its strict verifier/parser suites pass 25 tests.

The expanded bank readiness check requires completeness markers; it is not an
independent hash validation of every sprite pixel. Programs are compared in full
against authenticated descriptor renders. Gameplay qualification is documented
in the current integration and carried HOST/ARM audits. ROMs, generated data,
saves and evidence stay private. All native jobs use the shared flock and the
2 GiB guard; no Actions or SD writes were made.
