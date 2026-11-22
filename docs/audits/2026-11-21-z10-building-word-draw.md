# Packed Terran base buildings on Z10

The Z10 trial adds the completed Command Center, Barracks and Supply
Depot to the existing 15-frame packed building bank. It uses the normal
Z10 sprite bank and shared Mode 4 compositor. The original packed words
and frame-pointer mappings are preserved. Building positions, painter
order and clipping stay in the shared draw path.

## Ordinary Blood Bath comparison

Both ROMs start from blank SRAM and choose Terran versus one named Zerg
CPU through the normal Z10 Custom Game menu. They then apply the recorded
field buttons from the [Terran Blood Bath win](2026-11-21-blood-bath-terran-win.md).
The prefix ends after 12,512 physical-button updates at simulation frame
12,120. All 51 action wallet checks and 15 world digests match. The 15
display cuts agree on both indexed pages, palette, display register and
Carrier source clock. All 293 stage samples have identical frames and
camera coordinates. The production view admits five completed Barracks.
Field tape call 12,200 admits a Command Center and five completed Supply
Depots.

| Ordinary controller span | Mean cycles, control | Mean cycles, packed | Mean change |
| --- | ---: | ---: | ---: |
| Full opening and production prefix | 794,797 | 741,689 | −6.68% |
| Production, calls 8,000–11,500 | 984,244 | 885,713 | −10.01% |
| Late production, calls 11,500–12,500 | 1,100,359 | 977,033 | −11.21% |
| Five-Depot window, calls 12,000–12,500 | 1,116,498 | 998,730 | −10.55% |

In the late production window, sampled actor and effect drawing drops
by 121,650 cycles per call. Full-prefix peak cycles fall from 6,206,318
to 6,193,502. The full-prefix means equate to 21.11 and 22.62 emulated
main-loop updates per second including UI and presentation waits. A
full Z10 Custom Game win and physical GBA timing are separate checks.

## Build and check

```sh
make verify-zerg-z10
make test-sc64-z10-building-words-pixels
```

The extra frames use a separate generated bank. Set
`Z10_TERRAN_BUILDING_WORDS=0` for just the preceding Z10 bank.
`Z10_BUILDING_WORDS` still controls the packed compositor. Changing
banks invalidates the existing Z10 building-word configuration.

The normal flag-off ARM rebuild reproduces the authenticated control
ROM byte for byte. The candidate adds 72,048 text bytes and retains
IWRAM `0x03004d60` and EWRAM `0x02039b30`. Its fact table matches the
control and passes 2,019 checks. The sanitizer fixture passes 2,160
full-screen comparisons with the 18 packed frame pointers and checks
the unpacked fallback. The asset-free build produces its verified
64 KiB ROM and has identical build plans with the Z10 option off or on.

| Local input or receipt | SHA-256 |
| --- | --- |
| Ordinary input tape | `a5935d2918b44ceb98fd91e2e25de74b7197d6e297e6cb12980def38fa4341da` |
| Carrier control and flag-off ROM | `86324a99034096e9448d2bf486eedb949a0681d15688cc6db7112bfc26430bf0` |
| Packed candidate ROM | `ea3faa53ff31553e91e3888c58746161d9ac9681bffd6a45885df4e14ea32308` |
| Packed candidate ELF | `2bdb3246471ec251577f9b70d0f5df21e0e6225095cb8aeb4e9aa785cc2dc291` |
| Paired replay comparison | `a8be611864ad5e32b6bd6100da1b3495c526da86e5b587b2b984f18144be75df` |
| Packed candidate link receipt | `caf7b1a97392b1d8544c5ce5907a815e58c53a0c885ee54f6c00a6cf26ed3832` |

The [Terran and P10 trials](2026-11-21-terran-building-word-draw.md) use
the same compositor and their own verified sprite banks.
