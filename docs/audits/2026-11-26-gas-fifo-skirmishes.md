# Gas FIFO in full Custom Game battles

Six paired three-AI HOST authors used the canonical Custom Game controller and the same settings within each pair, with ordinary gas assignment enabled. Stock uses the existing multi-worker gas timing; the candidate enables `SC64_GAS_SERVICE_TRIAL`. The candidate was built from `528d9082`; its differences from the requested `b10ae9cf` concern journal handling and tests, not normal simulation. The paired runs are separate evolving matches: once behavior differs, neither resource balances nor match outcomes isolate a single causal mechanism.

The authored settings match within each pair, but Blood Bath draws its start order from guest timing. The actual ROM-probed start orders differ between stock and FIFO despite identical lobby waits: Terran 54 → 57, Zerg 114 → 120, and Protoss 114 → 120. Outpost has no randomized order (0 in both builds). Consequently the Blood Bath outcomes and resource totals are also confounded by different spawn assignments and do not isolate the gas rule.

Both maps and all three human races were covered. All 12 original authors produced positive human gas-bank checkpoints. Eleven original traces reached and replayed real ARM results; the twelfth stalled. Two additional alternate-plan traces also reached and replayed real ARM results. At the nearest checkpoint before frame 12,000, an independent ordinary-input ARM run matched each author's full worker rows, refinery rows, site coordinates and depletion, and all four owners' ore/gas banks, after 99–140 matching prior checkpoints. These cuts are not a full unit-state comparison at every update. Raw inputs and captures remain local; no game assets are included here.

| Human / map / opponents | Stock outcome, frame | FIFO outcome, frame | Total site depletion stock → FIFO near frame 12,000 |
| --- | --- | --- | ---: |
| Terran / Outpost / ZPT | Win, 26,511 | Win, 27,748 | 4,208 → 3,440 |
| Terran / Blood Bath / ZPT | Win, 26,861 | Loss, 41,190 | 4,480 → 3,880 |
| Zerg / Outpost / TZP | Win, 26,277 | Loss, 20,823 | 3,200 → 2,720 |
| Zerg / Blood Bath / ZZZ | Win, 68,369 | Win, 81,312 | 4,416 → 3,344 |
| Protoss / Outpost / PTZ | Win, 32,381 | No result by 247,332 | 4,864 → 3,712 |
| Protoss / Blood Bath / TZP | Win, 39,650 | Loss, 14,857 | 3,496 → 2,664 |

The Outpost Protoss FIFO result is a HOST controller stalemate after 250,000 calls, not a cartridge-confirmed ending. Its first 100,000-call author trace and ARM economy cut are retained. The 250,000-call HOST trace SHA-256 is `bb5bbe7de0cc9711d4ea105f64abf7cd8d2a5b805e4808910bef78d8e29b8180`. A separate paired Outpost plan using `LINE=66,WORKERS=14,PRODUCERS=3,FIRST_ATTACK=7000` produced a stock loss at 22,648 and FIFO win at 35,394, both replayed to real ARM RESULT. This plan was selected after the stalemate; it does not replace the original pair or establish a general balance improvement.

The depletion column sums **all** source-site gas used by every owner, including sites no longer held by their original owner. The paired checkpoints are the nearest prior checks, at frames 11,769–11,987, not synchronized identical worlds. Depletion is not a human income counter. Gas in each bank is net of spending and is not cumulative income. For example, at the nearby controller-log samples, Terran Outpost shows completed human weapons/armor levels 2/2 in both modes, with banks of 618 and 530 gas respectively; four generic completed upgrades entail at least 400 gas paid, separately from the remaining bank. Zerg Blood Bath has levels 2/1 stock versus 0/0 FIFO at nearby controller-log frames, so similar low banks there do not mean equivalent purchased benefits. Army size and completed level observations are controller telemetry, not an independent per-purchase audit.


## Replay evidence

Each cell gives uninterrupted ARM checkpoint count / fresh-save continuation checkpoint count. “Loss” was checked through RESULT with the read-only loss wrapper and has no continuation or result-reload claim. Every win entry also passed victory-result reload and byte-identical victory SRAM checks.

| Human / map | Stock | FIFO |
| --- | ---: | ---: |
| Terran / Outpost | 220 / 92 | 234 / 106 |
| Terran / Blood Bath | 303 / 122 | 509 / loss |
| Zerg / Outpost | 225 / 108 | 183 / loss |
| Zerg / Blood Bath | 678 / 520 | 803 / 623 |
| Protoss / Outpost | 243 / 137 | HOST limit |
| Protoss / Blood Bath | 420 / 325 | 185 / loss |

The alternate Protoss Outpost plan has 158 loss checks on stock and 259 / 172 checks on FIFO, including its fresh-save continuation and victory-result reload.

The exact controller traces, read-only checkpoint economy records, ARM checkpoint cuts, ROM/ELF/author hashes and replay results are retained locally under `/private/tmp/sc64-gas-economy-artifacts`. `manifest.json` pins product and author inputs; each `<mode>-<race>-<map>-<opponents>/economy.json` contains all sampled workers, refineries, banks, site positions and depletion. The HOST observer only appends read-only fields: after removing them, every author trace is identical as parsed JSON to its canonical author trace. Each corresponding `arm-economy/evidence.json` independently compares that extra data at the selected cut. These local files and private ROM inputs are not included in the source tree.


The following SHA-256 values pin each authored input tape (stock / FIFO). The two Outpost Protoss alternative tapes are listed separately. ROM and author hashes are in the local manifest noted above.

| Case | Stock trace | FIFO trace |
| --- | --- | --- |
| Terran / Outpost | `8e3800a8c4af4700d1ff87cc4a97ceb481731f98bf025ad5ff25dcc5434fb178` | `0146370b30c344f28a5e6fdd5d8fa116ebbd6c02c28ff862657dd0584b444b4c` |
| Terran / Blood Bath | `ab0f18323b802910d383819e203baa9c5050834a3e0aef2d19e2c7b870ce7990` | `8879a16f5e68cbdec067e6d56b8bef2bb05cfbc66280eed485c775fa43a1831e` |
| Zerg / Outpost | `6bd708f5b772d0c692a5e9b6f793c8c5bb184118c9b583ab058c9cb9674e0691` | `d1f9c0409d5fbfbb4cf2903373fd6b6c988ba1646058f56614a509e2a29d30d4` |
| Zerg / Blood Bath | `5438324244ff1cfbd9fc724a912973674e65132e24ee332d42f0940eb24a6363` | `9427164ec918b1cea9b1bc9795d06c6e455c6f192575932e74abb24d7b7294b7` |
| Protoss / Outpost | `42ea51caa73faed29258cdb9427388a662e87487b9926dca9e0b061444939e69` | `35f918e15d84dee61a54a8ecc98bc4bee951b8d56f3f06e72a46a6205db6861f` |
| Protoss / Blood Bath | `480271d09d10155232cfbffbce42a26a207911fae677f50344a723a4820145f0` | `978306da9d59398b09694730578c1d13de79b9898693472bb613c46fa3ecca78` |
| Protoss / Outpost / alternate | `7859e9348318dd27d75e15b3160d53586da2e0722019e2719666a369056131a2` | `b8fa8b2480c6fa7ef081dfb8de8f9dc943fa06c4d46b68fe611598c4ec35a716` |

## Updated FIFO controller routes

These ordinary-input HOST controller settings were selected after the preceding FIFO losses. Each also passed a strict physical-input ARM replay, a fresh pause-save continuation, and a fresh victory-result reload with unchanged victory SRAM. Each has positive gas-bank checkpoints; the gas policy itself was not disabled. These wins qualify the individual routes, not balance across starts or plans.

| Human / map / opponents | FIFO start order | Result frame | Full / fresh-load checks | Plan |
| --- | ---: | ---: | ---: | --- |
| Zerg / Outpost / TZP | 0 | 28,301 | 243 / 53 | `LINE=38,WORKERS=12,FIRST_ATTACK=4500` |
| Terran / Blood Bath / ZPT | 57 | 24,758 | 289 / 141 | `LOBBY_WAIT=8,FIRST_ATTACK=3500,PRODUCERS=3,WORKERS=10,EARLY_WORKERS=14,TECH_AFTER=2,LAB_AFTER=12,ATTACK_MIN=10,ATTACK_RATIO=2,ATTACK_MARGIN=2,SUPPORT_RATIO=3,GROW_FRAME=2000` |
| Protoss / Blood Bath / TZP | 201 | 44,163 | 499 / 345 | `LINE=66,TECH_FRAME=8000,LOBBY_WAIT=13` |

Zerg Outpost is fixed-start and the Terran route keeps the original FIFO Blood Bath order 57. The Protoss route uses order 201 rather than the original FIFO order 120: its lobby wait changed from 18 to 13. For the original order 120, 360 randomized HOST controller variants lost; the search is not exhaustive and is not ARM evidence for those variants. The prior Protoss loss and the other original-plan outcomes remain valid observations.

The tuned input tape SHA-256 values, in table order, are:
- Zerg / Outpost / TZP: `8874deec01a90f2c0b76d37193bdb9d48bfd4033706c9ff948790222847d42db`.
- Terran / Blood Bath / ZPT: `aedfd89f34eceabdd48e451be2aa222f6ff2634a5159a606f83bf90a13d33688`.
- Protoss / Blood Bath / TZP: `86903f0f4a84f2e51064b278c9413b30b3c44ac0be6b32960a02f1aae59b2e09`.

The local replay receipts, selected HOST plans, search logs and tapes remain under `/private/tmp/sc64-gas-economy-artifacts/tuned`.

## Scope and follow-up

A win replay uses the canonical physical-button tool: it checks recorded world/owner/bank observations throughout, exports the battle pause, continues from that save in a fresh process, compares the later checks and victory SRAM, and reloads the victory result. Loss traces use the same physical input and checkpoint comparator through the actual loss RESULT; the win-only canonical replay does not test loss save continuation. A low gas bank can reflect real spending, while an observed site-depletion difference can reflect different worker assignments, destruction or ownership after the games diverge. These matches alone are not proof of native fidelity or a balanced change.

Follow up with native N64 measurements at a lightly staffed site and a busy saturated site: track arrival order, concurrent occupant/timers, cargo delivery, site depletion, and paid tech/army benefits using comparable starting states. Separately extend FIFO and stock cartridge qualification across additional plans, human races and maps, with exact frame and save-continuation evidence; retain the full input tapes and per-check snapshots.

## Blood Bath matched starts

The original Blood Bath pairs above used the same lobby configuration and controller plans, but the stock and FIFO lobby start orders differed (Terran 54 versus 57; Protoss and Zerg 114 versus 120). To control that difference, new FIFO traces adjusted only the initial lobby wait until the stock start order, player and bot owner/race assignments matched. The human controllers use the same plan as their stock counterparts; their later inputs respond to their independently evolving worlds. This is an additional controlled-start comparison, not an identical-input or balanced-game proof.

| Human / opponents | Matched order | Stock ARM result | Original FIFO ARM result, different order | Matched FIFO ARM result | Matched FIFO checks |
| --- | ---: | --- | --- | --- | ---: |
| Terran / ZPT | 54 | Win, 26,861 | Loss, 41,190 | Loss, 21,283 | 255 |
| Protoss / TZP | 114 | Win, 39,650 | Loss, 14,857 | Loss, 21,076 | 207 |
| Zerg / ZZZ | 114 | Win, 68,369 | Win, 81,312 | Loss, 63,923 | 673 |

The matched results replayed the HOST-generated physical buttons on the FIFO ARM cartridges from HOME, matching each recorded checkpoint and the actual loss RESULT screen. The loss verifier does not claim save continuation or result reload. The stock wins are the fully qualified original results described above. The trial cartridge is from `528d9082`; lobby waits differ across the pair to match start order. Each result remains one controller plan and one starting arrangement, and changed outcomes do not by themselves isolate the cause of later match divergence.

Matched FIFO trace SHA-256: Terran `52305dfb2f21a8d3bf29c4e5773cc303bc72cc0c22fa1731fea7131de72c5cf0`; Protoss `894e553ecf4b12c9d905d1eadd5bc76c6d9f6debb8c12490f2cb669c755738aa`; Zerg `ded1da2f1cb6ceeb7a66f488afc34ce8c0d7b79fd6333973412948283f8dffce`. The pinned products, input traces and replay receipts are retained locally; no cartridge assets are committed.
