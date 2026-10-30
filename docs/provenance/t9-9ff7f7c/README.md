# T9 qualification build provenance (runtime 9ff7f7c)

This folder records the executed builds of the two ROMs behind the T9 record
in `docs/VALIDATION.md`: the canonical Terran ROM and the T9 normal-menu replay
ROM. It applies only to runtime `9ff7f7c` (`codex/source-release`), as merged
into the PR branch. It does not describe later runtimes.

Produced on 2026-10-29 at commit `70b86da7111c9ce0e838078de4490f1ba719dbbf`
with no tracked changes in `starcraft/`, `common/`, `tests/`, `tools/` or the
`Makefile`. `git diff 9ff7f7c 70b86da -- starcraft common` is empty. The
`Makefile` differs from `9ff7f7c` only by the added T9 replay rules; the
canonical Terran recipe is unchanged.

```sh
python3 tools/sc64_t9_build_provenance.py --out build/t9-provenance \
  --expect starcraft-terran=685788ff1909fd532d4d128e18d5bd4c108b13b581c4d248672e9900783954df \
  --expect starcraft-terran-t9-arm-replay=3a8986dcf5e71e1c4aef7d984e883904ad80cf2e912ccb8740da40e0f19d5276 \
  --retained starcraft-terran=build/starcraft-terran.elf.qualified \
  --retained starcraft-terran-t9-arm-replay=build/starcraft-terran-t9-arm-replay.elf.qualified
```

## How it was captured

The tool deletes the two ELFs and runs the real `make` targets
`build/StarCraft-Terran-GBA.gba` and `build/StarCraft-Terran-T9-ARM-Replay.gba`.
It holds the host controller binary, trace and tape with `make -o`, and refuses
to run if a dry run lists any recipe other than the two links, `size`,
`objcopy` and `tools/fix_rom.py`. `CC` is a wrapper that records the driver's
argv and then runs `arm-none-eabi-gcc -v -MD -wrapper SUB` with that argv
unchanged. `SUB` records each subcommand's argv exactly as executed. It changes
one thing: the `-MD` file of each `cc1` is renamed per unit, so every unit keeps
its own dependency file. Without the rename, all units write to the same
`build/<elf>.d`.

Both rebuilt ROMs match the qualified ROM hashes byte for byte. The rebuilt
ELFs differ from the retained ELFs that produced the qualifying ROMs in 12 bytes, all
in `.strtab`. Those bytes are two local `FILE` symbols that the driver names
after its random temporary objects (`ccXXXXXX.o`). Every other section,
including code, data, IWRAM and all DWARF sections, is identical; see
`*/retained-elf-compare.txt`. This is why the ELF hash changes on every build
while the ROM hash does not.

## Files

Per ROM (`starcraft-terran/`, `starcraft-terran-t9-arm-replay/`):

| File | Contents |
| --- | --- |
| `commands/00-driver.args.txt` | The `arm-none-eabi-gcc` argv make passed, one argument per line, in order |
| `commands/NN-<tool>-<unit>.args.txt` | Each executed `cc1`, `as` and `collect2` argv in execution order. `cc1` receives the `-I` options before the `-D` options, plus driver-added `-D__USES_INITFINI__`. |
| `tu-flags.txt` | The ordered `-D`, `-U`, `-I` and `-include` options of each `cc1` (extracted from `commands/`) |
| `deps/<unit>.d.txt` | Raw `-MD` output of each preprocessed unit (C and `audio_bank.S`); `start.s` is assembled directly and has none |
| `inputs-sha256.txt` | SHA-256 of every consumed input: each `.d` prerequisite, the sources, `common/gba.ld`, the `.incbin` audio blob, the linked `libgcc.a`, the `Makefile` and `tools/fix_rom.py`, plus the trace and tape for the replay |
| `inputs-kind.txt` | Why each input is listed |
| `toolchain-sha256.txt` | The executed `gcc`, `cc1`, `as`, `collect2`, `ld`, `size` and `objcopy` binaries |
| `driver-v-commands.txt` | The command and include-search lines printed by `gcc -v` (diagnostics omitted) |
| `outputs-sha256.txt` | The rebuilt ROM, ELF and link map |
| `retained-elf-compare.txt` | Byte, section and symbol differences from the retained ELF |

`make-log.txt` holds the recipe lines make executed (the wrapper is shown as
`CAPTURE_CC`). `toolchain.txt` lists the tool versions and the capture commit.

Paths are as the compiler saw them, so some headers appear as
`tests/../starcraft/...`. From the repository root, with the private inputs
prepared, `shasum -a 256 -c docs/provenance/t9-9ff7f7c/*/inputs-sha256.txt` checks them. The private inputs
themselves stay out of Git; only their paths and hashes are recorded here.
