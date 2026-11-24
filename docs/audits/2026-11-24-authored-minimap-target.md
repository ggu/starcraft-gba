# Authored skirmish minimap and target costs

The asset-free crowd fixture runs ordinary update and draw entry
points after setting up 76 units. On `35781d0f`, the renderer calls
signed software division 600 times per update in each of three
eight-update cuts. Those calls cost about 184,200 guest cycles.
The 24×24 minimap sampled each column again for every row.

The change computes its 24 column indices once per draw and keeps
the same center-of-cell sampling. The same instruction cuts record
48 renderer calls to signed division, costing about 14,736 cycles.
The authored target search also evaluates squared distance and the
existing tie break before querying fog. A candidate that cannot
replace the current best skips the fog lookup. The full scan still
keeps the first eligible slot at equal distance, while owner lists
compare slot IDs on ties.

## Paired authored ARM replays

Both 600-update pairs match sampled world state, camera, both
physical Mode 4 pages, palette and display settings on every update.
Cycle means include the normal frame pacing waits.

| Camera | Mean frame cycles before → after | p95 before → after | Updates over two VBlanks before → after |
| --- | ---: | ---: | ---: |
| Panned | 672,914 → 609,143 | 1,123,174 → 842,282 | 196 → 115 |
| Fixed | 631,696 → 580,117 | 1,076,364 → 788,015 | 147 → 72 |

In the panned pair, the mean draw cost drops from 476,353 to
297,605 cycles and the mean simulation cost falls from 142,367
to 128,322. The paired fixture links with 40 fewer bytes of IWRAM
code and the same EWRAM endpoint. The render function's ARM
stack allocation grows from 140 to 228 bytes. The asset-free
Frontier build passes the existing linker memory limits.

All six ordinary Frontier launch and early SRAM continuation pairs
match between the two ARM cartridges. The cases cover both maps
and one to three CPUs. Each test selects the match with normal
buttons, saves from pause after about 120 updates and advances
another 120 after loading in a fresh emulator process. The two
builds agree on the saved SRAM bytes, sampled state, both video
pages and palette.

The minimap change applies to the asset-free Frontier skirmish
renderer. The target change applies to non-race builds that use the
authored search. The race cartridges have their own terrain raster
and indexed target path, which already ranks candidates before
checking eligibility. An observational breakpoint census on the
exact `1f0ad66e` Protoss cartridge counted 172 nearest-enemy entries,
152 indexed queries and zero fallback scans in four late three-CPU
Melee updates on Blood Bath. Those four updates offer no optimization
opportunity in the race fallback. Race cartridge pacing remains an
independent measurement.

| Local receipt | SHA-256 |
| --- | --- |
| 600-update panned comparison | `48092c4c991eebeaa1414b1aac066bfb887fbb9ea45627124dd732343cc33b6c` |
| 600-update fixed comparison | `6e6b7c8266f38851bcd663e493bc07b9eed576f3c08567494101ca0058771d24` |
| Baseline instruction attribution | `2c8db715f76b522f45a1687d4f19db3aebdc29c450bd0cc78d53ba34ab11ac26` |
| Candidate instruction attribution | `2dcd67fead4237bc9ab74306ba49c298448cf5add93e62a53f3a28d2f12e4d3c` |
| Six ordinary Frontier launch and save pairs | `4755b7b9ec12d5b5df049a9fcfe247a26acf29072fdfa63b1bda96d34dc2fe57` |
