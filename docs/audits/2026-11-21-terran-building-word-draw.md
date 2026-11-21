# Packed Terran base buildings

The optional `TERRAN_BUILDING_WORDS=1` recipe packs the completed Command
Center, Barracks and Supply Depot from the normal Terran cartridge's
prepared sprite bank. Their original raw IDs are 106, 111 and 109. The
shared Mode 4 compositor selects the packed frame by the exact source
frame pointer and retains the same sprite origin, viewport clip and depth
turn. Construction, overlays, and other building frames use their
existing draw paths. The option defaults to zero.

## Ordinary Blood Bath replay

The paired normal Terran cartridges start blank SRAM at HOME and play the
[complete Terran versus Zerg Blood Bath route](2026-11-21-blood-bath-terran-win.md).
Both reach victory at call 27,281 and game frame 26,692. All 186 gameplay
observations and 158 action checks match. The 19 image cuts match both
indexed pages, the display register, palette and Carrier source clock.
All 202 timed stage samples have matching game frames and camera anchors.

| Ordinary controller span | Mean cycles, control | Mean cycles, packed | Mean change |
| --- | ---: | ---: | ---: |
| Full replay | 1,003,694 | 918,786 | −8.46% |
| Production, calls 11,500–12,500 | 1,047,680 | 925,408 | −11.67% |
| Production, calls 16,500–17,500 | 1,202,527 | 1,090,568 | −9.31% |
| First attack, calls 17,748–19,000 | 1,809,153 | 1,683,442 | −6.95% |
| Late fight, calls 19,000–24,000 | 1,127,298 | 997,533 | −11.51% |

Mean actor and effect drawing drops by 124,210, 112,363 and 125,674
cycles at the three production and first-attack sampling windows.
Full-route peak cycles increase from 9,772,120 to 9,792,663 (+0.21%).
The mean equates to 16.72 and 18.26 emulated main-loop calls per second,
including UI and presentation waits.

## Build and check

```sh
make TERRAN_BUILDING_WORDS=1 verify-terran
make test-sc64-terran-building-words-pixels
```

The prepared sanitizer check compares all 38,400 indexed pixels over 360
draws, spanning four horizontal alignments and clipping at both field
edges. It checks the three selected raw frame pointers and the existing
fallback. The normal ARM candidate on `f42b516b` uses the same 38 frozen
prepared inputs as the Carrier control, plus the locally generated bank.
The link uses 71,936 additional text bytes and retains the control RAM
endpoints `0x030049f8` IWRAM and `0x0203aa0c` EWRAM. Cartridge header and
ELF payload checks pass. The flag-off compiler argument vector matches
the authenticated control vector.

| Local input or receipt | SHA-256 |
| --- | --- |
| Ordinary input tape | `a5935d2918b44ceb98fd91e2e25de74b7197d6e297e6cb12980def38fa4341da` |
| Normal Carrier control ROM | `455b9e1c1b74a5b8cfe5491d861a6b01b904a6c10270136eb3068c43ae576bf8` |
| Packed candidate ROM | `05521b327f87500601a9e049f4c4d34a32cd8a0bb9f29e557bc01b7cdd4e115c` |
| Packed candidate ELF | `731b150f90a815a03ba0bb9531a79109b5b8222139c9fe28cd5b7d2b832e2a6d` |
| Paired replay comparison | `3890fbf63f20939ae232700a0cf7597ce006d7990767c3f9abc75e8e64e6d774` |
| Packed candidate link receipt | `7a086bdc751db40bc14fbf6b8e0e0e19cdf9a13b6f92dd668bc4adec78bb66c2` |

## Other cartridge profiles

The authenticated Z10 and P10 frame banks map the same three raw IDs to
71,968 packed bytes each. Showcase's Command Center crop is one row
higher, making its full set 72,504 bytes. Each profile needs its own
prepared bank. P10 should add these three frames to its existing packed
Hatchery, Overmind and Nexus selection. For each profile, check selected
pointers and pixel parity, ARM link and stack margins, and ordinary
Custom Game where multiple completed Terran structures are in view.
