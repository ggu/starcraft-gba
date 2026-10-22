# Development

The default StarCraft skirmish builds without ROMs, extracted assets, or reference checkouts. It requires GNU Make and `arm-none-eabi-gcc` to build the GBA image. Host-side checks also require Clang and Python 3. mGBA is needed only to play or run emulator workflows; place `mGBA`/`mgba` on `PATH` or set `MGBA` to its executable path for the wrapper.

## Asset-free workflow

```sh
./dev doctor
./dev build
./dev test
./dev play starcraft
```

The build produces `build/StarCraft-GBA.gba`. You can also build that target directly with `make build/StarCraft-GBA.gba`. `./dev play starcraft` launches the ROM in mGBA and keeps local emulator state under `.local/`. The game code is in `starcraft/`; the GBA display and input support shared with it are in `common/`.

## Optional unified profile

The unified campaign/custom-game profile requires a user-supplied StarCraft 64 USA ROM, an entitlement to use it, and the pinned `sc64-maps` extractor. The extractor is GPL-3.0-or-later and is not vendored. The repository wrapper's `./dev sc64 prepare --rom PATH --profile unified` command checks the ROM and writes derived inputs under `.local/`; the compiled image and reports go under `build/`.

```sh
./dev sc64 prepare --rom "/path/to/StarCraft 64 ROM or ZIP" --profile unified
make unified test-unified
./dev play unified
```

The supported input is the verified USA image (or a ZIP containing it). Preparation reports validation errors if the input does not match. Do not commit or distribute the ROM, imported maps or art, generated headers, or captures containing game graphics. The clean asset-free build and its contributors do not require this optional workflow.

## References and scope

The reference revisions, files consulted, and reduced-mechanics notes are in [REFERENCES.md](REFERENCES.md). License status and provenance are in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). Build and test coverage is summarized in [VALIDATION.md](VALIDATION.md). The engine uses fixed-size data for GBA memory; campaign triggers, AI, unit systems, and presentation remain intentionally incomplete.
