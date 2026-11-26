# Campaign victory saves on the packaged defense-targeting build

The save compatibility checks use the race ROMs built at
`51c7f18f` and the standard cartridges qualified after #913. Each side
started at HOME in a fresh mGBA instance with the same earned SRAM.
The table lists the results available for comparison.

| Cartridge | Earned results checked | Next-chapter behavior |
|---|---:|---|
| Terran10 | T1–T10 | T1–T9 advanced one chapter. A at T10 restarted T10 at frame zero. |
| Zerg10 | Z1–Z10 | Z1–Z9 advanced one chapter. A at Z10 restarted Z10 at frame zero. |
| Protoss10 | P1–P10 | P1–P9 advanced one chapter. A at P10 restarted P10 at frame zero. |

All 30 comparisons matched the observed terminal world, including live units,
buildings and their queues, mineral fields and resource balances. Each result
had the expected cumulative mission mask, victory flag, zero mission faults
and no defeat flag. Re-exporting immediately after HOME L reproduced the
original SRAM bytes on each build. Pressing A preserved those completion
bits on both builds. This covers loading and advancing an earned result.
Playing the intervening missions on the new cartridge is a separate check.

The final Zerg comparison restored 124 units and 135 buildings at frame
57,633. Its fresh SRAM SHA-256 was
`1123eb7c9e8977228e33d24db85f14953a202da2a4c731bfdb4b6d940768b954`.
A restarted Full Circle with the Zerg completion mask `0x3fc003`.

To repeat a comparison, use both ROMs' matching ELFs, the headless input
runner, a single cumulative victory journal and a fresh output directory.

```sh
python3 tools/sc64_compare_campaign_victory.py --race protoss --chapter 7 \
  --reference-rom "$OLD_ROM" --reference-elf "$OLD_ELF" \
  --rom "$NEW_ROM" --elf "$NEW_ELF" --runner "$RUNNER" \
  --victory "$P7_SAVE" --out "$OUT"
```
