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
Before enabling the split, compare it with the current packed-walk release
and verify all four late Zerg ROMs.
