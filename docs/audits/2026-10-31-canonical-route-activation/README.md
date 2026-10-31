# Canonical route activation, 2026-10-31

`dc4e8d4` enables `SC64_ROUTE_WORD_CLEAR` and `SC64_ROUTE_VISIT_GRID` in
the shared flag vectors for the Terran T1–T10, Zerg Z1–Z6, Protoss P1–P7
and Showcase products. Campaign and skirmish use those same engine vectors.
This changes neither route traversal policy nor simulation cadence, actor
limits, mission selection, save format or AI count. All races and one to
three opponents remain available on the existing skirmish maps.

`CANONICAL_ROUTE_INDEX=0` selects the comparison baseline. Only the values
0 and 1 are accepted. The mode participates in the existing configuration
key so a rapid 1→0→1 switch rebuilds consumers even on macOS Make 3.81;
repeating an unchanged configuration does not rebuild. L_FAST_PATH,
PARTIAL_FLOOD and DECAY_FREQ are not enabled by this change.

## Evidence and limits

The [route investigation](../2026-10-31-route-performance/README.md) records
the same two-option comparison on a custom guest-ARM Zerg replay: loop total
693,086,989→690,269,992 cycles, orders total 167,904,237→164,257,276,
and peak orders 1,181,113→1,011,625. All 90 sampled checkpoints matched.
This is about 0.4% less total loop work and 14% less peak orders work in
that sample, not a general FPS or physical-hardware result. The normal
product layouts below differ from that custom timing probe.

Local qualification at source `dc4e8d4` passed:

- The word-clear, component-visit and 16 policy/split route oracles, plus
  the rapid configuration-toggle fixture.
- A separate checkout with no private inputs: source boundary, `./dev doctor`,
  `./dev build` and `./dev test`. The test run reported 791 Python checks
  with 23 optional skips, eight ancillary checks and the default C checks.
- All four normal ARM builds with unchanged linker reserve guards. Showcase
  also received an independent ROM header, size, SRAM and ELF-payload check.
- Actual four-product host flag vectors under AddressSanitizer and
  UndefinedBehaviorSanitizer: 24 same-process save/reload continuations per
  product, 96 total, covering both map choices, all human races and one to
  three AI slots. These are not fresh-process ARM SRAM continuations.
- `test-sc64-canonical-tracer-profile`, `unified`, `test-unified`,
  `test-terran` and `test-sc64-terran-full-campaign`, all with zero exit.

| Normal product | BSS end | IWRAM end |
| --- | --- | --- |
| Terran | `0x020370b8` | `0x03004b70` |
| Zerg Z6 | `0x02037124` | `0x03004c40` |
| Protoss P7 | `0x020382e4` | `0x03004a18` |
| Showcase | `0x0203df38` | `0x03004e08` |

The 104 prepared-input hashes matched before and after the ARM builds. A
later disappearance of an old private input location broke its symlink;
the source VFX header was recovered by reversing the lossless indexed-lookup
transform and accepting it only after its SHA-256 matched the original
`0e03fcc54e860ab7d7a9904bcacdd47077045754294193ea7759dc9c949d9845`.
It now resides in a durable ignored input directory. A final inventory again
matched all 104 original hashes. The relocation changes the resolved path,
not the input bytes; private recovery and build receipts are retained locally.

Campaign coverage remains T10/Z6/P7, 23 of the planned 30 original missions.
The remaining campaign work, ordinary Reaver/Scarab service, broader frame
times and physical GBA performance remain open. No private input, ROM,
generated asset header or capture is included here. All checks ran locally;
no GitHub Actions were dispatched.
