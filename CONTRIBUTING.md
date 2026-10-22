# Contributing

The default skirmish is designed for asset-free contributions. You can edit and build the original-art game without any StarCraft installation or ROM:

```sh
./dev doctor
./dev build
./dev test
```

To play it, install mGBA and run `./dev play starcraft`. Build only the StarCraft ROM with `make build/StarCraft-GBA.gba` if you do not need the wrapper. See [the game guide](starcraft/README.md) for controls and mechanics.

The optional unified profile needs a verified USA StarCraft 64 ROM that you are entitled to use, plus the external extractor described in [Development](docs/DEVELOPMENT.md). Keep the ROM, converted assets, generated headers, captures, and saves in the ignored local directories. Do not attach or commit ROMs, game assets, extracted data, screenshots containing imported game graphics, or save files. Reports should include the commands run, relevant source changes, and concise test output without proprietary content.

The project's overall source license is pending the owner's decision. The optional importer has an SPDX GPL-3.0-or-later header and its dependency is separately GPL licensed; see [Third-party notices](THIRD_PARTY_NOTICES.md). Do not add a license declaration or change the importer's license without coordinating with the project owner.
