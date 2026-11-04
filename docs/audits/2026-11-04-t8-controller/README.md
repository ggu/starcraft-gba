# T8 controller assault selection

On release base `88c17f15`, the canonical Terran-flag T8 ordinary-input host controller reached its existing 200,000-call limit at frame 195,891 without winning and with zero mission faults. The final state contained many surviving, idle Wraiths scattered across the map. The assault helper double-taps a Wraith to select its type on screen; repeatedly choosing the first selectable Wraith can leave other groups idle.

The controller now anchors that double-tap on an idle Wraith. The route still uses normal buttons through `game_update()`, with live state used to plan inputs. No world state, objectives, success assertions, or call limit changed. T10 shares the helper.

With the private 2026-10-31 verified inputs, canonical Terran flags, and ASan/UBSan, the T8 baseline failed as above; the candidate won at call 170,733, frame 165,826, with zero faults and the existing victory, hero and completion checks satisfied. The shared T10 controller also won. Logs are local at `/tmp/sc64-t8-baseline.log`, `/tmp/sc64-t8-candidate.log`, and `/tmp/sc64-t10-candidate.log`. The builds and runs were serialized with the shared native lock and a 2 GiB free-space guard.

This is one host route, not an ARM replay, save continuation, performance measurement, physical hardware test, or proof of N64 equivalence. The complete Terran aggregate was not rerun here.

## Combined release verification

At merge commit `6e70f7d1` (repair plus canonical `659bfa4e`), the following commands passed locally with the same locked native execution and free-space guard:

```sh
make test-sc64-terran-full-campaign
./dev sc64 build --profile terran
./dev sc64 test --profile terran
python3 tools/check_source_tree.py
```

The aggregate ran the T1–T10 controllers and passed, including T8 at the same call and frame above. The profile build linked and verified the 32 MiB ROM (SHA-256 `37ed9c9695bcc956daa55cda22cfe2380ef157698af372189f23ec206ab40ba3`); the profile test passed. The source boundary check passed for 1,574 indexed text files.

Inputs were copy-on-write copies of the existing `.local/sc64` tree overlaid by the verified 2026-10-31 pinned set. Its local receipt is `.local/pinned-inputs/20261031-verified/receipt.json` in the main checkout, SHA-256 `a79b9ca34ef292bb218ea4eb2f24549626734b46e7437c8000fcce4c6eec6068`; all 100 `.local/sc64` entries listed in that receipt matched their recorded SHA-256 in this worktree. The receipt's other nine entries were not needed for this check and were not verified here. The remaining copied files outside that receipt were not independently hashed. Input files and outputs remain private.
