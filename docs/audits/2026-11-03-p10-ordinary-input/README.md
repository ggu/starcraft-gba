# P10 "Eye of the Storm" won with ordinary input

P10 now has an ordinary-input host win, an input-only ARM replay that matches it,
and a pause-menu save that continues to the same win in a fresh process. It stays
an explicit candidate; canonical Protoss stays P7.

## Engine changes

All are gated on `SC64_P10_RUNTIME`.

- **Raynor's base can grow.** `race_owner_fields_race()` lets P10's human train and
  build Terran as well as Protoss. Before, production and construction compared
  against the side's single race, so Raynor's base could only fight.
- **Commands follow the selection.** `race_command_race()` gives a selected SCV or
  Terran building Terran build rows, labels and train-type cycling.
- **SCVs stay to construct.** A Terran builder stays with its building, as in the
  Terran campaign, rather than leaving as a Probe does.
- **Nearest depot.** `SC64_NEAREST_RESOURCE_DEPOT` joins the P10 flags. Otherwise
  every worker delivers to the first town hall, so SCVs would carry minerals from
  Raynor's base to the Nexus across the map.

With P10 off, these products are byte-identical to the base branch: P7, P8, P9,
Zerg Z7, Terran and Showcase.

## The route

`tests/sc64_p10_campaign_run.c` is built from the P8 controller. All orders are
ordinary key input through `game_update()`.

Three of the four Zerg attack towns are nearer the Protoss base, and Raynor's base
sits on far more minerals, so the route grows Raynor's base:

- **Opening.** Protoss Probes start mining and Zeratul, who is permanently cloaked,
  hides in the far corner. The Gantrithor flies round the Spore Colonies to Raynor's
  base, because it has no attack and its loss ends the mission.
- **Economy.** SCVs walk toward the patches first, because those lie just past the
  base's sight and the mineral order needs a visible patch. Then up to 18 SCVs, five
  Barracks, an Academy, and mostly Marines with some Firebats.
- **Defense.** The army meets raiders within 520 pixels but ignores idle guards in
  the nearest town, then walks home. Attack orders otherwise chase raiders into their
  towns, where the army dies a few at a time.
- **Assault.** At 60 units the army follows the unit with the shortest walk to the
  Overmind and destroys it.

## Results

The tape was run at commit `TESTED_COMMIT`, on base `d60e75eb` plus #253 and #254.
Inputs were cloned from the 2026-10-31 pinned set into a private `.local`, with the
codec-2 audio bank. Host runs use ASan and UBSan. ARM runs use mGBA 0.10.5, one
isolated process per run, log mask 7, no debugger. Replays write no game state.

| Check | Result |
| --- | --- |
| Host controller win | Frame 72,441 (73,829 calls), all three heroes at full health |
| Pause-menu save point | Frame 37,534, structure under construction |
| Fresh-process resume on host, observations matched | 36 of 36, same win |
| ARM full replay, observations matched | 74 of 74, same win |
| ARM prefix, observations matched | 39 of 39 |
| Fresh-process resume on ARM, observations matched | 36 of 36, same win |
| ARM SRAM against host | Record identical; only the unwritten bytes differ (0x00 and 0xFF, from byte 16,324) |
| `make test-sc64-p10-mission` | Pass |
| `make test-sc64-p10-carried-profile` | Pass: 234 configurations |
| `make protoss-p10` | Pass |
| `__bss_end` / `__iwram_end` | `0x020394f4` / `0x03004a58`, below the unchanged guards `0x0203e000` / `0x03005000` |

| ARM timing (whole tape, menus and briefing included) | Value |
| --- | ---: |
| Frames | 73,829 |
| Mean cycles per frame | 1,828,195 (9.2 fps equivalent at 16,777,216 cycles per second) |
| Mean update cycles | 1,026,234 |
| Peak frame cycles | 39,610,502 |

This is far below the 30-fps target. The peak was not broken down.

The ROMs under test were built before two later edits that gate engine lines for other
products and remove dead controller code. The rebuilt P10 ROMs are byte-identical, and
the controller's input records hash the same, so the results above apply.

## Reproduce

With private inputs prepared and no shared input regenerated:

```sh
make test-sc64-protoss-p10 sc64-p10-arm-replay SC64_AUDIO_DIR=.local/sc64/audio-codec2/generated
```

Then run each ROM with `tools/sc64_p8_arm_replay.py --mission 29 --prefix P10 run`, and
compare `p8-lines.txt` as in the [P8 audit](../2026-10-29-p8-campaign/README.md).

## Not done

- **The Gantrithor still has no attack.** Its hangar needs the Carrier training, flight
  and save code that open draft #212 changes, so this leaves it alone. The win does not
  depend on it.
- **Availability tables.** P10's PUNI, UPGR and TECx tables are not applied, so Terran and
  Protoss products are limited only by this engine's own rules, not the source's lists.
- **Terran menu coverage.** Only the five base build rows exist. Factory and Starport
  rows belong to the late Terran runtime, which is not in this profile.
- **Protoss base.** The route lets it fall, keeping Zeratul hidden. Defending both bases
  was not attempted.

## Not claimed

- **Fragile route.** This is one tape, without a start-offset sweep. It shows the mission
  can be won, not that this route is reliable.
- **Fidelity.** The AI backend, trigger cadence and transmission durations are authored,
  not native. Defiler spells and mission text are absent.
- **One save point.** Other save states are not covered.
- **Interface.** The Terran menus were exercised by the controller, not checked on screen.
- **Replay ROMs.** ARM results come from the replay probes, which share the candidate flags
  but have their own main loop. The production ROM was linked and verified, not replayed.
- **Hardware.** Nothing was run on a physical GBA.
