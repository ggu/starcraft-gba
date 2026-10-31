# Scarab experimental port onto AI economy shared

The private merge at `91e1857` retains public Scarab `ca0cc4c`, private probe
`c5cb1cb`, and shared AI economy `7a4cc09`. All Scarab activation and probe
gates remain off by default. The older native-order audit and retained oracle
vectors describe their original source and test setups; they are not new
oracle results for this port.

A current-source, sanitized Terran host compile and actual `game_update`
attack fixture passed nine cases: basic, upgrade, full pool, building,
building destruction, changed ownership, slot reuse, shields and fractional
damage. Two autonomous flight cases passed, including loss of a moving target;
the packed-only dispatch fixture passed its fresh STOP, parent-loss and
retained-target checks. An initial dispatch run mistakenly combined that
fixture with autonomous waypoint control and failed its injected-waypoint
assertion; the corrected packed-only run passed. This does not establish full
native flight, terrain pathing, projectile timing or save compatibility.

All four normal gate-off ARM links passed unchanged reserve guards. BSS/IWRAM
ends were Terran `020370c0/03004b78`, Zerg `02037134/03004c48`, Protoss
`020382ec/03004a18`, and Showcase `0203df40/03004e08`. Terran and Zerg
IWRAM are eight bytes above shared; the other endpoints match. One current
Terran link with the experimental weapon, flight and namespace gates enabled
also passed unchanged guards (`020370f0/03004e68`); this is not admission or
a fit guarantee for other profiles. All 104 prepared source inputs matched
the frozen hashes before and after these links.

A fresh default build passes and has the same text/data/BSS sizes and symbol
addresses as an independently built `7a4cc09` baseline, but its ROM hash
`73c0deeeb36cca4cb8614248a7e60cadac11a0ec542f2ece31c37b171e2c56b8`
differs from baseline `e612df7f0bd1417754e8665ca293bc53eedb194d55f93e2b996ea54ede1bd69c`.
The game-source preprocessing differences in this configuration are added
constant-false or constant-true Scarab checks; compiler output still differs.
Do not claim binary identity. The focused tests and compiled-off inspection do
not prove whole-game equivalence.

Private inputs, compiled executables and detailed command/run receipts remain
ignored under the isolated worktree. No assets are committed. Flight, splash,
slot/pool and save limitations in the preceding audit remain open; no Reaver
admission or public push follows from this checkpoint.

A prepared-worktree `./dev test` did not pass: its Python suite reported three
canonical renderer roster dry-run failures because the frozen input subset did
not include `palette-remap.bin` needed by make dependencies (842 tests, 17
skips). That result is not a clean default-profile test. An independent
unprepared check is tracked separately in the port receipts.
