# Carried Protoss campaign saves on the November 20 release

These replays use the normal Protoss 10 cartridge at runtime `e3cb1c9`.
They start at HOME in an empty mGBA process, use ordinary buttons to save
from the pause menu, and load the exported 32 KiB journal with L from a
separate HOME process. The observer reads guest state and never writes it.

| Shared input | SHA-256 |
| --- | --- |
| Protoss 10 ROM | `ed455cec25f7f08228f2b03b780b4f7140a6718f6e8692e09b482c70b6f4b768` |
| Matching ELF | `1b23b937b80b0dd494768d6714322b929d34e1c5f55b263e240236cec05d4455` |
| Physical input observer | `f25b89241f1209d947dd275a2b953dfc0b7ee164e4f73fa63710855c0e63418d` |

## P5 Choosing Sides

At tape call 2,767 the controller pauses at frame 2,700, during the Shuttle
extraction route. B writes the first sequence in the normal journal. Title L
restores the same frame, persisted unit and building observations, scenario
state, selection and remembered command. The original journal SHA-256 stays
unchanged after the remaining tape.

All 5,172 tape calls match seven scenario checkpoints, 12 briefing pages and
five gameplay dialogue pages. At frame 5,057 the mission records victory and
its completion bit. Tassadar is alive, owned by the player, and present at
the extraction point. The scenario reports no fault or unsupported action.

The replay averages 20.74 controller updates per emulated second. This
includes the HOME and dialogue input segments and excludes the time between
the two emulator processes. Physical GBA speed remains unmeasured.

| P5 input or receipt | SHA-256 |
| --- | --- |
| Controller tape | `b0806cae59b0e595e5f9de874d8f6c5a556587fd1a12ae532f6d228e01e2f10d` |
| Authored page log | `60767da76b9f10d75451bc104093f5454c785d25c9c77bfe111dc09c3cd4c318` |
| Physical SRAM | `011a995ae4271dc2abdb21407221eacf96df17ca18e68fa280629ffce71c2c9a` |
| Fresh-process victory receipt | `4e282fdbcbb6edf520926015e708d1c66de14fcfc4ad031af988ff604d8ebff0` |

## P8 The Trial of Tassadar

The HOST controller compiled with the normal P10 flags reproduces the
historical winning tape byte for byte. In mGBA the same tape passes the old
Fenix loss point and uses START and B for a journal at frame 30,825. The
export matches the November 16 physical journal. A fresh HOME process
loads it through L and reproduces the saved world and persistent trigger
state. The journal stays byte-identical through the replay.

All 41 named checkpoints and recorded key-edge frames match. The eight
briefing pages use the original tape. Twelve gameplay pages take 24
additional physical inputs while the simulation frame is frozen. At
frame 122,809 Fenix has 240 HP and 240 shields and the Hyperion has 850 HP.
The Stasis Cell is destroyed, all three schedulers report zero faults, and
the objective flag and completion bit are set. The run uses 124,998
recorded calls and 125,022 physical calls.

In a prepared tree, `make build/px-p8-normal-home` builds the controller.
Run it with a new `trace.jsonl` destination and capture its standard error
for the briefing log. Use both as digest-pinned inputs to
`tools/sc64_p8_normal_home_replay.py --fresh-save`. The observer used here
emits DMA logs on standard error. The local replay sent the guest and
fresh-process logs to `/dev/null` to keep that chatter from tripping the
existing file-size cap. The input checks, observations and SRAM checks
were unchanged.

| P8 input or receipt | SHA-256 |
| --- | --- |
| Regenerated controller tape | `43a5e267c1992bcc63f21d62691a3fb43e899cc578c032895b924c9c38bd046e` |
| Briefing and controller log | `0d929179f10faf52c28a9465c508022cbc765b57bd0e828ddda8c72329631206` |
| Physical SRAM | `eb85fbbf33fc2d14b4e75c8b8050e6f283a553c61341b306997093b82d62f1db` |
| Fresh-process victory receipt | `06d229f16ee159c39f2553598bd8bd065f9642a823ab6cdee689fbe4aa4c6e3b` |

Both results cover the indicated save cuts and controller routes on this
release. Other save points, the rest of the current-release campaign
matrix, and hardware timing still need separate checks.
