# Carried Z4 on the normal Z10 cartridge

The Z4 controller's live mineral selection now restores route state after
ranking the patches, before giving the selected Drone an order. That
keeps observer queries from changing how the game moves units. The failure
diagnostics run after the recorded game and are unchanged. The game engine,
objectives, source map, and default ROM flags are unchanged.

With prepared inputs at `4fb3fd2`, two ASan/UBSan HOST runs won at frame
34,984, call 35,186. All ten live observer scopes changed route state and
restored it, verified by the route scope hashes. The full input tapes are
byte-identical (SHA-256
`86a2e728e365361d86e26ef194c41f5d2768647e8d168d78c1911c8f6a563abc`).

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0 -DROUTE_PURITY_DEBUG' build/test-sc64-z10-carried-z4
./build/test-sc64-z10-carried-z4 --limit 150000 --trace /tmp/z4-current-pure.json
```

The same tape won from fresh HOME in mGBA on the already verified normal
Z10 ROM from `cf78ea0`. The engine, data definitions and build flag
fragments match `4fb3fd2`; the intervening source changes are documentation
and Z3 test dependencies/controller work. The ROM was not rebuilt.

All 42 checkpoints matched across 38 named fields, including countdown,
hatch effects, economy, Mutalisk count and position, source owner counts,
and the result. There was no mismatching checkpoint. At frame 34,984,
Kerrigan was alive, the Force 3 Command Center had been destroyed, and
the unsupported actor count was zero. Fourteen authored messages needed
ordinary A presses and releases. Those 28 physical updates left the
simulation frame unchanged, so the full run used 35,214 physical updates
for the 35,186-call HOST tape. There were no guest-memory writes.

- ROM SHA-256: `042cb21810f6adc725aab9c0a90d450d4e59e3beb4e1c38237c1fc05bfb61062`.
- ELF SHA-256: `ef4543577af8551efcee7c1d9d15547cdb37b019617e1a7244f5eca02cdadf00`.
- Full replay receipt SHA-256: `0b12ff2971438a0bb2035557b237b2223fef791d79ecb09daaf8363a7675bcf7`.

This is one normal-ROM route, not an all-actor-state comparison, a
save/resume result, or hardware performance evidence. The [earlier
campaign replay inventory](2026-11-12-normal-campaign-replay-gaps.md)
remains pinned to its own source and cartridge hashes.
