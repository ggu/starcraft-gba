# Skirmish pause and mining

## N64 reference

Two fresh ares v148 sessions reached the same Protoss custom-game opening
using the controller route previously used for Challenger Melee against
one Zerg computer. The opening frame showed 66 minerals and 4/9 supply.
Start opened the pause menu. The first two pause samples, 800 emulator
run steps apart, showed the same field arrangement behind the menu.

In the first run, pressing Start again opened the highlighted Options
row. In the second, three Down presses highlighted Resume and A closed
the pause menu. The field then showed 74 minerals, rising to 106 in the
next sample 900 emulator run steps later. Supply remained 4/9. The pause
menu covers the resource HUD, so these are readings before pause and
after resume. The emulator run-step labels locate the captures.

| Resume run sample | Run step | Visible state |
| --- | ---: | --- |
| Opening | 4,201 | 66 minerals, 4/9 supply |
| Pause | 4,251 | Options highlighted |
| Pause after idle | 5,051 | Options highlighted |
| Resume selected | 5,141 | Resume highlighted |
| Returned to field | 5,191 | 74 minerals, 4/9 supply |
| Continued mining | 6,091 | 106 minerals, 4/9 supply |

The isolated runs used ordinary controller input and fresh saves. Both
manifests passed. The resume run's controller log SHA-256 is
`e5415e785e4e052ac8dffdee4b471fb32a67de9f9611be31cdb57bf018bf68af`.
Raw captures and saves remain local.

## Frontier comparison

The Foundry, Rovers, and Bulwark host builds each entered a three-computer
match using ordinary buttons. At game tick 1,201, Start opened pause with
416 minerals. After 800 neutral updates, both the game tick and mineral
balance were unchanged. Start resumed the match. After 200 gameplay
updates, the tick reached 1,401 and the mineral balance reached 456.

The faction suite also checks that 800 neutral updates in pause leave the
game tick and mineral bank unchanged before saving. All three sanitizer
builds passed.
