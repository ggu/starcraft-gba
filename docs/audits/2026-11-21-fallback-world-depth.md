# Fallback death effects in shared world depth

The [N64 depth key](2026-11-19-world-depth.md#world-depth-regression-cases)
uses sprite elevation, then world anchor Y for elevations 0–4, then
the turret flag and sprite pool ID. The [native death witnesses](2026-11-21-shared-world-depth-regressions.md#n64-reference)
place attached images in the dying sprite's own turn and give detached
effects separate keys. The Overlord keeps elevation 16 through its
nested death images. Detached Overmind rubble uses elevation 1.

The shared GBA Mode 4 painter used to draw authored fallback bursts after
all units, even in builds with a depth-enabled death bank. A ground
collapse could cover a southern unit or a flyer.
The fallback now captures the dying actor's elevation and enters the
same queue as live actors and bank-backed effects. Collapse uses the
actor's elevation. Fallback rubble and remains use elevation 1.
Explosion primitives represent detached effects at actor elevation
plus one. That last rule is a fallback convention. The bank-backed
path continues to use each image's recorded attachment and elevation.

The queue reserves the greater of two entries per death or the bank's
existing per-death bound. The prepared production bank already reserves
two. The elevation fits in the existing effect row. Effects remain
transient and add no journal bytes.

## Replay cases

Stage these cases with camera, palette and animation held constant.
Capture an empty frame, each isolated actor or effect, the expected
composition and the reversed composition. Require a nonempty mask
where the orders differ and zero incorrect pixels on both indexed
pages. Keep the cursor outside the overlap and verify the top HUD
rows 0–11 and bottom HUD rows 120–159. Compare platforms at their
own scale and palette. Keep ROMs, saves and images in local evidence.

| Case ID | Setup | Expected foreground |
| --- | --- | --- |
| `fallback-collapse-ground-y` | Stage a fallback Zerg ground collapse on layer 4. Overlap a Tank one or two pixels south, then north. | Southern Tank, then collapse. |
| `fallback-collapse-below-air` | Overlap the same collapse with a Wraith. Record both actor anchors and the render tick. | Wraith. |
| `fallback-remnant-below-ground` | Advance the collapse beyond the burst. Overlap its layer-1 remnant with a Tank. | Tank. |
| `fallback-air-burst` | Overlap a Wraith fallback burst with a live Wraith. | Layer-19 burst. Keep this authored fallback check distinct from native image parity. |
| `fallback-pool-at-capacity` | Fill the fallback pool with visible building deaths while bursts and rubble are both live. Repeat with anchors in unseen cells. | Two entries per visible effect, with rubble on layer 1 and bursts on layer 5. No entries in unseen cells. |
| `native-pool-id-reuse` | Observe live sprite IDs on two layer-4 different-kind actors at the same sprite Y. Record a second cut after last-image retirement and FIFO reuse. Use the same opaque pixel for the picker. | The greater native sprite ID wins when the elevation, Y and turret flag tie. |

The five fallback cases have staged GBA sanitizer checks. Run them in
Campaign, Custom Game and Showcase after staging. Capture the normal
N64 render sequence at death with the actual sprite and attachment
IDs. The native pool case starts with the [Terran 10 key witness](2026-11-21-native-depth-followup.md#native-sort-and-attachments)
and needs opaque active-camera overlap.

Record the source revision, profile, ROM and tape digests, camera,
simulation tick, effect age, anchors, visibility, render keys, active
page, and distinguishing and incorrect pixel counts. For pool ties
also record the allocator IDs, last-image retirement and free-list
head before and after reuse.

## Verification and remaining gaps

The Showcase HOST test under ASan and UBSan covered five fallback
overlaps on both pages. It found 26 distinguishing pixels in each
ground-collapse and flyer cut, 31 for the remnant, and 45 for the air
burst. Every expected foreground and HUD check passed. The 21
bank-backed overlap cases and the normal Siege Line death test also
passed. The filled fallback pool enqueued all 32 parts in visible
cells and dropped them all after visibility was cleared.

Terran T10, Z10, P10 and Showcase linked with the existing prepared
inputs, passed the ROM and payload checks, and kept their IWRAM and
EWRAM endpoints. Paired ordinary mGBA runs matched both indexed pages,
active page, palette and world state at two cuts in each Campaign
opening and the first Showcase scene, plus four cuts on each Custom
Game map on all four products. The Custom Game sample averaged at
most 1.14 additional guest cycles per update. The ordinary Terran
Last Stand replay also matched both pages at all 95 cuts through
defeat. Both runs averaged about 602,957 guest cycles per update.
Timing a dense active fallback pool and measuring physical frame
pacing remain open.

All three race cartridges also passed 54 ordinary Custom Game
pause saves and fresh HOME continuations, sampling both maps, every
human race, and one to three named CPU seats. Each case matched 120
updates after fresh load against its uninterrupted control and
preserved the SRAM bytes.

| Receipt | SHA-256 |
| --- | --- |
| Showcase sanitizer overlap and capacity checks | `ee0f95c9b1aa7689f971c07a7733065752d7f5630d28e475f81d9e813bb71336` |
| Campaign and Showcase openings | `fbe225ac9d8cf4fd226931decc3d07b21758baafa542c589017862be7506cccd` |
| Both Custom Game maps | `9aa3e35b9b8047a20f36e362a64dcf93972ba221fddbeef312cc68a22a211f5a` |
| Terran Last Stand | `a97cfbb97d40b36b1b1e1183b162f36099a04a03077a81e1663521619091d45c` |
| Custom Game pause and fresh HOME continuation | `db9e75c79d79fe46de6b02720bbc2c5400ffd2ebf84168ecd0daefe1a39253b4` |

Exact ties still use GBA kind and slot. A shared native-style
sprite FIFO needs to account for allocation, last-image retirement
and reuse across units, structures and detached effects. Several save
configurations already fill the SRAM payload, so a persisted allocator
needs a measured encoding and compatibility plan. Also audit legacy
weapon fallbacks that still draw after the world queue. Identify
reachable attacks with missing depth metadata and record whether
their native images attach to the shooter or spawn separately.

The repeat N64 capture remains open. The available headless ares
runner reached its ready marker but failed to capture a frame, and
its interface cannot load the retained Blood Bath state. The
native comparison above uses the retained observer records.
