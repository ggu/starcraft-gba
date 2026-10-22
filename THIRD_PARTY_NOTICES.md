# Third-party notices and provenance

This project uses external projects as research references. Their source checkouts are not vendored in this repository. These references do not imply that their source code is copied into the default StarCraft game.

| Project | Pinned revision | License/provenance |
| --- | --- | --- |
| [BWAPI](https://github.com/bwapi/bwapi) | `d727fed68558c506163048ea889131d8cbb33915` | LGPL-3.0 with additional permission, as identified by its included license files. Used as a reference for unit/weapon identities and default numeric values. |
| [OpenBW](https://github.com/OpenBW/openbw) | `4b046d5f65302b10cb0a745f0fecd37ec85b20a8` | No root license file was present in the inspected snapshot. Used as a behavioral and format reference; do not copy its implementation. |
| [sc64-maps](https://github.com/dmang-dev/sc64-maps) | `09b57f787a247107d1de82b8c5eab2734667ea1d` | GPL-3.0-or-later extraction tools. Used by optional ROM preparation. Its source is external and is not bundled here. |

`tools/sc64_import.py` is marked `GPL-3.0-or-later` in its file header and imports the external `sc64-maps` extractor. The full GPL text is included at [LICENSES/GPL-3.0-or-later.txt](LICENSES/GPL-3.0-or-later.txt). Preserve that notice and license when redistributing the importer. Optional generated outputs are derived from user-supplied game data and are not distributed by this project.

`docs/reference-stats.json` records sourced values and identifiers used during implementation. See [docs/REFERENCES.md](docs/REFERENCES.md) for source locations and interpretation. These gameplay facts and identifiers are provenance data, not copied BWAPI implementation code.

`tools/fix_rom.py` writes the standard 156-byte Nintendo Game Boy Advance boot logo and cartridge header sequence into generated ROM images. This hardware boot metadata is distinct from the game's authored art and is excluded from the project's GPL-3.0-or-later grant for authored files. The project does not claim ownership of or grant rights to that Nintendo material.

Project-authored files, including the game and shared platform code, are licensed under GPL-3.0-or-later; see [README.md](README.md#license) for the grant and [LICENSE](LICENSE) for the full terms. Existing file-level notices remain in force. This grant does not relicense third-party material or claim rights over every byte in generated ROMs. None of the source-code licensing described here grants rights to StarCraft or StarCraft 64 ROMs, artwork, maps, sounds, or other game content.
