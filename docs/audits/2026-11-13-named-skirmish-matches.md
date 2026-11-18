# Complete HOST matches with independently chosen named CPUs

The ordinary-input skirmish controller can take one lowercase `t`, `z`
or `p` per active seat. Existing invocations still rotate the bot races.
The replay verifier accepts repeated or reordered named races, but checks
the contiguous owner slots, the live race in each slot, and the terminal
configuration. Sparse seats, Random and Melee have separate opening and
save checks; this controller format does not cover them.

Three fresh Terran-profile ASan/UBSan runs launched from HOME with the
lineups below and reached a loss when the human's last building was gone.
These are completed ordinary matches, not winning strategies. In the
three-CPU Blood Bath run, two opponents had no actors left at the human
loss; the surviving Terran occupied the third CPU slot.

| Map | CPU choices | Loss frame | Trace SHA-256 |
| --- | --- | ---: | --- |
| Outpost | Terran | 11,485 | `f5292f64cda492ffbf75ab0ba8cca6df257ccdc6e9ed21d7a089de03893f95e1` |
| Blood Bath | Terran / Terran | 18,580 | `62b9c648cfd81ff5a597bcbc9872bd706af5b55573e349432327349549134981` |
| Blood Bath | Protoss / Zerg / Terran | 22,423 | `bb61569eaa0ffbdb06c78be440fadbda1a576018969ad0ff2fdc8c6e5ef8397c` |

With prepared Terran inputs, reproduce the last case with:

```sh
make build/test-sc64-skirmish-match-terran
./build/test-sc64-skirmish-match-terran 1 0 3 /tmp/sc64-blood-pzt.json 90000 pzt
```

Exit 3 denotes an ordinary loss. The three traces passed the replay
parser; a lineup shorter than the active count was rejected before a
trace was created. No ROM bytes changed.

## Three named opponents on the staged Terran cartridge

The Blood Bath Protoss / Zerg / Terran lineup now reproduces from HOME
on staged Terran `e22f6cc` under the physical mGBA observer. All 25
observations match, including 23 action checks. The human loses on call
22,578, frame 22,423, with three units and zero buildings. The third
CPU remains Terran and has 39 units and 22 buildings; the first and
second CPUs have no remaining actors. The same 568 key transitions
drive the result without guest writes or save input. Mean whole-loop
cost is 801,412 cycles (20.93 emulated updates/s), with a peak of
18,502,418 cycles. This is neither a hardware frame-rate result nor
an N64 retirement or winning-strategy witness. The one- and two-CPU
tapes still need their exact ARM checks.

- Staged Terran ROM SHA-256: `6f869a5e295aa95d9ecfecebc0f18eba4be4b8ce5b5f5a7ee67a6ba664304a0c`.
- Matching ELF SHA-256: `586c4b497c8fcf39fa2ce28d6ba8022688017d0be8af4110a36e79f7be7b7077`.
- Complete replay receipt SHA-256: `c6c1311dd7879ecb0cb937783ae661d04be34bd2e9c737b7847e1798df9e44f1`.
