# Z10 ordinary-input FIFO continuation

At FIFO candidate `528d9082`, the Z10 cartridge continued a retained Z9 victory
through Full Circle using physical mGBA input. A fresh process loaded the
result; the observed terminal world and scheduler matched, and its re-exported
SRAM was byte-identical. The match won at simulation frame 58,485, after 60,652
controller calls. All 25 checkpoints matched their observable HOST fields,
including unit and building counts, larva and queues, supply computed from the
cartridge's rule tables, bank, cursor, AI town state and scheduler fault. The
controller's private route-leg and peak counters are not cartridge state;
`countdown` is a hardcoded placeholder and is not compared. No speed or hardware
claim follows from this replay.

The normal controller walks each authored briefing page, renders every update
and acknowledges modal text with ordinary inputs. Its normal-only strategy uses
three Drones per Extractor, sends the Mutalisk force at ten, and stops it below
three. The legacy Z8/Z9/Z10 controller path keeps its original briefing and
strategy. The opt-in target compiles the Z10 cartridge preprocessor flags plus
`SC64_GAS_SERVICE_TRIAL`, `SC64_GAS_SERVICE_QUEUE_SAVE` and HOST modal controls.
It does not change or enable a product ROM.

| Input or output | SHA-256 |
| --- | --- |
| Z9 victory seed | `71e4d60f9e6a6f99c84ce3a7fb00d31a77c0fa6e8eba7e3af252c52e150e4b97` |
| Normal Z10 tape | `4aa0ba1f52decc0f7cfb05977947ce8bb5b918f17c46120c18a9ebaaa217fd3c` |
| FIFO trial Z10 ROM | `0b9f5f766cab83c8a7032ae999e7ab14a8b159e1c9b2e713bba001df0bbb4972` |
| Matching trial ELF | `f50e1965a1ad8733d3a8a972e1c7cc438bfd35acb86e263b5ef83e494fa60fe4` |
| Physical runner | `050e5e7dcab772798780a11d1c6c3b4b84135e4b681aa11e08f666738ebaa9bf` |
| Z10 victory and fresh readback | `a16e05d869d18388d8c09e23bb5946a0316b87fa11845eeca9859f43a89ee4da` |

To repeat with prepared private assets and a fresh output directory:

```sh
make Z8_DESCRIPTOR="$descriptor" build/test-sc64-z10-normal-home-gas-fifo
build/test-sc64-z10-normal-home-gas-fifo "$private_tape" 400000
python3 tools/sc64_z10_fifo_normal_replay.py --seed "$z9_victory" \
  --trace "$private_tape" --rom "$fifo_rom" --elf "$fifo_elf" \
  --runner "$physical_runner" --out "$new_output_dir"
```

The replay checks all Z1–Z9 completed bits and Z9 victory, loads via ordinary
HOME input, presses A on the result, splices in the Z10 controller's input tape,
and requires every listed checkpoint and the final completion mask. The tape,
cartridge, saves and generated inputs remain private.
