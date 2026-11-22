# The Big Push from HOME on the normal Terran cartridge

The exact November 13 KODAK Terran ROM (`cbba1c5`) boots fresh at HOME,
selects The Big Push through the ordinary campaign picker, and reads nine
briefing pages. It then replays the retained probe-entry HOST tape using
physical buttons only. Seven authored gameplay pages are acknowledged
without advancing simulation. Guest observations are read through the
matching ELF; no guest memory is written and no SRAM is supplied.

All 272 retained observations matched. They include the Command Center
relocation, first combat contact, paid production, supply growth, and
attack orders. At tape call 143,260 (physical call 143,321, game frame
139,216) the mission won: no hostile buildings, the hero alive, completion
bit 11 set and zero trigger faults. The host tape has a single verified
route; this does not establish other routes or fresh SRAM continuation.

Mean main-loop cost was 842,000 guest cycles (19.93 emulated updates/s),
with a peak of 22,546,637 cycles. Those include rendering, interface and
display waits. No physical GBA timing was measured.

| Pinned input or receipt | SHA-256 |
| --- | --- |
| Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| HOST tape | `5aee3cbc5125c79ec99414386d6a49122aff0e5fbe465e968b905aaf2f62d182` |
| Physical replay harness | `52dd306b60fd14ebc08904c8dcb447a4ed8208ae14f4d85af743f539b42a35a3` |
| Full replay receipt | `89b89813b394f4b813f0c33d1bd3b9526396cc4841e9572b7e888bb72ea2cc9d` |

## November 20 fresh SRAM and result autosave

The Big Push wins from HOME on the normal Terran ROM from release
`9ee056c2`. The current sanitized HOST controller reproduced the earlier
143,260-call tape byte for byte. The physical replay matches 272
observation labels across 268 distinct cuts and 13 gameplay fields.
After a fresh SRAM load it matches the remaining 265 labels.

The ordinary campaign picker opens all nine briefing pages before
deployment. Physical controls acknowledge a page at call 1,404, frame
1,321. Six more pages appear at call 143,245, frame 139,201. The replay
samples each call after the final assault order, so acknowledgement
preserves the held-button tape and the current simulation frame.

At call 2,761, frame 2,568, the Command Center relocation and worker
landing observations have matched, with neutral input and an empty selection.
START and B write a 32 KiB save with one valid bank and one erased bank.
The sequence-one payload is 15,832 bytes with outer tag `0x76435442`
and inner tag `0x72435442`. A fresh emulator loads it through HOME L.
The named world, full late-mission state, cursor and remembered action
match the pause cut. The recreated camera matches at (1756, 1910).

The mission wins at frame 139,216 with the Norad II alive, no hostile
buildings and zero trigger faults. Victory preserves bank one and
writes a checksum-valid sequence-two payload of 15,832 bytes. A third
emulator loads that result through HOME L, restores selected unit
slot 0 and keeps the world frozen across eight neutral calls with an
empty impact queue. Its entire 32 KiB readback matches the victory SRAM.

The split replay averages 849,264 guest cycles over 143,326 main-loop
calls, or 19.76 calls per emulated second. The measurement includes menus
and pause/load. Physical GBA timing remains unmeasured.

With prepared Terran inputs, build `build/test-sc64-t8-campaign-run`
and generate the HOST tape with `--limit 200000 --trace "$trace"`.
Run the [controller replay](../../tools/sc64_t9_normal_replay.py) with
the matching ROM, ELF and port-free physical observer.

```sh
python3 tools/sc64_t9_normal_replay.py --chapter 8 \
    --trace "$trace" --rom "$rom" --elf "$elf" --runner "$runner" \
    --out "$out" --save-tick 2761
```

| Pinned input or receipt | SHA-256 |
| --- | --- |
| Normal Terran ROM | `8dba4961a022a94348efb8638cb6035341b4c1afb875ae299807ee44a35c6262` |
| Matching ELF | `6323a02f990f8f67646181f8102880ac8c9a16454e4357a4dd153fd4af4b8115` |
| Physical observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |
| Sanitized HOST tape | `5aee3cbc5125c79ec99414386d6a49122aff0e5fbe465e968b905aaf2f62d182` |
| Replay harness | `ea56d69b5e1ca89f7211cf9a787925da1d42d4cfb66bff3edf4fca0eaf46e521` |
| Pause SRAM | `71cee1e5eecdd3555295f779c21de32021de3a6a48136bd7ed267d41cf89d3aa` |
| Victory SRAM and fresh readback | `9559040ebf56d6f293d2412d2f41b412d370948ad8ac533789560396678cf3a0` |
| Full replay and both journal checks | `4c81f046d3a6e3811ed8b71777a24799fc81e702f8345685f033f92f78c3e0fb` |
