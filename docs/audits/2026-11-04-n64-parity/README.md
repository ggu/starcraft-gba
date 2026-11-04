# N64 parity audit

We aren't at parity with the N64 campaign yet. The N64 stops the mission for text boxes with
a portrait and speaker name, and the port shows none of those. Its advisor does much more
than ours, and it has screens we don't. Briefings, unit responses and the pause flow are close.

Voice for those text boxes is not a gap. The N64 doesn't have any: the boxes are silent text.

This audit changes no game code. It contains no ROM-derived text, art, frames, saves or traces.

## How to read the list

Every entry names its evidence:

- **N64**: observed on the N64 ROM running in ares.
- **ROM**: read from the ROM's own data with a private script.
- **GBA**: observed on a prepared GBA ROM.
- **Code**: read from this repository's code or docs.
- **Not checked**: not looked at, so treat it as unknown.

ROM costs are estimates unless they say "measured". For scale, the three prepared Transport1
ROMs measured here have between 3.3 and 5.7 MB free in their 32 MiB image.

## What to build next

Listed by how much each one closes the gap for its cost.

| # | Item | N64 | Port today | ROM | CPU | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **In-mission text boxes** | Modal box with portrait, speaker and text; A advances. The mission stops while it is up. Used by 215 Transmission and 114 DisplayText rows in the 30 missions. | Missing. Z8, P8 and P10 run a silent 4-second timed pause. T1 and P2 show a short authored status line, and play continues. | About 19 KB of text plus an index, about 30 KB in all. 26 speaker IDs against 23 briefing portraits; how they map is **not checked**, so extra art may be needed. | Almost none: the simulation is stopped and the box draws once. | N64, ROM, GBA, Code |
| 2 | **Advisor alerts** | The ROM holds alerts for attacks, finished research, upgrades and add-ons, low energy, blocked placement, nuclear launches and landing. | Missing. Only the nine refusal lines are imported. | About 0.3 MB for about 25 clips at 6.8 KB per second. | Low: an event check plus the decoder voice we already budget. | ROM, Code. When they fire on the N64: **not checked**. |
| 3 | **Z8 wave pacing** | Native waves come later, further apart and smaller than ours. See [Z8 waves](#z8-waves). | Hand-written waves are much harsher, which blocks the Z8 controller. | None. | None. | N64, ROM, Code |
| 4 | **Annoyed responses** | Tapping one unit repeatedly plays them. | Not imported. | Measured with our encoder: 106 clips, 181 s, 1.23 MB. | Negligible. | N64, ROM, Code |
| 5 | **Mission-end score screen** | Tabbed table per player: units, structures, resources and score, with elapsed time. | Result panel with no score table. | 30 to 60 KB using existing chrome. Counters add under 100 bytes of RAM and need save bytes. | Negligible. | N64, Code |
| 6 | **Mission title card and campaign opening text** | Each mission opens with a title and location over art. Z1, P1 and the first Brood War row add a screen of story text first. A boot intro also plays. | Missing. The ROM boots to the home menu and the briefing opens directly. | A few KB with text over existing art; up to about 1 MB with imported art. | None in play. | N64, GBA |
| 7 | **Game settings** | Sliders for game speed, cursor speed, music and effects volume and more; toggles for unit speech, selection, building sounds and transmissions. | No settings screen. | Almost none. Needs save bytes. | None. | N64, Code |
| 8 | **Camera bookmarks** | The D-pad selects map locations in play. | Missing. | None. | None. | N64, Code. What they store: **not checked**. |
| 9 | **Two tutorials** | Boot Camp and Officer Training head the Terran list, with 27 Transmission rows. | Missing. | About 4 KB of text, plus two small maps. Needs item 1 first. | Same as a small mission. | N64 (menu only), ROM, Code |
| 10 | **Native AI scripts** | Missions run script programs. | Hand-written AI backends. | The script data is under 60 KB. | Not costed. Attack-group behavior is still unknown, so this is the largest engine job here. | ROM, Code |
| 11 | **Engine gaps** | Defiler spells; the Gantrithor. | Defiler spells are missing and the Gantrithor has no attack. | Not costed. | Not costed. | Code. N64 side: **not checked**. |
| 12 | **Cheats page** | Pause-menu page with locked rows. A P10 trigger's text mentions a cheat code. | Missing. | Almost none. | None. | N64, ROM, Code |
| 13 | **Encyclopedia** | Per-unit page with cost, stats, a sprite and a description. | Missing. | Tens of KB of text, reusing unit art. | None in play. | N64, Code |
| 14 | **Scenario presets** | Setup Custom plus nine preset matches. | Custom Game with two maps. | Depends on maps. | Same as skirmish. | N64, Code |
| 15 | **Two-Player** | Melee setup with map and race choices. | Missing. It would need link play. | Not costed. | Not costed. | N64 (menu only), Code |
| 16 | **Brood War episodes** | Episodes IV to VI: 28 scenario entries in the ROM, with still-image cutscenes and captions. | Missing. | Not costed; well beyond the free space in today's ROMs. | Not costed. | N64 (first row only), ROM, Code |

## What already matches or is close

| Area | N64 | Port | Status | Evidence |
| --- | --- | --- | --- | --- |
| Briefing dialogue, speakers and portraits | Page counter, portrait, speaker, text | Same content, more pages at 240 by 160: Wasteland is 5 pages on the N64 and 9 here | Matches, reflowed | N64, GBA |
| Briefing objectives | Fixed panel beside the text | Appended as extra pages | Approximated | N64, GBA, Code |
| Skipping a briefing | Start skips | Start deploys | Matches | N64, GBA |
| Briefing audio | **Not checked** | **Not checked** | Unknown | None |
| Unit responses | Selection and command speech plays | Imported ready, selection and command lines | Matches, apart from item 4 | N64, Code |
| Refusal lines | The minerals refusal shows a text line and plays voice | Toast plus the imported line | Matches for minerals. Gas and supply: **not checked** on the N64. | N64, GBA, Code |
| Pause menu | Options, Goals, Abort Mission, Resume; Load/Save sits under Options | Resume, Save, Load, Restart, with the objective on the same panel | Approximated | N64, GBA |
| Goals | Mission objectives page | Objective box on the pause panel | Matches in purpose | N64, GBA |
| Saves | Two slots on Load Saved | Pause-menu save and Load Game | Approximated. Slot behavior: **not checked**. | N64, GBA |
| Resources and supply | Top right, with icons | Top strip | Matches, moved | N64, GBA |
| Minimap | Overlay at bottom left, with a fade setting | Bottom right panel | Matches, moved | N64, GBA |
| Selection | Unit count for groups; name, portrait, HP and upgrade icons for one unit; rings and bars | Panel with portrait, queue and upgrades | Approximated for the small screen | N64, Code |
| Commands | C-button prompts and a 3-by-3 build grid with costs | Command list | Different by design for the GBA pad | N64, GBA |
| Music | Not judged by ear; audio plays during text boxes | Roles read from the N64 selector | Track changes: **not checked** on the N64 | Code |

## Why voice isn't on the list

The ROM's trigger rows name a sound for each Transmission. Across the 30 missions, 203 of the
215 rows name a silent clip or none. The other 12 name beeps or a few short unit sounds.

Captured audio agrees. A local transcriber found no speech while Wasteland or The Hammer Falls
text boxes were up. As a control, the same pipeline did transcribe unit responses and the
minerals refusal. Z8 and P10 audio were not captured, so for those two this rests on the ROM rows.

Voice also wouldn't fit. The 19 KB of text is about 3,800 words, or about 25 minutes spoken. At
our codec's 6.8 KB per second that is about 10 MB, against 3.3 to 5.7 MB free, and the rows name
no such recording. Don't build it.

## Z8 waves

This bears on the blocked Z8 controller. The evidence supports softening our waves, but this
audit doesn't change them.

The native wait counter drops by one per native step: both AI instances moved 258 then 265
while the step counter moved 258 then 265, read from private save states. That means the script's
wait values can be read as native steps. At the 42 ms Fastest step, compared with our 30 Hz
wave timers:

| | Ours | Native, Fastest | Native is |
| --- | --- | --- | --- |
| `Zer8` first attack | 160 s | about 273 s | 1.7 times later |
| `Zer8` gap between attacks | 80 s | about 336 s on average | 4.2 times longer |
| `Ze8T` first attack | 160 s | about 22 minutes | 8.4 times later |
| `Ze8T` gap between attacks | 120 s | about 10 minutes on average | 5 times longer |
| Units per attack | 6 rising to 16 | about 5 on average for `Zer8`, about 1 for `Ze8T` | Much smaller |

The exact operands and group contents stay in the private report, as in the
[static decode](../../research/z8-native-ai-policy-programs.md).

There are limits on this:

- The native numbers are the script's attack commands. The live counters confirm that `Zer8`
  moved past its first attack, but no wave was watched arriving.
- Attack-group behavior is unknown. A group may wait for its units or shrink if they aren't ready.
- Our waves send whatever idle units exist, so the comparison isn't exact.
- Trigger-created raids are separate. They weren't compared.

A run with no input also lost on the N64 at 566 native seconds, about 6 minutes at Fastest,
when a Dark Templar escaped. That is the mission's own defeat rule, and an Advisor box announced
it. The elapsed-seconds counter and both AI counters stayed frozen while that box was up, which
also confirms that text boxes stop the mission clock and the AI.

## What was reached

No N64 mission was played to a win, so nothing here is evidence of N64 winnability.

| Where | Reached | Seen |
| --- | --- | --- |
| N64 Terran | T1 Wasteland, T10 The Hammer Falls | Briefing, text boxes, HUD, selection, build grid, pause pages, voice control |
| N64 Zerg | Z1, Z8, Z10 | Opening text, title cards, briefing, Z8 text boxes and idle timeline to defeat, score screen |
| N64 Protoss | P1, P10 | Opening text, title cards, briefing, P10 text box |
| N64 Brood War | First row past P10 | Still-image cutscene with captions, opening text |
| N64 menus | All | Scenario, Load Saved, Two-Player, Encyclopedia. Credits were not opened. |
| GBA Terran | T1 | Home, mission list, briefing, HUD, status line, pause panel |
| GBA Protoss | P2, in the P10 candidate | Mission list, briefing, HUD, status line, pause panel |

Later GBA missions are from code. The GBA ROMs were built on 3 November in a worktree whose
head is now `c81c2919` and weren't rebuilt for this audit, so the free-space and GBA screen
results are from that build rather than the commit this audit sits on.

Saved progress starts empty, so reaching later N64 missions needed edits to a private save
state: the published progress bytes, and the menu row byte that picks the mission. These edits
were only for observing missions and are not gameplay proof. Nothing was changed once a mission
had started.

## Method

ares reads its keyboard state through one system call. A private copy of ares, re-signed locally
with a small interposing library, reads key state from a shared file, so input can be scripted
in the background without using the real keyboard. The installed app and its settings were left
unchanged. The library also takes the audio instead of playing it, which keeps the Mac silent and
lets it be analyzed locally.

Frames came from the emulator's own screenshot command into the ignored folder. The DebugServer
and GDB were not used. All frames, audio, saves and scripts stay under ignored `.local/`.

The transcriber is a small local model, so its text was only used to detect and classify speech.
Whether a clip is an advisor alert comes from that classification, not from native code.
