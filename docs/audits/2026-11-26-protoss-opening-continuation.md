# Protoss mission 2 from the standard cartridge's P1 victory

The standard Protoss10 ROM loaded the SRAM produced by the P1 run in
[#927](https://github.com/ggu/starcraft-gba/pull/927). Pressing A on that
result started P2. Its controller replay matched 41 periodic observations of
the elapsed mission time, objective flags, phase, win/loss state, friendly and
enemy unit counts and building counts. The final observation at input call
41,409 showed victory at frame 39,217 with objective flags 908 and phase 3.
Both completion bits were set. A fresh emulator loaded the victory world
and re-exported identical SRAM.

| Input or output | SHA-256 |
|---|---|
| Standard Protoss10 ROM | `967e2b35152df87ac05f98d4ee0ccd81f3c1634066503306257eb7450296d4ff` |
| P1 victory used as input | `a5183059b513536784634e374ce4ab56cba0ecca3abdbe38037ea46415ae7475` |
| Modal-aware P2 controller trace | `58de24bb80a51b00588fb2c6339320cdb4c9bddd4728d3d9092342867e2217bb` |
| P2 victory and fresh re-export | `3fa8e918f1b0a2d5d2b981beb01264b61d243dfdaf5637296ec9c1fa5a65a9bc` |

With the matching private build inputs, generate the recording and play it
against a normal cartridge. Set the four shell variables to the matching
ROM, ELF, headless debugger runner and P1 victory save. Use a new output
directory for each replay.

```sh
make build/px-p2-normal-home
build/px-p2-normal-home --controller 3 --trace build/p2-home.json
python3 tools/sc64_p2_linked_replay.py --rom "$ROM" --elf "$ELF" \
  --runner "$RUNNER" --seed "$P1_SAVE" --trace build/p2-home.json \
  --out build/p2-linked-result
```
