# Armed type lookup on the Zerg cartridge

The normal Z10 cartridge uses the existing generated weapon capability
bits to check whether a unit type can damage either an air or ground
target. The target scan still checks the live unit, visibility, target
class and damage rules. The asset-free tests compare the combined bits
with both reference weapon queries, and the prepared Z10 game fixture
checks all 50 types and 128 values outside the roster.

The Terran, Protoss and Showcase cartridge files are byte-identical to
release `f11e089`. The Z10 build with
`CANONICAL_ARMED_TYPE_TABLE=0` is also byte-identical to that release.
Both flag variants generate the same 7,375-byte combat fact header.
Z10 retains EWRAM end `0x02039870` and IWRAM end `0x03004f60`.
The instrumented fresh Z8 continuation leaves at least 7,216 bytes of
the observed IWRAM stack canary untouched over 1,200 updates.
Moving the targeting dispatcher into its own source header keeps
the translation unit within the source index limit. All four ROMs
match their respective pre-move candidate builds byte for byte.

## Local replays

The ordinary three-CPU Blood Bath replay matches all 31 world and
display observations, 16 fresh continuation cuts, route counters, and
the exported SRAM. The same Custom Game camera tape samples both
pages at 41 cuts per cartridge. All four Campaign or Showcase openings
match their baseline. The three race cartridges pass the 54 named
Custom Game start and fresh HOME continuation cases, and all 1,134
normal-menu launches. The launch matrix checks both maps, all human
races, and every assignment of one to three CPU seats on each cartridge.

| Replay window | Mean guest cycles | P95 guest cycles |
| --- | ---: | ---: |
| Three-CPU Blood Bath, 3,600 updates | 825,087 → 820,015 | 1,115,670 → 1,109,226 |
| Zerg 10, full ordinary continuation from pause | 1,334,618 → 1,318,488 | — |
| Zerg 7, dense window | 1,853,881 → 1,824,133 | 2,525,941 → 2,497,979 |
| Zerg 7, late peak window | 2,308,831 → 2,279,828 | 3,285,267 → 3,254,419 |
| Zerg 7, late following window | 2,236,521 → 2,209,296 | 3,128,945 → 3,104,763 |

The Zerg 10 replay wins at frame 51,588. All 21 paired continuation
checkpoints and terminal state match. The third process loads the
victory from HOME and re-exports identical SRAM. The source and
candidate both retain the same pause and victory journal bytes.
The full Zerg 7 replay wins at frame 228,128 after 231,031 controller
updates. All 254 world checks, ten display cuts and six route cache
cuts match `f11e089`. The pause and victory SRAM match byte for byte,
including the victory readback from fresh HOME.

| Local evidence | SHA-256 |
| --- | --- |
| Busy Blood Bath comparison | `846dfc7633cf11223d4e84d71e51724ec9c003582049bddd2c42d4385d577645` |
| Custom Game save matrix | `cb64457cf82006f6b63411f76ed059ed822428c422770ac92dedba721f6ed56c` |
| All Custom Game seat assignments | `1f119bfa0d730d7c8acfb9092db9d79009054435e834fa4bdfe9c3ca7c60fd65` |
| Zerg 7 full campaign comparison | `18e3d8defe0c2b474342ff12098cc3a235339dce26472e7cfe1e7a6d218471c3` |
| Zerg 10 candidate continuation | `828c302506383d916aca260f1583fa826d8be36a99f67bd976c4c0a0ae8e5c13` |
| Zerg 10 candidate victory readback | `b92566caa780d6522fa74787e2b6c03402b03d8914ca851ce34b52c1ef7471f6` |
| Z8 stack observation | `a8673adcc6c73a3a1051ae7e0344aeef4148e3f234dc3bb37ac15b470149e484` |

The 30 fps update budget is about 562,000 guest cycles at GBA clock
speed. The measured busy Custom and campaign windows exceed that
budget. Physical frame pacing needs its own measurement.

## Static sprite elevation trial

A separate trial replaced the elevation switch with a byte table and
deferred the flying-type fallback until the raw ID was unknown. The
static oracle matched 2,052 results including both out-of-range integer
extremes. In the 1,248-update P10 replay the map mean fell from 420,679
to 419,674 cycles and the full-update mean was effectively flat.
The Blood Bath mean rose by 124 cycles. Video and state matched in both
replays. Keep this trial local and prioritize the larger route and order
costs.

The local P10 receipt SHA-256 is
`a93af787011189625f8d6361ee1f9b69895f2097cac5510c01d269d1ddb1a98a`.
The Blood Bath comparison SHA-256 is
`87b4fb7fcda034f0334c648d4c33fc006eda338e12cb7077896e5b8e00669664`.
