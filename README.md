# StarCraft: GBA Outpost

StarCraft: GBA Outpost is an original-art, asset-free RTS skirmish for the Game Boy Advance. Defend a small Terran base, gather minerals and gas, build an army, and destroy the Zerg Hatchery. It is a compact demake with a hand-authored scenario and simplified rules, not a full StarCraft port.

The optional unified build adds six reduced campaign missions—two each for Terran, Zerg, and Protoss—and three-faction custom games. It is a GBA adaptation built from the user's own verified StarCraft 64 USA ROM. It does not execute or reproduce the original N64 engine. Imported maps and graphics stay local and are not part of the source release.

## Play the asset-free skirmish

Build with `arm-none-eabi-gcc` and GNU Make:

```sh
./dev doctor
./dev build
./dev play starcraft
```

`./dev test` runs the host-side rule checks. `./dev play` requires mGBA. The [StarCraft guide](starcraft/README.md) describes controls, gameplay, and the current limits. See [development](docs/DEVELOPMENT.md) for tool requirements and build details.

## Optional unified missions

This profile requires a legally obtained StarCraft 64 USA ROM that you are entitled to use. The project does not provide the ROM, imported content, or converted assets.

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 ROM or ZIP" --profile unified
make unified test-unified
./dev play unified
```

Preparation validates the supported ROM and stores generated inputs under ignored local directories. See [the unified guide](docs/SC64-UNIFIED.md) for missions, setup, and limitations. Contributors can work on the default skirmish without obtaining or supplying any game assets.

## Contributing and provenance

See [CONTRIBUTING.md](CONTRIBUTING.md) for the asset-free workflow and what not to include in contributions. Gameplay references and pinned commits are in [docs/REFERENCES.md](docs/REFERENCES.md); third-party code and data provenance are summarized in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

No project-wide license has been selected yet. The license for the authored project files is pending the owner's decision. The optional importer is separately marked GPL-3.0-or-later; see [LICENSES/GPL-3.0-or-later.txt](LICENSES/GPL-3.0-or-later.txt) and the notices. A source license does not grant rights to StarCraft or StarCraft 64 game content.

## Validation

The [validation record](docs/VALIDATION.md) lists available automated checks and the hardware-testing status. Physical GBA testing is not claimed unless a result is recorded there.
