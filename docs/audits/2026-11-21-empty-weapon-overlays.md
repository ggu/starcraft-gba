# Empty weapon overlay streams on the current P10 renderer

The probe skips attacker image and frame resolution when the requested
source attack animation has an empty overlay stream. Projectile Init,
Flight, and Death still resolve their own body image and timeline. Only
the child pass skips a zero-event overlay stream. Nonempty streams retain
the existing phase, anchor, underlay, overlay, and detached sprite calls.
The isolated implementation at source `7c0ababb` uses
`SC64_WEAPON_SOURCE_EMPTY_OVERLAY=1` to select the probe.

## Ordinary Blood Bath comparison

The two P10 cartridges use the `defd807a` build inputs and differ only
by the probe flag. The disabled ROM reproduces the release. Both pass
header, payload, and RAM checks with 40 pinned inputs. IWRAM ends at
`0x03004e20` and EWRAM at `0x0203b52c` on both. The candidate removes
80 linked text bytes.

One human Zerg and Zerg, Terran, and Protoss computers launch from HOME.
The human holds neutral and the fixed comparison times 1,000 advancing
updates at frames 15,102–16,101. All gameplay frames and route counters
match. The 21 sampled cuts match world and tracer state, both indexed
pages, palette, active page, and camera. The cuts observe 101 active
trace instances in total. Start/B at frame 16,101 writes the same 32 KiB
SRAM in both builds. Fresh HOME/L resumes to frame 16,281, matching the
respective uninterrupted gameplay and the paired build's sampled output.

| Emulated guest cycles | Release | Probe |
| --- | ---: | ---: |
| Mean | 1,055,382 | 1,055,105 |
| p95 | 1,334,206 | 1,334,697 |
| Peak | 1,813,283 | 1,813,379 |

Mean falls 0.026%, p95 rises 0.037%, and peak rises 0.005%. The saved
0.95% mean result from an earlier indexed skirmish used a different
renderer and build profile. This current workload provides no material
speedup. Keep the option off in regular products.

## Checks and receipts

The asset-free weapon sampler group passes under ASan and UBSan, and
the enabled resolved-trace fixture passes separately. Both flag states
also pass the generated indexed source-weapon fixture, including the
Tank muzzle and launch, Siege Tank attachment, and Scout trail and
impact timeline. The enabled Showcase HOST engine fixture passes all
66 source weapon profiles, eight headings and full lifetimes, plus 15
projectile depth, attachment and HUD overlap cases. The original-art
build, source boundary, and local doctor checks pass.

| Item | SHA-256 |
| --- | --- |
| Disabled P10 ROM | `e743a47c3a526cb021e802cfb4b124086dbee41118c183bf0e2f36a3003bbe85` |
| Enabled P10 ROM | `8a4e072cf77395bc2118f147e625c568550376e874bee5940cddc44e35f180bc` |
| Disabled replay | `89f11e585830a99acf51dfc87edc877b2ced752fa81e1cab986453c48bd237a2` |
| Enabled replay | `17eb10d95df6480ef6d6126a46c50c6775f55e2f927916a4586ce9ef209d2d0e` |
| Paired comparison | `be960ffd9ca6ea23ce776e3e09a5d2bbb49f85e4210bae7686a77c213973ba17` |
| Common pause SRAM | `d740b43f9993d34183d7615b59a69b1368fc0a2b99961d02d405a46265301fcf` |
