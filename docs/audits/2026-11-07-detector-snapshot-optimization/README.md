# Shared detector snapshot reuse

The shared Arbiter detector builds optical unit/building lists once inside a source tick. Their live/hidden/transport eligibility, current controller binding, source slot and visibility inputs do not change while that tick consumes the lists; the loop only updates cached `weapon` fields. The change resolves binding and human source slot once when constructing the snapshot and reuses the list's already-validated optical rows during its consumption. The no-list scalar route keeps its original checks. This does not change source-owner mapping, detection cadence, detector geometry, visibility rules, actor caps, or persistence.

An independent review found no actionable defect in the snapshot lifetime or binding hoist. The independent scalar owner-mask oracle passed 2,000 randomized worlds, and explicit binding/source/owner combinations and snapshot rebuilds after movement, capture, building completion and death passed. Prepared P10 and Z9 shared-sight sanitizer fixtures passed. These authored cases do not establish all runtime states or native parity.

On current source `8e9f0a0a`, a same-source opt-in comparison replayed the ordinary P7 combat input through normal P10 ARM binaries. Flag-off was byte-identical to the current control ROM `eae4a4e9...`; flag-on ROM was `317cc67d8f48b6771b3ebf4497c52b290466de49d097133b7b1bfdea65b34cd9`. Over 1,000 observed calls, 958 advanced and 42 did not; the frame/screen/mode timeline and three modal acknowledgments matched. Eleven sampled selected-world/fog/full displayed indexed framebuffer and palette hashes matched. Mean advancing loop cycles fell from 2,372,998 to 2,306,991 (2.782%, about 66,007 cycles); sampled maximum fell from 3,959,806 to 3,956,748. This is one emulated combat window, not every-frame or full-state equality, physical hardware, or a near-30-fps result.

A separate ordinary-input Protoss custom opening with one active AI matched ten selected-world/fog/full displayed framebuffer/palette samples through 2,500 input calls. The final sample was frame 2464 with four human and nine enemy units and four/five buildings. It is an opening/technology scene, not a battle or victory qualification.

The production source `10bda18e` is based on `fd30c5ff` and enables both shortcuts within the shared observer adapter without changing the normal release flags. Its normal P10 ROM exactly reproduces the measured `317cc67d...` ROM, so the two P10 replay pairs apply to that binary. All six Terran, Z9, P10, Showcase, Z7 and Z10 normal ARM links passed unchanged linker guards. Their 908 selected source/private/toolchain inputs were hashed before and after and checked against the qualified 976-entry package manifest, except the intentionally updated observer adapter and the separately qualified current `game.c`. Tightest EWRAM margin is 780 bytes (Z9/Z10), while Z7 retains 1,168 bytes to its IWRAM guard. These are static link margins, not runtime stack high-water or hardware evidence. The exact current warp-audio bank was used. Receipts and ROM/ELF/map hashes remain ignored under `.local/perf-current/`.

The unrelated detector-owner union and optical-cell memo remain rejected/default-off: both slowed this workload. The broader 30-fps and all-campaign/race qualification goals remain open.

At runtime `10bda18e`, a clean separate checkout without private inputs passed source-boundary, `./dev doctor`, `./dev build` and `./dev test`; Python discovery reported 1,023 tests and 26 skips, and the complete `./dev test` took 448.1 seconds. Prepared `make unified` also passed using the current warp-audio bank, with 595 copied inputs unchanged. The broader `test-unified` graph could not be executed from the frozen package bundle: its default BC cover directory is absent, and after using the verified cover overlay and marking the existing hash-pinned generated files old, the graph still requires the missing P5 sprite header. The Terran full-campaign test graph similarly requires the missing Terran-late `sc64_races_generated.h` after that overlay. No private inputs were generated or fetched to fill those gaps, and neither broader check is claimed as passing. The targeted P10/Z9 source-sight fixtures and six normal links above are the available prepared-profile evidence.


## Qualified Mac package

The private local package `StarCraft-GBA-2026-11-07-d3535af6` reuses the four
qualified normal ELFs after a zero-diff production-source/build-definition
comparison against merged `d3535af6`. Root independently read back the 908
selected inputs, all six ELF/map pairs and exact measured P10 ROM equality.
All four 32 MiB ROMs pass logo/header/title/code/SRAM checks, and the 19
payload/metadata files match their checksum list. The local README explains
product selection, scope and the Protoss custom retest. No SD write occurred.

| Product | SHA-256 |
| --- | --- |
| Terran | `00b01425480fb1b01cd4720ce36fe9eba274dc34cde2de351e8909ff743d70f8` |
| Zerg Z9 | `aee95a66560b5fc5de7ae7dff8d906cd6035ba62eb50c6051ea6a1301b6d1549` |
| Protoss P10 | `317cc67d8f48b6771b3ebf4497c52b290466de49d097133b7b1bfdea65b34cd9` |
| Showcase | `8f5bc974319a40ab6abaaff99222db2b4bb16cade5e1f1eb28321ef66f6f34a5` |

These are the normal products: experimental typed research and early shared
Egg stages remain disabled. The content count remains 29/30, with full current
campaign, native parity, long custom games and near-30-fps qualification open.
