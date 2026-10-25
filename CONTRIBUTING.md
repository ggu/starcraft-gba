# Contributing

The default skirmish is designed for asset-free contributions. You can edit and build the original-art game without a StarCraft installation, ROM, extracted assets, reference checkout, or emulator. Follow the clone and tool installation steps in the [README](README.md) and [Development](docs/DEVELOPMENT.md), then run:

```sh
./dev doctor
./dev build
./dev test
```

The build creates and verifies `build/StarCraft-GBA.gba`; tests run host-side checks. mGBA is needed only when you want to play: install it as described in [Development](docs/DEVELOPMENT.md), then run `./dev play starcraft`. See the [game guide](starcraft/README.md) for controls and mechanics.

The optional unified profile needs a verified USA StarCraft 64 ROM that you are entitled to use. Its first preparation fetches the pinned `sc64-maps` extractor and requires Git and network access. See [Development](docs/DEVELOPMENT.md) for supported input formats and the ROM check. Keep the ROM, converted assets (including the sound and music bank), generated headers, captures, and saves in ignored local directories. Do not attach or commit ROMs, game assets, extracted data, screenshots containing imported game graphics, audio recordings of imported sound or music, or save files. Reports should include the commands run, relevant source changes, and concise test output without proprietary content.

Project-authored files are licensed under the GNU General Public License, version 3 or any later version (`GPL-3.0-or-later`); see [LICENSE](LICENSE). Submit contributions under the same license and preserve existing copyright and license notices. Third-party material and external dependencies retain their own terms; see [Third-party notices](THIRD_PARTY_NOTICES.md). Do not include code or content that you lack permission to contribute under the applicable terms.
