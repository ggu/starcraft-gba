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
