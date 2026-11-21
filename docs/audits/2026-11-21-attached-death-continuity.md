# Attach death images to the dying actor's draw order

Retail N64 Blood Bath, driven with ordinary inputs in ares, supplied an
active-combat witness. At frame 17,147 an SCV at (58, 107) had 10 HP
and sprite ID 2138 at elevation 4. At 17,150 it had zero HP and
order zero with the same sprite, anchor and elevation. Its sprite
pointer persisted through 17,167 and was absent at 17,168. The
neighboring live SCV at (122, 107) had the same depth fields and
sprite ID 2137. The anchors were 64 pixels apart. The cuts establish
sprite lifetime through death. They leave the opaque equal-Y pixel
comparison and native ID reuse open.

Read-only inspection of the saved frames finds a coherent list with
backreferences to sprite 2138. The head and tail fields clear when
the sprite pointer disappears. The list-field offsets are deductions
from these cuts. The earlier native comparator audit establishes
the sort rule independently of the image identifiers.

| Native evidence | SHA-256 |
| --- | --- |
| Pinned death capture receipt | `9f716ee3f8dfb0ea13fe65830082f88a1d7b2368127b30d6730663cf294ffd93` |
| Lifetime capture receipt | `c9fbb83a43df5afbdea51876b85160012f5ca19a6d5eb750fe43c2f693bccad8` |
| Lifetime sample rows | `1c3c74f588e766f79b931016908a997dcf4936339f4a81f3c63a6fc3c433ce57` |

The shared GBA renderer records the originating actor slot when it
spawns a death effect. Images that belong to the actor and fallback
collapses use that actor's last elevation, ground Y and local tie
rank. Detached sprites and rubble keep individual draw entries. A
bounded queue merges actor-attached entries into the normal one-word
painter heap. A reused live actor slot wins an exact tie with its
older collapse, and multiple collapses retaining the same slot draw
oldest first. This rule uses GBA slot order. Complete native
cross-kind sprite-ID parity remains a separate qualification.

The sidecar is transient and clears at world start and load. The
20-byte effect row and SRAM layouts stay compatible. The four ARM
links add 32 bytes to EWRAM and retain their IWRAM endpoints. The
attached queue holds at most sixteen entries, one per death effect.

## Validation

- Sanitizer tests preserve the foreground in the lower- and higher-slot
  Zergling versus Marine cases before death and after the attached
  collapse begins. Both pages pass with 65 live and 58 attached
  distinguishing pixels per ordering. Actor-slot reuse, effect
  replacement, reset, detached bursts, nested images and clipping
  also pass. EWRAM and VRAM pool tests emit identical results.
- ARM ordinary openings match world and both indexed pages at frames
  61 and 121 in all three race cartridges and Showcase. Siege Line
  and the Terran last stand match sampled world, palette, HUD and
  displayed-page phase. They change viewport pixels in 24 of 91 and
  15 of 57 cuts respectively. Parallel EWRAM and optional VRAM
  Siege Line builds match all 91 world and two-page cuts.
- All 54 Custom Game cases start from ordinary input, pause and
  continue from fresh HOME. They cover both maps, all three human
  races and one, two or three independently selected AI positions.
  The paired lobby and world receipts and all 108 SRAM exports
  match the preceding build.
  In a separate busy Blood Bath save replay, all 31 world and indexed
  video checkpoints and 16 fresh continuation pairs match, with
  identical route counters and pause journals.
- T1, Z2, Z5 and P5 pass full ordinary routes, fresh pause
  continuation and third HOME result readback on the new builds.
  The recorded gameplay fields and pause, victory and readback
  journals match the previous release.
- Z6 completes the ordinary route with 65 matching observations.
  Its frame 18,721 pause includes two queued impacts. A fresh HOME
  process restores the world, selection and pending-impact packet
  and wins at frame 56,913. A third HOME load holds the terminal
  world for eight idle updates and re-exports the exact journal.
- P6 matches all 113 retained checkpoints across 101,370 ordinary
  calls. The fresh HOME process resumes the frame 64,800 pause and
  wins at frame 99,906. A third HOME process holds the result for
  eight neutral updates and re-exports the identical victory journal.
  Its bytes also match the retained P6 victory from the previous build.

| Guest timing over the paired ARM replay | Previous mean cycles | Updated mean cycles |
| --- | ---: | ---: |
| Siege Line, 451 updates | 887,835 | 888,578 |
| Terran last stand, 285 updates | 602,957 | 602,958 |
| Busy three-CPU Blood Bath, 3,600 updates | 921,496 | 921,678 |

These results measure guest cycles. The busy run still exceeds the
561,792-cycle target for 30 fps.

| Playtest ROM | SHA-256 |
| --- | --- |
| Terran | `68c1d21fcfe050b230306649285b1af8509652225a12a3fe6b3ecacd8fafdfdf` |
| Zerg | `e39909df3ccfce7a09e0ab837084dc47c536e94043fe624c52b785e95cc180ef` |
| Protoss | `f6c36c6305aa6dd1f0e695e0711cfc5a1f09c75b26ee381089dc00c26494f1b0` |
| Showcase | `243a82326b3273f24b191bd3a363d1f2c543cabd25e18e69b671330ad4181fe2` |

| Local evidence | SHA-256 |
| --- | --- |
| Paired HOST storage output | `0970530ec39164667c815967133cbeff8bcaa476d13bb03205b15bbc72fd0dca` |
| ARM Showcase scene comparison | `d50eff922d3f62811789d06384e13b14024b2317445974505ed5c5660617442d` |
| ARM Showcase storage comparison | `69e590ed29c99a09352fa1d11101a86d3df3e3867882b0e5cbeeeb5d9e5fd4ec` |
| Custom Game matrix | `b481d6d722b80d12acb80dd39b6bad3315f0bade9872db9c62ffb4940775e3e1` |
| Busy Blood Bath comparison | `84f81dd96c7272e1565b96b5110035d2a1d8383de9fca95866470adf2f8cc723` |
| T1 and P5 comparison | `f393e7c2746cc141ffe32805df15906a55cae5b97b4118d12603b1647a135b89` |
| Z2 comparison route | `7f6614a2dda1b9e56ac03863fb5bbf329b4f794f08188e13519d01885db1f06f` |
| Z2 result readback on both builds | `1bb2146e2f6dfee3faeda93200df1333f19f82b84a3e1b55e1415b90ad077630` |
| Z5 comparison route | `6e7ea520fdaea89087087861ec33662796143d2c9aedc20f70a14927f70043f8` |
| Z5 result readback on both builds | `05863abe7f900fcbe3bc49679427fd1fae72de3c4a19066b4559fc9c11130d52` |
| Z6 route | `014460ec973bc7fe761fe3c96ce1cd975cc8288c8f0deab011ed1b923562100c` |
| Z6 result readback | `a684fc44add4a68caf658eddaa1881491ead37e832c8eb483dd61a2c9863ecc9` |
| P6 route and result readback | `28ea5393079461f91f251e3b1f0d7073166240fe244e78f59e39371efa721e29` |
