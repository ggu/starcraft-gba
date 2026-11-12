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

- ROM SHA-256: `6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
- ELF SHA-256: `9bbe176b7d2916f945aa75691feccc2d1743e8b9e18b6e40807c99e05883c307`.
- Earlier tape divergence receipt SHA-256: `1024db764073c02eaed9be4747d8161d9c2333a777906018c1d3ef93c21a18f2`.
- Fresh baseline prefix receipt SHA-256: `2dd14cd77770ea7a05f27b7bfd522199f80bc2dd86e58e9bec1006ae9fa72221`.
- Winning HOST tape SHA-256: `26aecd7758ac368b9f8e34cb1764f47bf12c0bf6d7f5e8607aec9e096ae610bf`.
