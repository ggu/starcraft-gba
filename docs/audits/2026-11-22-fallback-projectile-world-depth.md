# Fallback projectile world depth

The USA N64 renderer sorts by sprite elevation, world sprite-anchor Y at
elevations 0–4, turret flag, and live sprite pool ID. An independent
weapon sprite starts one elevation above its target, or one above the
shooter for a point attack. Target removal leaves the projectile's
elevation intact. These rules come from the
[native render records](2026-11-21-native-depth-followup.md#native-sort-and-attachments)
and [native bullet constructor](2026-11-19-world-depth.md#native-bullet-depth).

A fresh ares Wasteland session captured an SCV crossing the southeast lip
of a Command Center. Its green selection ring overlaps the building art.
The next state, at step 62,179, has the SCV at (1189, 1797) and the Command
Center at (1056, 1744). The inputs advance emulation between the
screenshot and save. Use this cut to find the setup for another capture.
An opaque body comparison needs the record and pixels from the same
stopped update. The local screenshot digest is
`96a95b79d4184bc15cc349dd632304a2841952a114365b92ae2d6573a84ce8f6`.

Moving fallback weapons used to draw after all actors. Now the
shared Mode 4 painter queues their existing art using the target's
launch elevation plus one. Low layers use the current moving head Y
or the impact Y. The raster draws once and retains the captured layer
in transient tracer state after target removal. Legacy effects without
launch metadata, composite flashes and stationary
bursts still need individual ownership and layer evidence.

The normal race and Showcase ROMs use Mode 4. In the experimental
Mode 0 tile build, static actors and the dynamic raster use separate
hardware planes. A test of Mode 0 needs to check those planes,
including fog and dirty refresh while scrolling. Exact ties between
ordinary Mode 4 actors still use GBA kind and slot. A native-equivalent
pool ID needs allocation, attachment, retirement, and reuse evidence
before it replaces that policy.

## Harness cases

Record each case with the
[`depth_pair` replay assertion](2026-11-21-world-depth-replay-contract.md#replay-cases).
Use a fixed camera, fog state, animation tick and palette to compare
both composition orders. Require a nonempty distinguishing mask, zero
incorrect pixels and unchanged HUD bands on both indexed pages.

| Case ID | Cuts and expected output |
| --- | --- |
| `fallback-projectile-world-depth` | Fire an authored Plasma or Needle diagonally at a Bunker. Overlap a Tank two pixels north and then south of the moving head. Repeat with a Wraith and at Plasma impact. The layer-4 projectile is in front of the northern Tank and behind the other two bodies. Force fallback by omitting the individual source weapon profile in a local fixture. Record the shot's captured layer and current Y. |
| `fallback-projectile-target-reuse` | Replace the original layer-3 target with a layer-4 actor in the same slot after launch. Repeat the overlap and impact cuts. Also fire at a layer-18 Wraith and check that the resulting layer-19 shot covers the same Wraith during flight. Include the first frame without a live trace. |
| `native-moving-building-depth` | On the N64, move an SCV behind and then in front of the Command Center in the Wasteland setup. Capture both image records, world sprite anchor Y, actual draw admission and the exact callback for each stopped update. Compare opaque body pixels independently of the selection ring. Repeat with an Overlord crossing a Hatchery and capture its ordinary A result separately. |

Exercise Campaign, Custom Game and Showcase through ordinary controls
where the actors are available. Keep the staged fallback test separate
from footage of an ordinary source-backed attack. Record the native
pool IDs and GBA slots separately. Store binary captures and saves locally.

## Qualification

The prepared Showcase sanitizer fixture calls the shared fallback
emitter directly. It checks seven moving styles, including the first
visible Needle frame and its first impact frame. Each of the 60 ground
compositions has 3–14 distinguishing pixels, with zero mismatches on
both pages. The layer-19 Plasma impact also draws in front of the
target Wraith. Removing launch metadata in the fixture reproduces
the older after-world composition. The fixture checks both HUD bands.

Terran, Z10, P10 and Showcase pass their ARM ROM and RAM guards.
Campaign and Showcase opening cuts match at frames 61 and 121.
The three-CPU Z10 Blood Bath holdout matches 31 world and video
checkpoints, 16 fresh continuation cuts and the pause SRAM. Mean
emulated cycles per update change from 817,911 to 818,261 (+0.04%).
The ordinary holdout checks integration and pacing. Use the staged
fixture for the order-sensitive fallback pixels.

| Local receipt | SHA-256 |
| --- | --- |
| Showcase weapon sanitizer fixture | `0423a96707267d3b51000c75bdfb73e80df3880bce82846c474419020bad9f8e` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Blood Bath comparison | `05129273d198af9e8a1aa52b6207873811af6e7bf2cbee6754b7de03d047369f` |
