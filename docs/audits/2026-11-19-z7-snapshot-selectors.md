# Source-slot selectors in the Z7 count snapshot

The dense Z7 replay spends roughly 121,000 guest cycles between the
masked count snapshot and the campaign scheduler. The active mask
`0x1b` needs two Command counts and two geographic Bring counts.
Command counts drive the ordinary loss and win conditions. Bring
tests Force 1 actor rectangles against source locations 3 and 5.
[Shipping cycle attribution](2026-11-19-contact-bucket-boundaries.md#remaining-cycle-budget)

`SC64_Z7_CENSUS_OWNER_TABLE` fills a 12-byte table of predicate bits
from the same owner resolver once per snapshot. Each occupied actor row
then looks up its source slot. An inactive or out-of-range slot still
fails when a live row uses it. Extended active slots can match the
current-player selector. Raw type validation still includes force
members when validation would have run in the previous census.

Run the prepared host fixture with
`make CLANG='clang -D_FORTIFY_SOURCE=0' test-sc64-z7-census-owners`.
Both reference and owner-table variants pass all 32 masks against
the world counts and check extended owners, inactive and invalid slots,
fault clearing, and building-only criteria with malformed force members.

The owner-table-only Z10 ARM build keeps the shipping IWRAM and EWRAM
endpoints. From blank HOME, it matches 131 historical gameplay
checkpoints, every frame and route entry in the 480-call window, and
nine raw actor, resource, and display cuts.

| Dense Z7 guest cycles | Shipping | Owner table |
| --- | ---: | ---: |
| Mean snapshot to scheduler | 120,933 | 91,166 |
| Mean advancing loop | 2,481,146 | 2,451,104 |
| Whole-window p95 | 3,628,357 | 3,584,397 |
| Whole-window peak | 4,841,218 | 4,801,699 |

Mean snapshot cost falls 24.61%. The full loop improves 1.21%.

## Rectangle cache comparison

A separate probe caches raw type and rectangle extents in eight
stack entries, keyed by raw ID and actor kind. Its host fixtures
check key replacement, source building identity, half-open edges,
and distant invalid actors. Two ARM variants carry the cache on
its own and alongside the owner table. Both match the same 131
checkpoints, 480 frame and route records, and nine raw world and
display samples. The existing IWRAM and EWRAM endpoints still pass.

| Dense Z7 guest cycles | Rectangle cache | Owner table plus cache |
| --- | ---: | ---: |
| Mean snapshot to scheduler | 114,895 | 95,954 |
| Mean advancing loop | 2,475,322 | 2,456,407 |
| Whole-window p95 | 3,622,998 | 3,596,314 |
| Whole-window peak | 4,833,760 | 4,806,764 |

The rectangle cache alone saves 0.23% mean. The combination saves
1.00% against shipping, while the owner table alone saves 1.21%.
The cache uses 224 bytes of stack and remains an experiment.
Its source is isolated at `c5cfdd79` on the local geometry branch.

Retained N64 evidence verifies force-selector expansion. The GBA
fixtures verify strict rectangle overlap in source pixels. In Z7,
four triggers can set the same switch, so that switch alone cannot
prove a native Bring edge. A native condition result or trigger
retirement at a box-only crossing would isolate it. [Native selector observations](../REFERENCES.md#optional-starcraft-64-missions)

- Owner-table replay SHA-256 `4d861c3646178bec9a114fc629a94b390826205ef3bab4d50813adf4b591fd8b`.
- Owner-table entry intervals SHA-256 `b17e1992c9e3e6c8680fd27ab58851459a6599e4029337fc8ba2d0cea0e8a678`.
- Rectangle-only replay SHA-256 `cc0c88fea734052b1f6c87dad6241935ce2ad87b6a4a527957c9b001eb93083b`.
- Owner and rectangle replay SHA-256 `3283ec4647024cb26df9aa5e0dc2600a449d63eb046c8a56d38a07945e1fee06`.

## Reuse live validation

The owner table also composes with `SC64_Z7_VALIDATED_CENSUS` and
`SC64_Z7_FUSED_PROVENANCE_PROBE`. The first option reuses checked
raw-to-live type results. The second combines product provenance
and validation in one scan. On an invalid row it completes the same
remaining provenance writes and leaves counts unpublished. The
composed prepared sanitizer fixture passes these fault cases and all
32 masks.

| Dense Z7 guest cycles | Shipping | Combined validation and owner table |
| --- | ---: | ---: |
| Mean reconcile and validate entry interval | 35,662 | 34,232 |
| Mean snapshot to scheduler | 120,933 | 78,828 |
| Mean advancing loop | 2,481,146 | 2,436,342 |
| Whole-window p95 | 3,628,357 | 3,583,499 |
| Whole-window peak | 4,841,218 | 4,788,680 |

The composed Z10 build matches the same gameplay, route and display
samples and fits the shipping RAM endpoints. The full-loop mean falls
1.81%, giving 6.89 emulated gameplay updates per second in this window.
The measured savings come mainly from the census rather than the
combined reconciliation scan.

With the fused scan disabled, the owner table plus validated census
averages 2,443,371 cycles. The independently measured snapshot is
79,203 cycles. Fusing reconciliation with validation saves a further
7,029 mean full-loop cycles in this comparison.

`make zerg-z10 Z10_Z7_CENSUS=1` selects these options for the normal
Z10 recipe. It defaults to 0, and the config stamp tracks either
direction. The recipe compiles the same ROM bytes as the measured
candidate. The Terran, Protoss and Showcase profiles built with all
three flags are identical to their shipping ROMs.

Run `make CLANG='clang -D_FORTIFY_SOURCE=0' test-sc64-z7-census-composed`
for the reference, owner-table and composed host fixtures.

- Composed replay SHA-256 `c786ed1426f9c05d019bc1150879e8dab340c115e07b9c3fd3c65c7147c44ab3`.
- Validated owner table without the fused scan SHA-256 `3d9bad8d7b1820b8296967c0f46fad12440c98bc9e3c019fd33c4cc0dd159a68`.
- Other-profile byte comparisons SHA-256 `65ae68ca6b2fea927c378484da29cb587f33f07fce0dc79b6b4a223d32d2528b`.

## Compose with the route work

The validated census also runs with the four routing probes from
draft #576. The regular Z10 recipe retains the combined route build's
IWRAM end `0x03004e18` and EWRAM end `0x02038d20`. The physical
replay matches all 131 historical checks and 480 route and game frames.
The nine raw world, palette and two-page display cuts still match.

| Dense Z7 guest cycles | Shipping | Routing and census |
| --- | ---: | ---: |
| Mean advancing loop | 2,481,146 | 2,176,394 |
| Whole-window p95 | 3,628,357 | 2,873,070 |
| Whole-window peak | 4,841,218 | 4,388,417 |

The mean falls 12.28% from shipping and 2.04% from the route-only
candidate. The combined window runs at 7.71 emulated gameplay updates
per second. [Routing-only comparison](https://github.com/ggu/starcraft-gba/pull/576)

- Joined routing and census replay SHA-256 `6b9a036e774dd747cc8f58ad566b2f5cb55314da818988e740b653565558e5a9`.
- Joined entry intervals SHA-256 `e9eef535bc3e8a8ded3fa59f149898374f702a587429387f9818435935ef8f7e`.

## Finish Z7 from a shipping-ROM save

The exact shipping Z10 ROM matches 248 historical checkpoints from
HOME to controller call 188,324 and game frame 185,926. Ordinary
Start and B produce a 32 KiB SRAM with the same bytes as the retained
November 13 journal. The replay verifies the marker, sequence, payload,
checksum and frame. [Original save and reselection procedure](2026-11-12-z7-normal-physical.md#fresh-sram-on-the-november-13-z10-kodak-cartridge)

Fresh shipping, census, and routing-plus-census processes load that
same journal through the title screen with L. The first resumed
gameplay update uses A to reselect the Hatchery as in the original
procedure. All three match the five remaining historical checkpoints,
the six raw world, palette and display samples including load, and
win at frame 194,998 with zero trigger faults. The SRAM input stays
byte-identical across the three loads.

| Fresh-load continuation | Mean guest cycles | Emulated updates/s |
| --- | ---: | ---: |
| Shipping | 685,736 | 24.47 |
| Census | 674,040 | 24.89 |
| Routing and census | 673,506 | 24.91 |

The prefix and continuation establish one complete shipping route
with a fresh-process save. The candidate evidence covers the busy
HOME prefix and the late continuation from the shipping journal.

- Shipping save export SHA-256 `32e4fc474ce300a2af68ca94da1f198bc2ca3a5fddd9555dac03e6a180603808`.
- SRAM SHA-256 `632a64001689bbbc35ae7a2ca4cea20b93c94a1187c5ee7cdb95ffe88b05662f`.
- Shipping, census and joined raw continuation SHA-256 values are `1ae0ee66b32b2e2791b35fe67d65a7f798b0b4b523c093e95eb205a944138ea9`, `6f738a69b43caf8417a6b3b8c5c373707320c6912b2ba9eff1ef55333899e49f`, and `701d95931687849e4ffe38ef870473c4c19dfe0d7d847d577e5c50b1780127bf`.

The native pilot found an erased desktop Flash save and a single
fresh-menu seed shared by the archived Flash copies. The original
ROM and save were read-only inputs. Validate the box-only Bring
edge from an ordinarily progressed N64 Z7 session.
