# Fixed byte reader for typed unit saves

In the original normal P10 HOME replay, the victory transition takes
76,631,269 guest cycles in a bounded instruction trace at call 47,194.
The result autosave accounts for 71,802,053; repeated typed-unit decoding
accounts for 52,008,246 across save preparation and validation.
The per-bit field reader alone accounts for 27,156,609 cycles. In
comparison, the second canonical lookup performed during validation
accounts for 1,106,052 cycles. Keep that lookup and every validation
stage in this experiment.

`SC64_TYPED_UNIT_CHUNK_READER` substitutes fixed byte loads and shifts
for the per-bit loop in the shared 28-byte typed unit decoder. Each
read uses only the bytes spanned by that field, so an unaligned row is
supported without reading past the last byte. The field sequence,
marker, empty row, bounds, canonical rules and atomic publication are
shared with the original. Encoding, detached preflight, bank selection
and device writes do not change.

The asset-free ASan/UBSan differential compares 16,388 valid, mutated,
arbitrary, empty and invalid-dimension inputs with the original reader.
It exercises all 224 bit positions, unaligned rows ending at the
allocation boundary, byte-identical re-encoding of accepted rows and
unchanged output on failure. Both configurations also pass the existing
12,000 randomized codec pairs and boundary fixtures.

Run `make test-sc64-p7-typed-codec test-sc64-p7-typed-chunk-reader`.
For the asset-free ARM comparison, build `make sc64-typed-unit-arm-bench`
and run the two ROMs to the `bench_done` symbol, then read the nine
32-bit `bench_result` words from their matching ELFs. The words are
encode fault, valid cycles, valid digest, valid rejects, malformed
cycles, malformed accepts, changed outputs on rejection, row count
and completion marker `0xc001c0de`.

| Synthetic ARM workload | Original cycles | Byte reader cycles |
| --- | ---: | ---: |
| 2,880 populated rows | 39,041,201 | 13,633,841 |
| 2,880 rows failing the late path-timer bound | 45,549,764 | 20,119,364 |

Both valid runs produce digest 4,037,428,501 and reject none. Both
malformed runs reject all rows without altering the destination. These
measurements include the benchmark's loop and checks, with synthetic
canonical facts and no save-device or UI work. They are emulator
measurements, and do not predict a full autosave speedup. The probe
defaults off.

## Normal P10 SRAM compatibility

A fresh candidate process loads the existing pause save written by the
normal KODAK P10 cartridge at frame 36,761. The loaded world and all
five tail checkpoints match the original reader through victory at
frame 46,057. The entire 32 KiB result journal matches the original
fresh-resume journal byte for byte, including the untouched sequence-1
bank and the sequence-2 result save. This covers one authentic old
pause journal; it does not change or bypass detached bank validation.

Result journal SHA-256:
`c69ff8edddd990e94f3f9418e5b2198cb47e649a555afb36aca5d6f54de58cbf`.
Fresh-resume receipt SHA-256:
`df391ae95300ae8bf162d323626038a78478b7cd9c64006dd46285ed8f001bdd`.

## Normal P10 result latency

The reader-only P10 candidate on merged source `5877e6d` replays all
47,194 physical-button updates from HOME and wins at frame 46,057.
All 20 gameplay checkpoints, ten briefing records, 14 authored pages
and 21 paired prefix video/state samples match the normal KODAK
control. The uninterrupted pause and result SRAM exports are also
byte-identical to the existing KODAK pause and fresh-resume result
journals. The original bank is unchanged after the result save.

The KODAK loop counter at the result transition drops from 76,499,788
to 37,506,196 guest cycles (−50.97%). Comparing the bounded original
instruction trace above and the candidate trace at the same input,
the autosave scope drops from 71,802,053 to 33,715,475 cycles, and
typed-unit decoding from 52,008,246 to 13,921,619. The repeated
canonical lookup is still present and unchanged in cost. The largest
loop is now the existing 45,252,677-cycle depot stall. Full replay
mean moves from 1,907,674 to 1,906,514 cycles (−0.061%); this improves
save latency, while ordinary combat throughput is unchanged.

ROM verification passes; BSS and IWRAM ends are unchanged at
`0x0203c0e0` and `0x03004ab0`. This is one campaign and its matching
journal, not a full ARM matrix or hardware timing qualification.

Candidate ROM SHA-256:
`81b34c1b83b92e47fc2ed2a1de8060bd3e27adc46e1cf8eb3508bb21c2e031b5`.
Full candidate receipt SHA-256:
`3f84960d7c704619f415ac319590f16c2d6c7cac2f116def404fbaac1ee11de0`.

## Direct Showcase comparison

The normal staged Showcase control uses source `c906058`. The
candidate uses the same compiler arguments with only
`-DSC64_TYPED_UNIT_CHUNK_READER` added, immediately after the compiler
executable. Both ROMs verify as 32 MiB. IWRAM ends at `0x03004cf0`
and BSS at `0x02039c24` on both. Text shrinks by 392 bytes.

Both cartridges start the same physical Outpost match with a Terran
human and Terran, Zerg, and Protoss opponents. After 4,500 updates,
ordinary pause and save inputs create the first bank; after 600
more updates a second save creates the newer bank. The second
pause is at frame 5,101, with 51 live units and 32 buildings.
Both 32 KiB exports match byte for byte at each save. In each
export after the second save the complete first bank is unchanged.

Both cartridges load the same second-save journal in a fresh process.
The loaded world, indexed pages, palette, and seat assignments match
between cartridges, as do the same observations after 60 further
updates. The continued world matches the uninterrupted run. In a
separate fresh process, flipping one payload bit in the newest bank
without changing its checksum makes each loader select the original
saved world in the older bank. The paired fallback observations match.
Neither load rewrites its input journal. This fallback case tests
checksum rejection; the malformed typed-row differential above
separately tests decoder rejection.

| Ordinary button update | Original cycles | Byte reader cycles |
| --- | ---: | ---: |
| First pause save | 10,481,730 | 8,845,385 |
| Second pause save | 23,583,390 | 18,842,364 |
| Fresh load from the second bank | 22,575,641 | 18,853,101 |
| Fresh load with the newest bank damaged | 21,841,966 | 18,555,850 |

These are guest cycles for the complete button updates. The second
save is 20.1% shorter and the ordinary fresh load is 16.5% shorter.
The result qualifies one populated Showcase custom save path; it is
not a hardware timing measurement.

- Control ROM: `d5b983eaa42c499664605c3404144566a5655f3ead5ebb807e79fc80b8eb464b`.
- Reader-only ROM: `5b9cf1ec7cee3809cec4a84130e60d656a26d9a2c4706d6c83f793bb98d33adb`.
- First / second journal: `0dd3a27af39186ddc1355e20e0c6c68a88ba4336be319aa483eb6d05bf2374ad` /
  `7e8471dd38ad8586c7962521b1babaaf9a979c090c1bf12130fe979b84acde4a`.
- Paired receipt: `2f889436bf3abda8ea7e1158fc8bbf3d96cb39748f3d2445ec061978dcc27ee7`.
