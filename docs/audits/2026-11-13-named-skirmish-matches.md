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
trace was created. These traces still need exact normal-ROM verification
with `tools/sc64_skirmish_normal_replay.py`. No ROM bytes changed.
