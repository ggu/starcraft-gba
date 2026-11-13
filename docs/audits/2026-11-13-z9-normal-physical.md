# Carried Z9 on the normal Z10 cartridge

The earlier winning Z9 HOST tape no longer matches the normal Z10 cartridge
from `6e843d814da131950c92da79901985fc2e05ff78`. At call 2,118, game frame
2,000, the cartridge had 120 enemy units and the tape expected 123. The
other named fields matched. A fresh ASan/UBSan HOST run using the matching
Z10 flag vector also had 120 enemy units there. The fresh tape's first
seven checkpoints then matched the cartridge through frame 10,090, including
the ordinary pause save and return to play.

The old controller stalled before it started the source countdown. It kept
sending the air sweep toward a Cannon on the flank and never advanced past
the same leg. Limiting the sweep to threats near the road alone still stalled:
the controller failed to select a Mutalisk group for repeated attacks on a
Zealot. The updated test controller also retries ordinary selection outside
the initial search near the air group's average position. It buys and orders
units through the normal controls. Mission scripts and the engine are unchanged.

With those two changes the matching HOST configuration wins at frame
72,628, after 74,386 calls, with no fault. Two Z10-flag runs and one
standalone Z9-flag run produced byte-identical tapes. A local test hook
hashed the covered route state after each game update and before the next.
The controller did not alter that route state. Its Drone reaches the
formation, waits for the source countdown to finish, and brings the new
crystal home.

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0' build/test-sc64-z10-carried-z9-run
./build/test-sc64-z10-carried-z9-run .local/sc64/z9-current.jsonl
```

The unchanged normal cartridge won from fresh HOME in mGBA at the same
frame. All 32 checkpoints matched across 15 named fields, including the
countdown and three active AI towns. The controller's private waypoint index
is not a guest field. The tape contains 10,044 held-key transitions and an
ordinary pause save. Six A presses and releases dismissed authored pages
without advancing the simulation, for 74,398 physical updates in total.
The replay runner made no guest-memory writes and did not load SRAM.

The live scheduler reported 14 trigger rows and no faults. The observed
countdown passed 582, 417, 228 and 96 seconds before reaching zero. At
frame 59,894 a live human Drone acquired the new crystal at (896, 1456).
The crystal followed that same Drone to (269, 240) inside source location 1.
The human source outcome became victory, and the primary objective and
campaign completion bits were set. This checks the port's authenticated
trigger program and its crystal carry, with no claim of native timing
or AI parity.

The full replay averaged 1,762,501 guest cycles per main loop (9.52 emulated
updates per second), including rendering, UI and presentation waits. It
does not meet the near-30-fps target or measure physical GBA frame rate.
This covers one complete route from HOME; it does not test Z10 SRAM reload.

- ROM SHA-256: `6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
- ELF SHA-256: `9bbe176b7d2916f945aa75691feccc2d1743e8b9e18b6e40807c99e05883c307`.
- Earlier tape divergence receipt SHA-256: `1024db764073c02eaed9be4747d8161d9c2333a777906018c1d3ef93c21a18f2`.
- Fresh baseline prefix receipt SHA-256: `2dd14cd77770ea7a05f27b7bfd522199f80bc2dd86e58e9bec1006ae9fa72221`.
- Winning HOST tape SHA-256: `26aecd7758ac368b9f8e34cb1764f47bf12c0bf6d7f5e8607aec9e096ae610bf`.
- Full physical win receipt SHA-256: `9309bbf77599bf779bb45443fe5f52d7766800731c21466b3d6d6a7362503281`.

## Exact November 13 KODAK cartridge

The same tape won from fresh HOME on the exact `cbba1c5` Z10 cartridge.
All 32 checkpoints matched after 74,386 tape calls and 74,398 physical
calls, with the six authored acknowledgments and no loaded save or guest
writes. The source countdown expired, and the same live Drone carried
its crystal from (896, 1456) to (269, 240), inside source location 1.
Victory at frame 72,628 set the objective and completion bits with zero
trigger faults. Mean cost was 1,748,268 cycles per loop, or 9.60 emulated
updates/s including the UI and presentation waits.

- KODAK ROM: `e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907`.
- Matching ELF: `74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1`.
- Physical observer: `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549`.
- Replay harness: `5c57f0a65c1af8f9f216cd664050125aaea602dc42a8cdd1b06e0dc3ad03bba6`.
- Full KODAK receipt: `556cf4dd17b91d6705c101b4b6920712b70ec21fd632fe274144baf6bd394e89`.
