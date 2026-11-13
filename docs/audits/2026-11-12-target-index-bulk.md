# Batched target index geometry checks

The shared target index rebuild used to validate the same map dimensions
for every actor. The optional owner-list builder validates them once,
then checks each actor's coordinates and links the resulting cell. It
keeps the stock insertion order, dirty-head tracking, invalid-coordinate
fallback and conservative occupancy masks. The standalone builder and
cross-cell movement still use the checked insertion path.

| Normal Z10 campaign window | Mean loop cycles, stock → probe | Mean update cycles, stock → probe |
| --- | ---: | ---: |
| Calls 19,000–19,999 | 1,532,236 → 1,518,653 (−0.89%) | 791,066 → 777,731 (−1.69%) |
| Calls 35,000–35,999 | 1,914,170 → 1,897,618 (−0.86%) | 991,873 → 975,629 (−1.64%) |

Both cartridges use packed walking and the split terrain cache. With the
same 32 KiB SRAM and physical controller input, all 34 gameplay checkpoints
and 24 drawn-page, palette, camera and fog samples match. The baseline
ROM is `b1a0831adbf2d1b15cf8ae669047be43a64547246815f7a0470eda955252fc9f`;
the probe is `e9cad13c53f31c2170abcd7dbff2f856f5c6e8cb3758027d9b42e63d0872c207`.
Both pass the Z10 ROM verifier and original memory guards. The probe adds
344 bytes of linked text, with unchanged EWRAM and 144 bytes less IWRAM.
A separate build of this source with the probe off reproduces the exact
baseline ROM.

Asset-free ASan/UBSan differential checks run stock and probe in the
24×24, 32×24 and 32×32 target grids. The six configurations cover 60,000
randomized builds plus exact dirty-order checks, partial and invalid
geometry, loaded or reassigned units, cell crossings and overflow recovery.

Use a clean build tree for each flag setting:

```sh
python3 -m unittest discover -s tests -p test_sc64_target_owner_index.py
make SC64_DEFENSE_PROBE_FLAGS=-DSC64_TARGET_BULK_REBUILD_PROBE verify-zerg-z10
```

The default remains off. Next measure the other cartridge layouts,
long custom matches and the separate cross-cell move cost. These Z10
battle windows still exceed the 561,792-cycle frame budget.
