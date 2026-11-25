# Terran campaign from blank SRAM after the Custom Game Larva merge

The regular Terran cartridge built from the game sources merged in #913
(`d6fd232e`, ROM SHA-256 `17f3b4ce1d02e4619cb82828d283bdd523df9e2fe8afe9d12613bfaa41ea86e7`)
won four consecutive chapters in mGBA. T1 started at HOME with erased SRAM.
Each later chapter started in a fresh process by loading the preceding
victory and advancing with A. Each mission included a mid-mission pause
save, a fresh load to continue play, and a separate HOME load of the result.
The result re-export matched the saved bytes in every chapter.

| Mission | Pause / result journal sequences | Win frame | Completion mask | Checked snapshots | Mean main-loop Hz |
| --- | --- | ---: | --- | ---: | ---: |
| T1 Wasteland | 1 / 2 | 10,120 | 0x10 | 12 | 29.46 |
| T2 Backwater Station | 3 / 4 | 29,366 | 0x30 | 31 | 29.48 |
| T3 Desperate Alliance | 5 / 6 | 54,000 | 0x70 | 56 | 29.18 |
| T4 The Jacobs Installation | 7 / 8 | 8,911 | 0xf0 | 10 | 24.60 |

T3 compared every recorded unit and building count, wallet, wave and
objective field at the countdown victory as well as its 54 periodic samples.
Older HOST traces remain readable and compare only the fields they recorded.
The rates come from emulated cartridge loop counters and include menus and
save/load. Displayed-frame rate on a GBA SP remains unmeasured.

## Repeat with prepared local game inputs

Build the standard Terran ROM and its matching ELF. Set `TERRAN_ROM`,
`TERRAN_ELF` and `PHYSICAL_OBSERVER` to their paths. Run the commands in
one Bash shell from the repository root. The output directory must be new.

```bash
set -e
work=$(mktemp -d)
make build/test-sc64-full-{t1,t2,t3,t4}-run
build/test-sc64-full-t1-run --trace "$work/t1.json"
build/test-sc64-full-t2-run --trace "$work/t2.json"
SC64_T3_TRACE_PATH="$work/t3.json" build/test-sc64-full-t3-run
build/test-sc64-full-t4-run --limit 30000 --trace "$work/t4.json"

cuts=(6222 18213 26993 4991)
previous=()
for chapter in 1 2 3 4; do
  python3 tools/sc64_t9_normal_replay.py --chapter "$chapter" \
    --trace "$work/t$chapter.json" --save-tick "${cuts[chapter-1]}" \
    --rom "$TERRAN_ROM" --elf "$TERRAN_ELF" --runner "$PHYSICAL_OBSERVER" \
    --out "$work/t$chapter" "${previous[@]}"
  previous=(--previous-victory "$work/t$chapter/victory-readback.sav")
done
```

The final T4 result SHA-256 was
`6901636bf2a2b5d523afbca3f69e9eb8fc2afad98a08e3389cf238873f2360b5`.
The runner SHA-256 was
`050e5e7dcab772798780a11d1c6c3b4b84135e4b681aa11e08f666738ebaa9bf`.
The verifier pins every input in each receipt. Local SRAM and original game
data stay out of the repository.
