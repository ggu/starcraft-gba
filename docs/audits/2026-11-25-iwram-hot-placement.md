# Hot ROM functions in the unused IWRAM stack reserve

`common/gba.ld` keeps IWRAM code below `0x03005000` and leaves the
11,776 bytes up to the stack top at `0x03007E00` for the main stack.
That reserve was never derived from the program. On `71937af5` the
three race cartridges end their IWRAM code at `0x03004B78` (Terran),
`0x03004FD0` (Zerg) and `0x03004F08` (Protoss), and 64–79% of late
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
GCC's call graph (`-fcallgraph-info=su`), checked against the linked
ELF and completed from it.

- Frame sizes come from the ELF's call-frame information. GCC's
  figure agrees for every function but one: on Zerg,
  `sc64_direct_target_can_damage` pushes its four register arguments
  and uses 32 bytes where GCC reports 16. A difference of any other
  kind means the dumps are from another build and fails the check.
- Every branch in the ELF from one function into another is an edge.
  This adds 91 to 110 edges per ROM that the graph leaves out, almost
  all to GCC's switch and division helpers.

The graph and the ELF leave seven things open, and the check has a rule
for each.

- A call through a pointer may reach any linked function that GCC's
  `-fdump-ipa-cgraph` dump marks `Address is taken`: 34 functions on
  Terran, 44 on Zerg and 48 on Protoss. A callback the linker
  discarded is not a target.
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
- The pointer rule also makes one cycle per ROM out of 8 to 15 trigger
  and save dispatchers. Each is declared as one activation. The basis
  is the code: GCC's dump shows every member installed through a named
  ops table or a bind function for one layer (source dispatch, count
  cache, world adapter, timed service), and their pointer calls go to
  the layer below or to game services.
- Each declaration carries a fingerprint of the cycle as it was read:
  the member functions and the direct calls among them and, for a
  dispatcher cycle, which members call through a pointer (with their
  site counts) or have their address taken. An undeclared cycle fails
  the check. So does a declared one whose fingerprint no longer
  matches, and a declaration that matches no cycle.

- Every linked function must be reachable from `main` or an interrupt
  handler. One that is not is called from code with no record: the
  reset code, a libgcc hook such as `__aeabi_idiv0` written in C, or a
  call made in inline assembly. The check fails instead of leaving its
  chain out. It also fails on an address-taken symbol that has no frame
  record (an assembly routine used as a callback) and on a direct
  callee that the linker replaced.
- Stack moved inside an inline `asm` statement or a naked function has
  no frame information. The sources have three `asm` blocks and none
  touches the stack or calls; a unit test keeps it so.

The build fails unless `__iwram_end` plus the bound plus a 512-byte
margin stays at or below the stack top.

| Cartridge | Stack bound | Deepest chain | IWRAM may end at | IWRAM ends at | Spare |
| --- | ---: | --- | ---: | ---: | ---: |
| Terran 10 | 7,792 | pause save, the dispatcher cycle, then a trigger action | `0x03005D90` | `0x03005AE0` | 688 |
| Zerg 10 | 5,848 | depth-ordered actor drawing | `0x03006528` | `0x03006108` | 1,056 |
| Protoss 10 | 5,940 | campaign tick, the dispatcher cycle, then a save validator | `0x030064CC` | `0x03006018` | 1,204 |

Values are bytes. The Terran and Protoss chains pass through the
dispatcher cycle and join code the game does not combine, such as a
save callback reaching a trigger action. They are bounds, and the
measured depth below is lower. The deepest chain without pointer calls
is actor drawing: `sc64_draw_depth_ordered_actors` alone has a
3,384–4,280 byte frame. Interrupt handlers run on the IRQ stack; their bound is 304 of
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

The IRQ stack reaches 296 bytes in these runs, 8 below its bound. The numbers are the same
before and after the relink. The [campaign replay
audit](2026-11-25-iwram-hot-campaign-replays.md) extends the
measurement to Protoss 1–9, Terran 2, Zerg 3 and Zerg 10, including
victory, result and briefing transitions. Its deepest main stack is
Zerg 3 at 5,720 bytes, 128 under the Zerg bound and 1,696 above the
relocated code, and the relinked low point equals the previous link's
at all 6,708 readings.

## Measured cost

All figures are ARM7 guest cycles in mGBA with no player orders. The
lists hold 12 (Terran), 15 (Zerg) and 16 (Protoss) functions. They were
chosen by profiling 32 late updates per map with every candidate
relocated, keeping, within each cartridge's byte limit, the functions
that saved at least four cycles per byte. On Terran the limit left out
two that qualified, `sc64_contact_memo_move_cells` and
`sc64_p3_can_attack_from`. Small wrappers that call back into ROM got
slower and were left out.

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

The [campaign replay audit](2026-11-25-iwram-hot-campaign-replays.md)
replays the recorded routes on both links for all 30 missions: 23 to
victory (Terran 1–10, Zerg 1–8, Protoss 1, 6, 8, 9 and 10), four
partial routes and three from pause saves, with saves, fresh reloads,
victory records and next-mission deployments. 73 of 74 pairs match
completely; the Protoss 4 tape, which no longer wins on the current
ROM, is identical on both links up to the same driver error. Zerg 7
runs 270,585 updates from HOME through all 206 checkpoints, and its
victory save equals the packaged run's. The display control register
differs at 49 of 15,860 samples, always while a queued page flip is
pending; the page due on screen and both pages' pixels match
throughout.

`make` with `SC64_IWRAM_HOT=0` reproduces the `71937af5` Zerg and
Protoss ROMs byte for byte and the Terran ROM's code and data. The
relinked ROMs differ from them in the placement of the listed functions,
their veneers and the ROM offsets that follow.

## Limits

These are emulator cycle counts. Hardware timing and hardware saves
remain open. The Showcase
ROM and the older mission ROMs keep `gba.ld`. The stack bound depends
on the ELF's call-frame information, GCC's call graph and
address-taken record, and the declared nesting of the two cycles per
ROM. The dispatcher cycle's single
activation rests on reading the bind code, not on a proof; a callback
that re-entered its own dispatcher would need a higher count and a
shorter list. The busy windows remain far above the frame budget:
this is a 3–8% step in a gap of 27–47% (about 45% on Zerg and
Protoss, about 30% on Terran).

The profiler, the stack-tracking runner and the saves are private local
tools and inputs. The lists can be re-derived with any per-function
cycle profile of a late match: relocate candidates, measure each
function's self cycles before and after, and keep those that pay for
their bytes within the limit that `gba_iwram_hot.py check` reports.
