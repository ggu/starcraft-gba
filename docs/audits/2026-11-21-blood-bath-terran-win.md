# Terran Blood Bath skirmish win on the normal cartridge

The same ordinary button sequence defeated one named Zerg CPU on Blood
Bath in three normal Terran ROMs. The port-free mGBA observer started
with blank SRAM and applied 2,278 recorded GBA button transitions from HOME
through the victory screen. It checked the selected lobby and matched the
frame and recorded resources at 158 action checkpoints. At call 27,281,
simulation frame 26,692, RESULT reported victory with 76 Terran units and
18 buildings. The Zerg side had zero units and buildings.

The controller recorded 9 SCV, 85 Marine, 29 Firebat and 24 Medic queue
transactions, 5 accepted Barracks placements and 6 attack-move orders.
It keeps the northwest building sites apart so units can leave their
production buildings and workers can reach resources. It adds workers and
supply before committing the army to the main Zerg base, then clears the
west expansion. The HOST controller reads game state to choose commands.
Only the held buttons are passed to the cartridge replay.

All 186 sampled launch, action, interval and terminal states matched across
three cartridges. Timing measures emulated main-loop work, including UI
and presentation waits.

| Terran build | Mean cycles/call | Calls/s | Peak cycles |
| --- | ---: | ---: | ---: |
| Released `23391ef1` | 979,650 | 17.13 | 9,563,141 |
| Sparse routing release `83b32e59` | 979,650 | 17.13 | 9,563,141 |
| Sparse routing and Carrier candidate `685d4a7d` | 1,029,927 | 16.29 | 9,828,247 |

The two release measurements match exactly at 26,725,820,207 total
cycles. The Carrier candidate raises mean cycles by 5.13% relative to
sparse routing for this Terran versus Zerg match. Repeat this gate when
changing skirmish rules, production, controls, or routing.

## Reproduce

Use the prepared Terran inputs and a matching normal ROM and ELF. The
physical mGBA observer is the one used by the normal campaign checks.
Choose a new replay output directory. The optional combined-arms policy
also supports the existing [Outpost win](2026-11-13-outpost-human-win.md).

```sh
make build/test-sc64-skirmish-match-terran
SC64_SKIRMISH_COMBINED_ARMS=1 ./build/test-sc64-skirmish-match-terran \
  1 0 1 /tmp/sc64-blood-bath-trace.json 75000 z
python3 tools/sc64_skirmish_normal_replay.py \
  --trace /tmp/sc64-blood-bath-trace.json --rom "$NORMAL_TERRAN_ROM" \
  --elf "$MATCHING_TERRAN_ELF" --runner "$SC64_HEADLESS_REPLAY" \
  --rom-sha256 "$NORMAL_TERRAN_SHA256" --output /tmp/sc64-blood-bath-check
```

On an Apple clang SDK that expands the host memory-function declarations,
pass `CLANG='clang -D_FORTIFY_SOURCE=0'` to `make` for this test build.

| Input or receipt | SHA-256 |
| --- | --- |
| Held-key trace | `a5935d2918b44ceb98fd91e2e25de74b7197d6e297e6cb12980def38fa4341da` |
| Physical mGBA observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Tested Terran ROM | `526a3329d85cc5b207088b0f7da681410ea3c5bbebd5b76c073ab6a807fd3701` |
| Matching Terran ELF | `be6383d0dd15069eb529fa2a4f6cd39c8bebce75004bb251f208854b799a3b02` |
| Released observed-state receipt | `9dccf58a28a0ef785cf4e2b61b08d4fda1fa78a8b246031823121ddb824e064c` |
| Sparse routing Terran ROM | `c32005c76a2346f287b7ecb7eb101d220b14a7448eea5a5dcce18f948ebd75d1` |
| Sparse routing Terran ELF | `3ab902e5031afa07b2b675c4715d449b8c4d633667b5d812659075d520cd29ba` |
| Sparse routing observed-state receipt | `ec41168dc535fe1ee4308555a9c41a6ce245d16678f9ef5bd3c4589a7ff58a2b` |
| Sparse routing and Carrier candidate ROM | `d9a2bd5f2d5255b4b9424239dc8fed187b89062cb97fe35206d12b12ff5ca255` |
| Sparse routing and Carrier candidate ELF | `6953e507e38c389bea2332c71c24061fde90a36ebf6568c4f1319ea2e7081356` |
| Carrier candidate observed-state receipt | `c84965d4cdf52c3860a2f798938ef89246aef9b11a79fad8ae8f3e86fbe78c9e` |
