# N64 custom-game opening observation

On 2026-11-04, an isolated ares session ran the user-supplied USA ROM from an
empty private save directory. Ordinary controller input selected Single-Player,
Scenario, Setup Custom, Melee, Challenger (96×128), Human Protoss and one
Computer Zerg. Start launched the match. The session used copied private ares
settings and an interposed input shim with a separate input file; it did not use
global keyboard input, modify an unlock state, forge a save state, or write
emulated memory. A separate emulator process owned its saves and screenshots.

The first retained gameplay capture, about five wall-clock seconds after Start
with the emulator unthrottled, showed a Nexus, four visible Probes and **4/9**
supply. No Pylon was visible in that viewport; this does not establish its
absence elsewhere. With no subsequent gameplay commands, another capture
showed workers in changed positions near the mineral line and minerals rising
from 74 to 274. This supports automatic gathering in this setup. Those readings
are after play began: the initial wallet and exact production costs and timing
were not measured. Wall-clock seconds are not a native game-clock measurement.

At source revision `6417737c`, `race_skirmish_init` in
`starcraft/race_adapter.h` starts the human with 200 minerals and zero gas,
spawns a Nexus and Pylon for Protoss, and spawns four Probes with mining orders.
The Nexus and Pylon provide 18 and 16 doubled supply units respectively
(`starcraft/race_rules.h`); the HUD divides by two (`starcraft/game.c`). Thus
that GBA opening provides **4/17**, unlike the observed N64 **4/9** for this
specific custom-game configuration. This is an observed supply-cap difference,
not proof of the N64 building inventory or a general rule for every map and
setting. No initialization change was made: Terran, Blood Bath, the exact
starting wallet, AI opening and associated fixtures need separate qualification.

The normalized ROM SHA-256 was
`e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`;
the ares executable SHA-256 was
`4ce08c105b4f930d9a86bf03c8ea51c5867fa3e29c942997a868b2b158a4d51d`;
the input shim SHA-256 was
`ba9227ef9a14deec020b62affc7d92eb015d51d44d90eb8edacf1cb784f7ff70`.
Private setup and captures remain under `.local/n64-ref/skirmish-nhQTNr/`;
they are not included in the source tree. The emulator process was terminated
after the observation. This single run does not establish native AI, timing, or
full skirmish parity.
