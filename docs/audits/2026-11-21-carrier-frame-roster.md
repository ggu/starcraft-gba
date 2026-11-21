# Carrier frame roster

The Carrier service previously scanned the entire actor table for each
training, flight and movement pass. It now collects live parents and
Interceptors once just before those passes. Each list keeps ascending
actor-slot order. Training may add a child that starts in the hangar.
It enters the next roster, and launching an existing hangar child
still leaves it available for movement on the same update.

Parents and children occupy opposite ends of one fixed-capacity slot
bank. End-of-update cleanup reuses the bank after scripted actor changes
and applies parent death before checking the collected child slots.
Direct helper calls outside the frame still scan the live table. The
three normal Carrier-enabled ROMs use eight-bit slot numbers. The
scratch bank and its counters add 208–256 bytes to their linked EWRAM
footprints after alignment, with the existing IWRAM endpoints. The
roster uses transient memory. Build with compiler definition
`-DSC64_CARRIER_FRAME_ROSTER=0` to select the prior scans. All four rollback ROMs reproduce the
preceding release byte for byte.

The shared service runs on Terran, Z10 and P10 and on the separate
Showcase Carrier preview. The normal Showcase ROM still uses its
existing scene and save profile. [Carrier availability](../CARRIER-PROJECTILES.md#current-status-2026-11-21)

## Local verification

With prepared P10 inputs, run `make test-carrier-frame-roster`. ASan
and UBSan cover direct helper calls, a trainer retired and recreated
in the same slot, parent death and reuse, all 240 child slots,
the skipped source phase, and ordinary flight and save/load.

The three race cartridges pass 54 ordinary Custom Game starts and
fresh HOME continuations. Both journals in each case match the
preceding release, for 108 byte-identical 32 KiB files. The cases
cover both maps, all human races and one to three AI seats.

Both P10 versions load the Eye of the Storm journal at frame 36,761
and win at frame 46,057. All 68 sampled world and indexed-page cuts,
the palette and the result journal match. The paired Showcase Carrier
preview starts Carrier Approach from the scene menu and matches
20 cuts through frame 301, with up to nine simultaneous pulses.
Its three-AI Blood Bath run matches 35 sampled cuts through the
frame-17,121 pause and fresh reload, with identical SRAM. Those
Custom Game samples have zero active Interceptor pulses.

## Emulated update cost

The matched 3,600-update Z10 Blood Bath sample has three AI seats and
live pulses. All 31 sampled worlds, projectile arenas, indexed pages
and palettes match, along with all route counters, the pause journal
and 16 further fresh-load samples. The matched Outpost Run tape has
18,743 ordinary controller calls and 95 matching world, palette and
indexed-page cuts. Its busy window covers 800 updates.

| Guest cycles | Prior | Roster | Change |
| --- | ---: | ---: | ---: |
| Blood Bath mean | 923,198 | 880,942 | −4.58% |
| Blood Bath p95 | 1,216,520 | 1,171,673 | −3.69% |
| Blood Bath peak | 3,806,440 | 3,760,529 | −1.21% |
| Outpost Run busy-window mean | 1,243,696 | 1,201,159 | −3.42% |
| Outpost Run busy-window p95 | 1,654,794 | 1,604,039 | −3.07% |
| Outpost Run busy-window peak | 2,669,640 | 2,677,608 | +0.30% |
| Outpost Run full replay mean | 831,053 | 818,774 | −1.48% |
| P10 resumed finale mean | 1,817,177 | 1,778,682 | −2.12% |

Busy averages exceed the approximately 562,000 guest-cycle budget for
30 fps. Physical frame pacing remains unverified.

The instruction observer attributes all 33 sampled updates and
reproduces the 31 world checkpoints on the roster ROM. In the middle
band, orders account for 251,694 guest cycles, the shared world-depth
pass for 163,191 and defense targeting for 31,458. The worst sampled
update still spends 2,435,266 cycles in orders. Prioritize the route
bursts for long stalls and the large draw passes for recurring cost.

| Local comparison | SHA-256 |
| --- | --- |
| Blood Bath | `b35d8a1ee7893ef6ae882c01b1422dee0058e16cdaad8c230f50f79438dc1392` |
| Outpost Run | `ffff124ec249f4553e71e152dd40606e6882856a59ebea631788f050a0fd0bcc` |
| P10 finale | `3ad71968aae7b646134f40a505aa207ed20c76b9ce7ada2eba8cbe77460ba50b` |
| Custom Game pause and fresh HOME | `ef742ead8201e29c6cc6c39003eff85bc99ca901e3eaa1ea3f758ba756cc3b6f` |
| Custom Game cross-version journal bytes | `5d712835be0925ae038955194459dd6d4c3a631f9485c27da6cd366fb83a3e1a` |
| Carrier Approach scene | `f0f008077ed1bde13b147a57a619b110276e0b31e64fb4b05f5e86865370c18b` |
| Showcase Carrier Custom Game | `c0337bf3b36eb635df9bc1b638a6e5f3a79ea674bd16922e8f19f9209bda4f9a` |
| Current instruction attribution | `fd76064d8dfa8ec4de155b4ab18e30c152889f4b05b6d98434313c7d244ca29e` |
