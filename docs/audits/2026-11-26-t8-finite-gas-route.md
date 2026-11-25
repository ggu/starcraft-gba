# Terran 8 controller budget with finite gas

The finite-gas campaign changes the economics of the previous Terran 8
controller route. On source release `cbb88e61`, the adaptive HOST author
reaches victory at update 290,459 with the normal controller policy. The
previous 200,000-update author limit stopped the same policy with three
hostile buildings remaining. The completed run had zero hostile buildings,
the Norad hero alive, objective flag 1, and no mission faults.

A physical replay of the older tape from the carried T7 victory first differed
at update 120,864 in minerals (1,896 expected, 2,046 observed) and gas
(33 expected, 97 observed). Continuing that diagnostic with those two
resource differences recorded reached another mismatch at update 125,000:
14 hostile buildings expected, 13 observed. It is not a matching route;
no wallet or objective differences are waived in the new physical replay.

The author observes the HOST game to choose ordinary gamepad input; it does
not establish physical cartridge behavior. The retained trace SHA-256 was
`96e9ef4157fd53aadaaa32716d96bba328d6a3cc2555b1eb802ab37c72dd5cd4`.
Physical replay, fresh result reload and carried campaign victory are pending.

With the normal private Terran asset inputs prepared, regenerate the route:

```sh
make build/test-sc64-t8-campaign-run
build/test-sc64-t8-campaign-run \
  --trace .local/sc64/terran-late/full-t8-controller.json
```

The default author budget is 500,000 updates; an explicit `--limit` may
select a smaller budget. `test-sc64-terran-full-campaign` uses the same
budget. The normal physical
replay validator accepts both the previous 143,260-update route and the new
290,459-update route and retains their distinct checkpoint and end-dialogue
requirements. The budget changes the author timeout only; no cartridge or
campaign rules change.
