# Packed completed-building sprites in Z10

The optional Z10 renderer packs the existing frames of eleven common
completed structures plus four Photon Cannon poses into the word blitter's
four alignment phases. It matches by the selected source frame pointer;
other buildings, construction, fire and damage use their existing paths.
The 1,800 prepared pixel comparisons cover all fifteen packed frames, all
four alignments, and clipped edges. A real unpacked building exercises the
ordinary byte blitter fallback.

| Normal Z10 window | Mean loop cycles, stock → packed | p95, stock → packed |
| --- | ---: | ---: |
| Calls 19,000–19,999 | 1,532,236 → 1,512,980 (−1.26%) | 1,945,889 → 1,918,143 |
| Calls 35,000–35,999 | 1,914,170 → 1,882,849 (−1.64%) | 2,285,229 → 2,238,625 |

This pair already includes packed walking and the split terrain cache.
It matches all 34 gameplay checkpoints and all 24 drawn page, palette,
camera and fog samples from the same physical SRAM replay. Before the
terrain split the probe also passed the paired replay, with a roughly
235 KiB increase in linked ROM text and unchanged RAM. The composed ROM
passes the current Z10 verifier. Its SHA-256 is
`cfe88d5585816ac775d630136cda7bd0fb0e2e994b5b5ab817d30e054206ec11`.
Switching the probe off reproduces the normal split-cache ROM
`b1a0831adbf2d1b15cf8ae669047be43a64547246815f7a0470eda955252fc9f`.

With prepared inputs:

```sh
make test-sc64-z10-building-words-pixels
make Z10_BUILDING_WORDS=1 verify-zerg-z10
```

## Current normal cartridge

After packed sight, the single terrain scan, defense indexing and
nearest-first depot selection, the normal candidate matches all 61 raw
gameplay checkpoints, 24 sampled drawn pages/palettes/camera/fog states,
and the same zero-fault win at frame 59,997 from physical SRAM. EWRAM
and IWRAM endpoints remain `0x02038af8` and `0x03004be0`. Linked text
adds approximately 235 KiB inside the existing 32 MiB cartridge size.

| 1,000-call window | Current control mean | Packed mean | Change |
| --- | ---: | ---: | ---: |
| Z10 19,000–19,999 | 1,539,918 | 1,514,736 | −1.64% |
| Z10 35,000–35,999 | 1,561,129 | 1,526,879 | −2.19% |
| Outpost, Terran vs. three CPUs | 1,322,351 | 1,322,320 | −0.00% |
| Blood Bath, Protoss vs. three CPUs | 1,190,546 | 1,187,739 | −0.24% |

Both ordinary-input custom matches matched every raw checkpoint through
the same result (76 on Outpost and 77 on Blood Bath). Enable for the
qualified Z10 bank. Other banks have no generated data from this trial.

A separate generic sprite experiment reused the already loaded source
word for mixed-transparency groups. It matched all 61 state checkpoints
and 24 video samples but saved only 0.09–0.13% mean loop cycles and used
120 more IWRAM bytes, so it remains out of the release. Neither change
solves the point-route spikes or the hardware 30 fps budget.
