# Target-index row occupancy audit

Runtime `b638b508512396fbfdf2b5569a7a494331473b6a` adds a per-row occupancy check to the indexed nearest-target search, on shared parent `bb55a1706631bd1a1281c77c595194c65c27aab2` (runtime `d570c6d8437d6f89cbbda120e960cd77384445ef`). Successful insertions set the owner's row bit. Removal and movement can leave stale set bits, which cost work but cannot hide an indexed target; insertion failure disables the index and leaves the existing scan fallback. The search retains its owner, row, column and entity order and tie rules. The existing grid limit is 32 rows.

The reference query in `tests/sc64_target_nearest_reference.h` was checked against the parent implementation and differs only in its function name. With prepared inputs, `make test-target-row-query` ran the Terran, Zerg, Protoss P7 and Showcase vectors under ASan/UBSan. Each vector reported 4,000 real-engine comparisons across 500 frames and 24,000 comparisons across 250 synthetic worlds, plus 500 invalid-index fallback checks. The fixture exercises row 31, ties, movement, deaths, owner changes and transported units. These are bounded tests, not proof of every game state.

On the same ordinary-menu Zerg-versus-three-AI replay, guest cycles for calls 8,021–9,020 changed from **715,369,996 to 714,202,651** (−1,167,345; −0.16318%). Orders changed from 169,104,540 to 168,031,600 cycles and map rendering from 277,242,545 to 277,361,639. All 90 sampled actor/state, fog and displayed RGB555 hashes matched over the complete 9,020-call run; 10 samples were in the timed window. Probe, tape and ordered compiler arguments matched, and 104 prepared input hashes were unchanged. This is one emulator workload, not a hardware or general frame-rate claim. The parsed private receipt is `.local/row-mask-perf-v1/paired-result.json`.

Qualification at `b638b50` was local and used no Actions:

- A clean asset-free source boundary check, `./dev doctor`, `./dev build`, and `./dev test` exited zero. The latter logged 776 Python tests with 23 skips, the separate eight-test suite, and the C fixtures.
- `make unified test-unified` passed with prepared prerequisites held fixed; its ROM SHA-256 was `4c3380df02dc2083f5e4a99cd779e7ca429483a41115e54bd7c6c82c747f9ff7`.
- Four normal Make cartridge products passed their unchanged linker guards, header, ELF-payload, padding and audio checks. IWRAM ends were Terran `03004d90`, Zerg `03004e60`, P7 `03004c40`, and Showcase `03004fc0`; BSS ends were respectively `020370b8`, `02037124`, `020382e4`, and `0203df38`. The 104 prepared inputs remained unchanged. The private product and payload receipts are `.local/row-full-qualification-v1/products.json` and `payload-validation.json`.
- The sequential full Terran host qualification and four-profile match/resume matrix passed; nine retained Terran traces and 18 match traces per profile matched the parent runtime. See `.local/row-full-qualification-v1/receipt.json` for scope and counts.
- The existing non-BC Z8 candidate vector passed host and ARM checks with 110 input hashes unchanged. This is not a Z8 winning replay or promotion claim; see `.local/row-z8-qualification-v1/receipt.json`.

No ROM-derived assets or private captures are included in this record. This change does not establish full campaign completion or near-30-fps performance.
