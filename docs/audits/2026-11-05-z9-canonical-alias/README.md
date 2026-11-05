# Canonical Zerg alias: Z9

The canonical `zerg` command selects the existing Z9 ROM and ELF. It does not
change game logic or the source data. The explicit `zerg-z9` command selects
the same files and shares `.local/play/zerg-z9`; `zerg-z7` retains its Z7 ROM,
generation-36 namespace and `.local/play/zerg-z7-transport1`. There is no save
migration. Z10 remains outside the canonical ROM.

At production source `3d209a64bd20fd2fa538fe1f6c0ff375c5d2e3db`, all nine
ordinary-input HOST campaign controllers won, nine normal-menu campaign
startup/save checks passed, and the 234-case skirmish startup/save matrix
passed. The normal Z9 ROM SHA-256 was
`cbc4fa7b702abf383d8a6c4b12975b6a4396708d419ef89fd577da30dae4db22`.
The strict current Z9 replay matched 83 full, 7 prefix and 77 fresh-process
resume checkpoints, with win at input call 82066 and game frame 79409. The
pause save is at call 6515. HOST and ARM each retained identical complete
full/resumed SRAM within their own backend. Across backends the committed
headers and payloads matched with valid checksums, outer `0x56435a60`, inner
`0x52435a60`, 16312-byte payloads and expected journal sequences; unused
filler differed (HOST zero, ARM erased `0xff`). Prefix seed and predecessor
slot were retained. These are recorded emulator paths, not arbitrary saves.
The original orchestration's first compare incorrectly expected 78 resume
matches; its strict comparison actually had zero mismatches and 77 matches.
The count was corrected and the closed HOST outputs were revalidated; the
failed wrapper receipt remains private with the qualification.

The source-only alias build was checked with `./dev sc64 build --profile zerg`
and `./dev sc64 status --profile zerg` using the 1,958-file verified prepared
snapshot and the separately pinned text header. A private PATH-scoped Make
wrapper passed each of those 1,959 paths as `-o PATH` to preserve the snapshot;
the resulting ROM matched the SHA above, and all 1,958 receipt hashes remained
unchanged. The initial attempt supplied `-o` through `MAKEFLAGS`, which Make did
not apply to the header; it stopped before compilation and is retained as a
failed setup attempt. Dry runs of `make zerg` and `make zerg-z9` produced the
same Z9 compiler command. Source boundary and focused Python routing tests
also passed.

A separate ordinary-input full-match HOST matrix used the Z9 feature vector:
18/18 matches reached a terminal result across two maps, three human races,
and one to three opponents. All 18 were human losses; the matrix accepts a
completed loss and does not demonstrate a winning strategy. Its controller,
limits, and result validator were unchanged. The separate 234 setup/save cases
are not a substitute for those completed matches.

Earlier Z7/Z8 carried ARM replays and continuation evidence are scoped to the
[carried audit](../2026-11-05-z9-carried-arm/README.md), before the modal
outcome fix. No all-nine current ARM replay claim is made. Native AI fidelity,
arbitrary saves, physical hardware and 30-fps performance remain unqualified.
Private traces, saves, ROMs, build logs and inputs remain ignored locally.

A separate source-only checkout at `a54215f4` started with no prepared `.local`
or `build` directory. `./dev build` and `./dev test` passed there: 989 Python
tests (26 skipped), the secondary 5- and 8-test suites, and the asset-free
native checks. Its default ROM SHA-256 was
`9fe916dcb7bd5aeb06476601847e4feebf6df2b2a2499554f8c16248b689a858`.
An earlier clean run stopped on one outdated test that still expected the
canonical `--zerg` verifier to identify Z7; the test was corrected while
retaining its explicit Z7 assertion, then the clean run above passed.
