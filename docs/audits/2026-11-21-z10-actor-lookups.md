# Immutable Z10 actor lookups

The normal Z10 recipe now uses the existing generated tables for actor
flags, flight and collision radius. The generator evaluates the original
functions for the selected roster. Flags or input changes invalidate the
generated header. The game keeps the existing fallback for out-of-range
types. Target permissions that depend on the live mission, actor, cloak
or fog still run in their original order. The tables add no saved state.

`Z10_FACT_TABLES=0` restores the previous recipe. Both normal Make
recipes were linked against the same 39 pinned inputs. The off ROM is
byte-identical to `68efae11`, the stock Z10 runtime on `68d4057b`.
The on ROM is byte-identical to the measured `76d15ed4`. Both retain
EWRAM end `0x02037f78`. IWRAM end moves from `0x03004d88` to `0x03004d30`,
an 88-byte reduction. The sanitizer oracle passes 2,011 checks of all
49 unit types, all 63 building types, and out-of-range inputs.

## Ordinary Z7 and Custom Game replays on `68d4057b`

Both normal cartridges independently write the same ordinary pause
SRAM at frame 5,641. Both fresh HOME loads use one ordinary A to restore
the Hatchery selection. The full 225,208-call resume wins at frame
228,128. All 254 named gameplay records, the counter boundaries around
2,319 timed updates, and ten palette, camera and two-page image cuts
match. The route counters agree in all sampled rows. Mean guest cycles
over the full resume fall from 1,324,227 to 1,308,709 (1.17%).

| Calls | Advancing updates | Mean cycles, stock → table | p95, stock → table | Peak, stock → table |
| --- | ---: | ---: | ---: | ---: |
| 47,758–48,237 | 477 | 2,159,356 → 2,135,331 | 3,145,671 → 3,117,796 | 5,062,030 → 5,023,805 |
| 153,812–154,741 | 926 | 2,685,259 → 2,645,051 | 4,595,827 → 4,549,569 | 6,856,377 → 6,816,902 |
| 154,742–155,650 | 905 | 2,633,273 → 2,592,310 | 4,610,035 → 4,574,889 | 6,419,346 → 6,394,813 |

Ordinary Z10 Custom Game replay covers the compact map with a Zerg
human and one Terran, and Blood Bath with a Protoss human and two CPUs
or a Terran human and three CPUs. Each pair saves at 360 updates and
resumes in a fresh process. Seats, saved world, restored world, the
240-update continuation and every paired SRAM export match.

## Shared world-depth release

On `defd807a`, the same Make vectors regenerate the identical table
header and pass all 2,011 fact checks. The control matches the released
Z10 binary `eeca18b2` byte for byte. The selected recipe produces
`0dfe67c1` with the same memory endpoints as the earlier table build.

Both ordinary Z6 runs write identical pause journals at frame 18,721
with Kerrigan selected and two pending weapon impacts, then restore
the same world from HOME in a fresh process. Each reaches victory at
frame 56,913, with all 65 named gameplay observations and eight dialogue
pages matching. Six Custom Game runs again match across the three
lineups above, including exact paired pause SRAM and fresh continuations.

The Z6 receipt SHA-256 is
`f8378246c3e5814305329bae4c3106cfc146640bbfe82dbbb9678e192b9c5e62`.
The Custom Game receipt SHA-256 is
`4345cad88eedcab1a517c3dae5870277a62aeed891f0eda66ce080692033c6fe`.

The independent 225,208-call Z7 resumes on this release also reach
frame 228,128 with all 254 named records, ten image cuts, the sampled
route counters and exact SRAM matching. Mean cycles move from
1,326,810 to 1,311,303 (1.17%). The early busy window improves 1.11%
and the two late windows improve 1.52% and 1.57%. The runtime averages
12.79 emulated main-loop updates per second across this tape.
The comparison SHA-256 is
`af170e1f3bc26c07f603569bc7107131f61110eff0291fcfef0984fe43ae802d`.

After composing with `64192d4e`, the tables build becomes `7dd47332`,
and the stock control is `7a290e51`. For each configuration the linked
instructions, symbols and data match its preceding depth-release
build after resolving 20 relocated linker veneers. The 647 adjusted
call sites still reach the same functions. The standalone comparison
keeps the measured cycle claims scoped to `defd807a`.
The composed Custom Game pair retains the same exact pause SRAM and
continuation in each of the three lineups. Its receipt SHA-256 is
`75d57b25ee826392d6d2f42aebd4acb821f66e718652153c67224fe367fd5bcc`.
The composed Z6 pair also reproduces the original pause SRAM, all
65 gameplay observations, eight dialogue pages, and victory from
both fresh loads. Its receipt SHA-256 is
`68335a9ccfd420464f1f97a64573acdd3c856777b454c77283d900640e7f6786`.

## Other target and table trials

The same existing tables on the Showcase profile free 392 IWRAM bytes.
An ordinary Siege Line replay matches all 24 sampled states, palettes,
displayed pages, HUD bands and both framebuffer hashes. Cycles rise from
119,287,017 to 119,531,328 (0.20%). The normal Showcase build retains
its current lookup selection while busier scenes are measured. P10
already uses the actor tables.

A separate geometric-first target query checks the best candidate
against the live policy and falls back to the original scan if it is
ineligible. It matches 28,000 HOST queries under sanitizers, 500 index
fallback controls, and the full Z7 resume with matching SRAM, 254 state
checks and ten image cuts. The three timed means change by +0.25%,
−0.10% and +0.55%. Its overall cycles rise by 0.22%. It remains opt-in.

The separate combat-only facts table replaces repeated type lookups
inside attack eligibility. By itself it cuts late Z7 window means by
2.64% and 2.89% and late zero-route mean by 3.38%. The full resume is
0.11% faster with matching SRAM, 254 state checks and ten image cuts.
On `defd807a`, adding it to generated actor tables reduces the same late
windows by a further 1.91% and 2.01%, and the full tape by 0.085%.
The composed table passes 2,727 fact checks and again matches all
gameplay, routes, image cuts and exact SRAM. Keep combat facts opt-in
while checking other profiles and pursuing larger sustained gains.

| Private receipt | SHA-256 |
| --- | --- |
| Actor tables, paired normal Z7 win and timed windows | `3f80566b8ffef82e7656501b38944860c55b5e8c9f6c5946fc7c0a217dd934f3` |
| Actor tables, paired Custom Game SRAM | `435e6af01b29e0821223134718dd60661fcaa38c960f08385578f0188b6c3911` |
| Geometric-first target paired win | `63e6b891afb81cff439efba6a65e44073c58e9a6f44b3712729cade7490e74be` |
| Standalone combat-only paired win | `d1ee25c67461550024eaeba7868ed9b1b4fbd17f451bd834dd3b40b0d12ad1bc` |
| Combat facts composed with actor tables | `d6faf72e99f42279ffda14cfc24ff8d92c69a02ba295ca20bc8fe274cce35457` |

These measurements count emulated guest cycles. Replay the late route
and no-route windows with the new stock build for the next experiment.
The current gameplay windows remain above the 561,792-cycle budget for
roughly 30 fps.
