# Wasteland opening: native and GBA runtime observations

A fresh USA N64 reset and a separate GBA physical-input launch reached
Wasteland. The N64 observation uses 7,200 ares runner steps. The GBA
observations use game frames 0 and 1 on an independent clock. The N64
replay used ordinary Start, Down and A input. A read-only ares debugger
observer sampled RDRAM after neutral continuation. Callback images
showed the opening Terran group and 10/18 supply. An earlier sample
showed 40 minerals.

For the GBA, normal button edges A, release, A, release, Start launched the
first Terran campaign mission from a fresh title. It opens campaign
gameplay at catalog index 4. The stock ARM ROM ran through the
physical-video input backend. ELF DWARF supplies unit and building
field offsets and widths. At frames 0 and 1 the active human force
has five Marines and five SCVs. Native big-endian actor records
report the same types, counts, HP and raw owners. All 13 positions
match after floor division of each native world coordinate by two.
The comparison includes those ten units, Raynor, the Command Center
and the Supply Depot.

GBA raw owner 1 maps to internal player side 0. Its supply state is
20/36 half-units, corresponding to the native 10/18 HUD. GBA minerals
are zero at frames 0 and 1. The first 40 appear at game frame 30
(physical call 35) during neutral input. The independently timed N64
capture leaves grant cadence open. The placement comparison covers
the 13 measured actors.

The native actor decoder located a unique 900-record allocation marker,
checked 312-byte record bounds and sprite-pool alignment, and read fields in
big-endian order. The GBA backend used read-only memory access and ordinary
buttons. The isolated observer printed an inherited `STATE_RESTORED_AND_READ`
label. This observation comes from a reset and ordinary menu input.
Native and GBA ROMs, RAM captures, save data and images remain local.

## Goals panel and opening bank

A second fresh ares session reached Wasteland through the native menu.
After an ordinary pause, the Goals panel listed finding Raynor,
building a Barracks and training additional Marines.

After the tutorial was dismissed through ordinary controls, the
native HUD showed 40 minerals and 10/18 supply. On the prepared
`43c5d98` Terran cartridge, a separate fresh launch again reads
zero minerals at frame 29 and 40 at frame 30. The native capture is
asynchronous and the independently run clocks do not establish the
grant cadence. The native goals image, native HUD image and current
ARM grant observer have the following local SHA-256 values.

- Goals image `5b42fd23fa8a30ef57b67f409f36da17b79b68774cb57e47836009cef7a7752c`
- HUD image `5e30f639c52d6cc0ae0dbbd4084bee13096bc68685459943e92ceacd5396e7bb`
- ARM grant observation `82e81e93052d02d168c0ced9f926939d43d17c6fb754c0cb71e76c1d8f57560e`

## Victory condition and initial group selection

The locally authenticated native scenario's victory record has three
Command conditions for player group 1. The native comparison helper
interprets comparison code zero as “at least.” The required counts
are ten Marines (raw 0), one Barracks (raw 111) and one Raynor (raw 19).
The native AtLeast count requires completed actors. At `c05ac442`, the
GBA counts a living player Barracks from foundation onward. The native
zero-actor defeat condition includes owned buildings. The GBA counts
only living mobile units in that check.

The separate native resource defeat condition uses the inclusive limits
zero Command Centers, zero Barracks, nine Marines and 399 minerals.
Incomplete structures count toward the first two limits. A further
native trigger checks for zero living Raynors across all players and
runs waits around a notification before defeat. The GBA resolves defeat
immediately. Capture ordinary native victory, a completed-building
boundary and the Raynor-death sequence for timing and presentation.

An ordinary R input at the fresh Wasteland opening selects the five
Marines in ares and in the Terran ARM replay. The five SCVs are excluded
in both. Use the same reset and mission launch for a harness case and
record the selected raw types and counts before and after the button
edge. The [earlier native actor audit](2026-11-07-native-actor-differential/README.md)
records a C-Right Move order changing five Marine orders. Use that
input and sample later unit positions for the movement cut.

| Local evidence | SHA-256 |
| --- | --- |
| Authenticated native scenario | `b03fcbd0d18c2165be82aaab841f54f49f08ae74191cd039a36885059fa81517` |
| Native victory record | `273129e1fcb56039d21fa5aa2101fa10fb477707ca7a62efe3e1a63723151aba` |
| Native R screenshot | `62614486e311d263dfb2ffde814d716894c21cba47f81a09a767201c24f2d9ba` |
| Native R selected-state observation | `791d796da0e254fabcf71ed20c5905fad55143e5b6c0615883df479a7e99e205` |
| Terran ARM R group observation | `e29102b56271d12110df7b24b2393d84443bef858c9c14326cc0e398e3921921` |

## Initial evidence

| Local evidence | SHA-256 |
| --- | --- |
| USA N64 ROM | `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020` |
| N64 observer source | `3952681eb40ec87687b12657cfe088f3a0b9458220d22cf26097dea515c978e0` |
| N64 observer binary | `92ca4f34e0886620d45cf8be310bfd487ac68a2b2cd9497d09f0380324f30bba` |
| N64 input command CSV | `3d9256471387180b6c3d805f7f0084d1752f4d84be7343c26c9056dcacce925f` |
| N64 final RDRAM | `3a187c5ceb6fe30f84a68b821c186769167bff8d6e2bce2560db91573f471ea5` |
| GBA ROM | `dcbaf755cc7d66dbf4304b212cd81ad1ce237945d030a2bbc050ca81c62856a4` |
| GBA ELF | `7a84ef56fec1332e5d62fe3461492a6513ee9e36ee68a4f366132997c98937ee` |
| GBA physical-video backend | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| GBA unit observer | `5ec00a19eaa78947971ed4008f8b331c2377dd587ac9315ecaf559ba56db24b9` |
| GBA building observer | `8ded4b66202c204e2fa3df1da3bf1581171b0b7483103693e6bb5648def2ffe7` |
| GBA neutral grant observer | `f8affd24fba52ff6e34804caaccd7eda9733ff6195fe87a0caa10620ed7a63a2` |
