# T10 from HOME on the normal Terran cartridge

The retained T10 host tape starts after deployment through the private
controller probe entry. Its 278 gameplay transitions replay unchanged
from a fresh HOME boot of the normal KODAK Terran cartridge (`cbba1c5`).
The port-free observer sends physical buttons and reads guest state. It
selects The Hammer Falls in the ordinary campaign menu and reads all 14
briefing pages before deploying on call 63. Four in-game dialogue pages
are acknowledged with press and release while the simulation frame stays
unchanged.

All six host observations match across 18 fields, including launch,
source opening, three progress checkpoints and the result. The cartridge
wins on physical call 3,517 (gameplay call 3,446), frame 3,361. The Ion
Cannon is destroyed, Raynor survives, the objective and mission completion
bits are set, and there are no trigger faults. Mean loop cost is 954,077
guest cycles (17.58 emulated updates per second); the peak is 25,790,057
cycles. This is one qualified route from HOME and does not measure hardware
speed or fresh SRAM continuation. No cartridge was rebuilt.

SHA-256 pins:

| Input or receipt | SHA-256 |
| --- | --- |
| Retained host tape | `a6c04ab0297bf7e0dea2182b895e4d45746d836f97ff8ef9032bcfe87d52a00e` |
| KODAK Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Replay harness | `5b95653d774e61fbc1a75265f266f13a9d12b83f578ad92e5902637c084b5aed` |
| Physical receipt | `ec2528877241c7726a98ffeba1b26f3b6f37c648319f8a3eb564b756fe1d0704` |

A separate T9 (New Gettysburg) check on the same ROM reached tape call
4,000 from fresh HOME. Nine briefing pages and four gameplay pages used
physical acknowledgments. All six checkpoints matched 18 guest fields
through physical call 4,049, game frame 3,946. The controller-local
`failed_action` field is not read from the cartridge. This is an opening
only; the full replay was not started after the disk guard was reached.
Receipt SHA-256: `bc49b057ca97b7902546c732fd290790a0db617190a00a0b86df83c4878e4867`.

## November 20 fresh SRAM and result autosave

The normal Terran ROM from release `f6869829` wins The Hammer Falls
from HOME after a pause save and a fresh emulator load. The current
sanitized HOST controller reproduced the earlier 3,446-call tape
byte for byte. The physical replay reads all 14 briefing pages and
acknowledges four dialogue pages at game frame one while simulation
stays paused. All six gameplay checkpoints match across 17 fields,
including two after the fresh load.

The controller pauses at gameplay call 2,342, frame 2,276, with no
selection or held input. START and B write a 32 KiB SRAM image with
one checksum-valid bank and one erased bank. The first bank uses outer
tag `0x76435442`, inner tag `0x72435442` and a 15,832-byte payload.
A new emulator loads it through HOME L. The named world, full
late-mission block, cursor and remembered menu action match the saved
state. The camera rebuilt from the cursor matches at (1544, 1764).

The Ion Cannon is destroyed at frame 3,361, with Raynor alive and zero
trigger faults. Victory preserves bank one byte for byte and writes
sequence two with a 15,832-byte payload. A third emulator restores the
result from HOME L and keeps the terminal world and mission state
frozen across eight neutral calls with an empty impact queue. The
pre-victory Command Center selection clears and command mode returns
to normal. The result readback matches all 32 KiB of victory SRAM.

The split run averages 1,020,626 guest cycles over 3,528 main-loop
calls, including menus and pause/load, for 16.44 calls per emulated
second. Physical GBA timing remains unmeasured.

The same replay harness still accepts the default
[New Gettysburg invocation](2026-11-13-t7-t9-normal-physical.md#november-20-current-terran-fresh-sram-and-result).
On this Terran ROM all 51 T9 checkpoints match, including 12 after
the cut at frame 35,363. Both SRAM images and the frozen victory
readback are byte-identical to the earlier T9 run.

With prepared Terran inputs, build `build/test-sc64-t10-campaign-run`
and generate the HOST tape with `--limit 100000 --trace "$trace"`.
Run the [controller replay](../../tools/sc64_t9_normal_replay.py) with
the normal ROM, matching ELF and port-free physical observer. Use a
new output directory whose parent exists.

```sh
python3 tools/sc64_t9_normal_replay.py --chapter 10 \
    --trace "$trace" --rom "$rom" --elf "$elf" --runner "$runner" \
    --out "$out" --save-tick 2342
```

| Pinned input or receipt | SHA-256 |
| --- | --- |
| November 20 Terran ROM | `8df962af5e292af798e2fe0934e2356cf929f3853d61c3a6b17e1fffbb86aa92` |
| Matching ELF | `484c6ea35ddd385223db78b7ce84d2555d8f752436cc710680b1a7ee9b2ca12a` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Sanitized HOST tape | `a6c04ab0297bf7e0dea2182b895e4d45746d836f97ff8ef9032bcfe87d52a00e` |
| Replay harness | `a4cb9f8bc5deb574ae791a878d685e276d99d5db683b29525169c439303b1f44` |
| Pause SRAM | `4f23a41c6cc575174713d479dcd99793c63ef12e8df427e663dcf970fc6c82bc` |
| Victory SRAM and fresh readback | `54ba0309ae345f76f18b798194d1bef7164580de750cc00725fefc6f0b50fca1` |
| Full replay and both journal checks | `51e538d932ff23bc134a368356ae37ccbcdebdcc6682cc6103725b64b1b143d9` |
| Default T9 rerun on the same ROM | `7117be4d86c61d0537f5b75f64706d77738c0b92cc303c45fc0e4781f241aa79` |
