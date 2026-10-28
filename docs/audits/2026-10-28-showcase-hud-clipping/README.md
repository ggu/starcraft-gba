# Showcase world/HUD boundaries

The Showcase profile now enables `GBA_SCENE_CLIP`, like the canonical race
profiles. `game_render()` restricts world rendering to rows 12 through 119,
then restores the full-screen clip before drawing the HUD. Unit, building,
vitals and effect blitters cannot write over either HUD band during that pass.

The prior Showcase configuration relied on drawing/copying the HUD after the
world. A simple completed-frame host reproduction retained an intact HUD;
the reported intermittent screenshot corruption was not independently
reproduced in a completed frame. However, the new regression fails against
the previous profile because unit drawing itself overwrites the HUD rows.
This change removes that unprotected intermediate write rather than claiming
to have established an additional cache-corruption mechanism.

`tests/sc64_showcase_game.c` exercises every compiled unit type at both map
edges, with four headings, and every building type. It checks poisoned HUD
bands byte-for-byte, requires visible world pixels, and compares the finished
resource band with a full redraw across alternating framebuffers. Both
packed and ordinary indexed sprite configurations pass under ASan/UBSan,
along with the existing scene, control and save-isolation tests. A private
host capture with tanks crossing both boundaries was visually inspected.

Clipping alone exceeded Showcase's IWRAM guard. The profile therefore also
selects `GBA_DIAMOND_ROM`, placing the infrequent procedural diamond helper
in ROM while keeping the imported sprite compositors in IWRAM. No reserve
was reduced. The imported audio-enabled ARM link ends BSS at `0x0203dfe0`
and IWRAM at `0x03004fa8`, below the existing `0x0203e000` and `0x03005000`
limits. The 32 MiB ROM has the expected `SC SHOWCASE` / `SCSH` identity,
header checksum and SRAM signature. This is a link and host-rendering
qualification, not a new hardware performance or campaign-playthrough claim.

Local validation: `./dev build`, `./dev test` (599 Python tests, 18 skipped,
plus the C checks), `python3 tools/check_source_tree.py`, and
`git diff --check`. Imported compiler commands were derived without running
Make recipes and executed with absolute runtime paths and read-only prepared
asset paths. No imported assets, captures or ROMs are included in this commit;
no GitHub Actions runners were used.
