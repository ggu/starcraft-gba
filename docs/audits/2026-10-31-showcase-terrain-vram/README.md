# Showcase terrain cache storage

Showcase now selects the existing `SC64_TERRAIN_CACHE_VRAM` storage split
already used by the normal Terran, Zerg and Protoss products. Its terrain
pixels, cache dimensions, simulation, actor limits and save format are
unchanged. `SHOWCASE_TERRAIN_CACHE_VRAM=0` selects the RAM-only comparison.
Only the Showcase compiler vector gains the storage define; the other three
normal product vectors remain identical.

The existing Showcase HUD/contact settings live with the new validated
storage setting in `config/showcase-cache/Makefile`. The managed profile
configuration key records the setting and the included module's content
hash, so rapid toggles or module changes invalidate consumers. Synthetic
Make checks cover toggle/rebuild/no-op behavior without modifying the linked
source module. Splitting the module also preserves the existing 512 KiB
source-file boundary; the boundary limit was not increased.

## Normal ARM products

Both actual normal Make Showcase products were built at source
`7fc4de22a14bc91816076cf20be4e54a5555b059`, with their ordinary `-O2 -g`
compiler flags. Independent checks verified the ROM header, SRAM signature,
ELF-derived payload and 32 MiB FF padding. All 109 frozen prepared input
hashes remained unchanged, including the separate current rectangle facts.

| Storage | BSS end | IWRAM end | Raw payload bytes | ROM SHA-256 |
| --- | --- | --- | ---: | --- |
| RAM only | `0203afa8` | `03004c78` | 28,966,916 | `75e5e0dcd3bd80765559804feac357da18e7d694bb85745e7ffd9033adaba07a` |
| Split VRAM | `02036fe8` | `03004c78` | 28,966,900 | `cff3ef0a25f537b6422f107658efe626bbd4f85e9817a7f9051bc3dd16c11e53` |

The split frees exactly 16,320 working-RAM bytes (68 rows of 240 bytes) in
this actual Showcase vector. Its OBJ scratch cache occupies 16,320 of the
16,384 reserved bytes, leaving 64 bytes outside the cache. This vector does
not compile the P7 projectile arena. A future arena-enabled vector has a
different split and needs its own link and runtime qualification; the
68-row memory result must not be assigned to it.

## Renderer and persistence checks

`make test-terrain-storage` runs the actual asset-free host terrain renderer
with ASan/UBSan in RAM-only, 68-row VRAM, and 48-row VRAM-with-arena-reservation
layouts. Each compares scrolling/visibility rendering against a clean
reference. VRAM variants poison the whole scratch region, require the cache
to change its own bytes, and verify every prefix/tail byte outside its range
remains intact. The reservation variant checks ownership, not a production
projectile lifecycle. All three passed, with `-g0` as their sole debug flag.

The actual Showcase product-vector prepared renderer fixture passed 512
full-scene equality cases, with 224 partial cache reuses, including camera
phase/edge changes, fog, construction and mineral depletion. It used host
ASan/UBSan with `-g0`; it is not a hardware renderer test.

The normal split-VRAM ROM passed a fresh-process custom-game save check:
map 1, Protoss human, mixed Terran/Zerg/Protoss opponents, 9,000 passive
updates before pause-menu save and 900 updates of continuation. The restored
persisted entity/economy/queue fingerprint matched the saved state and its
fresh continuation matched the uninterrupted reference. The 32 KiB SRAM
was unchanged by loading. This fingerprint does not include every transient
path/UI/audio/cache field.

## Same-save guest comparisons

Each pair uses the same retained map-1 Protoss-human save and the same
port-free mGBA backend. Only scheduled controller inputs are written.
Across 901 active frames per pair, full captured Unit, Building, mineral,
visibility/exploration, bank and generic-service clock/cache-due bytes match.
Each pair also matches nine full displayed-frame/palette hashes. This is
explicit captured-state coverage, not equality of every engine global.

The following are mean guest cycles over 900 updates, RAM-only to split VRAM:

| AI opponents | Draw | Update | Full loop |
| ---: | ---: | ---: | ---: |
| 1 | 249,724 → 233,424 | 340,092 → 340,073 | 646,266 → 617,222 |
| 2 | 273,452 → 257,864 | 463,753 → 463,753 | 849,685 → 840,012 |
| 3 | 280,734 → 265,227 | 386,038 → 386,036 | 778,524 → 745,441 |

Full-loop p95/p99/max was 824,507/842,568/1,160,246 to
807,260/842,342/1,144,669 (one AI), 856,216/1,123,341/1,404,347 to
842,997/1,123,466/1,404,556 (two), and 842,738/1,154,110/3,370,163 to
842,541/1,172,375/3,154,499 (three). Thus some full-loop tail values increase
despite lower draw means. The full loop includes presentation waiting;
these bounded emulator workloads do not establish universal improvement,
steady near-30-fps operation, hardware performance or campaign completion.

## Source checks

A separate clean, unprepared worktree at `7fc4de2` passed source boundary,
`./dev doctor`, `./dev build` and the complete `./dev test`: 886 Python
cases with 23 optional skips, five and eight ancillary cases, and all native
fixtures. Its checks ran serially under the shared build lock. Ten focused
Make lookup/profile tests separately passed. The final source-boundary check
with this audit indexed 1,401 text files. An independent source review found
no concrete blocker in the scoped cache change. None of these checks
qualifies the separate experimental shared Carrier/Defiler backend.

The original input, source/ROM/runner hashes, literal build logs, maps,
payloads, every captured-state digest, all cycle samples and fresh-process
save receipts are retained in ignored `.local/qualification/` of worktree
`showcase-vram-cache-20261031`. No game assets, ROMs, saves or captures are
committed, and no GitHub Actions or SD writes were performed.
