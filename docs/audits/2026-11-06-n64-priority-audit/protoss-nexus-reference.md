# Fresh N64 Protoss Nexus observation

On 2026-11-06 one owned USA N64 ROM was played in a single fresh ares process with ordinary controller input, an independent empty save directory, copied settings and a private input-shim key file. The session used no guest-memory writes, debugger, imported save, altered unlock, or global keyboard inputs. This is a bounded observation, not a Reaver ammunition or full-match qualification. The private screenshots and process log remain under `/tmp/sc64-n64-fresh-protoss-20261106`; no reference art or ROM data is committed. The native lock was held for the session; the process was terminated and reaped before release.

ROM SHA-256 `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`; ares executable `4ce08c105b4f930d9a86bf03c8ea51c5867fa3e29c942997a868b2b158a4d51d`; input/audio shim `ba9227ef9a14deec020b62affc7d92eb015d51d44d90eb8edacf1cb784f7ff70`. A shim audio pacing option was active; screenshot wall-clock differences are not game ticks or verified native timing.

From Single-Player, ordinary controls chose Scenario → Setup Custom → Melee, River Styx, Human Protoss and Computer Zerg. The screenshot at `04-11-19` displayed 50 minerals and 4/9 supply after gameplay loaded; this is a visible snapshot, not an established initial wallet timestamp. Selecting the central structure with A showed the named **Nexus**, 750/750 and supplies 9 (`04-11-27`). C-right opened its build grid (`04-11-37`); after a nonproductive A press, C-right again displayed a queue/progress indicator and supply 5/9 (`04-12-00`). At `04-12-28` the progress indicator had cleared while supply remained 5/9 and workers were visible. This supports a naturally completed Nexus production command in this session, but the resulting individual was not selected and its identity was not independently established. The wallet increased during autonomous mining, so these images do not establish a precise payment, birth time, or causal debit. A later ordinary selection identified **Mineral Field** with 1,396 minerals (`04-12-49`), but no controlled depletion interval was recorded.

The fresh setup did not provide a readily accessible Reaver or Robotics Facility within this bounded session. Reaver starting ammunition, purchase, launch and impact remain unobserved. A future controlled run should reach those prerequisites through ordinary play or a documented legitimate unmodified save, then identify the Reaver and wallet/stock before and after each command, with explicit timestamps and fresh-process provenance. Do not treat the Nexus evidence as a Reaver proxy.

## Current GBA rule comparison

Parent independently inspected the named Nexus, queue and cleared-queue
captures. At draft `19435672`, `starcraft/race_rules.h` sets Nexus maximum HP
to 750, shields to 750 and supply provision to 18 half-supply units (9 HUD
supply). These specific stock values agree with the named N64 Nexus display.
This does not verify damage, armor, regeneration, build time, initial wallet,
all other unit profiles or the engine's time conversion.

Parent preserved the closed session under ignored `.local/n64-ref/nexus-production-20261106/` with a private SHA-256 manifest for all 26 captured session files (about 14.5 MB). No private capture payload is part of this source report.
