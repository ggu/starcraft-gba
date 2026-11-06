# Early shared-Egg integration HOST qualification

The integrated source checkpoint `59179efcd229c49bbe2ddcf098c73f2c225c8bea`
(tree `0066df03b67be218768540edb84b278991619e5a`) merges current main
`fb2e414f` with the separately recorded Z1–Z4/Z6 HOST stages and Z5
applicability audit. Normal product flags remain unchanged. Synthetic test
headers were renamed to satisfy source-path policy; explicit HOST-only fixture
switches select them without affecting private-bank or normal includes.

A serialized native run with the shared lock, at least 4 GiB free, 256 MiB
per-file limit and bounded commands completed **41 steps with zero failures**.
Every log was scanned for sanitizer diagnostics even when its exit code was
zero; UBSan was configured to halt on error. The 594 private input hashes and
five genuine predecessor save hashes were checked before, between and after
steps. The ignored structured receipt SHA-256 is
`70c0ff4d157aee9360baed4fe507e698c82b48d49124a21a5c623891dc448862`;
its private input-manifest SHA-256 is
`5cd042dfbfa326864c109de94ee41b7947727850c57997774e2404ea39970783`.

The run passed Z1, actual-bank Z2/Z3/Z4/Z6, separate synthetic-denial Z2/Z4/Z6,
Z6 capacity, Z8/Z9, shared capacity, deferred scheduler and custom fixtures.
It also passed fresh-process Z1/Z2/Z3/Z4/Z6 writer/readers, the Z4 pending
retry/terminal/malformed suite and genuine old-tag Z2/Z3/Z4/Z5/Z6 readers.
The Z5 reader performed 180 advancing no-input updates and left its 32 KiB
source image unchanged. These are constructed HOST fixtures, not campaign
wins. Actual-bank and synthetic cases remain distinct: Z4/Z6's actual bank has
no denied larva product supported by the tested vector, so denial checks there
use explicitly synthetic data. See the per-mission audits for source hashes,
old-save provenance, prior failed attempts and complete limits.

`tools/check_source_tree.py` passed for 1,854 indexed text files and
`./dev doctor` passed. The full `./dev test`, default build, optional unified
matrix and ARM links were **not** rerun at this integrated head. No emulator or
hardware was used. Z4 remains a combined Z9+Z4 HOST composition, and its
persisted CreateUnit retry is an explicit bounded GBA policy, not N64 or PC
fidelity. Standalone product vectors, memory/stack, all transfer and trigger
schedules, arbitrary old saves, ordinary-input campaign progress, Z8/Z10
campaign gates and near-30-fps hardware behavior remain open. The separate
release-enablement draft #298 is not included in this integration branch.
