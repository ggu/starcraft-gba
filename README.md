# StarCraft GBA Demake Project

This project is an original-art, asset-free RTS skirmish for the Game Boy Advance. Defend a small Terran base, gather minerals and gas, build an army, and destroy the Zerg Hatchery. It is a compact demake with a hand-authored scenario and simplified rules, not a full StarCraft port.

Optional StarCraft 64 profiles add imported campaign scenarios and three-faction custom games. The canonical profiles expose Terran 1–10, Zerg 1–6, and Protoss 1–6: 22 of 30 campaign entries. All six Protoss chapters have current canonical-flag host wins; P6 also has an input-only ARM win and fresh-process SRAM continuation. Earlier P1–P5 ARM evidence remains scoped to the preserved P5 build. See the [P6 promotion audit](docs/audits/2026-10-27-p6-promotion/README.md).

Performance remains below the 30-fps goal. The current P6 route is substantially below that target; see the promotion audit for its measured cycles and limits. The preserved P5 build measured about 26.6 fps equivalent on P1/P2 and 16.1–18.9 on P3–P5. Earlier Terran T7/T10 probe-entry traces measured about 22.4/18.8; see the [Terran audit](docs/audits/2026-10-26-terran-word-promotion/README.md) for scope. No physical GBA result is claimed. This adaptation uses a user-supplied, verified StarCraft 64 USA ROM and does not reproduce the N64 engine. Imported maps and graphics stay local.

## Purpose and rights

This independent research and hobby project explores real-time strategy engine design on the Game Boy Advance's resource-constrained hardware. It is not affiliated with, sponsored by, or endorsed by Blizzard Entertainment or Nintendo. References to their games, platforms, and trademarks identify the subject of the work or compatibility; they do not imply authorization.

The default skirmish uses project-authored game art and requires no commercial game files. The optional StarCraft 64 importer requires user-supplied, lawfully obtained game data. The project does not distribute original StarCraft or StarCraft 64 ROMs or extracted game assets. Do not upload those materials or generated builds containing imported game assets to this repository, its issues, or its releases.

Import tools grant no additional rights in game content. Importing, using, or sharing that content requires the necessary rights or applicable legal authorization; possession of a ROM alone does not establish those rights. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for third-party material and licensing boundaries.

## Get started

Install the [required tools](docs/DEVELOPMENT.md#install-the-tools) first.
Building and testing need no emulator or commercial game files.

Clone the public source repository; no GitHub account is needed to download it:

```sh
git clone https://github.com/ggu/starcraft-gba.git
cd starcraft-gba
```

From the repository root, run:

```sh
./dev doctor
./dev build
./dev test
```

`doctor` should report the build and test tools as `OK`; `build` creates and verifies `build/StarCraft-GBA.gba`; `test` runs the host-side checks and exits successfully. No StarCraft installation, ROM, extracted game data, or emulator is needed for these checks. See [Contributing](CONTRIBUTING.md) for repository guidance.

Playing is optional. Install mGBA using the instructions in [Development](docs/DEVELOPMENT.md), then run `./dev play starcraft`. The [StarCraft guide](starcraft/README.md) describes controls, gameplay, and current limits.

## Optional StarCraft 64 profiles

This profile requires a legally obtained StarCraft 64 USA ROM that you are entitled to use. The project does not provide the ROM, imported content, or converted assets. Preparation accepts a `.z64`, `.v64`, or `.n64` image, or a ZIP containing exactly one 32 MiB N64 image. It normalizes byte order and checks the resulting image against SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`.

```sh
./dev sc64 prepare --rom "/path/to/your/StarCraft 64 ROM or ZIP" --profile terran
./dev sc64 build --profile terran
./dev sc64 test --profile terran
./dev sc64 play --profile terran
```

The canonical Terran profile includes all ten Terran campaign missions; canonical Zerg and Protoss each include their first six. Use `--profile zerg` or `--profile protoss` with the same build/test/play commands. Each ROM retains Outpost and Blood Bath and supports any human race with one to three independently selected AI races. Canonical campaign coverage is 22/30 missions; Zerg 7–10 and Protoss 7–10 remain unqualified.

Preparing Terran generates its complete ten-mission input set; preparing only Zerg or Protoss does not. Prepare each canonical race before `make race-campaigns`. Canonical Zerg uses the Z6 identity and `.local/play/zerg-z6`; canonical Protoss uses P6 and `.local/play/protoss-p6`. Explicit `protoss-p5-shuttle`, `zerg-z4`, `zerg-z3`, `protoss-p3` and `protoss-p4-storm` retain their real ROMs and save paths. No save conversion is performed. See [the campaign guide](docs/RACE-CAMPAIGNS.md) for mission and save-format details.

The first preparation fetches the pinned `sc64-maps` extractor into ignored `references/`, so Git and network access are needed. ROMs, extracted data, generated headers, captures containing imported art, and saves stay in ignored local directories; do not commit or distribute them. Contributors can work on the default skirmish without this profile. See [the unified guide](docs/SC64-UNIFIED.md) and [development](docs/DEVELOPMENT.md) for details and troubleshooting.

The [campaign guide](docs/RACE-CAMPAIGNS.md) records the staged profile
history, current mission status, and shared skirmish setup. The combined
Terran profile uses bounded pools of 192 units, 96 buildings, and 80 mineral
fields; prepared canonical GBA ROMs are padded to 32 MiB. The original 32 MiB N64 image and
every converted map, sprite, and generated header remain private local inputs.

## Contributing and provenance

AI agents should start with [AGENTS.md](AGENTS.md), which maps the source,
profile flags, verification commands, and asset boundaries. A starter prompt is:
"Read AGENTS.md, implement the requested change in the default skirmish, keep
it independent of game assets, and run ./dev build and ./dev test."

See [CONTRIBUTING.md](CONTRIBUTING.md) for the asset-free workflow and contribution boundaries. Gameplay references and pinned commits are in [docs/REFERENCES.md](docs/REFERENCES.md); third-party code and data provenance are summarized in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## License

Copyright (C) 2026 StarCraft GBA contributors.

The project-authored files in this repository are free software: you may redistribute and modify them under the GNU General Public License, version 3 or, at your option, any later version (`GPL-3.0-or-later`). They are provided without warranty; see [LICENSE](LICENSE) for the full terms.

Preserve existing file-level copyright and license notices. Third-party material and external dependencies retain their own terms, as described in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This license grants no rights to StarCraft or StarCraft 64 ROMs, artwork, maps, sounds, or other game content, or to Blizzard or Nintendo trademarks.

## Validation

The [validation record](docs/VALIDATION.md) lists available automated checks and hardware-testing status. Physical GBA testing is not claimed unless a result is recorded there.
