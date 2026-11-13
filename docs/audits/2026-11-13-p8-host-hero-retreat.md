# P8 ordinary controller protects Fenix on HOST

The [retained route](2026-11-13-p8-staged-route-loss.md) lost Fenix at
frame 10,905 on staged P10. The opt-in HOST controller now reads
ordinary briefings, records Fenix and Hyperion health, location and
orders, and allows both heroes to retreat from a nearby enemy on the
nearest-depot profile. It selects actors and sends move orders through
ordinary buttons. The route fingerprint is checked across each
controller interval.

The first opening witness passed frame 13,063 with both heroes at full
health. A fresh ASan/UBSan run using the staged P10 gameplay flags won
at frame 122,809, call 124,998. Fenix had 240 HP and 240 shields,
Hyperion had 850 HP, the Stasis Cell was gone, and all three scheduler
fault counts were zero. The trace includes eight ordinary briefing
pages. This HOST build did not enable the test-only in-game modal flag,
so a cartridge replay must account for those authored pages. There is
no ARM win or save continuation claim here.

| Local receipt | SHA-256 |
| --- | --- |
| Full HOST held-key tape | `43a5e267c1992bcc63f21d62691a3fb43e899cc578c032895b924c9c38bd046e` |
| Full HOST telemetry | `2bcf79de3dbeb69d86bd3cbafe94cfe7baa3e39e6d27b389905536a00ee9153d` |
