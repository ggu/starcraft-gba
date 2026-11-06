# Worker resource-cycle ordering

The previous worker state machines set 45/60 work timers only after pickup. Loaded workers bypass those timers, and deposit overwrote them with 20, so the first pickup was immediate and later pickups waited only the post-deposit value. Both shared and legacy paths now start the existing configured timer on arrival, spend exactly 45 mineral or 60 gas stationary updates including the first, then collect; deposits and pickups leave a zero timer. Movement does not decrement it. Fresh gather commands and direct interruptions clear old progress while preserving cargo.

These configured values are retained from the engine, not a native-timing claim. The pinned OpenBW reference has 75 mineral / 37 gas timing, and its clock relationship to this engine is unresolved.

The existing Unit work_timer wire field and save tags are unchanged. Old empty-worker saves with timer N resume with N stationary updates; the former code collected on N+1. Loaded old saves return cargo through the existing route. A fresh-process P10 test used baseline 89c025d6's real writer and the candidate reader with a synthetic old timer of 20; no retagging was used. This is one custom-game configuration, not a full campaign compatibility proof.

P10 ASan/UBSan authored fixtures cover both resources across three races, two repeated adjacent-depot trips, field debit, travel, interruptions, mismatch cargo, depletion, no depot, and a current writer/reader journal round trip. All six normal ARM vectors linked with existing guards; the smallest EWRAM margin was 796 bytes (Z9 and Z10) and smallest IWRAM margin 336 bytes (Z7). Margins are static and do not measure runtime stack high-water. The change alters economy, so controller outcomes and broader campaigns remain separate gates.

On candidate `84b9b8f988c96c7ec64347a7df1cbb2d1ae90191`, the clean asset-free `./dev test` passed in 455.21 seconds. A P10 HOST P3 controller run with the previously recorded eight input hashes won at world frame 46009 (sequence 5, primary objective complete); this is one controller run, not physical-play or other-campaign evidence. Its first attempt stopped at an input preflight because a borrowed generated directory lacked the P8 battery header; a second stopped at compilation because the warning suppression was ordered incorrectly. Both were corrected without altering generated inputs or controller behavior before the successful run. The source-only test fixtures use authored setup; they do not establish native timing.

## Reproducing the profile fixtures

With the P10 profile inputs already prepared, run the existing profile target. `WORKER_CARGO_CPPFLAGS` permits a verified cover include directory when that bank is stored separately; set it to match the local prepared inputs:

```sh
make WORKER_CARGO_CPPFLAGS=-I.local/hardware-feedback/verified-cover test-sc64-worker-gather-p10
```

The target builds and runs the cycle and current-journal fixtures as well as the existing gathering, depletion, cargo-save and AI checks. The same cycle fixtures are included in the existing per-profile `test-sc64-worker-gather-<profile>` targets. Generated inputs remain private.

For the optional baseline-writer/fresh-reader compatibility fixture, use two separate, already-prepared worktrees with the same verified P10 inputs. Set `OLD` to the unchanged baseline worktree, `NEW` to this worktree, and `JOURNAL` to an ignored local file. The baseline consumes the new test fixture and Make fragment only; its engine sources remain unchanged. The extra `-f` may emit duplicate-target warnings because the baseline also includes its own older fragment.

```sh
make -C "$OLD" -f Makefile -f "$NEW/tests/sc64-worker-cargo/Makefile" \
  WORKER_CARGO_CPPFLAGS=-I.local/hardware-feedback/verified-cover \
  WORKER_CARGO_LEGACY_SOURCE="$NEW/tests/sc64_worker_gather_legacy_fresh.c" \
  build/test-sc64-worker-legacy-p10
make -C "$NEW" WORKER_CARGO_CPPFLAGS=-I.local/hardware-feedback/verified-cover \
  build/test-sc64-worker-legacy-p10
"$OLD/build/test-sc64-worker-legacy-p10" write "$JOURNAL"
"$NEW/build/test-sc64-worker-legacy-p10" read "$JOURNAL"
```

At `61b21aba0503b9ebe56c018bbf70f979e35a2fbb`, the P10 profile target and the separate baseline-writer/candidate-reader recipe passed. The clean source boundary, `./dev doctor`, and `./dev build` also passed. Plain unified build/test were not run: inventory of the prepared copy showed a missing private interface input and a construction generation step. No private assets were fetched or regenerated.
