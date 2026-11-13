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
so the cartridge observer handles those authored pages separately.

| Local receipt | SHA-256 |
| --- | --- |
| Full HOST held-key tape | `43a5e267c1992bcc63f21d62691a3fb43e899cc578c032895b924c9c38bd046e` |
| Full HOST telemetry | `2bcf79de3dbeb69d86bd3cbafe94cfe7baa3e39e6d27b389905536a00ee9153d` |

## Complete HOME win on staged P10

The same held-key tape wins from a fresh HOME boot on the normal staged
P10 ROM. The physical mGBA observer made no guest writes and loaded no
save. It matched 41 observations and every recorded key-edge frame,
including the opening where the old tape lost Fenix. Eight briefing
pages used the original tape; twelve in-game pages took 24 additional
physical button calls while the simulation frame was frozen. Their
row, action and page data were checked against the ROM table.

Victory comes at frame 122,809 after 124,998 tape calls and 125,022
physical calls. Fenix has 240 HP and 240 shields, the Hyperion has
850 HP, the Stasis Cell is gone, and all three scheduler fault counts
are zero. This is one route on staged P10. It does not qualify the
older KODAK ROM, a save continuation, or hardware frame rate.

| Input or receipt | SHA-256 |
| --- | --- |
| Staged P10 ROM | `0505b233994dca90cda47d92f1a153a14f01e781d980b3498390d3b4005e7c41` |
| Matching ELF | `6319d1647b73ffa7ac9c0f165c1db1a8dbdf3a5792dc066c97024203f6672d44` |
| Physical mGBA observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Full HOME receipt | `8dd9310a569246f50e3f155a4cf8e881a779f1fb02cfc3778c53f9e099ea6cd8` |
