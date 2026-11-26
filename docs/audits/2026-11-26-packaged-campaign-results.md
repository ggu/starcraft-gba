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

## All thirty missions played on the packaged release

Every chapter on the `51c7f18f` release reached victory in mGBA under
ordinary input. Each chapter either consumed the preceding packaged victory
or an independently earned standard-cartridge save whose bytes matched it.
The first chapter of each race started from HOME without a save. After every
victory, a fresh emulator process loaded the result and exported the same
32 KiB SRAM as the corresponding standard-cartridge run. These were chapter
replays with verified handoffs, including the staged pause saves in the
longer routes.

| Cartridge | Chapters played | Verified terminal SRAM for the finale (SHA-256) |
|---|---:|---|
| Terran10 | 10 | `147b1f62ca27455d99094ae8f16f7d55df14094f7b4ddbbe49a4c88ca4063eb4` |
| Zerg10 | 10 | `1123eb7c9e8977228e33d24db85f14953a202da2a4c731bfdb4b6d940768b954` |
| Protoss10 | 10 | `020cde76c8fd953851c7eb1ec2282cad9d0990486079b68aebee9a82565e9e86` |

The private combined receipts are retained as
`packaged-51c7f18f-all-30-replays.json`. Their SHA-256 is
`795756b148f0d2f13fef84d5824962b5b07d5f010857fbdede0deb6e78d7aad4`.

## Release with defeat Overview and unknown-terrain drawing

Release `b7e69893` includes the shared Custom Game defeat Overview and
the opaque fill for wholly unknown terrain. All 30 previously earned
victories fresh-loaded and advanced on the corresponding updated race ROM,
including all three finale retries. Comparing against the standard ROMs
matched the observed world, re-exported SRAM and next-chapter state.

Each of the four updated ROMs ran 27 Custom Game cases: every human race
against one, two and three explicitly selected CPU races, on Outpost and
on Blood Bath under both Free for All and Melee. Each case saved at 60
updates and matched 60 further inputs after a fresh reload. A passive
Protoss human also lost a three-AI Blood Bath match at frame 15,348. The
result loaded in a fresh process, A opened Overview, holding A preserved
the screen and the next A restarted with the same seats. The older Terran
defeat save retained the same behavior. Terran T10 played through victory
and reproduced the earlier finale SRAM.

These checks used mGBA. The 30 full campaign replays above apply to the
preceding `51c7f18f` package, and the GBA SP has not been timed for this release.

| Product | Cartridge SHA-256 |
|---|---|
| SC TERRAN 10 | `8f3ca435bd7b2c901e0205bda22f2475a1b8c2a92322fedefc44d3e454dc5f9e` |
| SC ZERG Z10 | `46accd07a40c6b655709947c62ac50522ca7973d9c4a5724b6608080dc8c5cb0` |
| SC PROTOSS10 | `705f79cdc4b24565eb69e856181c1b28a57c81c091e097036fbe49f38d2488e8` |
| SC SHOWCASE | `ac97b2480b602c72e93bfa767fe68db01c482d3374d300e9f22c2649435c72ac` |

## Release with Z10 ARM-mode steering

Release `c23009f8` adds #958 to the guarded Zerg10 build. The other three
packaged binaries are byte-identical to `b7e69893`. All 30 campaign victory
comparisons were repeated. The updated Zerg ROM separately passed all 27
Custom Game launch/save/continuation checks. The crowded Outpost three-AI
sample used 1.21% fewer whole-loop cycles; the two Blood Bath windows improved
by 0.39% and 0.45%. All three matched gameplay checkpoints, rendered page
contents and fresh SRAM. These workload-specific improvements leave the
30 fps target open. See [#958](https://github.com/ggu/starcraft-gba/pull/958).
