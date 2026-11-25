# Protoss finale from an existing P8 victory

The packaged Protoss10 cartridge cleared P9 at frame 101,649 from a retained
P8 victory. A fresh emulator loaded and exported its victory SRAM. Starting
from those exported bytes, P10 cleared at frame 53,225. Its victory also
reloaded and re-exported byte for byte. Both runs used ordinary controller
inputs in mGBA on the 32 MiB cartridge with finite gas enabled.

The earlier P9 tape first disagreed with the current ROM at call 49,977.
It expected 50 human units and 163 enemies; the ROM had 42 and 168. The
previous gas-disabled ROM also disagreed at that checkpoint. Re-authoring
the route against the current engine yielded a new winning tape. Both the
old and updated host recipes at the current revision generated exactly the
same tape. The Make change keeps future P9 controllers aligned with the
cartridge's settings; it did not cause the recovered win.

| Input or result | SHA-256 |
| --- | --- |
| Packaged Protoss10 ROM | `19d1cfb354796fa7656080d29a60ff0fde5c62b8d58f25eeba7481cf1b88f81e` |
| Starting P8 result | `dd39755042242391d0a78f825565e6fe9409d1872c741d57e683801aa41fd121` |
| Regenerated P9 input tape | `d39b718d5ab7456f46a21580455940a1d41ed028fcccbe35e5889d9d36c67721` |
| P9 result passed to P10 | `d7ae94c99ebf10bfe2cc719d8ddbc86e3c3192a40873abf4d5de1abe0dc24979` |
| P10 input tape | `f6b955d6c51f92512a6a8efd370343d2e23c9be40799dce42b51a43c0f7393b6` |
| P10 result | `020cde76c8fd953851c7eb1ec2282cad9d0990486079b68aebee9a82565e9e86` |

## Repeat the linked check

With the prepared campaign inputs, generate normal HOME tapes in a private
directory. The linked replay replaces the chapter picker prefix with a HOME
load and the A press on the preceding victory screen. It plays the rest
of the original tape and checks the mission-specific observations against it.

```sh
make build/test-sc64-p10-p9-normal-home build/test-sc64-p10-normal-home
build/test-sc64-p10-p9-normal-home "$private/p9.jsonl"
build/test-sc64-p10-normal-home "$private/p10.jsonl"
python3 tools/sc64_protoss_linked_replay.py \
  --p8-save "$p8_victory" --p9-trace "$private/p9.jsonl" \
  --p10-trace "$private/p10.jsonl" --runner "$physical_mgba_runner" \
  --rom "$protoss_rom" --elf "$matching_elf" --out "$private/new-chain"
```

Use a P8 victory carrying credit for the preceding Protoss missions and a new
output directory. Each mission writes its victory SRAM and a fresh-process
readback. The JSON report includes the observed checkpoints and input hashes.
This run covers the last two missions from a retained P8 result. The earlier
chapters and physical GBA pacing require separate qualification.
