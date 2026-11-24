# Native fog memory probe

A fresh ordinary-input Protoss Melee/Challenger run reached the starting base and selected a Probe. The UI showed Move on C-Right; the existing callback frontend mapped D-pad Right but not C-Right. Pressing its D-pad Right produced `MAP LOCATION NOT SELECTED` rather than moving the Probe. No resource cluster was observed after vision left: the starting minerals remained in Nexus sight. The process returned 0 at 4295 callback steps in 87.314 seconds with three frames, 1,843,200 image bytes and checked inputs unchanged. Its binary SHA-256 was `eb8d30c00dee5176188cdc91367c219c9c3aa73d0079cd6a0273bd3ba8490aa5`; selected-Probe frame SHA-256 `33ddb6451465a543f7fe1a263488f56aa3c70682f29463cb3fec89bc0321297e`; D-pad result frame `7538a04f3cba0e79b9d23e466805683df737aff0994533d18518ebaf9e56e6d3`; command, input and callback SHA-256 values `c243e9972736c3000e25a159b2de8d67a53b2b19bc091689e77b0c10e9bcce13`, `7986e36f58348cb65075a6698601446a42cab2b7ea9de6f73f7745c3633c387c`, `73fb842baa9e266d164e1e81186a03f1ba5522ab73f45a52eb95f28ae09d422e`. Private logs, ROM copy, saves and frames remain ignored. Callback steps are not game-frame timing; no audio was retained.

The pinned ares controller at `ares/n64/controller/gamepad/gamepad.cpp` separately names `Right`, `C-Right`, `C-Up`, and `C-Down`. The experimental frontend now resolves and logs these distinct callback nodes and bounds the expanded command mask. `tests/sc64-ares-input/mapping.cpp` statically checks independent D-pad/C direction mappings; it passed as part of a separately locked source build using existing pinned upstream libraries, whose hashes were unchanged. This frontend supports genuine emulated controller input only; no guest memory, save-state or actor changes are involved. A further fresh bounded run is required to observe the requested fog-memory sequence.

A single further fresh run used the extended frontend and the same setup. The callback input log recorded C-Right input ID 256 pressed; following selection and that input, a Probe appeared near the targeted base-edge area. This establishes callback delivery and is consistent with movement, but the samples do not establish a remote resource cluster or an explore→leave→revisit sequence. The session ended while the next movement input was being prepared, before further commands. It returned 0 at 5967 observed steps in 98.597 seconds, with four frames totaling 2,457,600 bytes and checked inputs unchanged. Frontend binary SHA-256 `45c0110d1a1427d5844e961de564f052136b9f2533854c5b3f1bc8fdae64d1e4`, C++ source `511955afa7e2c84425422ee553336d76689e5d4f3d652843f5952f250100582f`, mapping header `ed89745167b8d654eb0fe60c83258cfa484c078b7d056e5fe94d66911ed5750b`, and wrapper `a78ded1cf6f1082e1f1d7c337b81fd83cd9f883d021c5086c76c23892a398220`. Command, input and callback SHA-256 values were `dddd852b867d2f1dd25e083f90277d935ede3ca8f39c2afd769e7c34b69b0087`, `aa0509945967446df0602900e4203b7ff82bbfa4a44a73fa4d7cb1df81bc21e5`, and `655837d5836cfe11b49b006e6fba0c0003d45aa93451648e210f327649f633cb`. The last frame SHA-256 was `96ddc1e05abd89076c353e157b1711f76e66354f9b283b321659a259f0269491`. The pinned source revision, ROM and MoltenVK identities match the prior fog audit; all private frames, logs, saves and copied ROM remain ignored. No audio was retained. This result does not establish whether native terrain or resource art is remembered after vision leaves.

A subsequent fresh session replayed the known setup and base-edge selection, then used a preplanned 8,207-step input sequence to move the selected group farther right twice and pan back across the area. The prepared command file SHA-256 was `0c1354c48a5e82de877ebea76287184d9b8b73a646fc873d761ee1e1d76905a5`. At observed steps 7017 and 8067 the selected group appeared beside rocky coastal terrain; on the return pan, step 8137 showed darkened terrain with visible texture and step 8172 showed the Nexus and starting mineral line. These samples do not uniquely establish that an identifiable previously seen cell was outside all Probe and Nexus sight, and no remote mineral cluster was observed. They therefore do not establish remembered terrain or resource behavior.

The same C-button frontend binary SHA-256 `45c0110d1a1427d5844e961de564f052136b9f2533854c5b3f1bc8fdae64d1e4` returned 0 in 77.392 seconds with seven copied frames totaling 4,300,800 bytes and checked inputs unchanged. Selected group/coast frames at 7017 and 8067 have SHA-256 `ad5d6886c70d34a46b7544c0c6a2f57deb585aa92fc5647f342c009378e3849f` and `c8d16b4aa19c2cfc258521f3faf898c18978736cff8333e41a6915746e4a20de`; the return-pan frame at 8137 is `478658b2467f047ce43396dcabdad67ae407ee6f85d288e4017d32364cf591c2`. The actual command, input and callback records hash to `ca2a01134ddd4b2cd83058621b03dbf4f276386d01aae8df8dee30928f789326`, `5dd71502affab304fc8d91db8bd357f4ebe2d5a7a58364d1146a1bde0cd137cc`, and `85f185d720c8d6d0c38b1712a47a4dbd2b9db4545388dd0a56341bfd70a84c64`. The ROM, source and library checks matched the preceding run. Frame and controller records, images, fresh save and copied ROM remain private; no audio was retained, and callback steps are not game frames.

## Scout and return

A fresh Protoss Custom Game reached the starting Nexus. R selected all
four Probes. A rightward camera pan showed a dark purple viewport, with
the four Probes near the base on the minimap. C-Right sent the group
toward the right-hand viewport. The Probes reached a shoreline with
a visible line of cliffs and blue water. A leftward pan and a second
C-Right command returned the group to the base area. The next rightward
pan showed the same coastline as dim textured land and water, with
the group away from it on the minimap.

| Callback step | Observation | Land patch median intensity |
| ---: | --- | ---: |
| 5,945 | Remote viewport before scouting | 4.85 |
| 7,155 | Shoreline with the four Probes nearby | 41.29 |
| 8,765 | Coastline after the Probes returned toward the base | 22.74 |

The last column uses the median of `(54R + 183G + 19B) / 256` for
pixels `x=60..151`, `y=85..129` in the 640×240 callback buffers.
It describes displayed pixel intensity. The three frames establish
retained terrain in this sampled area. Exact visibility boundaries,
remote mineral and geyser memory, and correspondence to the GBA
rendering remain open.

The rebuilt callback frontend uses the pinned ares source
`0aafd85789215e84e1e43415c07d4c88461b7899` and the installed
MoltenVK from the earlier audits. The run returned 0 in 60.336
seconds with ten local captures. The wrapper verified the ROM,
library, binary and checked source inputs after execution. Clients
reading the interactive bridge can encounter emulator diagnostics on
stdout, including a Flash status line during startup. Wait for the
protocol response before sending the next command. Images and input
records are retained only in the ignored local run output.

| Local receipt | SHA-256 |
| --- | --- |
| Interactive frontend | `f20a98e70f2a7899216793cdba71d5942b77b70e7405cf762cc60f4dde22d44e` |
| Run manifest | `8baa3d8142aad56387b2ecf7718c68a7f7012ae39c6329f03eea15e23b506b59` |
| Controller commands | `ffdbe11dd284d5824d8661f8181e2aad1c3d5e9c3b66f30b328147b6504d12e2` |
| Viewport before scout | `8c4923a905d7f9abd06189d19cf8b6ed2ca10e95f3e40cbf49308e154aae1fc4` |
| Lit shoreline | `2c7fd74ed9466ba6e103e9f02c11c5e69f53b89b9bd57367cc3a4bc01c8994a1` |
| Dim shoreline | `b4a44539f6b57d32a95f84926bd6e5f7d43655092afb39a6bc49da0d3b33efe6` |

## Remote mineral patches

A fresh run sent the four selected Probes down the coast, then
scrolled farther south. Two blue crystal patches appeared near the
bottom of the field at step 8,669. A leftward move returned the Probes
to the base area, where all four were visible at step 10,128. The
return pan shows the same two crystal groups in dim form at step
10,547. The cyan minimap mark in that area contains the same seven
qualifying pixels in both resource frames, near `(100, 193)` in the
640×240 callback buffer. The mark is absent from the earlier step
8,489 frame before the crystal groups appear. The bright and dim
frames show a minimap mark associated with this discovery. The
replay leaves hidden depletion, remote geysers, and the exact marker
color and visibility thresholds open.

GBA Mode 4 and tile resource painters currently require live sight.
The minimap terrain is followed by building and unit dots. A shared
resource memory should retain the last observed presence of a patch
while it is hidden, then clear it when the player sees it depleted.
Using the live amount to draw a remembered marker would reveal enemy
mining through fog. Include the resource state in save/load and cache
coverage. Legacy saves require a conservative initialization because
they carry exploration and current mineral amounts without the last
observation.

The pinned native bridge completed in 70.335 seconds with 17
local frames and verified its checked inputs after the run.

| Local receipt | SHA-256 |
| --- | --- |
| Run manifest | `4d9fad8b5e2b4c6e17d7192a0ccba70bf66c5b6fb305f7e58d22f66d6df57b1d` |
| Controller commands | `7481c84e9fbc15d60e3f2d42bc7ff1c3305d852c52b8e126f7cffaf8c4eb7204` |
| Before the crystal groups appear | `00205a563b739481378d5b5129cf33d084351edfee204dd8275e176f5e5af4be` |
| Lit crystals | `3fbf19212e5a2d8a021f77ae8bb9da09ea145774c92820e3fca22f8d0a7ef841` |
| Four Probes after returning | `491dab9203daab9494b941d1de097b1b2a1916bb3171ed6a2435b29d6da3ba15` |
| Dim crystals | `386db62c84e53febfdf9544fb526c30283155c5d7ea330abba0d35faf1f166d4` |
