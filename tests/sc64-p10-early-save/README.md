# P10 early physical save probe

`physical.py` exercises one early neutral save point in P1–P7. It requires a normal combined P10 ROM and matching ELF, plus a physical runner with read-only access to the modal count at VRAM `0x06013fe0`. Inputs select the mission and acknowledge visible modal pages, advance 30 gameplay frames, save with START/B, and resume. A fresh runner loads exported 32 KiB SRAM with L on HOME. The script compares saved/restored and uninterrupted/resumed states at 120 further gameplay frames. It records scheduled inputs and hashes the runner, ROM, ELF, and SRAM. Use a new output directory per run. Run under the host's native-build lock if other native jobs share resources.

The comparison covers selected live unit and building fields and active queues, mineral fields and resource banks, frame, mission, supply, campaign state, P3–P7 mission persistent structures, and P7 projectile storage when those symbols exist. It does not compare every serialized field. Scratch world contexts, transient UI, path caches, render/audio state, and other actor or effect systems are outside the fingerprint. Neutral continuation and one save point do not prove victory, arbitrary-save coverage, or all source mechanics.

Example (repeat `--mission` from 1 through 7, changing the output path):

```sh
python3 tests/sc64-p10-early-save/physical.py \
  --runner /path/to/physical-runner --rom /path/to/product.gba \
  --elf /path/to/product.elf --out /tmp/p10-p1-evidence --mission 1
```
