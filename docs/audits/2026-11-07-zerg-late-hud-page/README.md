# Z9/Z10 page-resident HUD

The normal Z9 and Z10 vectors now use the existing `SC64_HUD_VRAM_BANDS` option. Instead of keeping 12,480 bytes of copied HUD bands in EWRAM, the renderer reuses pixels already present in the two Mode 4 framebuffer pages, tracking page versions and clear generations. `ZERG_Z9_HUD_PAGE_FLAGS=` is available for same-source OFF comparisons; Z10 inherits the Z9 setting. No actor, impact, SRAM format or guard is changed, and normal builds do not enable experimental research or Flash flags.

At runtime source `fae57fd81bfdc2fe282c1d9863581825dee30b33`, guarded normal ARM links increased both Z9 and Z10 EWRAM margin from **780 to 13,244 bytes**, a net saving of **12,464 bytes**. IWRAM margin stayed 2,072 bytes. Same-source normal Terran, P10, Showcase and Z7 OFF/ON control binaries were byte-identical. Four independent poisoned-page HOST coverage runs, Z9/Z10 each OFF and ON, passed their full-clear framebuffer checks. The compiler-vector audit confirmed each late-Zerg pair differed only by the HUD flag and output paths. All 593 prepared input hashes remained unchanged. Build receipt SHA-256: `a0ef54dd679623a3df5b926cb8c95d7b3c10b77db2df15ec6117eb408be074e9`.

An asset-free checkout at the same source passed `python3 tools/check_source_tree.py`, `./dev doctor`, `./dev build` and the full `./dev test` (455.5 seconds), with no detected sanitizer diagnostics. Default-check receipt SHA-256: `404b332aa7f7cc9295ab199c023dc074388e7532e947ffe47a20ded249257393`. Prepared optional `test-unified` inputs are incomplete at the default BC cover and P5 sprite paths; it was not run. Do not regenerate private assets to fill this gap.

The physical-input comparator (`tools/sc64_zerg_normal_hud_pair.py`) used separate OFF/ON normal ROMs, the audited read-only mGBA observer, and a virgin 32 KiB SRAM backing. It compared the complete displayed 240x160 indexed frame, 512-byte palette and named world, cursor, selection, menu and toast state at every observed physical step. For each case the guests created identical checksum-valid normal SRAM images, then separate fresh processes loaded those images. At the L-load boundary before release, the named serialized world and cursor matched the saved checkpoint; UI/selection/toast and load-recomputed fog/camera were compared strictly OFF versus ON but recorded separately across the save/load boundary. Each pair continued for 60 further neutral steps. No guest memory or registers were injected and no journal was retagged.

| Product and scene | Physical steps | Result receipt SHA-256 |
| --- | ---: | --- |
| Z9 custom map 0 | 721 | `7eefe256596bb361c217cddff533082dd22c6bdbf7b41700e9ad2c4584276c7c` |
| Z9 custom map 1 | 723 | `4e1a7fa34f9f5b74dd5d0a59ad83a96b64d090532a4d4fd0130f5774c1d77ec9` |
| Z10 custom map 0 | 721 | `d8155ecc14ec8d31dd9bd9bc2accae33d515aa4a85dd0b715f83644dce51533d` |
| Z10 custom map 1 | 723 | `97b35c74be7347ee8034e34aaa5d25554f7ba069df1a295e66fc26e2f0bc5373` |
| Z9 campaign mission 9 | 717 | `12c42c4566bd53a3f7af4a1304edd9bc2322d2ad826877b68c3345613b475364` |
| Z10 campaign mission 10 | 719 | `7b59736d084b87bfccd110705d52d21f95d429e616e02ef6cb334e8dc76aa4b0` |

At the observed save and load checkpoints all six cases had no selected building. They do **not** independently witness restoration or reset of a selected building. An earlier experimental-Q8 probe did capture a selected Forge in an uninterrupted reference and no selection after load; its strict cross-path pixel failure is preserved separately and is not reclassified as a pass here. An initial normal-comparator attempt with runner `4199...` stopped before its first lobby checkpoint because that runner denied read-only DISPCNT access; the actual observations used the previously audited bounded observer `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d`, with physical guest writes denied. Normal driver source for the first guest was `3b5fadc1`; the remaining five used map-option successor `0f4df204`.

The measured cycles include menus, pauses and save operations. They do not establish an FPS result. These bounded scenes do not establish campaign wins, arbitrary save points, runtime stack high-water, hardware behavior, audio continuity or full-profile gameplay. Raw frames are compared in the guest checks; private artifacts, logs, SRAM files and ROMs remain outside version control.
