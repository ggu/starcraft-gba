# In-mission text boxes

The N64 campaign stops a mission to show a box with a head, a speaker's name and text.
The canonical race products now do the same: Terran, Zerg and Protoss, including the Z8,
Z9, Z10, P8, P9 and P10 candidates. The boxes are silent, as they are on the N64.

The text, names and heads are game content. They stay in the ignored
`.local/sc64/text-box/generated/`, and nothing here quotes them.

## What a player sees

A box opens over the battlefield when its trigger row runs. A or Start advances: a long row
takes more than one page, shown as `1/2` under the head. Nothing else does anything while a
box is up, so the pause menu can't open. The mission clock, orders, production and AI stop,
and music keeps playing. When several rows fire together, they show one after another.

The authored status lines that T1, P2 and others used to show are gone from these products,
since the source's own text replaces them. The objective on the pause panel is unchanged.

## Speakers and heads

The audit counted 26 speaker IDs against 23 briefing portraits. Those portraits aren't what
the boxes use. Native code at `0x8003FB48` picks everything from a table in the ROM, read here
at offset `0xC76BC`, with one byte per unit ID plus three Advisor entries:

- The byte, minus one, is an image in BOLT directory `016`. That bank has 95 heads of 30 by
  28 pixels, drawn with the world palette.
- A Transmission row names its speaker by unit ID. A DisplayText row always gets the Advisor
  for the player's race, through table entries 229 to 231.
- The name comes from the unit-name table in `00A/004`, as `0x8007B710` does it. A map can
  override it, as `0x80138564` does through a table of string IDs. Six maps do this for a
  speaker, and the importer reads their `UNIS` sections for it. The use-default check there is
  from the PC format: it wasn't confirmed in native code.
- Unit 106 is shown as the Advisor, through a check in the same function.

So extra art is needed, and it is small: the rows use 19 heads, 15,960 bytes in all. The 24
speaker units still in use after disabled rows are dropped share them with the three Advisors.
The heads go through the same palette remap as the sprites. Some colors shift, since the
GBA palette has fewer colors for them.

| ROM cost | Bytes |
| --- | --- |
| Heads | 15,960 |
| Wrapped text, 372 pages | 19,713 |
| Row tables and page pointers | About 5,000 |

## Which rows are included

`tools/sc64_text_box_assets.py` reads all 30 maps and packs 325 rows: 213 Transmission and
112 DisplayText. The source has 215 and 114. Dropped are the four with the disabled action
flag, the same flag every runtime here skips. A Transmission at location 0 would be dropped
too, since `0x80080DA8` completes it with no box, but none remains in these maps. Z7 has no
text rows at all.

Both native checks that could hide a DisplayText row pass in the state read: option bit `0x400`
at `0x800D13F0` is set, so the one row without the always-show flag still shows. Native code
also shows the box only to the player the trigger runs for. Runtimes here post a row only for
the human's runner.

## How it works

**The simulation doesn't know.** `game_update()` becomes a small wrapper. While a box is up,
a frame goes to the box and the simulation takes no step. Otherwise the old function runs,
under a new name and with its body unedited. That matters because any edit to the old body can
change GCC's inlining and move `update_orders` into IWRAM.

**A box is a pure insertion.** Rows queue during an update, and the box opens after it. A
box's own frames change no game state, so the inputs on either side produce the world they
would have produced with no box. The trade-off: other actions in the same update, such as a
camera move that follows a Transmission, have already happened when the box opens.

**A press arms and a release advances.** This keeps a dismissing press out of the battlefield.
It also means the frame after a box sees the same previous keys as the frame before it, which
is what keeps a key tape exact. Start also advances, as it does on the N64, which lets a tape
holding A across a box dismiss it without releasing that A.

**No RAM grew.** `__bss_end` and `__iwram_end` match the base in all nine products. Z9 and Z10
have 8 bytes of EWRAM to spare, so the queue lives in the last 32 bytes of the padding after
the second mode-4 page, `0x06013FE0`. It holds up to 13 rows. A full queue drops the new row,
which never blocks a trigger. The optional death-effect VRAM storage now stops short of it.

**Saves.** A box is never saved. The pause menu can't open while one is up, so no ordinary
save captures one. A load or new game clears the queue. The save layout doesn't change.

## Trigger points

Each runtime posts at the place it already executes the row:

| Missions | Where | Key |
| --- | --- | --- |
| T4, T5, T6, T7 to T10 | World callbacks for actions 7 and 9 | Source string |
| Z4, Z5, Z6, P4, P5, P6, P7 | Each runtime's action handler | Source string |
| Z8, Z9, Z10, P8 | Timed-action start, and the presentation callback | Source string |
| P9, P10 | Action switch | Source string |
| T2 | The existing `bw_message()` callback | Trigger and action |
| T3, Z3, P3 | Model events | Trigger and action |
| T1, Z1, Z2, P1, P2 | Mission conditions | Trigger and action |

T1, Z1, Z2, P1 and P2 are hand-written, so they have no trigger program to hang rows on. Their
tutorial rows use `race_campaign_text_tick()`, which checks the source's counts every eight
updates. It remembers what fired in the high bits of the saved `objective_flags`, which adds
no save bytes. Some source conditions are approximated:

- Waits before these rows aren't timed, so a row can open a few seconds earlier than on the N64.
- P1's row about a Gateway without power uses "no Pylons anywhere" for the source's region test.
- P1's base-clear test uses the existing base rectangle.
- Z2's "Chrysalis in danger" row opens when its carrier is lost. The source tests a region.

## Differences from the N64

- **Layout.** At 240 by 160, text sits to the right of the head, four lines to a page. The
  N64 fits every row on one page, and here 47 rows need more than one.
- **Trigger timing after a box.** The port keeps its existing holds after a Transmission: 4
  seconds in Z8, Z9, Z10, P8 and P10, and about 3 seconds in the older runtimes. On the N64 the
  action waits only for the box, so the next action runs as soon as it closes. Keeping the holds
  is what keeps every recorded tape valid. See [the audit](audits/2026-11-04-text-boxes/README.md).
- **The top-right resource display.** The N64 hides it while a box is up. The GBA keeps its HUD.
- **Background.** The N64 box is translucent. Mode 4 can't blend, so this one is a solid panel.

## Replay tapes

No recorded tape had to change. Tape players now dismiss boxes themselves, between tape
ticks, through ordinary input to `game_update()`:

- `tests/sc64_text_box_replay.h` does this for the P8, P9, P10, Protoss and T9 ARM players.
- `common/main.c` does the same for its debug scheduler.

Host rule fixtures have no one to press a key, so under `GBA_HOST` a box counts as read and
closes at once, unless a fixture defines `SC64_TEXT_BOX_HOST_MODAL`.

## Checks

```sh
make test-sc64-text-box
make test-sc64-text-box-game
make sc64-text-box-arm
```

The first needs no game data and runs in `./dev test`. The second needs the prepared Terran
inputs and runs in `./dev sc64 test --profile terran`. The third builds ARM probes, which run
under mGBA through `tools/sc64_p8_arm_replay.py --prefix PX run`. To refresh the private data:

```sh
make sc64-text-boxes
```

`./dev sc64 prepare` runs the same importer.
