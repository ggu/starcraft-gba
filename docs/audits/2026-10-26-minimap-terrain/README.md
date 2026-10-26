# Minimap terrain batching

The imported minimap previously issued a clipped pixel call for each visible
or explored cell in its 24-by-24 raster. The optional `SC64_MINIMAP_BLIT`
path builds the same 576 indexed pixels in bounded stack storage and uses
the existing sprite compositor. Unknown cells are transparent and retain
the caller's backing panel. An all-empty raster skips the compositor.

The helper reads the same visibility/exploration arrays. Actor dots, camera
outline, pings, hidden/burrowed filtering and gameplay updates are unchanged.
The scalar fallback remains available. The feature is selected for canonical
Terran, Zerg and Protoss builds and Showcase; explicit legacy P3/Z3 and staged
profiles retain their existing flag selections.

There are no new persistent buffers, actor/save fields or pool limits. The
measured helper stack frame is 608 bytes; the nested render path is about
944 bytes, within the existing stack reserve. Linker and SRAM guards are
unchanged. The original-art build remains independent of private inputs.

## Pixel and replay evidence

The focused graphics oracle compares actual compositor output with independent
per-pixel reference drawing over poisoned framebuffers. It covers empty,
visible, explored, checkerboard and arbitrary nonzero visibility bytes, plus
scene clipping. The private full-minimap fixture compares 1,800 whole frames,
including actor dots and camera outlines, without skips.

Fresh isolated, no-GDB P4 replay runs use the same source/data, canonical flags
and 1,990-input tape; only the minimap flag differs. Both consume all 104,850
updates and win at frame 104,346 with both heroes alive and in the escape
area, 87 creates, fired bits 8,184 and no runtime fault. Terminal hashes match:
units `283197403`, buildings `3409887890`, P4 `2439291336`.

| Emulator cycles | Scalar baseline | Batched minimap |
| --- | ---: | ---: |
| Weighted full-update mean | 1,003,726 | 987,671 |
| Weighted simulation-update mean | 576,485 | 576,458 |
| Weighted render mean | 426,917 | 410,889 |
| Maximum complete 1,000-update window mean | 1,375,284 | 1,367,700 |
| Maximum individual full frame | 21,347,331 | 21,347,295 |

Each mean weights 104 complete 1,000-update windows and a final 850-update
window. Full cost falls 1.60%, render cost 3.76%, and equivalent full-replay
rate rises from about 16.7 to 17.0 fps. The worst frame is effectively
unchanged. This does not establish 30 fps or physical-GBA performance.

Accepted instrumented replay ROM SHA-256 values are:

- Baseline: `8f7218ca3a3e58325b6e76a01d2df0d1abd7a2edcfae73662cca988ea38d559b`
- Candidate: `7b539f88bce4c8a7e44e6564c3e542513af1a29681492304df1bc3a54695a9c7`

Compiler dependency checks verify the intended tracked/private source paths.
This is a fresh baseline, rather than an assumed reuse of an older ROM with
different layout. Rebuilding the exact replay wrapper with tracked
`starcraft/game.c` and `starcraft/sc64_minimap_terrain.h` produces the candidate
SHA-256 above byte-for-byte; the full ARM evidence therefore qualifies the
public implementation too. Private sources, logs, manifests and ROMs remain under
`.local/build/p4-minimap-blit/final-wrapper/` and are not committed.

## Publication checks

Public-source checks pass:

- `./dev build`, `./dev test`: 493 Python checks plus host fixtures, including
  the new scalar/batched minimap oracle. The helper and graphics implementation
  are both instrumented with ASan/UBSan in the focused fixture.
- `make unified test-unified`.
- `make test-canonical-render race-campaigns showcase test-race-campaigns`:
  canonical frame/HUD/cache checks, all 54 menu launches and 702 startup/save
  configurations, P4 controller win and four production ARM links.
- `./dev sc64 test --profile terran`: all ten actual-win mission controllers
  and applicable full-profile checks.

Production IWRAM/BSS ends remain Terran `0x03004e08/0x0203ddd4`, Zerg
`0x03004e70/0x0203c410`, Protoss `0x03004680/0x0203d490`, and Showcase
`0x030049e8/0x0203dee8`. No reserve assertion was relaxed. All verification
runs locally; Actions remains disabled/manual-only.
