# Current Z9 custom phase budget

On source `94bc32f8cee27b001390433ce919c8e0b91a0ed4`, the actual packaged
32 MiB Z9 ROM SHA-256
`d86f1e23753caa7ab50a93c544758041a0c35796f677abae6c4e137a276e44b3`
and corresponding ELF SHA-256
`3a9ad92c63103608c1cef4d506ac69ed9666baa51a14b2d3c603f484408ac104`
were measured in mGBA with the pinned physical-button backend. This is an
emulator investigation of one custom opening, not physical hardware FPS,
campaign qualification, a complete match or a near-30-fps claim.

Normal buttons configured map 1, Zerg human and Terran/Zerg/Protoss AI. The
human stayed passive while AI built. After 4,000 play updates, START/B saved a
32 KiB journal; fresh boot/L loaded it and 500 neutral updates reproduced the
uninterrupted selected persisted-state endpoint. The seed SHA-256 was
`7ffbfb8f5ae36fb73ab57f3f5b8fd5039bb6df43e82e3b93d08c7edeca3ac0c4`.
The state had 46 units/17 buildings at frame 4,001 and 48/21 at frame 4,501.
All 500 measured calls advanced in play. Screen and mode matched every call;
selected persisted state and workload were sampled at six positions. The
backend does not expose guest framebuffer or palette reads, so these runs do
not establish guest visual parity.

A separate earlier prepared-bank ROM had SHA `14d6a529...`; it shared source
and a qualified vector but differed from the package in 778 bytes, including
header bytes and map-blocking data. Both independently produced this seed and
this exact 500-call cycle stream. All figures below refer to the actual `d86f`
package, not a claim that those ROMs are identical.

## Existing production counters

The production ROM already maintains the counters below. The backend reads them
outside each timed guest update, without guest writes. A separate unprobed run
matched every per-call elapsed cycle count and all six selected-state samples;
there was no observed guest-cycle overhead from the reads. Package, ELF,
backend, source/tool and seed hashes were checked before and after.

| Scope | Mean cycles per advancing call | Maximum |
| --- | ---: | ---: |
| Input-sample-to-input-sample elapsed | 611,692 | 3,102,721 |
| `demo_last_cycles` (update/audio/render loop) | 611,297 | 3,102,228 |
| Update, nested in loop | 256,744 | 2,809,762 |
| Orders, nested in update | 167,005 | 2,712,349 |
| Other update stage, nested | 54,729 | 309,680 |
| Visibility, nested in update | 22,665 | 35,500 |
| Map, nested in render | 187,181 | 214,055 |
| Background, nested in map | 84,604 | 88,187 |
| Unit/effect drawing, nested in map | 49,895 | 80,156 |
| HUD, nested in render | 77,928 | 181,518 |
| `gfx_draw_cycles`, before presentation wait | 290,571 | 413,566 |
| Audio IRQ cycle-counter delta across call | 12,075 | 60,829 |

These intervals overlap: IRQ can interrupt any measured work; map contains
background, buildings and units; and draw/presentation definitions differ.
Do not add them or subtract an entire scope as a promised gain. The elapsed
clock includes the normal presentation behavior and is not a hardware FPS
measurement. The 559,000-cycle period is a useful comparison to 30 Hz, not a
proof that this emulator workload meets or misses a physical frame-rate target.

The existing route counters registered 64 full field builds and 1,446 hits,
with at least one build in 28 of 500 calls. The slowest call, 402, spent
3,102,721 cycles elapsed, including 2,712,349 in orders, and registered 10
builds/20 hits. Other high calls 223, 255, 256 and 373 registered 7, 7, 4 and
5 builds respectively. The slowest no-build call was 1,051,819 cycles. These
are correlations; existing counters do not time individual calls or identify
which caller caused the rebuilds. An initial read-run wrapper exited 1 because
it incorrectly treated its successful subprocess exit code zero as false;
its actual subprocess and all comparisons passed. That result was retained,
the wrapper was fixed, and a new complete run passed with the same data.

## Guarded detailed probe

A separate clean source checkout staged all 392 package-manifest dependencies
with verified hashes. Its control build reproduced the delivered ROM exactly;
the second build only enabled existing `SC64_DIAGNOSTIC_ORDERS` and
`SC64_DIAGNOSTIC_UPDATE` counters. Its ROM SHA-256 was
`60279685368fc9983e0986efd9d087c863507f5a83e21d1500a07d2f9a0a16e4`.
Normal linker guards passed: control BSS/IWRAM ends `0203dce4/03004b20`,
diagnostic `0203dd0c/03004be8`. These are link bounds, not a dynamic-stack
measurement. Every one of the 500 selected persisted-world fingerprints,
frame advances, screens and modes matched the control; this does not compare
all transient engine state or guest pixels. The normal control reproduced the
package's per-call elapsed cycle stream exactly, and all source/input hashes
were checked before/after.

The instrumented loop averaged 612,265 cycles (+0.094% net versus the
611,692-cycle control); code placement and nested timer calls can perturb
individual scopes even where net change is small. Diagnostic means were:
orders 172,688 cycles; movement calls inside orders 48,125; order setup 41,705;
order target queries 1,797; and the custom-mode mission/AI stage 9,198. The
movement counter's peak was 1,133,414 and AI-stage peak 263,612. At call 402,
orders measured 2,717,345, while instrumented `move_toward` measured 577,950
and setup 41,897. Those nested scopes do not account for all orders work, and
some early movement returns bypass the optional accumulator. The cause of the
remaining tail is not established by these values.

Next, measure individual synchronous field-build calls and their callers,
including depot/worker and formation/ranged paths as well as `move_toward`.
Keep the full field, exact FIFO/tie behavior and unchanged actor/update limits;
use diagnostic-only bounded counters, recheck code-layout overhead and
per-update semantic state against this exact package seed, then compare a
candidate against independent route oracles and representative ordinary
inputs. The route counter correlation justifies attribution, not an assumption
that extra cache capacity or any particular optimization will help.
