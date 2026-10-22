# Contributing

The default skirmish is designed for asset-free contributions. You can edit and build the original-art game without a StarCraft installation, ROM, extracted assets, reference checkout, or emulator. Follow the clone and tool installation steps in the [README](README.md) and [Development](docs/DEVELOPMENT.md), then run:

```sh
./dev doctor
./dev build
./dev test
```

The build creates and verifies `build/StarCraft-GBA.gba`; tests run host-side checks. mGBA is needed only when you want to play: install it as described in [Development](docs/DEVELOPMENT.md), then run `./dev play starcraft`. See the [game guide](starcraft/README.md) for controls and mechanics.

The optional unified profile needs a verified USA StarCraft 64 ROM that you are entitled to use. Its first preparation fetches the pinned `sc64-maps` extractor and requires Git and network access. See [Development](docs/DEVELOPMENT.md) for supported input formats and the ROM check. Keep the ROM, converted assets, generated headers, captures, and saves in ignored local directories. Do not attach or commit ROMs, game assets, extracted data, screenshots containing imported game graphics, or save files. Reports should include the commands run, relevant source changes, and concise test output without proprietary content.

The project's overall source license is pending the owner's decision. The optional importer has an SPDX GPL-3.0-or-later header and its dependency is separately GPL licensed; see [Third-party notices](THIRD_PARTY_NOTICES.md). Do not add a license declaration or change the importer's license without coordinating with the project owner.
