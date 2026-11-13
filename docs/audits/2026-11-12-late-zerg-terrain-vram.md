# Split terrain cache in late Zerg cartridges

The late Zerg build retains a 25,920-byte terrain band in EWRAM. The probe
uses the existing split cache: 67 rows in the Mode-4 OBJ VRAM arena and
the remaining rows in EWRAM. The first 256 bytes in the OBJ arena remain
reserved for soft fog. No terrain, fog or sprite data is replaced.

| Normal Z10 window | Mean loop cycles, stock → split | p95, stock → split |
| --- | ---: | ---: |
| Calls 19,000–19,999 | 1,568,109 → 1,544,764 (−1.49%) | 1,965,123 → 1,952,941 |
| Calls 35,000–35,999 | 1,995,868 → 1,970,108 (−1.29%) | 2,375,134 → 2,352,400 |

The two ROMs play the same physical input from the same SRAM save and match
all 34 sampled gameplay checkpoints. All 24 sampled drawn pages, palettes,
camera positions, visibility and exploration also match. The terrain
storage and soft-fog host fixtures pass. Z10 EWRAM end moves from
`0x0203c9b8` to `0x02038ae8`, freeing 16,080 bytes; IWRAM is unchanged.

The standalone split probe was measured before packed walk frames were
enabled. Its ROM SHA-256 is
`5eaa340c4ad9c603aabd850a47d4360a6cf3c189d4a8a2e5a1af48390d1c7d7b`.
## Late Zerg release check

The split adds `SC64_TERRAIN_CACHE_VRAM` to the Z7 build flags inherited by
Z8, Z9 and Z10. All four cartridge verifiers pass, including their original
linker guards. It does not change the Terran, Protoss or Showcase flags;
those already select their own cache layouts.

| Normal Z10 window | Mean loop cycles, packed walk → packed walk + split | p95, packed walk → packed walk + split |
| --- | ---: | ---: |
| Calls 19,000–19,999 | 1,555,399 → 1,532,252 (−1.49%) | 1,960,472 → 1,947,402 |
| Calls 35,000–35,999 | 1,939,948 → 1,914,154 (−1.33%) | 2,309,588 → 2,282,985 |

This pair also matches 34 gameplay checkpoints and 24 samples of the
drawn page, palette, camera and both fog arrays. The packed-walk baseline
is `d7c219637dbebe8223001a35beb717678cc008ec370f2062ace095196b7aff67`.
The packed-walk and split ROM is
`5a422fe34f19d67cb70d00b0286b33393fb1ad26a9530e36a62b7a73e7c73095`.

After merging the opt-in defense source from #399, the default-off
cartridge was rebuilt and replayed from the same save. The new exact ROM
SHA-256 is `b1a0831adbf2d1b15cf8ae669047be43a64547246815f7a0470eda955252fc9f`.
Its 34 checkpoints and 24 video/fog samples still match the packed-walk
baseline. Mean loop cycles are 1,532,236 and 1,914,170 (p95 1,945,889 and
2,285,229). These windows still exceed the 561,792-cycle budget; this
change is useful for RAM headroom and does not establish 30 fps.
