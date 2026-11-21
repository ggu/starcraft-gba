# Minimap and field hint costs in the race cartridges

The Z10 release at `61f705cc` composes all 576 minimap terrain pixels
every frame. A profile of 33 guest updates during a three-CPU Blood Bath
continuation attributed about 22,393 cycles to that path. The same
profile measured 40,780 cycles for the two terrain viewport band
transfers on a typical update. Those copies restore the playfield
before drawing actors.

Z10 now uses the incremental minimap raster already used by Terran,
P10 and Showcase. Visibility and exploration transitions maintain
its 24×24 colors. Drawing copies the raster and paints the live
markers over it. Z10 takes this option from the canonical cache flags.
The raster adds 576 EWRAM bytes to Z10. Its EWRAM ends at `0x0203a3c8`
and its IWRAM ends at `0x03004ea8`. The other three products' RAM
endpoints agree with the control, including Showcase at `0x03004fc0`.

The shared field hint pass also finds the first friendly selected worker
in the scan that determines whether units are selected. It reuses the
current target when testing for a mineral patch. Command execution
continues to query the current world when the player presses A.

## Blood Bath comparison

| Z10 variant | Mean cycles | p95 | Peak |
| --- | ---: | ---: | ---: |
| Release control | 876,399 | 1,167,433 | 3,757,850 |
| Minimap raster | 855,641 | 1,147,353 | 3,723,637 |
| Minimap and shared field hint | 842,472 | 1,134,841 | 3,714,007 |

The combined build saves 3.87% mean, 2.79% p95 and 1.17% peak guest
cycles across the 3,600 advancing updates. All 31 sampled world,
palette, display and route comparisons and 16 fresh HOME cuts agree.
Both builds export the same 32 KiB pause SRAM. The 30 fps budget is
561,792 guest cycles. All 3,600 updates in the combined build still
exceed that budget. Physical frame pacing still needs measurement.

## Control, campaign and memory checks

The four normal ARM links pass ROM, SRAM and RAM checks. All race
Campaign openings and the Showcase opening match world and display
at frames 61 and 121. The 54 Custom Game save and resume cases produce
108 exports that match the current control bytes. The ordinary P10
tail reaches victory at frame 46,057 with all 68 world and display
cuts and the exported SRAM matching the control.

Z7 starts from HOME, pauses at frame 5,641, resumes in a fresh
process and wins at frame 228,128. The 254 world cuts, ten paired
display cuts and six route-cache samples agree with the release.
The pause, victory and fresh result re-export journals match byte
for byte. Mean cycles fall 1.48% in the early dense window and
1.12–1.13% in the two late windows. One late-window peak rises
0.13%. In Terran Outpost, all 90 world and display cuts agree on
ordinary inputs. Mean cycles fall 0.80% in its 800-update busy
window and 0.58% over the complete 17,636-call tape.

The HUD cache and current worker gather sanitizer fixtures pass with
all four prepared profile flags. A separate Z10 hint check covers
visible minerals, a friendly refinery, a double tap and a transported
worker. Z10 and P10 each match the previous hint outputs across 1,200
randomized worlds with transported selection present. An optional
Scarab build also verifies that a selected Scarab alone gives no
selection hint. The older worker STOP fixture expects the historical
gas-order behavior and fails the same cargo assertion on the release.
The current gather fixture covers retargeting and cargo preservation.

Instrumented P10 and Showcase runs retain the same observed stack
canaries in the busy Outpost and Showcase scene replays. The P10
victory replay leaves 6,504 untouched bytes from `0x03005000`.
Those observations cover the exercised tapes.

## Native minimap observations and replay cases

Two local ares captures associated with ordinary native Blood Bath
updates 17,144 and 17,166 share the apparent base camera. In the
80×46-pixel minimap interior sample, 103 RGB pixels differ and 93 of
those differences involve a strongly red pixel in at least one cut.
All 218 near-black pixels in that sample agree. The captures come
from separate restores. Callback timing, marker ownership, and
fog flags remain open.

A separate ordinary native replay starts both cases at update 1,365.
The neutral run keeps the Command Center and minerals on screen.
Sustained right input moves the cursor and scrolls to a dark field.
The minimap viewport rectangle goes from the left side to the right.
The neutral and right-input screenshots follow wall-time-matched
runs ending at updates 1,552 and 1,595, respectively. Their saved
states were reopened for capture. Use a frame-matched replay with
synchronized camera and fog state for exact minimap pixel assertions.

For the next harness cuts, capture each indexed page at visibility
enter and leave, marker motion and deletion, camera scroll, pause
overlay, and fresh HOME restore. Compare the minimap rectangle and
its border, then compare the rest of the HUD. Keep the native masks
and palette indices alongside each cut wherever the emulator can
synchronize them with the world state.

## Follow-up cost measurements

Two exact 64×64 depot-flood trials passed the facility oracle and
the paired Blood Bath state, display and save checks. Precomputed
packed-direction masks increased mean cycles 0.34% and peak 7.24%.
Deferring packed writes until the flood stopped increased mean 0.21%
and peak 4.31%. Both increased p95 0.31%. The current attribution
samples also perform 19–42 target queries each update, so lazy
target-index construction would still rebuild in every sampled update.

The combined profile measures about 13,829 self cycles in the
projectile overlay iterator in its middle band. A non-looping
timing specialization preserves sampled frames, routes and saves,
with a 0.027% increase in mean guest cycles. Keep the existing
iterator and prioritize larger repeated work in orders and drawing.

| Local receipt | SHA-256 |
| --- | --- |
| Current control instruction attribution | `efdf17fd625339f1e004ed0197035a2002249b83bb2c7fb0acb9790c7c8b0dd1` |
| Combined instruction attribution | `c5806dde7d8f767f0888b9731f8fd805ef66238ab419b7473859260dedd04f88` |
| Z10 raster-only comparison | `a881a356812f484c8ae1e484f66ef29af07a1d8c3b90606c365fe387a6883fc7` |
| Combined 3,600-update comparison | `6968a1e12a3df5554476b6754be82d8eac7b898aeb1555fa15ba82dc09bd351b` |
| 54 Custom Game fresh continuations | `fa57ff674179846234b0086ee53909f990ee0c23a26ae39cb9dfe7ed3ff266df` |
| P10 tail through victory | `4eb2dc78de551b05787202c922dcd6f2cd2712f02b8accfa91156d96be46e9a8` |
| Z7 complete pause, victory and result readback | `2c5ce98869ad2f8c76c27e461fba6a0b23ed2e782b20264017604a0c6af7c0c9` |
| Terran three-CPU Outpost | `bee9471ccd5c8b5bd8485b521ff5606e00783d733adb899714067684337b8121` |
| P10 and Showcase stack canaries | `35c6711d01a6ec8c5f7137c3f158d6063cf0165e562f3bca30e17d2f79632ec2` |
| Native minimap crop comparison | `5d99bb93f8eec6dfafbce55215b70460038c7c661fd2b807f9f4de6ba6ce27ef` |
| Native neutral viewport capture | `19d555e11066cb1c7a121353c239f7621d830db1f4dcf109f0a1a9f75a14b142` |
| Native right-input viewport capture | `34c1fd207f4e4defd422914075dc8f8c4e53907fb201195552e3e15a1a177b7c` |
| Facility packed mask comparison | `a64b38230e83a936356d527001d353d4236516be1d85fa603d7778de32821f04` |
| Facility deferred write comparison | `75c8fcfd58d451240542e8c8cd72435917c0041661f61dfa86374803532ad592` |
| Non-looping overlay comparison | `97f84876c7b631bcc5ced2ab60a61d79ef66463986abe5f4a8d5b5e743c38cf5` |
