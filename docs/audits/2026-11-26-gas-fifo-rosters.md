# Gas FIFO: normal Custom Game roster openings

The integrated P10 trial cartridge passed all 234 ordinary-button Custom Game opening and save cases: two maps, three selected human races, and each ordered selection of one, two or three Terran, Zerg and Protoss CPU races. This is normal Custom Game mode, including normal Blood Bath; Melee is outside this matrix. The ROM SHA-256 is `64b0d722b9eb7bc61af8c819bf4c4912cf723916b1bda6fc20b8e13fd7c75ffb`, identical to the documented post-integration P10 rebuild. The corresponding ELF SHA-256 is `3bb8bc13cb4916f80b118d30544ade8382ade85b1f08cf6346a9ce4932df7d2f`. The runner SHA-256 is `050e5e7dcab772798780a11d1c6c3b4b84135e4b681aa11e08f666738ebaa9bf`; all three hashes were unchanged across the run.

Each case starts through the physical lobby, verifies the occupied CPU rows, resolved races and start positions, advances 120 neutral updates, saves through the pause menu, reloads that SRAM in a fresh process and compares the saved named-world fingerprint. The fresh and uninterrupted processes then advance 60 updates and must agree, with SRAM unchanged by loading. The 234 unique cases and their individual checks are retained locally in a source-free JSON receipt (SHA-256 `cb7616f773eb44c131859e640758735ceaa791f83bfeb5e6649699da65a00c4e`). ROMs, ELFs and save images are not included.

Reproduce the same checks with a matching FIFO ROM, ELF and physical mGBA runner, using a new output directory:

```sh
python3 tools/sc64_fifo_roster_matrix.py --runner "$runner" --rom "$fifo_rom" \
  --elf "$fifo_elf" --output "$new_output_dir" --jobs 2
```

The opening saves are before refinery contention; they do not test queue ordering or active-service persistence. Those have separate [continuation evidence](2026-11-26-gas-saturated-fifo.md#experimental-gba-save-continuation). These cases do not establish full match wins, every sparse seating layout, Melee combinations or physical hardware behavior. The [full-match matrix](2026-11-26-gas-fifo-small-skirmishes.md) has a different, longer scope.
