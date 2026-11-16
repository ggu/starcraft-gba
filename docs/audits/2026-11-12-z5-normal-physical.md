# Carried Z5 on the normal Z10 cartridge

The Z5 test controller now restores the live path grid, route cache and
scratch state after route checks. The door relabel step keeps its own
connectivity labels, but puts the engine grid back before issuing input.
Nested checks in the same decision still share their temporary cache. This
changes only the controller and its build dependencies; all movement,
attack, cloak, door and pause actions still go through ordinary input.

On `a55ddef` with prepared inputs, two ASan/UBSan HOST runs won at frame
48,347 with 23 orders and 650 input transitions over 48,803 calls. In each
run 272 of 309 observer scopes changed route state and restored it. The
debug checks verify both the restoration at scope exit and that no route
state changes between game updates. The full tapes were byte-identical
(SHA-256 `3bd6863852c2b273e356a23197687313473ce60025bb8a922f058d475fc54d63`).

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0 -DROUTE_PURITY_DEBUG' build/test-sc64-z10-carried-z5
./build/test-sc64-z10-carried-z5 --limit 60000 --verified .local/sc64/z5-current-pure.json
```

The ARM witness uses the already verified normal Z10 cartridge from
`cf78ea0481a70ec617edfba5ea2001e4f537c8e5`. The engine, common code, config
and build fragments match `a55ddef`; intervening changes were documentation
and Z3/Z4 controller test recipes. The ROM was not rebuilt.

From fresh HOME in mGBA, all 55 physical replay checkpoints matched the
HOST tape across 20 named fields, including the 14-field Kerrigan record.
The final result at frame 48,347 had Kerrigan alive at full health,
Switch 110 set, the campaign completion bit set, and zero adapter faults.

Twenty authored text pages took ordinary A presses and releases. Those 40
additional physical updates did not advance the simulation or change the
source pause flags, bringing the total to 48,843 physical updates. The
original tape's A press at call 47,351 separately acknowledged the source
`PauseGame` at frame 46,895; it changed the source pause flags from
`(1, 0)` to `(1, 1)` while the frame stayed frozen. The replay used physical
buttons and read-only observations throughout.

- ROM SHA-256: `042cb21810f6adc725aab9c0a90d450d4e59e3beb4e1c38237c1fc05bfb61062`.
- ELF SHA-256: `ef4543577af8551efcee7c1d9d15547cdb37b019617e1a7244f5eca02cdadf00`.
- Full replay receipt SHA-256: `7d285e087c3557eaad81b4b65e6ff1bc581e5491cf1cea32b15371e466e5d38e`.

This verifies one normal-ROM winning route. It does not qualify Z5 save
and resume or GBA hardware performance.
