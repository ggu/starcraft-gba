# Zerg 7 replay authoring after shared depth picking

On source-release `71809328`, the prior ordinary input tape writes the
same pause journal at gameplay frame 5,641. After fresh HOME, the first
observed difference from `bf663a2` is the A press at call 7,752.
Two idle Drones share world position (1140, 1684). The cursor is
(1139, 1686), and a friendly Hatchery is within pick range. The
older mixed picker chooses slot 78. The shared picker considers
both Drones and selects slot 91, which paints in front at the GBA tie.
The retained tape later produces a different economy and unit count.

The Z7 HOST controller can now render between input updates with
`--render-input-frames`. A physical cartridge calls the renderer
between updates, which puts the camera and last painted walk state
where the picker can read them. A HOST run that skips rendering
leaves the camera at stale coordinates and can predict a different
selection even with the same gameplay flags. Use route-purity checks
when generating a trace and keep the rendered option in the receipt.

The rendered controller reaches HOST victory at game frame 287,798
after 289,599 calls. The exact normal ARM cartridge matches all 60
named observations through call 23,571. At call 23,879, HOST gas
is 3 and ARM gas is 11. A tighter observer sees the deposit at
call 23,714. Drone slot 115's first observed positional difference
is at call 22,926, before it reaches the extractor and returns.
Slot 40 has an earlier positional difference at call 21,435.
The packed-route and isolated cloak-query ARM trials reach the
same aggregate observations. Isolate the first underlying movement
difference before using this tape as an exact HOST-to-ARM oracle.

| Local receipt | SHA-256 |
| --- | --- |
| Read-only input comparison on `bf663a2` | `14cf2c14527c28a2bf3b503f3df3ad151510b14b6e4d9c7b20cd357826c9b391` |
| Read-only input comparison on `71809328` | `8419f926bff478830379cf78d56bd1fce2c27f522764533c40860d4efa6f979c` |
| Rendered HOST controller tape | `90ef711583c5d9286bb68bf870dfc8d0d5410f2b9d64b0a7de8863d0faf8f277` |

## Route output packing trial

A local ARM trial packs the existing dense route bytes into the same
three-bit fields using a 148-byte IWRAM kernel. The compiled function
preserves the ARM callee-saved registers and alignment. Dispatch requires
positive dimensions, a word-aligned destination and complete 32-cell
rows with the standard padding. The C path handles the other layouts.

All four product ROM and RAM checks pass. The Z10 link reaches
`0x03004ff0`, 16 bytes below its IWRAM guard. The kernel pushes 32
bytes of registers while active. The P10 tail retains all 68 world,
two-page, palette and fog cuts, all 9,489 frame and route rows, victory
and SRAM. Main-loop mean changes from 1,550,076 to 1,549,962 guest
cycles. At full-point-fill calls 38,952 and 45,452 the whole update
loses 73,208 and 63,195 cycles, respectively. The 3,600-update Blood
Bath mean is flat at about 821,000 cycles, with matching sampled display
and world state, fresh continuation and SRAM.

A synthetic ARM probe checks 24 cases against an independent
per-cell bit packer. The test exercises 64-, 96- and 128-column
layouts, random high bits, aligned outputs and six fallback cases
with nonstandard padding, an unaligned output or a non-word row.
Every output byte and guard matches. On the aligned cases the IWRAM
kernel uses 592,164 cycles to write 41,472 bytes, versus 858,960
for the C word packer. Placing the same ARM code in ROM uses
1,401,480 cycles. This is isolated emulator timing. Runtime stack
watermarks and substantial sustained gains are still open. The
trial remains private.

| Local evidence | SHA-256 |
| --- | --- |
| P10 paired candidate replay | `21eec34d94e309166c4cca04e9becc634802c55f7dd882f516d3af3d49b9a68c` |
| Busy Blood Bath paired comparison | `4531f03b5aa4cfaffffdb7985544d0feaa68c295fc48d5814a478ecdad7a7b19` |
| Direct IWRAM ARM oracle | `2e57a2cd196842572d737231f21f87ba4d223a9c87924b8494e2ca4b0bef3d2e` |
| Direct ROM ARM oracle | `008a54c00b90b17e58ce03fe362575f198f281ead2102649ce56afcb6769d17c` |
