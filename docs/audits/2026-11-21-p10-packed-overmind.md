# Packed Overmind draw in P10

Three physical Eye of the Storm replay cuts draw the same completed
101×66 shelled Overmind frame once through the indexed byte blitter.
Breakpoints at the blit call and return on the authenticated control
identify the pixel pointer as the linked Overmind source symbol. The
observer also checks three live return addresses on the guest stack.

| Controller call | Game frame | Byte blit cycles including interrupts |
| --- | ---: | ---: |
| 45,231 | 44,123 | 113,378 |
| 45,466 | 44,352 | 102,035 |
| 45,467 | 44,353 | 102,039 |

The normal P10 cartridge packs that frame alongside the Hatchery. Run
`make P10_OVERMIND_WORDS=0 verify-protoss-p10` for the indexed Overmind
control. The existing Mode 4 lookup matches
the chosen frame pointer. The generator reuses the prepared P10 source
bank and adds 54,912 bytes across four alignment phases. The existing
renderer, save format, and campaign behavior apply. All four combinations
of the Hatchery and Overmind switches
select their expected bank. With Overmind off, the Hatchery-only header
is byte-identical to the released header. The other cartridge recipes
keep their existing selections.

Run `make test-sc64-p10-overmind-words-pixels` with prepared inputs. The
sanitized fixture compares all 38,400 indexed pixels in 240 renders with
left, right, and both HUD edges clipped across all four phases. It still
checks an unselected building through the indexed fallback.

The ordinary-input A/B uses the frozen P10 inputs from `f01263ef` and
changes only the generated packed-header include. Both cartridges boot
in fresh mGBA homes, load the same campaign SRAM from HOME, and follow
the same controller suffix to victory at frame 46,057. All 17 sampled
world states, palettes, active page flags, and pairs of Mode 4 pages match.
The samples include 161 active route-field hashes. The 114 timed updates
match frame numbers and route build/hit counters.

| Controller calls | Physical cycle mean, control → packed | Actor draw mean, control → packed |
| --- | ---: | ---: |
| 43,595–43,615 | 1,946,775 → 1,946,787 | 271,583 → 271,634 |
| 45,218–45,275 | 2,016,329 → 1,990,655 | 355,482 → 329,369 |
| 45,435–45,469 | 2,324,554 → 2,289,012 | 531,038 → 494,510 |

The late two windows reduce physical mean cycles by 1.27% and 1.53%.
The last window reduces the peak from 3,408,670 to 3,351,693. Mean physical
cycles across all 114 sampled updates drop by 1.14%. Calls 45,231,
45,466 and 45,467 drop by 56,148, 87,019 and 55,499 respectively.
These are emulated GBA cycle measurements.

Header and ROM payload verification pass. Linked text grows by
54,792 bytes within the same 32 MiB ROM. IWRAM and EWRAM endpoints remain
`0x03004e20` and `0x0203b52c`. Both replays change no bytes in the
256-byte stack guard. Deepest observed changed bytes remain 4,648
for the system stack and 296 for IRQ.

The measured frame belongs to the P10 campaign actor. In the historical Zerg-player
Blood Bath window, all 20 late samples admit only the already-packed
Hatchery. A separate ordinary Blood Bath launch as Protoss against three
CPUs draws the Nexus instead. Bracketed indexed Nexus draws cost
83,643, 90,161, and 89,957 cycles at frames 120, 180, and 240.
The Nexus is an existing generator choice for a separate benchmark.

| Retained private receipt | SHA-256 |
| --- | --- |
| Original Overmind attribution | `db840e10830eaf6ccf714095ba59e501bf1671c656ee15de9b7c4a6aececc1b9` |
| P10 ARM link | `12306dba5d5057153610e24f767e762ffebc6e83ed828bd44061a11ce9694c9d` |
| P10 ordinary-input comparison | `b0d7ad83598592b86c205bd202eeb6d317016e78f95cc15210660b30a706606f` |
| Blood Bath Nexus attribution | `242aecc0096637daeafcb30748211dba91494c789e8696bcf6a38d130f91866d` |
