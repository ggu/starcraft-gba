# Standard Protoss from the Homeland victory

## P8, The Trial of Tassadar

The same standard Protoss10 cartridge loaded the verified
[Homeland result](2026-11-26-protoss-p7-linked/README.md).
A on RESULT entered mission slot 27 at frame zero. Victory arrived
at frame 93,065 after 94,786 linked calls. All 38 checkpoints and 12
field dialogue pages matched. Fenix and the Hyperion survived. The
Stasis Cell was destroyed, campaign phase was 3, all three runtime
runners were fault-free, and completion mask `0xfc0000c` retained
all eight prior victory bits.

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
