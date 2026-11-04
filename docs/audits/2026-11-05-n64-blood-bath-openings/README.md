# N64 Blood Bath custom-game openings

On 2026-11-05, three separate ares sessions started with empty private save
directories and ordinary controller input. Each selected Single-Player →
Scenario → Setup Custom → Melee → Blood Bath, a 64×64 map, with one Computer
Zerg and both remaining slots Closed. The human race varied. Copied emulator
settings and an interposed controller-input file were private to each process;
no shared saves, global keyboard input, modified unlock state, forged state, or
emulated-memory writes were used. The emulator processes were terminated after
observation.

| Human | Visible supply | Opening viewport | Limit |
| --- | --- | --- | --- |
| Terran | 4/10 | Command Center and workers near minerals | Capture followed unthrottled play; 722 minerals is not an initial wallet. |
| Zerg | 4/9 | Hatchery and Overlord visible in the later viewport | A transition frame displayed 50 minerals and 4/9 while partially rendered; its exact game clock and delivery history are unknown. A later no-command capture displayed 370 minerals. |
| Protoss | 4/9 | Nexus and clustered workers; no Pylon visible in the viewport | The first gameplay transition under the shim's approximate audio throttle displayed digits resembling 40, but the HUD may have been updating and its initial value is unverified. |

The two Zerg captures, taken without gameplay commands between them, support
income occurring automatically in that setup. The observed 4 used supply does
not by itself prove four visible workers in every screenshot, and a viewport
cannot establish that no other building exists elsewhere on the map. Starting
wallets, production times, exact spawn coordinates, and full base inventories
remain unmeasured. These are three observations, not proof for other maps, game
types, opponent settings, or native AI parity.

At source revision `11fd1dd5`, `race_skirmish_init` in
`starcraft/race_adapter.h` gives each race 200 minerals, spawns its town hall
and four workers with mining orders, adds a Depot for Terran or a Pylon for
Protoss, and adds an Overlord for Zerg. The corresponding supply rules in
`starcraft/race_rules.h` and HUD division in `starcraft/game.c` yield GBA
opening supply 4/18 Terran, 4/9 Zerg, and 4/17 Protoss. The observed N64
Blood Bath caps therefore differ for Terran and Protoss in these setups; the
Zerg cap agrees. This does not prove the N64's complete starting building
inventory or justify an initializer change without additional qualification.

The normalized ROM SHA-256 was
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`,
the ares executable SHA-256 was
`4ce08c105b4f930d9a86bf03c8ea51c5867fa3e29c942997a868b2b158a4d51d`,
and the shim SHA-256 was
`ba9227ef9a14deec020b62affc7d92eb015d51d44d90eb8edacf1cb784f7ff70`.
Private captures and settings remain under ignored `.local/n64-ref/` directories
`openings-eQG7Q6`, `openings-rNg22U`, and `openings-qwe5JZ`, respectively.
The earlier Challenger observation is documented in the
[skirmish opening audit](../2026-11-04-n64-skirmish-opening/README.md).
