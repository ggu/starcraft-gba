# Carried Z6 on the normal Z10 cartridge

The Z6 test controller now restores the engine's route state after
attack-move reachability checks, waypoint searches, and stalled-combat
reports. Queries that belong to one decision still share temporary route
state. Periodic read-only observations cover the source trigger model,
Kerrigan, transport research and cargo, and Force 2 actors in the two
objective areas. The controller still builds and fights through normal
inputs; the engine and mission data are unchanged.

On `a0d064c` with prepared inputs, two ASan/UBSan HOST runs won at frame
56,913 with 57,562 calls and 2,322 held-key transitions. In each run,
34 of 39 observer scopes changed route state and restored it. Debug
assertions verify the restore and check for mutation between game updates.
The full tapes are byte-identical (SHA-256
`32460d8296bb322d935c3a9f8de935885cdfbe52ff3eca09a901e623d6729769`).

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0 -DROUTE_PURITY_DEBUG' build/test-sc64-z10-carried-z6
./build/test-sc64-z10-carried-z6 --trace .local/sc64/z6-current-pure.json
```

The physical replay uses the existing normal Z10 cartridge from
`6e843d814da131950c92da79901985fc2e05ff78`. The engine, common code, config
and build flag fragments match `a0d064c`; the intervening changes are Z5
controller tests and their documentation. The ROM was not rebuilt.

From fresh HOME in mGBA, all 65 observed checkpoints matched, including
four with Kerrigan embarked as one unit of cargo on the researched
Overlord. At the final result, she was alive and unloaded at (951, 1010),
the source victory and campaign completion bit were set, and the adapter
reported zero faults. Force 2 counts in locations 4 and 5 were 0 and 4,
matching the HOST run. The test's chosen carrier slot identifies the
actor to observe; cargo and research are independently read from the game.

Eight authored text pages took physical A presses and releases. The
additional 16 updates left the simulation frame unchanged, so the replay
used 57,578 physical updates for the 57,562-call HOST tape. The emulator
received ordinary key input and no guest-memory writes.

- ROM SHA-256: `6224e1821edacff7a88908d2a9b5bc64b496094a567213153a1796c3d36715d3`.
- ELF SHA-256: `9bbe176b7d2916f945aa75691feccc2d1743e8b9e18b6e40807c99e05883c307`.
- Full replay receipt SHA-256: `97b1da4fd2e047d4035121c96dc34a396b82193a41fc59ccb60fcb631d4c3018`.

This covers one normal-ROM route. Save and resume and GBA hardware
performance are outside this replay.
