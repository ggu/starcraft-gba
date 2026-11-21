# Shared persistent collision buckets

The Terran, Zerg 10, Protoss 10 and Showcase targets reconcile the existing
collision buckets across updates and skip removals for unlinked slots.
The index remains a derived runtime cache and uses the existing save format.
The strict Nydus placement query uses the legacy Zerg/Protoss 18-pixel flying
search reach. A real-engine differential exposed missing flying candidates
in the earlier nine-pixel persistent reference.

`make test-sc64-persistent-lifecycle` runs the asset-free ASan/UBSan fixture.
It checks 10,000 deterministic birth, death, reuse, transport flag, movement,
type and bounds changes, index linkage after reconciliation, and 29,790 strict
placement comparisons against the persistent scalar reference. The prepared
real Z10 engine contact fixture additionally passed 4,220,593 ordinary and
4,220,593 strict checks, 649,326 masks, 133,840 inactive-index masks and 71,902
mutations, with no failures. That fixture used the preceding prepared Z10
engine. The real spawn fixture passes
birth, reuse, bounds fallback and transport comparisons against its full-world
scan. Both reports are retained privately, without game assets in this change.

The proper Z7 Nydus HOST fixture produced byte-identical control and candidate
reports for strict exit placement, paid Egg transit in both directions,
concurrent passengers, blocked exits, endpoint death and slot reuse, ownership
change, save/load, malformed saves and fresh boot. Its report SHA-256 is
`3548d4615fe102d3fbc90ec84f6b3382a3e4d9a71c9bdde47c054aa1e1784753`.

## Current four-cartridge qualification

The control is source release `c05ac442`, including the shared changed-row
sight update. On Zerg 10, both ARM binaries resume the same ordinary Blood
Bath pause and run 3,600 updates. All 31 world, two-page and palette cuts,
route counters, 16 fresh-continuation cuts and the saved journal match.
The ordinary Protoss 10 late replay matches 68 cuts, per-call route state
and SRAM, and reaches victory at frame 46,057. The Terran Outpost tape
matches 90 cuts across 17,636 calls.

| Normal ARM replay | Mean guest cycles | p95 guest cycles | Peak guest cycles |
| --- | ---: | ---: | ---: |
| Zerg 10 busy Blood Bath | 833,082 → 825,585 | 1,124,208 → 1,114,632 | 3,702,740 → 3,694,114 |
| Terran Outpost, 800 busy updates | 990,259 → 985,530 | 1,281,307 → 1,292,346 | 1,948,363 → 1,944,721 |

The full Protoss 10 tail uses 15,268,498,787 guest cycles versus
15,388,866,488 on the control, a 0.78% reduction. The Terran full mean
falls 0.44%, and its busy-window p95 increases 0.86%. These timings
are guest cycles. Physical frame pacing remains open.

Campaign and Showcase openings match on the four exact products, as do
41 Blood Bath camera cuts per product. The three race cartridges pass
54 ordinary Custom Game starts and fresh 120-update continuations across
both maps, all human races and one to three named CPU seats.

| Cartridge | EWRAM end | IWRAM end | Untouched stack in paired Outpost canary |
| --- | --- | --- | ---: |
| Terran | `0x0203a6dc` | `0x03004b38` | 8,048 |
| Zerg 10 | `0x020396a8` | `0x03004ef0` | 7,244 |
| Protoss 10 | `0x0203af68` | `0x03004e10` | 7,248 |
| Showcase | `0x0203899c` | `0x03004e68` | 7,568 |

The instrumented candidate and control match every measured world and
framebuffer cut. They have the same minimum stack watermark within each
case, including 6,832 bytes in Showcase Siege Line and 6,504 bytes
through the late Protoss 10 victory.

The Z7 replay matches 228 world checkpoints and nine display cuts
through call 155,650. Its dense and two late windows reduce mean cycles
by 1.72–1.74%. The older winning tape reaches call 231,031 on both
current builds without satisfying the victory assertion. The observer
does not retain final state after that assertion. Recapture the first
changed input at call 17,264 and qualify the current winning path.

| Local evidence | SHA-256 |
| --- | --- |
| Zerg 10 Blood Bath and save continuation | `19a36f3f5b231b1a86b3e6ba8222fecea78313c4e220b81668a50d9d814e199f` |
| Protoss 10 late replay | `232f555dfd70d9d763cd4968d2c7f930dd3b0ff819781a7b0c89b0dd602057ec` |
| Terran Outpost replay | `dca5c1568df28ea1a424793fcabdd8fac6c999ce615666d51f3f668f8ffb68ab` |
| Custom Game start and fresh continuation matrix | `d3e3c3fdd50fdd48b4088cdc9b61daeeb4ea80f0f18f56fd49ba1c9032c83658` |
| Four-product stack watermarks | `e58fb36732fb49628d16f59984c5edd06f174007febd80c48292dc53360a83a8` |
| Z7 paired observations through call 155,650 | `262176feb7956650507262d43a0147637e6ad4b5bc48d1b0bf7e2a9c1c030638` |
| Z7 terminal assertion logs | `50bb329c1b89fc661df67c12e9b6ebea1bbf78db33cb0f0245c9694cb302b433` |
