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

## Late missions played on the packaged ROMs

Starting from earned predecessor saves, the following missions ran through victory
in mGBA using ordinary controller input on the packaged ROMs at `51c7f18f`.
Each victory was loaded in a fresh process. The exported SRAM matched the
corresponding victory earned on the standard cartridge byte for byte.

| Mission | Winning frame | Matched checkpoints |
|---|---:|---:|
| T9 | 49,486 | 54 |
| T10 | 3,361 | 6 |
| Z8 | 70,229 | 29 |
| Z9 | 67,745 | 30 |
| Z10 | 57,633 | 26 |
| P9 | 101,649 | 43 |
| P10 | 53,225 | 22 |

The P10 run consumed the P9 save exported by the packaged ROM. T10, Z9 and Z10
consumed standard-cartridge predecessor journals that matched the separately
replayed packaged victories byte for byte. See below for additional packaged mission runs.

## Early mission pacing on the packaged cartridges

The packaged Terran ROM started T1 from erased SRAM and used each exported
victory to start T2 through T4. T5 loaded the identical T4 journal from
the standard cartridge and completed its 39-stage paid-production route.
The packaged Zerg ROM also completed Z3 from an earned Z2 victory.
All six fresh victory exports matched the standard cartridge byte for byte.

| Chapter | Main-loop updates | Standard updates/s | Packaged updates/s |
|---|---:|---:|---:|
| T1 | 10,195 | 29.46 | 29.46 |
| T2 | 29,414 | 29.48 | 29.48 |
| T3 | 54,022 | 29.18 | 29.18 |
| T4 | 8,975 | 24.60 | 24.60 |
| T5 | 146,309 | 24.59 | 24.59 |
| Z3 | 69,243 | 23.51 | 23.51 |

The emulator measured guest-clock cycles across each recorded route, including
menu, save and load inputs. T1–T3 approach 30 main-loop updates/s; T4, T5
and Z3 are slower. The figures do not measure display refreshes or GBA SP
hardware and do not predict a crowded three-AI match.
