# Development

The default StarCraft skirmish builds without ROMs, extracted assets, or external reference checkouts. The required tools are Git, GNU Make, `arm-none-eabi-gcc` with its `objcopy` and `size` tools, Clang, and Python 3.10 or newer. The project uses Python's standard library; no `pip install` step is needed. mGBA is only needed to play or run emulator workflows.

## Install the tools

### macOS

Install Apple's Xcode Command Line Tools for Clang, Make, and Git, then install the ARM toolchain and Python with Homebrew:

```sh
xcode-select --install
brew install arm-none-eabi-gcc python
```

Homebrew's [ARM GCC formula](https://formulae.brew.sh/formula/arm-none-eabi-gcc) provides the `arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, and `arm-none-eabi-size` commands. If you want to play, install the optional [mGBA app](https://formulae.brew.sh/cask/mgba-app):

```sh
brew install --cask mgba-app
```

The wrapper checks `PATH` and also looks for `/Applications/mGBA.app/Contents/MacOS/mGBA`.

### Ubuntu 24.04

Install the build and test tools with the system package manager:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends gcc-arm-none-eabi binutils-arm-none-eabi clang make python3 git
```

To play, install the optional mGBA SDL package from Ubuntu's [package archive](https://packages.ubuntu.com/search?keywords=mgba):

```sh
sudo apt-get install mgba-sdl
```

On Ubuntu, the emulator executable may be `/usr/games/mgba`; if it is not on `PATH`, set `MGBA=/usr/games/mgba` when running the play command. If the package is unavailable for your Ubuntu release, use the package search page to check the release and enabled repositories.

Other systems need equivalent versions of these tools. `./dev doctor` reports any missing required command. To check the optional emulator too, use `./dev doctor --emulator`.

## Asset-free workflow

After cloning as described in the [README](../README.md), run:

```sh
./dev doctor
./dev build
./dev test
```

Doctor should print `OK` for ARM GCC, ARM objcopy, ARM size, Make, Clang, Python 3, and Git. Build creates `build/StarCraft-GBA.gba` and verifies the GBA ROM. Test runs the asset-free host checks and exits with status 0 when they pass. `make build/StarCraft-GBA.gba` builds the ROM without the wrapper; `make verify` builds and verifies it.

Playing is optional and requires mGBA:

```sh
./dev doctor --emulator
./dev play starcraft
```

The wrapper launches the built ROM and keeps local emulator state in `.local/play/starcraft/`. The game code is in `starcraft/`, including `starcraft/game.c`; GBA display and input support is in `common/`. Host-side behavioral tests are in `tests/`, and build, conversion, validation, and emulator helper scripts are in `tools/`.

## Optional unified profile

The unified campaign/custom-game profile requires a user-supplied StarCraft 64 USA ROM that you are entitled to use. The supported input is a `.z64`, `.v64`, or `.n64` file, or a ZIP containing exactly one 32 MiB N64 image. Preparation normalizes byte order and verifies SHA-1 `a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`. It does not download a ROM. The pinned `sc64-maps` extractor is GPL-3.0-or-later and is not vendored; the first preparation clones it from its public upstream into ignored `references/`, which requires Git and network access.

```sh
./dev sc64 prepare --rom "/path/to/your/StarCraft 64 ROM or ZIP" --profile unified
make unified
make test-unified
./dev play unified
```

Preparation writes extracted and generated inputs under ignored `.local/`; ROM builds and reports go under `build/`. Do not commit or distribute the ROM, imported maps or art, generated headers, or captures containing game graphics. Default-profile contributors do not need this workflow.

See [SC64 unified](SC64-UNIFIED.md) for implemented missions and limitations, and [SC64 development notes](SC64.md) for import details.

## T4 internal actor/world checks

The Jacobs Installation data and actor rules have an internal, gated host-test
path. This is not a playable campaign entry: the frontend catalog remains
closed to T4, and no frame-rate result is claimed. After preparing the local
unified inputs, run:

```sh
make test-sc64-t4-actors
make test-sc64-t4-storage sc64-t4-storage-probe
```

The actor tests load the 95 source placements, check supported actor roles,
sight, combat and bounded rescue behavior, and validate the special Beacon's
save representation. Storage tests cover the T4-only 128×128 layout. Both
targets use private generated inputs under ignored `.local/` storage. The
scenario still lacks implemented doors, traps, teleports, trigger objectives
and completion behavior. See [Validation](VALIDATION.md) for the exact test
coverage and limits.

The private numeric scenario import and the asset-free trigger/query core have
separate checks:

```sh
make sc64-t4-scenario test-sc64-t4-scenario
make test-sc64-t4-triggers test-sc64-t4-queries test-sc64-t4-actions
```

The first line needs the retained records produced by local race preparation.
Re-run preparation after updating an older checkout: T4 retention now includes
FORC, OWNR, SIDE, UPRP and UPUS, in addition to the original eight sections.
It reconstructs the full source sections from the generated C schema and
checks their canonical digests. The second line needs no ROM or generated
data and is included in `./dev test`. The runner is not connected to T4
gameplay yet: world effects, SRAM integration, door/teleport behavior and a
controller completion proof remain required. T4-only door/trap rules and a
private source-frame accessor are available for that integration; neither
door animation/collision nor trap firing is connected yet.

## Troubleshooting

- If `doctor` reports a missing required tool, install that tool and confirm its command is on `PATH`. ARM GCC also needs the matching `arm-none-eabi-objcopy` and `arm-none-eabi-size` commands. On a custom toolchain prefix, set `PREFIX` to the prefix used by all three commands.
- Cloning the public repository over HTTPS needs no GitHub login. If cloning fails, check the repository URL and network access. Pushing changes requires write access and authentication; for HTTPS, use `gh auth login` and `gh auth setup-git`, or configure a GitHub SSH key for SSH.
- If `./dev sc64 prepare` rejects the ROM, make sure it is the supported USA image and that a ZIP contains exactly one 32 MiB N64 image. The normalized SHA-1 must match the value above. The project cannot supply or identify a ROM for you.
- If an optional profile build says generated headers are missing, prepare that profile first with `./dev sc64 prepare --rom PATH --profile PROFILE`. These generated inputs are private local data and are not checked into the repository.
- If `./dev play` cannot find mGBA, install it or set `MGBA` to the executable path. For example, on Ubuntu: `MGBA=/usr/games/mgba ./dev play starcraft`.

## References and scope

Reference revisions, files consulted, and reduced-mechanics notes are in [REFERENCES.md](REFERENCES.md). License status and provenance are in [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md). Build and test coverage is summarized in [VALIDATION.md](VALIDATION.md). The engine uses bounded, fixed-size data for GBA memory. Campaign triggers, AI, unit systems, and presentation remain intentionally incomplete.
