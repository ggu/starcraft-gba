# Visible source-effect overhead

The four delivered source-weapon products (Terran, Zerg Z6, Protoss P7 and
Showcase) cache each tracer's derived duration and bypass the off-screen proof
when either captured endpoint is already on screen. Other products retain
their previous flags. `CANONICAL_TRACE_DURATION_FLAGS=` disables the duration
cache independently of `CANONICAL_SOURCE_VFX_CULL_FLAGS`.

## Duration lifetime contract

`sc64_trace_duration` depends only on the immutable packed tracer identity,
captured endpoints and compiled source banks. Its result does not depend on
remaining life, current unit state, research, camera or game time. A 32-entry
`u16` array costs 64 bytes in the delivered products; it is outside every save
structure. Zero denotes unknown. Nonpositive and greater-than-65535 results
are returned normally and remain uncached, so no duration is truncated.

`add_tracer` is the sole runtime identity writer. It fills the selected slot's
cache after replacement; aging changes only `life`. `clear_tracers` combines
all existing trace clears with cache invalidation, including new games,
campaign starts, Showcase starts and race loads. The original-art save loader
invalidates after restoring saved tracers. New identity mutation paths must
store or invalidate the derived duration too. No Unit, Tracer, packed save
row, save identity, simulation value or cosmetic-overflow policy changes.

The visible-anchor check returns to the existing renderer, never to a cull.
Unsigned camera-relative arithmetic avoids signed overflow. Extreme wrapped
inputs can only take this conservative return. Supported off-screen effects
still use the bank-derived proof from the earlier cull audit.

## Current-main qualification

The baseline is main `4ea280c` / PR120. The paired candidate includes the
visible-anchor bypass and duration cache. Normal product defines and existing
absolute prepared inputs are retained, with the cache flag added. Both sides
start Blood Bath through the menu, choose the ROM's race against three mixed
AIs, then leave the human unattended to defeat with a fixed starting camera.
All semantic checkpoints match host and ARM; sampled host RGB555 frames also
match the baseline (12 combined records for Terran; 14 for each other race).

| Profile | Measured-run mean FPS, baseline → candidate | Mean cycle change | Final window FPS |
| --- | --- | --- | --- |
| Terran | 28.39 → 28.73 | -1.21% | 25.29 → 26.96 |
| Zerg Z6 | 28.16 → 28.66 | -1.75% | 26.11 → 28.32 |
| Protoss P7 | 28.53 → 28.72 | -0.66% | 23.19 → 24.65 |

These are bounded emulator cycle measurements, not physical-GBA timing or a
steady-30-FPS guarantee. The mean divides total measured cycles by measured
updates rather than averaging rounded window rates. No whole-campaign win,
all-camera, all-map or maximum-army-size performance claim follows.

Other local checks:

- Default build and full source-only suite: 647 Python tests, 20 optional
  skips, plus native fixtures; source boundary and whitespace pass.
- The asset-free engine fixture covers creation, expiry, replacement, original
  oldest-slot choice, restart, original-art save restoration and large-value
  rejection. Direct bank-backed ASan/UBSan builds pass 1,650 source duration
  cases per race profile, including changed endpoints and pose duration.
- The source VFX prepared-bank oracle still passes 1,410,048 cases, with
  visible-anchor boundary and extreme-coordinate checks.
- All 72 normal-menu host skirmish/save/reload cases pass across three races.
- Cache-specific SRAM checks save at frame 9001 with live cached traces
  (Terran 3, Z6 12, P7 3). Both in-place menu loads and new-process HOME loads
  clear every cache entry before continuation, then reproduce the reference
  world hashes at frame 9901. ROM/SRAM paths and hashes match per pair; six
  isolated owned emulator processes are confirmed terminal.
- Four normal ARM products pass unchanged memory guards and 32 MiB/header
  verification. IWRAM ends remain `03004df0`, `03004ee8`, `03004b30`,
  `03004fa8`; BSS ends become `0203707c`, `020370e8`, `020382a8`,
  `0203df28`. The cache adds exactly 64 bytes. Showcase has only `0xd8`
  bytes left below the BSS guard; it needs a new link check after changes.

Local evidence directories are `.local/build/vfx-duration-combined-paired/`,
`.local/build/vfx-duration-current-products/` and
`.local/build/vfx-duration-fresh-sram/`. They contain compiler argv, source and
prepared-header hashes, include traces, raw logs, ROM hashes and owned-process
terminal records. No Actions, GDB, private asset generation or SD writes were
used. Private game inputs and generated products are not public source.
