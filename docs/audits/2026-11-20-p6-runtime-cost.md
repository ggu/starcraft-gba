# Into the Darkness runtime costs

The November 20 normal P10 ROM loads the [verified P6 journal at frame 64,800](2026-11-20-current-p6-save.md)
with HOME L and follows the same controller tape. In three 700-call
windows, the release spends 361,000–376,000 cycles per advancing update
outside the profiled orders and sight stages. A read-only instruction
trace matches the release timeline and loop counters for 157 advancing
updates in the first window. Scenario processing averages 239,767 cycles,
wall traps average 6,331, and burrow release averages 141,627 before the
orders timer starts.

The scenario count snapshot now resolves its active source selectors
before scanning actors. It uses the retained source owner and current
player ownership, which can differ after a rescue. Invalid source
owners still make the affected queries use their existing full scan.

Burrow release only needs to know whether any hostile center is within
96 pixels. Its grouped pass builds a transient list for each 128-pixel
Y band. Each burrower visits the bands that intersect its inclusive
radius and then applies the original distance check. Worlds with a
Y coordinate or owner outside the admitted domain use the full scan.

## Paired ARM measurements

Both runs use the same normal P10 compiler vector and private inputs.
They match all 2,100 update observations for frame progression, screen,
camera and route counters, plus 25 sampled worlds, fog states, displayed
pages, both indexed pages and palettes. The table covers advancing updates
and uses nearest-rank p95.

| P6 tape calls | Advancing updates | Release mean | Updated mean | Release p95 | Updated p95 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 65,754–66,453 | 607 | 1,078,462 | 947,651 | 1,282,606 | 1,154,572 |
| 88,201–88,900 | 694 | 989,308 | 869,537 | 1,136,686 | 1,019,374 |
| 99,501–100,200 | 685 | 1,034,959 | 918,299 | 1,188,164 | 1,071,854 |

Mean cycles fall 11.27–12.13%. These windows still exceed the 561,792-cycle
budget for 30 updates per second. The P10 link has the same IWRAM and EWRAM
endpoints. The burrow routine's immediate stack frame grows from 288 to
560 bytes, and the scenario routine's frame grows from 784 to 800 bytes.
These sizes come from the linked prologues and exclude nested calls.

## Fresh SRAM through victory

The updated P10 ROM runs the full route from HOME, writes the same journal
at frame 64,800, and restores it through HOME L in a new emulator process.
The two runs have identical SRAM images and all 113 named checkpoints,
including victory at frame 99,906 with both heroes alive. The recordings
cover all six briefing pages and 17 authored dialogue pages.

Across the 101,370 recorded calls, total emulated cycles fall from
129,594,538,941 to 108,005,510,733, a 16.66% reduction. Recorded calls
per emulated second rise from 13.12 to 15.75. This full-route measurement
includes briefings, dialogue and save/load inputs. The simulator and
hardware frame-rate gates are still open.

The Command/Bring sanitizer oracle matches 12,000 runtime ticks, and
the burrow fixtures match the independent full scan with and without
axis culling across 150 worlds. The geometry oracle checks 100,000
coordinate pairs.

## Product checks

The four normal ARM products pass the ROM header, SRAM detection and RAM
guards. Terran 10 and Zerg 10 reproduce their prior ROM bytes. The
Protoss 10 and Showcase products both compile the P6 routines and keep
their prior IWRAM and EWRAM endpoints. Ordinary-button opening checks at
60 and 120 calls match the sampled world, palette and both indexed pages
on every product.

The three-AI P10 Blood Bath comparison matches its 32 world and frame
observations and all 1,600 camera, simulation frame and route-counter
rows. In the final 1,000 advancing updates the mean changes from
998,498 to 998,513 cycles. The p95 changes from 1,397,222 to 1,397,180.

| Receipt | SHA-256 |
| --- | --- |
| Release sample | `b7c35305713a474fa7c8bfc91033e2d36ea38a631dcccef3b29eabc2771e9ba8` |
| Updated sample | `31c4fc89af522033f9a66e13242579f97825dd639daf25d3d2b6eaab614d2f7f` |
| Release instruction trace | `1de579d43328990763d50d2df5ba8e2195608156172530f474d4fbc6989047ab` |
| Updated normal P10 ROM | `b130dd4a1958af79b09946b6c2fd4e7f1b1be69870ce7a3b073842508d50f3d7` |
| Full updated campaign replay | `7fd6b192c8ec9da721fdf4e00ff32af91041255fd1d163dd44eae4a5363b3624` |
| Four-product opening check | `a01b114d49fd5f20829363e32f470127c6a02e874e970d4dc6bd40006706e388` |
| Blood Bath release | `d8d07cc28a4ea5fa7e735073ec9e083f27d190c445e5867260d5c7dbd7829cca` |
| Blood Bath update | `ec9a45eeb8816e2e2661f14ed57e8dfa1ba842dc5b373261718e0ef85fa49e29` |
