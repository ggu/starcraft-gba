# Release skirmish Support Bay gate

Normal custom-game menus previously offered the Robotics Support Bay even
though its Reaver product is deliberately rejected until the Scarab lifecycle
is complete. The skirmish menu and shared admission now exclude that dead-end
Protoss build option. Its catalog record remains available for existing data;
campaign paths and working race chains are unchanged. This is a conservative
gate, not an implementation of Reaver or Scarab services.

At source `a1e1cfdb0e99d316878cf94fbc08bdcc12b8ff84`, the independent menu
fixture and actual Terran, Zerg, Protoss and Showcase runtime fixtures passed
under ASAN/UBSAN, including no-payment Reaver rejection and Support Bay
admission checks. Canonical Terran, Zerg and Protoss same-process skirmish
save/reload continuations passed. These are host fixtures, not fresh-process
SRAM or a complete player campaign proof.

Four normal ARM builds passed the unchanged EWRAM/IWRAM guards. Normal Terran,
Zerg Z6 and Protoss P7 ROM verifiers passed; Showcase produced its normal fixed
ROM and has no corresponding verifier target. ROM SHA-256 values were:

- Terran: `b247ab15b273b662997cfb632adf88359034715cba7fdd0155c2a082bf538530`
- Zerg Z6: `982bb516b5859b70589dc7e9a80f44e005256262e6d1ed622378afd621f5648c`
- Protoss P7: `edc8baca2a24aaa94343daaeab2cc2f808752abaa73573819b8d05393106bddb`
- Showcase: `423bfb9e19804e2b7af86a907c5b8ba48a750006aaa13f28e9fd996717cc0409`

A separate clean unprepared checkout passed source boundary, doctor, default
build and full default tests: 855 Python tests with 23 skips, plus five- and
8-test suites and native fixtures. All 109 pinned inputs and auxiliary hashes
and tracked sources remained unchanged. Local receipts and logs are under
`/tmp/sc64-release167-gate-focus-20261101/` and
`/tmp/sc64-release167-gate-clean-output-20261101/`; no assets or ROMs are
committed. This does not establish all 30 missions, full roster fidelity,
physical-device performance or a 30 fps result.
