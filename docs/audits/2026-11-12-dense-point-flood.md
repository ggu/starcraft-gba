# Dense point-route flood experiment

This probe replaces the packed visited-grid updates during complete point
floods on 64×64 maps. It expands walkability into a padded byte field, visits
each cell once, and packs the directions back into the same cache entry.
The seeds and left/right/up/down traversal order are unchanged. The byte field
uses a provably disjoint part of the existing route scratch. It leaves the
retained topology snapshot, cache policy, save format, other map shapes and
depot/perimeter/ranged floods alone.

`make test-sc64-route-dense-flood` compares against the existing sanitized
route-cache oracle on five map shapes. All 321,445 queries agreed, including
cache residency; 21,704 resident fields were also checked against a fresh
flood. The probe remains off in normal products.

## Paired ARM results

The control is #386's `6ba59b1` Z9 ROM; the candidate differs by the dense
probe. Both use the same normal-menu Zerg tape and libmGBA runner. The pinned
1,000-update custom fight spans ticks 10,860–11,859. The sampled counts and
frame/screen state match; these are not exhaustive state hashes.

| Cycles | Control | Dense probe | Change |
| --- | ---: | ---: | ---: |
| Mean loop | 1,416,948 | 1,369,965 | −3.32% |
| p95 loop | 1,916,907 | 1,726,539 | −9.93% |
| Peak loop | 3,834,506 | 3,322,366 | −13.36% |
| Mean simulation update | 836,560 | 789,630 | −5.61% |

The ROM and reserve guards pass. EWRAM end stays at `0x0203c9b8`; IWRAM end
moves from `0x03004850` to `0x03004ae0` (+656 bytes), leaving 1,312 bytes
before the `0x03005000` guard. The mean loop is still about 2.44× the
561,792-cycle budget for 30 fps.

Control ROM SHA-256:
`939e7d09ee2da3a8c81c72ada84a5545d04c629959b8072ce11bf65e7fc69ad7`.
Probe ROM SHA-256:
`a8949ca409dd8e9a8865f0dc0e294f409b44f256e6090fb3882e25bba1076553`.
Tape SHA-256:
`13019d6073bbf365cae1833f6e9e161ba96d61f660e38f3bc948254073e792d2`.

The same paired comparison passed on the Terran and Protoss normal profiles,
and on an earlier Z9 skirmish window. All have zero sampled workload
mismatches. All three normal links pass, with unchanged EWRAM ends and
656–672 additional bytes of IWRAM.

| Product and tape window | Mean loop cycles | p95 change | Peak change |
| --- | ---: | ---: | ---: |
| Terran custom, 10,887–11,886 | 1,153,411 → 1,126,888 (−2.30%) | −10.79% | −22.27% |
| Z9 custom, 10,860–11,859 | 1,416,948 → 1,369,965 (−3.32%) | −9.93% | −13.36% |
| P10 custom, 10,888–11,887 | 1,524,331 → 1,497,523 (−1.76%) | −4.84% | −16.59% |
| Z9 early custom, 2,000–2,999 | 606,745 → 606,459 (−0.05%) | +0.03% | −14.94% |

Before enabling the probe, verify the remaining products and campaign
windows, including 64×64 campaigns, exact longer gameplay and save
continuations, and hardware timing. The quiet test is one early custom
window, not a guarantee across all maps and missions.
