# Current-release worker cargo integration

This draft integrates the persistent cargo work from `f6f4ff35` with release `6417737c`, which promotes the P10 default and repairs the T8 controller. The merge is `9027b95a`; the focused qualification is at `7adfef99`. The Make conflict retains the current generated icon/effect prerequisites and adds the cargo flag. No existing release or SD image is replaced by this draft.

The optional cargo suite now includes the actual P10 product vector as well as explicit P7. The flag-expansion check follows the P10 default. The invalid-candidate fixture explicitly clears its synthetic HOST journal before the second independent split-profile rejection case: after a deliberately malformed committed record is rejected, the production first-write guard correctly refuses an ordinary overwrite. That guard and the rejection/unchanged-world assertions are preserved.

With fresh prepared inputs cloned from the verified `6417737c` race package and the pinned extractor `09b57f787a247107d1de82b8c5eab2734667ea1d`, these checks passed locally under the native lock and free-space guard:

```sh
python3 -m unittest tests.test_sc64_worker_cargo_flags
make build/test-terran test-sc64-worker-gather-terran test-sc64-worker-gather-p10
./build/test-terran
```

The Terran and P10 checks cover ordinary worker inputs, resource identity, AI resumption and actual-codec current/legacy migration for all three human races. The P10 payload is 16,332 bytes. The split-profile run includes its malformed-candidate rejection cases. The first build stopped before compilation because the isolated tree lacked the pinned extractor; after copying that existing checkout, the focused build and runs passed. Inputs and outputs remain private.

This is not current-base ARM, fresh-process SRAM, full-campaign or performance qualification. Earlier November 1 evidence belongs to its recorded source and inputs, and does not qualify this new integration. The branch remains unmerged while broader current-profile checks are completed.

## Current namespace and fixture follow-up

The broader run initially stopped at the next independent T3 malformed-record case, and P10's profile assertions still expected the pre-cargo namespace. T3 now explicitly checks that ordinary saving is refused before clearing each rejected HOST record. The profile checks include the literal cargo XOR bit while retaining their generation constants. Their isolated wrong-generation cases remove valid predecessors before testing rejection, so correct journal fallback cannot mask that separate negative. The P7 foreign-journal preservation check still verifies unchanged bytes before fixture teardown.

At `3716f196`, the T3 save suite and P10 promoted-profile suite passed, including all 234 P10 host startup/save configurations. Independent cargo-enabled Battlecruiser flight, research and Yamato tests passed after four explicit between-case cleanup boundaries (`50f15ebc`); these checks used unchanged copies of all 1,734 recorded SC64 release inputs. Broader current-base ARM, migration, campaign and performance work remains required.

## Full Terran and real P10-save follow-up

At `ef4e6437`, `./dev sc64 test --profile terran` passed after the fixture fixes, including all ten Terran controllers. A genuine legacy P10 SRAM file was then produced through ordinary custom-game controls on the verified `6417737c` P10 ROM. Map 1, human Protoss and one Zerg opponent ran for 300 updates before a normal pause-menu save. Its SHA-256 is `76a533126c9da74472fed9f0ba36d2cfbd903acc84696f68e47bee3855543c75`.

The cargo P10 build (`6ec70bb51d3cee4d7be867a393b0e9f641f351b6ffaf5033dfefe99ad14de61b`) loaded that unmodified file through HOME/L. Captured frame 301, nine actors, three buildings, resources and lobby fields matched. Re-saving published the cargo record in slot 1 at sequence 2 and left legacy slot 0 at sequence 1 byte-identical. A fresh process loaded the new save and matched the same captured fields after 60 uninterrupted candidate updates. The read-only physical-input backend issued no guest-memory writes or forged/tampered records. This case does not exercise carried gas or unknown cargo flags, and its captured fingerprint is not an exhaustive world comparison or campaign-save proof.

The current Z7 ordinary-input controller reaches its existing limit with five enemy buildings remaining and zero mission faults; baseline attribution and strategy diagnosis remain open. The P1–P10 carried routes and other current ARM guards are being qualified. These are merge holds, not grounds to raise limits or weaken success checks.
