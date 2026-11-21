# Campaign continuation sweep on the attached-depth ROMs

The [attached-depth release](2026-11-21-attached-death-continuity.md)
contains the exact Terran, Zerg and Protoss ROM digests. These replays
use ordinary buttons in mGBA and read-only observations at retained
gameplay and interface checkpoints. Each listed mission starts at
HOME, reloads a pause journal in a fresh instance, wins and reloads
the victory from HOME. The third process retains the result over
eight neutral updates and re-exports the same 32 KiB journal.

## Remaining routes from the previous sweep

| Mission | Pause frame | Victory frame | Route receipt SHA-256 |
| --- | ---: | ---: | --- |
| Z7 | 5,641 | 228,128 | `c3ec9878835898feb8ccca630eba45a66b4c2b3eae8ca454b7d06a9201923286` |
| Z8 | 16,659 | 45,468 | `e5fe79a04d3176fb6e7546dbced4879845d60d8d868564628cc4b11e3798d31b` |
| Z9 | 6,069 | 72,628 | `081c63da54cc9938f2b6d38f959e9635c9fb548086373ff99e9c33cc75938bdd` |
| P7 | 8,845 | 55,892 | `b46788b0eac9c1993318d288bc70fc523edb7f15ba4b5698ceccd55d1e826a1c` |
| P8 | 30,825 | 122,809 | `88f5bbdaffe75696965d546b83dc5aa4282bb05406db4ccf1f09f94c115a2140` |
| P9 | 30,109 | 95,737 | `a155d28dc5486e9afd47ac0c1f39ee523add8177c18dd465685b1e1a2112df08` |

Z7 matches 254 world observations, six route-cache comparisons and
all three journals against the earlier normal-combat cartridge
`d664d71c`. Nine of ten captured display checks match. Both indexed
page digests differ at call 154,198, with matching palette, camera
and displayed page. The comparison leaves the source of this raster
difference open.

Z8 matches all 19 retained host observations. The terminal outcome,
bank, faults and raw actor and resource hashes match the preceding
qualified route. Z9 matches all 32 historical HOME observations
and all 29 post-cut observations on the fresh continuation.

P8 matches 41 sampled checks without frame drift or a gameplay
mismatch. P9 matches all 42 retained checks, including three
buildings still under construction at pause. These are route and
save checks. Native mission parity and physical pacing need
separate measurements.

| Result readback | SHA-256 |
| --- | --- |
| Z7 victory journal | `1acce341f61b2a1e37cadc1e0b304dd63b5b31352789d128fa2d7bf20d6eb627` |
| Z8 terminal comparison | `4ddaa0dc21434116f4192dcb38c8c6d1eda974d7caed3e18cee73e4c6890fef1` |
| Z9 terminal comparison | `b8e9f343e03ba8f94592289ccd528b2b932d257ad562864ec16d7a692b3f4784` |
| P7 victory journal | `dee1bd8a66002d1e0dfde9fd8beabc7c7e99c2ea4cb35f1f3642007b4a05eb09` |
| P8 victory journal | `371cbddfc0b8a88034f7f6be57b5c813c7ca140f8ec25b4eb0cec68a560d640e` |
| P9 victory journal | `5efb8db7b9f4b5f75d1e91ad36550ecd626c7e8df0ea7fb22f6d51978e6b06a2` |

## Exact current refresh for earlier missions

P1 and P2 retain the same victory journal bytes as the preceding
Protoss release. Both routes have the historical actor or economy
checkpoint differences previously documented for the older tapes.

T2, T3, T4, T6, T7 and T10 match the preceding normal Terran gameplay
observations and all three journals byte for byte. The paired
Terran comparison digest is
`611f977d74ebadc2eb7daa796731ebb4674ac56b4bf8db79b58bb5ca7b944d4b`.
The separate T3 and T6 comparison digest is
`39abaf70ad51ca6272efe2c138fcd82510484234df426fb974e0952913caa6f7`.
Z4 and P3 repeat their earlier journal result on the attached-depth
ROMs. P10 matches all 20 retained checkpoints, including the fresh
continuation, and re-exports the same victory journal as before.
The full Z10 route matches all 20 continuation checks, terminal
state and both retained journals on the attached-depth release.
Its third HOME readback digest is
`0866ad93f194d92654cb4068ce45bdef271fc598e94e17056a9b67e2ba0b7b5d`.

| Mission | Pause frame | Victory frame | Route receipt SHA-256 |
| --- | ---: | ---: | --- |
| T2 | 18,204 | 29,366 | `fff4d778e4f36fca597ef77dd0e8af78929b65bb483d5ba0f417bab4244a8d88` |
| T3 | 26,993 | 54,000 | `8463334dcb574017d3434c2961576fd8abea978f7cc9c90c446d3754963af587` |
| T4 | 4,968 | 8,911 | `9a53dcb519f8af6ac34e68375a33ad761d8450af46581ee51f7a2e0bea9e2c84` |
| T6 | 19,466 | 23,536 | `08cd481d485a79b961b41ef79e634eb1cf841c661a81761b039a51a70ff7a3d1` |
| T7 | 2,289 | 3,931 | `2c05125314520bcaeec88c199c809653cbb53319bb6b34a7acb72e482f7d4d8d` |
| T10 | 2,276 | 3,361 | `afb6159e5b74adb8bbbbcf1fb60864459ab134581ae09c41ab206635feb9d83a` |
| Z4 | 9,900 | 34,984 | `2e40e4c2ad4f9be7f4ee29f9d250e344d7b686737df36fb5c783868aa035a322` |
| Z10 | 2,526 | 50,193 | `79bbcee22edd976431544d94fc60d9deabf08d34daff9db7c86a9a16e847ad54` |
| P1 | 448 | 28,270 | `9b96a7cca2ee29c754ce13695780be95f8613ae4bad1fb226f3f893c85558639` |
| P2 | 372 | 33,876 | `f4e891399cb9b14d2b5935bb1cd9b97d0b7d079acb238901710e7b820feea241` |
| P3 | 23,993 | 49,263 | `15a620da7307b821e3e9ce64e780a709a55c3f1b1189ea426e6ae2cfb6ac5932` |
| P10 | 36,761 | 46,129 | `550b16d23ae527b86e119a02607847625361b4679c861bdb050d738dd59abeef` |
