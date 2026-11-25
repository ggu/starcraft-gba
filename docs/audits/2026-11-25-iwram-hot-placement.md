# Hot ROM functions in the unused IWRAM stack reserve

`common/gba.ld` keeps IWRAM code below `0x03005000` and leaves the
11,776 bytes up to the stack top at `0x03007E00` for the main stack.
That reserve was never derived from the program. On `71937af5` the
three race cartridges end their IWRAM code at `0x03004B78` (Terran),
`0x03004FD0` (Zerg) and `0x03004F08` (Protoss), and 66–77% of late
Custom Game cycles run from ROM.

This change bounds the stack per ROM and links each cartridge's
hottest ordinary ROM functions into the part of the reserve the stack
cannot reach. Only the link changes. The functions keep their Thumb
code; `common/gba_hot.ld` places the `.text.<function>` sections named
in `config/iwram-hot/<cartridge>.txt` after the `GBA_HOT` code, and the
linker adds the long-branch veneers. `SC64_IWRAM_HOT=0` restores the
previous link.

## Stack bound

`tools/gba_iwram_hot.py check` computes the longest call chain in
GCC's call graph (`-fcallgraph-info=su`) with GCC's frame sizes. The
graph leaves five things open, and the check has a rule for each.

- A call through a pointer may reach any function that GCC's
  `-fdump-ipa-cgraph` dump marks `Address is taken`: 37 functions on
  Terran, 49 on Zerg and 50 on Protoss.
- A function merged into an identical one is an ELF alias with no
  frame record. It resolves through its address (one to three per ROM),
  as a callee and as a pointer target.
- Every function linked into the ROM must have a frame record, and
  every function compiled without a clone suffix must appear in its
  unit's address-taken dump, so a partial or truncated dump fails. A
  missing interrupt handler fails too. Linker veneers use no stack. libgcc's
  arithmetic helpers count 96 bytes; the deepest chain disassembles to
  72 (`__aeabi_uldivmod`, `__udivmoddi4`, `__clzdi2`). Eight more bytes
  cover GCC's Thumb switch helpers.
- Recursion must be declared in the list with its nesting. Each ROM has
  one direct cycle, declared as two live activations: a building and
  its add-on through `race_bc_parent_detach`, or on Zerg a Nydus Canal
  and its exit through `race_nydus_note_building_lost`, which unlinks
  the pair before it destroys the exit.
- The pointer rule also makes one cycle per ROM out of 12 to 15 trigger
  and save dispatchers. Each is declared as one activation. The basis
  is the code: GCC's dump shows every member installed through a named
  ops table or a bind function for one layer (source dispatch, count
  cache, world adapter, timed service), and their pointer calls go to
  the layer below or to game services.
- Each declaration carries a fingerprint of the cycle as it was read:
  the member functions for recursion; for a dispatcher cycle, the
  members that call through a pointer or have their address taken, with
  their pointer-call site counts. An undeclared cycle fails the check,
  and so does a declared one whose fingerprint no longer matches.

The build fails unless `__iwram_end` plus the bound plus a 512-byte
margin stays at or below the stack top.

| Cartridge | Stack bound | Deepest chain | IWRAM may end at | IWRAM ends at | Spare |
| --- | ---: | --- | ---: | ---: | ---: |
| Terran 10 | 7,808 | pause save, the dispatcher cycle, then a trigger action | `0x03005D80` | `0x03005AE0` | 672 |
| Zerg 10 | 6,076 | campaign tick, the dispatcher cycle, then a save validator | `0x03006444` | `0x03006108` | 828 |
| Protoss 10 | 5,940 | campaign tick, the dispatcher cycle, then a save validator | `0x030064CC` | `0x03006018` | 1,204 |

Values are bytes. All three deepest chains pass through the dispatcher
cycle and join code the game does not combine, such as a save callback
reaching a trigger action. They are bounds, and the measured depth
below is lower. The deepest chain without pointer calls is actor
drawing: `sc64_draw_depth_ordered_actors` alone has a 3,384–4,280 byte
frame. Interrupt handlers run on the IRQ stack; their bound is 296 of
416 bytes on all three.

A private single-step mGBA runner records the lowest stack pointer of
every instruction. It loads each save, plays 40–60 updates, opens
pause and saves. The runs cover late three-computer Outpost and Blood
Bath games on each cartridge and pause saves from Terran 1 and 3–10,
Zerg 1, 2 and 4–9, and Protoss 10.

| Cartridge | Saves | Deepest main stack | Where | Free above the relocated code |
| --- | ---: | ---: | --- | ---: |
| Terran 10 | 11 | 5,928 | pause save in Terran 6 and 7 | 3,064 |
| Zerg 10 | 10 | 5,720 | drawing in Zerg 9 | 1,696 |
| Protoss 10 | 4 | 5,016 | drawing in late Outpost | 2,640 |

The IRQ stack reaches 296 bytes in these runs. The numbers are the same
before and after the relink. Protoss 1–9, Terran 2, Zerg 3 and Zerg 10
pause saves were not available for this measurement; the static bound
covers them.

## Measured cost

All figures are ARM7 guest cycles in mGBA with no player orders. The
lists hold 12 (Terran), 15 (Zerg) and 16 (Protoss) functions. They were
chosen by profiling 32 late updates per map with every candidate
relocated, keeping the functions that saved at least four cycles per
byte. Small wrappers that call back into ROM got slower and were left
out.

Paired runs load the same late three-computer save and compare 300
advancing updates.

| Cartridge and map | Frames | `71937af5` mean | Relinked mean | Change | Updates/s |
| --- | --- | ---: | ---: | ---: | --- |
| Terran 10 Outpost | 11,455–11,754 | 773,910 | 708,409 | −8.46% | 21.68 → 23.68 |
| Terran 10 Blood Bath | 11,122–11,421 | 870,175 | 810,434 | −6.87% | 19.28 → 20.70 |
| Zerg 10 Outpost | 11,122–11,421 | 1,057,283 | 983,020 | −7.02% | 15.87 → 17.07 |
| Zerg 10 Blood Bath | 11,283–11,582 | 1,021,878 | 949,066 | −7.13% | 16.42 → 17.68 |
| Protoss 10 Outpost | 11,335–11,634 | 973,020 | 905,076 | −6.98% | 17.24 → 18.54 |
| Protoss 10 Blood Bath | 11,002–11,301 | 1,002,396 | 972,855 | −2.95% | 16.74 → 17.25 |

Sampled world state, unit and building rows, both framebuffer pages,
the palette and the camera match on every update, and the pause saves
written at the end are byte-identical. All 1,800 relinked updates still
exceed the 561,792-cycle two-VBlank budget except 14 in Terran Outpost.

Whole matches start from the lobby with Terran, Zerg and Protoss
computers. Outpost runs 12,000 updates. Blood Bath Melee draws its
start locations from elapsed time, so a faster ROM starts a different
match; those pairs instead load one save written 31 updates after
launch and run to the RESULT screen, which both ROMs reach in the same
250-update block before update 11,750.

| Cartridge and map | Whole-match cycles | Updates/s after 9,000 | Updates/s after 11,000 |
| --- | ---: | --- | --- |
| Terran 10 Outpost | −4.84% | 19.52 → 21.40 | 19.63 → 21.05 |
| Zerg 10 Outpost | −4.82% | 17.97 → 19.67 | 17.94 → 19.39 |
| Protoss 10 Outpost | −2.21% | 17.22 → 17.92 | 16.57 → 17.41 |
| Terran 10 Blood Bath | −3.41% | 20.52 → 22.22 | 15.71 → 16.61 |
| Zerg 10 Blood Bath | −3.07% | 19.76 → 21.12 | 15.07 → 15.75 |
| Protoss 10 Blood Bath | −1.79% | 18.38 → 18.99 | 14.68 → 15.23 |

World state, both pages and the palette match at all 47 or 48 samples,
taken every 250 updates, in each of the six pairs. The Outpost pause
saves at update 12,000 are byte-identical. Openings stay at 29.82–29.86
updates per second in both builds.

The eighteen campaign pause saves listed above pair 150 updates each
with matching state, pixels and final saves. Their cycle change ranges
from −0.01% (Zerg 2) to −3.84% (Zerg 7); the lists were tuned on Custom
Games.

`make` with `SC64_IWRAM_HOT=0` reproduces the `71937af5` Zerg and
Protoss ROMs byte for byte and the Terran ROM's code and data. The
relinked ROMs differ from them in the placement of the listed functions,
their veneers and the ROM offsets that follow.

## Limits

These are emulator cycle counts. Hardware timing, hardware saves and
full campaign replays on the relinked ROMs remain open. The Showcase
ROM and the older mission ROMs keep `gba.ld`. The stack bound depends
on GCC's frame records and address-taken record and on the declared
nesting of the two cycles per ROM. The dispatcher cycle's single
activation rests on reading the bind code, not on a proof; a callback
that re-entered its own dispatcher would need a higher count and a
shorter list. The busy windows remain far above the frame budget:
this is a 3–8% step in a gap of about 45%.

The profiler, the stack-tracking runner and the saves are private local
tools and inputs. The lists can be re-derived with any per-function
cycle profile of a late match: relocate candidates, measure each
function's self cycles before and after, and keep those that pay for
their bytes within the limit that `gba_iwram_hot.py check` reports.
