# Dense depot flood continuation on Blood Bath

The regular Zerg cartridge reloads an ordinary three-CPU Blood Bath
journal through HOME and advances 3,600 neutral updates. At simulation
frame 19,360, the walkability grid opens six cells after a Sunken Colony
is destroyed. A read-only mGBA observer measures 34 depot field lookups
across three depots. The facility calls take 1,102,115 guest cycles,
including the first lookup's grid rebuild. All five external frame
clocks, route counters and world observations match the uninstrumented
replay.

Twenty-eight lookups hit an existing field. Three fills start a new
field and three continue a parked queue. The first reachability check
for depot 2 discovers 2,595 walk cells and takes 652,457 cycles. Its
next fill resumes from that frontier and discovers 97 additional cells.
All 34 requests use the depot circle. These costs make the long new
expansion the first target.

## Shared route change

For a parked-capable 64×64 facility search still expanding after 768
pops, translate the remaining availability bits into the existing
padded-grid scratch. Continue the same FIFO with the existing pop and
neighbour order. The byte grid records only new directions. A sparse
suffix updates its newly discovered cells in queue order. A longer
suffix packs rows and merges the new directions into the prefix,
restoring the exact remaining availability bits. The queue keeps its
original cell indices throughout. The next request can resume the same
parked frontier.

The threshold is a measured tradeoff. A 384-pop trial cut the longest
depot lookup more but raised a shorter resumed lookup from 147,342 to
251,050 cycles. A 512-pop trial with sparse publication improved the
overall peak by 3.26%, with both indexed pages identical at 31 cuts.
Its displayed-page bit differed at one cut. The 768-pop version
matches the displayed page at all 31 cuts. It applies through the shared
route code whenever the existing 64×64 frontier and dense-grid features
are active.

| Busy Blood Bath metric | Current release | Dense continuation |
| --- | ---: | ---: |
| Mean guest cycles per update | 812,646 | 811,651 |
| Nearest-rank p95 guest cycles | 1,102,874 | 1,099,651 |
| Peak in-game guest cycle counter | 3,677,137 | 3,566,597 |
| External clock at frame 19,360 | 3,677,630 | 3,567,090 |
| Inclusive depot field lookups at that frame | 1,102,115 | 991,445 |
| First depot 2 lookup at that frame | 652,457 | 528,947 |

The mean falls 0.12%, p95 falls 0.29%, and the peak falls 3.01%.
All 3,600 build/hit counter pairs, 31 world and indexed-video cuts,
16 fresh title continuation samples and the entire exported save match.
The breakpoint observer reproduces all 34 query results, route slots,
frontier sizes and discovery totals in the peak update. The instruction
observer measures the same lowest system stack pointer, `0x03006950`,
and IRQ stack pointer, `0x03007e98`, on both builds in that update.

The asset-free facility oracle passes six sanitizer variants. It checks
packed directions against an independent flood, including multi-depot
resumes that park after the dense transfer. All eight normal and preview
Terran, Zerg, Protoss and Showcase images pass the link guards with
identical RAM endpoints to their respective control. The 9,489-update
regular Protoss 10 victory replay matches all 81 world, fog, HUD,
framebuffer and route cuts and the victory journal. It still averages
11.01 emulated updates per second. Hardware frame pacing remains open.

The prior [native depot contact audit](2026-11-21-blood-bath-worker-opening.md#contact-follow-up)
measures N64 worker contact at a structure footprint. This change
preserves the current GBA deposit geometry. Keep contact parity in the
worker follow-up. For routing, prioritize earlier proof of reachability
when openings leave a valid old route, without skipping a nearer depot
that became reachable. The measured peak still includes about 870,000
cycles of separate point floods.

| Local evidence | SHA-256 |
| --- | --- |
| Control Z10 ROM | `00b7bd4e6796519319a0e5470cc4f82ee502cdc29b96b543476bde1a5f23777a` |
| Dense continuation Z10 ROM | `c86d475fd3c9edc46af5642e463c06c5c25a91caa790c4ff0764c0e666503d2e` |
| Control depot observation | `a116f19846959f6b0cbfd43d700ca0dbdb4acc5999b9ad5335f151ae4bea9ebb` |
| Dense continuation depot observation | `cd511b519604ac3dc18cb8b00ae936f034d42b61be8b53b874c75499f40a6a60` |
| Paired stack observation | `3bb681dffc60ea44a57fc5cbee9736bfcbe07c70f73963fd300a0b026eaa03d2` |
| Dense Blood Bath replay | `fe78614d6febc993a31221da3d167a061151eaaff650fd268c6518be1295bd3a` |
| Protoss 10 replay | `a9f38617e2dc39f3f1b55e53fd4e068c5e0d55d5a010081c2cb7fb4acc81e596` |
