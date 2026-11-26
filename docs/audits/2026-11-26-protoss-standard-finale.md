# Standard Protoss from the Homeland victory

## P8, The Trial of Tassadar

The same standard Protoss10 cartridge loaded the verified
[Homeland result](2026-11-26-protoss-p7-linked/README.md).
A on RESULT entered mission slot 27 at frame zero. Victory arrived
at frame 93,065 after 94,786 linked calls. All 38 checkpoints and 12
field dialogue pages matched. Fenix and the Hyperion survived. The
Stasis Cell was destroyed, campaign phase was 3, all three runtime
runners were fault-free, and completion mask `0xfc0000c` retained
all eight mission completions.

Fresh HOME L reproduced the winning world and the P8 runtime.
The independently rerun repository verifier produced the same result
and identical SRAM. Sequence 8 is the in-mission pause save at frame
30,611, and sequence 9 is victory. Both banks passed their checksums.

Regenerate the authored route using the standard prepared P10
source and build inputs at `82fc1a73`.

```sh
make build/px-p8-normal-home
./build/px-p8-normal-home p8-home.jsonl 2> p8-pages.log
```

Use the ROM, ELF and runner from the [P4–P6 qualification](2026-11-26-protoss-opening-continuation.md),
the exact P7 readback and a new output directory.

```sh
hash() { shasum -a 256 "$1" | cut -d ' ' -f 1; }
python3 tools/sc64_p8_linked_replay.py \
  --rom "$ROM" --rom-sha256 "$(hash "$ROM")" \
  --elf "$ELF" --elf-sha256 "$(hash "$ELF")" \
  --runner "$RUNNER" --runner-sha256 "$(hash "$RUNNER")" \
  --seed "$P7_SAVE" \
  --seed-sha256 e3f64eae475899f9e013988b0d93f16fecd42d21742e3876e9934016ba4aa1bc \
  --trace p8-home.jsonl \
  --trace-sha256 29894d507d3b850bca4bf1e2d8e63d6dd64da0bff372ec6ca71980896164d172 \
  --pages p8-pages.log \
  --pages-sha256 b0ec62378122e6513e91af87cedfd31aef65f90735e5b1e66fbc6e84929c359d \
  --out build/p8-linked-result
```

| Artifact | SHA-256 |
|---|---|
| P8 victory and fresh re-export | `49f80c1981a4fc57540dc184b113407cec7b2d394fcb75890e9111e0e866ee41` |
| Independent repository verifier receipt | `3a1aa9d7b30dc0a8d9968828e29220e7b0459cc15b736e13e398a67be3731429` |

## P9 from the P8 result

The public linked verifier carried the exact P8 readback into P9 at
frame zero. Victory arrived at frame 101,649 after 104,446 linked
calls. All 43 observations matched the regenerated controller.
Zeratul and Fenix survived. A fresh emulator restored the result and
exported identical SRAM with all nine mission completions in mask
`0x1fc0000c`. Both SRAM banks passed their checksums. Sequence 10 is
the in-mission pause save at frame 30,734 and sequence 11 is victory.

| Artifact | SHA-256 |
|---|---|
| P9 controller trace | `d39b718d5ab7456f46a21580455940a1d41ed028fcccbe35e5889d9d36c67721` |
| P9 victory and fresh re-export | `d7ae94c99ebf10bfe2cc719d8ddbc86e3c3192a40873abf4d5de1abe0dc24979` |

## P10 from the P9 result

The same verifier loaded the exact P9 readback, entered P10 at frame
zero and won at frame 53,225 after 54,445 linked calls. All 22 named
observations matched. The Overmind was destroyed and Zeratul,
Gantrithor and the Hyperion survived. The result registered no
unsupported actions. A fresh HOME L restored the winning world,
runtime and completion mask `0x3fc0000c`, covering all ten missions,
and re-exported identical SRAM. Both banks passed their checksums.
Sequence 12 is the in-mission pause save at frame 31,156, and
sequence 13 is victory.

Regenerate both tapes with the same prepared source and build inputs
as P8. Use the pinned standard ROM, ELF, observer and above P8 readback
with the existing public verifier.

```sh
make build/test-sc64-p10-p9-normal-home build/test-sc64-p10-normal-home
./build/test-sc64-p10-p9-normal-home p9-home.jsonl
./build/test-sc64-p10-normal-home p10-home.jsonl
python3 tools/sc64_protoss_linked_replay.py \
  --p8-save "$P8_SAVE" --p9-trace p9-home.jsonl --p10-trace p10-home.jsonl \
  --rom "$ROM" --elf "$ELF" --runner "$RUNNER" --out build/p9-p10-linked-result
```

| Artifact | SHA-256 |
|---|---|
| P10 controller trace | `f6b955d6c51f92512a6a8efd370343d2e23c9be40799dce42b51a43c0f7393b6` |
| P10 victory and fresh re-export | `020cde76c8fd953851c7eb1ec2282cad9d0990486079b68aebee9a82565e9e86` |
| P9 and P10 public verifier receipt | `f258a3f85ab44e676d07d0e2f0488a302acf2a055659371b4dc2727aff6c0742` |
