# StarCraft: GBA Outpost

StarCraft: GBA Outpost is an original-art, asset-free RTS skirmish for the Game Boy Advance. Defend a small Terran base, gather minerals and gas, build an army, and destroy the Zerg Hatchery. It is a compact demake with a hand-authored scenario and simplified rules, not a full StarCraft port.

The optional unified profile adds six reduced campaign missions—two each for Terran, Zerg, and Protoss—and three-faction custom games. It is a GBA adaptation built from a user-supplied, verified StarCraft 64 USA ROM. It does not execute or reproduce the original N64 engine. Imported maps and graphics stay local and are not part of the source release.

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

## Optional unified missions

This profile requires a legally obtained StarCraft 64 USA ROM that you are entitled to use. The project does not provide the ROM, imported content, or converted assets. Preparation accepts a `.z64`, `.v64`, or `.n64` image, or a ZIP containing exactly one 32 MiB N64 image. It normalizes byte order and checks the resulting image against SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`.

```sh
./dev sc64 prepare --rom "/path/to/your/StarCraft 64 ROM or ZIP" --profile unified
make unified
make test-unified
./dev play unified
```

The first preparation fetches the pinned `sc64-maps` extractor into ignored `references/`, so Git and network access are needed. ROMs, extracted data, generated headers, captures containing imported art, and saves stay in ignored local directories; do not commit or distribute them. Contributors can work on the default skirmish without this profile. See [the unified guide](docs/SC64-UNIFIED.md) and [development](docs/DEVELOPMENT.md) for details and troubleshooting.

The [race campaign profiles](docs/RACE-CAMPAIGNS.md) build separate Terran,
Zerg, and Protoss ROMs with shared map-select skirmishes, any human race, and
one to three opponents with independently selected races. Each currently
contains its first two campaign missions. Separate experimental builds add
Terran 3, Zerg 3 and Protoss 3; the combined `terran-t4` build adds Terran 4
through the normal campaign menu. The opt-in `terran-t5` build adds Antigan
Revolt through the normal campaign menu. Across these experimental profiles,
11 of the 30 catalogued missions are playable and the remaining 19 are
unavailable. The standard builds remain unchanged.

## Contributing and provenance

AI agents should start with [AGENTS.md](AGENTS.md), which maps the source,
profile flags, verification commands, and asset boundaries. A starter prompt is:
"Read AGENTS.md, implement the requested change in the default skirmish, keep
it independent of game assets, and run ./dev build and ./dev test."

See [CONTRIBUTING.md](CONTRIBUTING.md) for the asset-free workflow and contribution boundaries. Gameplay references and pinned commits are in [docs/REFERENCES.md](docs/REFERENCES.md); third-party code and data provenance are summarized in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## License

Copyright (C) 2026 StarCraft GBA contributors.

The project-authored files in this repository are free software: you may redistribute and modify them under the GNU General Public License, version 3 or, at your option, any later version (`GPL-3.0-or-later`). They are provided without warranty; see [LICENSE](LICENSE) for the full terms.

Preserve existing file-level copyright and license notices. Third-party material and external dependencies retain their own terms, as described in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This license grants no rights to StarCraft or StarCraft 64 ROMs, artwork, maps, sounds, or other game content.

## Validation

The [validation record](docs/VALIDATION.md) lists available automated checks and hardware-testing status. Physical GBA testing is not claimed unless a result is recorded there.
