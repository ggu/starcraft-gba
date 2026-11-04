# T8 controller assault selection

On release base `88c17f15`, the canonical Terran-flag T8 ordinary-input host controller reached its existing 200,000-call limit at frame 195,891 without winning and with zero mission faults. The final state contained many surviving, idle Wraiths scattered across the map. The assault helper double-taps a Wraith to select its type on screen; repeatedly choosing the first selectable Wraith can leave other groups idle.

The controller now anchors that double-tap on an idle Wraith. The route still uses normal buttons through `game_update()`, with live state used to plan inputs. No world state, objectives, success assertions, or call limit changed. T10 shares the helper.

With the private 2026-10-31 verified inputs, canonical Terran flags, and ASan/UBSan, the T8 baseline failed as above; the candidate won at call 170,733, frame 165,826, with zero faults and the existing victory, hero and completion checks satisfied. The shared T10 controller also won. Logs are local at `/tmp/sc64-t8-baseline.log`, `/tmp/sc64-t8-candidate.log`, and `/tmp/sc64-t10-candidate.log`. The builds and runs were serialized with the shared native lock and a 2 GiB free-space guard.

This is one host route, not an ARM replay, save continuation, performance measurement, physical hardware test, or proof of N64 equivalence. The complete Terran aggregate was not rerun here.
