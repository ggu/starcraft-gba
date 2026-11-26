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

## Protoss mission 3 from the same P2 result

On the same standard Protoss10 ROM, a fresh mGBA process loaded the P2
victory above. A on the result entered P3 (mission slot 22) at frame zero.
The November 25 recording reached its last checkpoint with P3 still in play.
The input recording generated against the current engine reached victory at
call 49,311, frame 48,801. All 48 periodic observations and the victory
observation matched the controller's screen, frame and win/loss flags.

The P3 controller timestamps inputs after applying them. For the linked replay
the trace's tick 42 resumes inputs after the result-screen advance. An input
transition at host tick T was scheduled at runner index T − 37 and the
post-update observation was read after T − 36 completed runner calls.

At victory the primary objective bit was set, campaign phase was 3 and the
completion mask was 0x40000c (mission slots 2, 3 and 22). The resulting world
had 51 units and 22 buildings. A separate emulator loaded the result, matched
the world fingerprint and campaign snapshot, and wrote identical SRAM.

| Artifact | SHA-256 |
|---|---|
| Current P3 controller trace | `3cf1e5dd626db3eacd980520de6e92d31aac16ff7d878046aa4b27e1da84e449` |
| P3 victory SRAM and fresh re-export | `09ed33e7311476ca976ecdb3d1443f3bc4da84930dc9b0445df5f59cbd5247d6` |

## Protoss mission 4 from the same P3 result

HOME L restored the P3 victory at frame 48,801 with completion mask
`0x40000c`. A on RESULT entered P4 (mission slot 23) at frame zero.
The stock controller reached the mission's 180,000-frame cap. Its escort
reached extraction, but one hostile inside its 400-pixel scan kept the
heroes parked. The controller now follows its existing safe route for
the heroes once the escort is within 360 pixels and the mission has
reached frame 64,000. The ARM replay uses the same standard ROM and
ordinary keypad buttons throughout.

The P4 result arrived at frame 84,220, on call 85,200 of the controller's
HOME tape. The linked replay matched all 96 named checkpoints and 13
field dialogue arm and release pairs. Both heroes survived in the
extraction region, Raynor at (1552,1984) and Tassadar at (1520,1995).
Campaign phase was 3, faults and unsupported actions were zero, and
completion mask `0xc0000c` retained P1 through P4.

A fresh HOME L matched the P4 result's world fingerprint, named controls,
P4 runtime block and completion mask. It re-exported identical SRAM.
The journal preserves the preceding P3 bank byte for byte at sequence
3 and writes P4 into the other bank at sequence 4. Both checksums match.

The recording used the P10 inputs of the standard ROM build at
`82fc1a73` with this controller change. Regenerate the tape in the
prepared P10 build tree.

```sh
make build/px-p4-normal-home
./build/px-p4-normal-home --attempt-tag linked-p4 2> p4-pages.log
```

Set `TRACE` to the resulting `.local/sc64/p4-controller-linked-p4-*-win.json`.
Set `ROM`, `ELF` and `RUNNER` to the standard pinned observer inputs
from this audit, and `P3_SAVE` to the exact P3 readback above. Use a new
output directory. The linked route advances directly from RESULT, so
the HOME tape's briefing inputs are validated and skipped in playback.

```sh
PAGES=p4-pages.log
hash() { shasum -a 256 "$1" | cut -d ' ' -f 1; }
python3 tools/sc64_p56_normal_home_replay.py --mission 23 --quiet-guest \
  --rom "$ROM" --rom-sha256 "$(hash "$ROM")" \
  --elf "$ELF" --elf-sha256 "$(hash "$ELF")" \
  --runner "$RUNNER" --runner-sha256 "$(hash "$RUNNER")" \
  --trace "$TRACE" --trace-sha256 "$(hash "$TRACE")" \
  --pages "$PAGES" --pages-sha256 "$(hash "$PAGES")" \
  --linked-seed "$P3_SAVE" \
  --linked-seed-sha256 09ed33e7311476ca976ecdb3d1443f3bc4da84930dc9b0445df5f59cbd5247d6 \
  --out build/p4-linked-result
```

| Artifact | SHA-256 |
|---|---|
| Matching standard Protoss10 ELF | `7daf03b78bf390feff8add23a4fecf08a5fc296758df0cb0f7c0c3b17e72787b` |
| Headless physical observer | `050e5e7dcab772798780a11d1c6c3b4b84135e4b681aa11e08f666738ebaa9bf` |
| P4 controller trace | `ac7be694ca11fcb2398a62c50be88d511981843de89188d031596d50b1046c25` |
| P4 victory SRAM and fresh re-export | `414162bb7ed8218ddf4abe6e4f1bb520ee4c21c0240d053614cf31e319a2d969` |
| Qualification receipt | `259a90bd01b6afff62c1494caa089663b6cabebd7f5f7a9d6de0c5e11096131d` |

## Protoss mission 5 from the P4 result

The same ROM loaded the P4 readback, advanced from RESULT into mission
slot 24 at frame zero, and won at frame 5,057 after 5,122 linked calls.
All seven controller checkpoints and five field dialogue pages matched.
Tassadar survived at (1704,1771). The campaign reached phase 3 with no
faults or unsupported actions and completion mask `0x1c0000c`.

Fresh HOME L matched the world, named controls, P5 runtime and
completion mask and re-exported identical SRAM. Sequence 5 in slot 0
contains the P5 result. Slot 1 retained the sequence 4 P4 bank byte for
byte. The journal checksum matches.

Build `build/px-p5-normal-home` and run it with the same logging
command, using a fresh trace directory. The verifier command above
uses `--mission 24`, the resulting `p5-normal-home-win.json` and its
dialogue log, the P4 readback path and hash, and a new output directory.

| Artifact | SHA-256 |
|---|---|
| P5 controller trace | `b0806cae59b0e595e5f9de874d8f6c5a556587fd1a12ae532f6d228e01e2f10d` |
| P5 victory SRAM and fresh re-export | `97762bd635561d2af4812afeb51f031d0b5b91b23824b2c9db7f72554141d992` |
| Qualification receipt | `db512f6997e85f21adb7ac07f0b555a8e77f2a779155f5761bf74fbd829ee51c` |

## Protoss mission 6 from the P5 result

The same ROM loaded the P5 readback and advanced from RESULT into
mission slot 25 at frame zero. Victory arrived at frame 51,746 after
52,709 linked calls. All 59 checkpoints and 17 field dialogue pages
matched. Zeratul and Tassadar survived. No faults or unsupported
actions occurred, the campaign reached phase 3, and the completion
mask `0x3c0000c` retained all six mission flags.

Fresh HOME L matched the world, named controls, P6 runtime and
completion mask and re-exported identical SRAM. Sequence 6 in slot 1
contains the P6 result and passes its checksum. Slot 0 retained the
preceding sequence 5 P5 bank byte for byte.

Build `build/px-p6-normal-home` and run
`./build/px-p6-normal-home --trace p6-home.json 2> p6-pages.log`.
The verifier command above uses `--mission 25`, this trace and
dialogue log, the P5 readback path and hash, and a new output directory.

| Artifact | SHA-256 |
|---|---|
| P6 controller trace | `812de47cfab5e7a1fb01c42e0bf664db7a874ab09a7651c689d5431a0aed54be` |
| P6 victory SRAM and fresh re-export | `076f0a2f0336ef86d900b0c99eae2a1416bb04e9f7c5a26f07d9f02b5f7d1c9d` |
| Qualification receipt | `0c9401ac3f37c4498876aef2701995d3c530468c4c9e2cabd06db0ebf939823d` |
