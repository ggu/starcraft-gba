# Skirmish opening supply adjustment

New custom matches no longer spawn an extra completed Depot or Pylon for any
player. Terran and Protoss retain their town hall and four mining workers;
Zerg retains its Hatchery, Overlord and four workers. Starting minerals remain
200. The rule only runs for new matches; loaded saves use their stored world.

This follows observed 4/10 Terran and 4/9 Protoss supply caps in N64 Blood Bath
Melee with one Zerg AI. The captures did not establish the full native building
inventory or AI opening. This is a bounded supply-cap approximation, not a
claim of full native startup or AI equivalence.

## Qualification

At merged source `7fee3810c5d8f32babd25ef7b2ad18ea2e4a92c7`, the normal
Terran, Zerg, Protoss, Showcase and unified builds passed their existing memory
guards. The four imported-skirmish products each passed 234 normal-menu opening
configurations across both maps, all human races, and one to three independently
selected AI races. Ordinary build-controller checks passed for the three race
products; their Terran and Protoss controllers now construct their supply
prerequisite through normal input before their combat producer. P10 and Z7
startup/save matrices each passed 234 configurations, and the unified suite
passed. An unmodified genuine pre-change P10 SRAM loaded, preserved its legacy
journal slot when re-saved, and matched a fresh-process 60-update continuation.
The clean default build and test passed on the pre-merge candidate `7d812209`;
the later merge changed shared engine code, so that result is scoped to that
source, not represented as a new clean run on the merged head.

The full host AI development matrix passed 272/272 on the merged source. The
late-small-army fixture previously deleted all opposing nonworkers near the
human, including units inside the AI home radius, and could not retain its
required three Marines at its fixed deadline. It now removes only units outside
the AI home deployed radius and waits in a bounded late window for three actual
recruits. The host fixture holds that cohort in place using idle orders before
release, with no new units, resources, or teleportation; all three must receive
fresh attack-move orders. This is a fixture arrangement, not evidence that the
same cohort naturally waits at home. The corrected late case passed on both the
pre-change baseline and candidate; all 16 commitment cases passed on the four
candidate profiles.

The merged build's BSS/IWRAM endpoints were Terran `02036f10/03004778`,
Zerg `0203d468/03004f88`, Protoss `020391d4/03004a58`, and Showcase
`02036d60/03004cc0`. The 1,796 prepared-input hashes were unchanged during
qualification. These tests do not establish native starting inventories,
starting wallets, AI equivalence, or physical-hardware performance.
