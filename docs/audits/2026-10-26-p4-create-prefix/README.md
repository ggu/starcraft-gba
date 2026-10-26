# P4 create-action validation cost

This follow-up is based on main `4b23a51`, including the stock-profile,
Photon Cannon and source construction-art corrections. All checks run locally;
GitHub Actions remains disabled and manual-only.

P4 validates dispatched create actions repeatedly during ordinary updates.
The generator now emits 154 read-only prefix endpoints for the 153
authenticated action rows. Their final count is 87 opcode-11 rows. This counts
each dispatched row once, independently of its quantity modifier, exactly as
the previous validator did. Two bounded lookups replace the repeated row scan.
The validator is also placed in IWRAM through the existing hot-code mechanism.
It retains every validation call, state check and rejection order.

Initialization checks the source action bounds before reading the prefix,
contiguous trigger ranges, every prefix edge and the total count. The generated
version and endpoint count have compile-time guards. The focused fixture checks
every endpoint and valid trigger cursor against an independent slow recount,
and rejects altered create counts. Synthetic generator tests distinguish row
count from action quantity. The old scan remains available with both flags off.

The prefix adds no actor or save fields, BSS, pool capacity or save generation.
Only canonical P4 enables the new flag group; legacy P3, Terran and Zerg retain
their existing selections. Existing prepared workspaces should regenerate the
P4 scenario header as described in [Development](../../DEVELOPMENT.md).

## Paired normal-input replay

The baseline filters only `SC64_P4_CREATE_PREFIX` and
`SC64_P4_VALIDATOR_HOT` from the actual canonical `PROTOSS_ROM_FLAGS`.
The candidate uses those flags unchanged. Both use the same source, authenticated
inputs, fixed input tape and private timing wrapper. Both isolated, no-GDB mGBA
runs consume all 1,990 transitions across 104,850 updates and reach the native
P4 win at frame 104,346, with 87 creates, fired bits 8,184 and no fault.
Terminal unit, building and P4 runtime hashes match exactly:
`283197403`, `3409887890`, `2439291336`.
An additional host-only state oracle compares 209 checkpoints at 500-update
intervals; every checkpoint matches between the two flag selections.

| Emulator cycles | Baseline | Prefix and hot validator |
| --- | ---: | ---: |
| Weighted full-update mean | 1,025,871 | 1,003,726 |
| Weighted simulation-update mean | 593,464 | 576,485 |
| Maximum complete 1,000-update window mean | 1,383,430 | 1,375,284 |
| Maximum individual full frame | 21,347,305 | 21,347,331 |

Means weight 104 complete 1,000-update windows and the final 850-update window.
Full cost falls 2.16%, simulation cost 2.86%. The equivalent full-replay rate
changes from about 16.35 to 16.71 fps. The maximum individual frame is effectively
unchanged; the change does not solve the large navigation hitch or establish
30 fps or physical-GBA performance.

The source scenario SHA-256 is
`40f30ccedf87a3b9c88308dc3092611ceff10185ddaf32471104902042a93ecd`;
the input header SHA-256 is
`335c2331442a1f6b3f0d686c2984819784d1af849d1eff984ac3ad4c34d190f2`.
The regenerated scenario header SHA-256 is
`a0c247f88c6fa1a8a6b925f01b1663ffbba883bb005c56f1921e4c9f52d8e6c7`.
Private wrapper, ROM, log and flag manifests remain under
`.local/build/p4-create-prefix-public/`; they are not source artifacts.

Fresh-process ARM save continuation remains pending. Earlier failed ARM suffix
checks are discarded: a copied nested wrapper's quoted relative include fell
through the compiler's search paths into an older checkout. Hashing the intended
source did not establish what the compiler actually read. Replacement wrappers
use verified source dependencies; private wrappers should use absolute source
paths or retain compiler dependency output.

The original tape checkpoint also depended on building selection and a held
gesture, which loading intentionally resets. A neutral checkpoint at update
35,500 / frame 35,308 avoids those assumptions. Its preserved 32-KiB SRAM
loads in a fresh host process through title-screen B; all 138 remaining
500-update world checkpoints and the strict final win match uninterrupted
play. The corresponding correct-source ARM check is still required before
this draft qualifies fresh-process continued-win behavior.

## Local regression

- `./dev build` and `./dev test`: 493 Python checks plus asset-free host fixtures.
- `make unified test-unified`: passed.
- `make test-protoss test-canonical-render`: passed, including the native P4
  controller win, malformed-state checks and the new prefix differential fixture.
- `make race-campaigns showcase`: all four ARM links passed existing reserves.

The Cannon test's Make dry-run parser now joins continued recipe lines before
shell tokenization; pixel assertions are unchanged. No private assets, ROMs,
generated headers or screenshots are committed.
