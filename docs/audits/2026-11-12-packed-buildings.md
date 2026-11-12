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

The default stays off pending more workloads and other sprite banks.
The remaining costly static-cache misses are a separate target: count
them first, then try dirtying old and new sprite bounds instead of
invalidating the whole terrain band.
