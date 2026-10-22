# Source snapshot provenance

This StarCraft-only repository starts from the authored StarCraft and shared
GBA code in the local `blizzard-gba` workspace at commit
`d802ac597cca9a294834d19a815f03616af6ca22` (2026-10-22).

The export includes the latest six-mission combined engine, custom games,
selection gestures, illustrated-menu renderer, import/conversion tools and
handheld display palette. The default original-art skirmish is independently
buildable without original game data.

This is a new source-only history. It omits the Diablo project, research
checkouts, original ROM, extracted maps/art, generated headers, compiled ROMs,
screenshots, save files, emulator reports and machine-specific SD-card setup.
The build entry point and project documentation have been adapted to this
standalone repository.

Original-game reference commits and relevant source locations are recorded
in `docs/REFERENCES.md` and the mission guides. External dependencies retain
their own licenses; see `THIRD_PARTY_NOTICES.md`.
