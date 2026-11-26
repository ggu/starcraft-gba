# Campaign coverage for a 128×128 route cache

Fifteen of the 30 original missions use a 128×128 map in the regular race cartridges. Zerg missions 5 through 9 qualify. The Zerg finale is 96×128. Any performance claim for a cache that requires both dimensions to equal 128 should be scoped accordingly.

Load each earned victory from HOME on its matching `77179474` race ROM in mGBA. Check that the expected mission is selected, the result screen is open, victory is set and defeat is clear. Read `race_cols` and `race_rows` from the matching ELF.

| Chapter | Terran | Zerg | Protoss |
| ---: | --- | --- | --- |
| 1 | 64×64 | 64×64 | 96×96 |
| 2 | 64×64 | 96×96 | 96×64 |
| 3 | 96×96 | 64×96 | 128×96 |
| 4 | 128×128 | 96×96 | 128×128 |
| 5 | 96×96 | 128×128 | 128×128 |
| 6 | 96×96 | 128×128 | 128×128 |
| 7 | 128×96 | 128×128 | 128×128 |
| 8 | 128×128 | 128×128 | 128×128 |
| 9 | 128×96 | 128×128 | 128×128 |
| 10 | 128×128 | 96×128 | 128×128 |

## Zerg holdouts against the wide-cache candidate

The Z10 control built from `758cd54d` is byte-identical to the packaged `77179474` Z10 ROM. The candidate runs the shared-store implementation from [PR #886](https://github.com/ggu/starcraft-gba/pull/886) with the cache enabled on Z10. From HOME, load each listed mid-mission save and advance 256 neutral controller inputs. All 256 advanced the simulation in each case. Values below are mean guest cycles per controller interval, including presentation.

| Mission and save | Start frame | Units / buildings | Control | Candidate |
| --- | ---: | ---: | ---: | ---: |
| Z1 battle | 7,598 | 59 / 19 | 561,073 | 561,075 |
| Z5 gate | 3,220 | 132 / 46 | 922,712 | 922,712 |
| Z6 storm | 91,441 | 105 / 30 | 925,166 | 925,550 |
| Z8 east assault | 66,866 | 83 / 61 | 579,625 | 579,626 |
| Z9 army | 31,301 | 145 / 151 | 1,017,985 | 1,017,985 |

At the initial boundary and every fourth update, compare the world fingerprint, full unit, building and mineral arrays, visible and explored grids, both bitmap pages, palette and display register. All 325 comparisons matched. Each pair also produced byte-identical pause saves, fresh HOME loads and SRAM re-exports. Z1 supplies a 64×64 control. These short windows exercise other missions around the Z7 battle used to evaluate the cache. The largest slowdown here was 0.042% in Z6. Test the full recorded Z7 battle and its victory before deciding whether the extra memory is worthwhile.
