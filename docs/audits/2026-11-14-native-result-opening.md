# N64 Blood Bath result cadence from a live opening

The [pinned result observer](https://github.com/ggu/starcraft-gba/pull/490)
replayed the owned N64 ROM's ordinary controller opening, then watched
the unchanged game through step 4,456. It reproduced both archived
frames (3,694 and 4,204) and both archived RAM views (4,201 and 4,215)
byte for byte. Its authenticated hybrid restart preserves the
cache-aware RAM view; it does not inject gameplay state.

The read-only 3,800-byte trace contains 95 events: 77 countdown reads,
three scans, 12 owner dispatches and three pass returns. The scans at
steps 4,223, 4,328 and 4,431 each dispatch owners 0 through 3 in that
order before returning. Each of the two complete intervals contains
31 decrements (30 through 0); neither has a reset. All eight pending
and published result slots remain zero. This verifies opening cadence
and owner order. It does not establish a defeat, damage after transfer,
or two eliminations within one native gameplay update.

A prior attempt stopped before gameplay on the 8 MiB process file
limit. The passing run kept both 8 MiB limits and used a macOS rule
that denies writes under `com.apple.metal` while allowing reads.
No sandbox exemption, cache deletion or limit increase was used.
The draft records the exact invocation and capture boundaries.
Next capture should reach a real last-building attack through the game
controls, then retain pre/post owner and result witnesses.

| Local receipt | SHA-256 |
| --- | --- |
| Opening manifest | `5f9491117b00fc57faefc8b90d5653ae1d84e152cf1aa55e41d6645a94d8ffb0` |
| Pinned opening tape | `da668b3e6826b61974364c671afd7f218e6288ef9e216bfabbdbfc83f761436b` |
| Result trace | `8eb9aed95f0b4a3b86216d0f4e89eaf75b161b15c2bcd322c3643c9425bb1635` |

## Ordinary-input Barracks construction

A separate bounded visual run continued the owned N64 Free for All
match beyond the opening using only controller input. R selected all
four workers in the opening formation. After the cursor selected a
single SCV, C-Left opened its Build grid at step 6,841. The grid
initially selected Supply Depot; a short left-stick pulse selected the
150-mineral Barracks. A opened its placement preview, with the valid
green footprint shown at step 6,951. A second A press placed it. The
worker approached the footprint at 7,159 and was at the scaffold at
7,659. The HUD went from 186 minerals before placement to 60 at the
scaffold, with other workers still mining.

This gives the elimination capture a reproducible ordinary-input route
through production setup, and supplies a Terran build-grid and placement
reference for the GBA controls. It is visual evidence only. The run did
not verify a completed Barracks, Marine production, or any result transition.
The capture kept both 8 MiB per-process limits, denied Metal cache writes,
isolated emulator saves, and reported unchanged inputs.

| Local receipt | SHA-256 |
| --- | --- |
| Construction manifest | `a71512f396ec39e0fa525b31301c463f0e4ead4669fe1ca9c990d68f6d143f3e` |
| Controller command log | `cbd83bae5a7eba2ad61fd11e43ba2f18aab6f1720a40037de8b9ce1cac6c0633` |
| Selected Barracks menu frame | `a6d6ddc0687c7e2e4cbd8b5961abc298d0f99d5fb9c9713ce068e33df57eded8` |
| Valid placement frame | `a5eaf390e5446af6ce6b8b8433532686e2b9c57cb23e44f3897544c5ef38ff9e` |
| Construction scaffold frame | `ded6d5550c8d40da5639a941a965e2bde8c651640abe777c257ade58b7a00f51` |

The next replay verified the completed Barracks at step 11,782. It
matched the preceding run's first five frames byte for byte before
advancing past step 9,332. The completed building can be selected and
exposes its production and lift-off controls. The longer local frontend
logs the first polled value and each change per controller input while
returning the held value on every native poll. Its wall-clock deadline
is bounded at 235 seconds with a 240-second supervisor; the 8 MiB file
limits, 131,072 input-log cap, and disk guard are unchanged. The run
finished in 157 seconds with unchanged inputs.

| Local receipt | SHA-256 |
| --- | --- |
| Completion manifest | `962a0e4aa4dcbaff5e8f91b6f0de12cba3e945a1daa8d30f79ac17c6ab35271a` |
| First frame with completed Barracks | `3807896eac0062e4751289f055e211462ab08d1cdf127b8f5ea06a9c20ed19b6` |
| Selected Barracks and its controls | `5457617031ec98b74953cde5ab5a689b1700dce64c3e9db4ad496e3b35d27ab9` |

For a selected Barracks, C-Right opens the production grid. It initially
selects the empty center tile. Up selects the Marine, whose grid entry
shows 50 minerals and one supply. A queues it: the next clean frame
shows the Marine portrait in the queue and supply rising from 4/10 to
5/10. This establishes the original production input and queue behavior.
Later frames have display corruption, so this run establishes neither
a completed Marine nor any live combat or elimination.

| Local receipt | SHA-256 |
| --- | --- |
| Training manifest | `33d749b91afd6a9f415e0bffb09149886028d211f4988f8e70147ee5ba8debca` |
| Marine selected (step 11,823) | `3b11c9e441e200490f2f88f80468ffcba59583a5fed1bfa1d930169c01cdc051` |
| Marine in training (step 11,843) | `4a30f2d705b16ba65fd10118220addc0c4ebe9dd9236bd383353eabb3dd87606` |

## Marine spawn and movement (November 15)

The next bounded visual run reproduces the ten earlier frames through
the training queue, then shows the queue empty and a separate actor
beside the Barracks. After closing the Barracks grid with B and moving
the cursor onto the actor, A selects it. The clean step 13,258 frame
identifies it as a Marine with 40 HP and shows the selection ring.

The player points southeast and presses C-Right. Two subsequent clean
frames show the selected Marine move from near the Barracks to the
cursor's destination. This is a verified ordinary-input spawn, selection
and move command. It does not establish combat, an attack order, or
elimination. The 19-frame manifest reports unchanged inputs and a clean
exit (191.314 seconds) under the same local input, file and disk guards.

| Local receipt | SHA-256 |
| --- | --- |
| Spawn and movement manifest | `941b58097f7889ed5cbf758218ad8575a0ff4259446f8d17baf28e0950fad8bf` |
| Controller command log | `4f19619b2e43978354c38eeaf23e35bd98118a362c6693fe14cdff2a85e725c9` |
| Selected Marine, 40 HP (step 13,258) | `e44a265d9ed306ff2831479c3700ce54413ba94213062bf9bd7aa1d93f619ba7` |
| Marine moving (step 13,427) | `3ee59ae537594ff096cdb8e763f30a44ce641bfe1827ddf42fd839adbc048ace` |
| Marine at destination (step 13,532) | `3f0217f62b8c0f38e50ae87b0ef36ef04984817f1144a1d0abe5802c1b949cab` |

## Hostile contact and Hatchery damage (November 16)

Start opens a pause menu with Options, Goals, Abort Mission and Resume.
The observed top-level menu offered no save action. The next separate
bounded discovery build instead replayed and matched the 19 archived
frames through the first Marine move, then advanced beyond the former
14,000-step allowance. It allows at most 28,000 steps and 32,768 input
records; it keeps the 235/240-second frontend/supervisor deadlines, 8 MiB
file limits and disk floor, with isolated saves and unchanged input pins.

The selected Marine moves south, revealing a red Zerg worker by step
14,893 and a live enemy Hatchery at 15,793. A subsequent replay matches
the anchor again and puts the cursor on the Hatchery. The native
contextual prompt at 15,846 is `B KILL UNIT`. A separate, fully scripted
replay matches that exact aiming frame, presses B and then uses A to
read the target's details. Clean frames show Marine fire and the
Hatchery's HP falling. They establish hostile contact, a player-issued
attack and actual damage given ordinary controller input. No building
destruction or competitive result has been observed in this sequence.

| Local receipt | SHA-256 |
| --- | --- |
| Contact and scouting manifest | `1cdd899ab1947b7608e565448e50bbc223661f344a023c77c4dd9dfa69b6ae99` |
| Marine at enemy base (step 15,793) | `b78c27900df257af66d10710dd07bd7d2bd7c1b0650c597a97e2d0e1d8a36fee` |
| Attack manifest | `52e34423ed584aa01b16c2ea85c6780a5793a7bbfcf272afab073d5f9e54a496` |
| Hatchery 1,222 HP (step 16,277) | `abcccd15227af5a53be929c3e4d0163b517fa0096461096c9e40ddda117d3b5d` |
| Hatchery 1,179 HP (step 16,697) | `2f94e0c4515e525dce109e0d4cb0f72e3df7d17c82a4707d3bd1cb65bb6320e9` |
| Hatchery 1,141 HP (step 17,117) | `23df433e226255480e33b150dfc4ffc6a8109308da9419544f7fb5039e2b702a` |

An endurance replay matches the 24 retained pre-attack frames and all
three HP anchors. It sends no further commands after targeting and the
original attack. At 27,899 the same Hatchery has 171 HP and the Marine
is still visible. The callback input log is byte-identical to the
shorter attack run; the additional commands are neutral waits only.
The run finishes in 159.496 seconds, with no result observed.

| Local receipt | SHA-256 |
| --- | --- |
| Endurance manifest | `d5d7f593e5afb8fb7bb3e5bd14552c26075aea2e657e17d05433b51bfd3030ab` |
| Hatchery 171 HP (step 27,899) | `6118e3bf1eabc6437e15a0f26d838114feae2f56d28c2cf4509550143113dd8c` |

The last replay uses a separate 31,000-step local visual limit. It
matches the 24 pre-attack anchors, the three earlier damage anchors
and the exact 171-HP frame. The callback input log still matches the
original attack. After neutral wait, the Hatchery is shown at 104 and
then 43 HP. By 29,999 it is destroyed: the selection and HP clear and
orange remains occupy the footprint. By 30,699 the footprint is clear.
Another enemy structure and mobile units remain visible. This is a
building-destruction witness, with no visible owner elimination or
match result. The clean 32-frame run takes 149.565 seconds; the same
wall-time, input-record, output and disk limits apply.

| Local receipt | SHA-256 |
| --- | --- |
| Destruction manifest | `50da1f7c17e2821245820b28016df1905d438302f35b6f1ecac7cf56ccdfdd64` |
| Hatchery at 43 HP (step 29,299) | `9cd06399ce8dc4ec9c2d9e3cd7afa2d3f91b59dc49a2f584d8213cff8beb2fb1` |
| Destroyed and deselected (step 29,999) | `f7d63bfe8db5e90926108a3c0f62ba0ac0cc2f23a1c3f5086b1a8f5301f14d60` |
| Cleared footprint (step 30,699) | `7de51ae9f648dc5432c2b9d42970e35faac3ea35c26fa137df957f976b11a4bc` |

## Identify the surviving structure (November 16)

A fresh owned-ROM replay matches all 24 pre-attack captures, the three
early damage frames, the 171-HP frame, and the exact destruction and
cleared-footprint frames. From command end 30,700, hold X=30,000 for 13
steps, release for seven, hold Y=-22,000 for 12, and release for 18. The
cursor hovers over the surviving red structure at observed step 30,749.
Press A for eight steps and release for 30. The clean step 30,787 frame
shows its hostile selection ring and identifies it as an Extractor with
750 HP. This confirms a second live hostile building in that base; it
does not establish that the Extractor is the owner's last building.

The replay finishes at step 30,788 under the same 31,000-step visual
limit and the existing wall-time, file, disk and isolation limits. No
second attack or competitive result was observed. The remaining 212
steps could not cover another sustained lone-Marine attack at the
measured rate. To reach the result within the bound, fork the earlier
opening with a larger force and pair any last-building kill with the
read-only result observer. A suggested north-pan route to reacquire the
existing Marine never ran: that separate attempt rejected a mismatching
opening capture before sending new input.

| Local receipt | SHA-256 |
| --- | --- |
| Extractor identification manifest | `5beee8297a3aedb81eaa08be53a8560761c3d78d7b99c439bd1e059ab8ac87cc` |
| Complete controller commands | `1652bd16f42525777a73f8ddec1b44311a622a87dbc5c7b75d874288fe9be843` |
| Hovered structure (step 30,749) | `f23c0584b59e8b1dc17d2b988ae30140d3d8244e11fc904b08aa695c706b67db` |
| Selected Extractor at 750 HP (step 30,787) | `972ae09bab4564a470286862b63068ca720061b68d16e68ecfa396433981bb7e` |

## Build and select a three-Marine force (November 16)

A new fork of the same blank-opening tape matches ten exact visual
anchors through observed step 11,843, with one Marine already queued.
Two ordinary A presses and releases add two paid queue entries.
The queue shows both new icons at steps 11,871 and 11,899, with the
wallet showing 153 and 106 minerals. The displayed price is 50, with
mining continuing during the presses. By step 15,844, the queue is empty
and three separate Marines are visible beside the Barracks (7/10 supply).
B closes the production grid; A selects the 40 HP Marine and R selects
the group, showing `UNITS: 3` and three green rings at step 15,961.

A prior attempt rejected the exact early visual anchor before sending
new inputs: eight pixels differed, though its other nine frames and all
129 controller transition rows matched. No tolerance was added. The
successful retry matches all ten anchors, under the same guard limits.
This establishes a larger ordinary-input attack force. It neither
executes a second attack nor confirms competitive elimination. The old
southeast route is a starting point; the later departure and group size
require a fresh check at the target before using B.

| Local receipt | SHA-256 |
| --- | --- |
| Completed force capture manifest | `c76e0081e547d30cca839b9e3304c65b779af596c3bfa407b00af415761873f8` |
| Complete force input tape | `6f624d1a8ee718a2c39c5ef94a86cde6fbd74e084bfb7b8478b5cc71075634b4` |
| Second Marine in queue (step 11,871) | `67f56dea7d9434a855da92916277f12bc95b86c742a2e4da96a01e4e7d0938e6` |
| Third Marine in queue (step 11,899) | `94e8b76fed0971b7ae6b0783203342a99cf2c4ed544067a80d8690a297be7489` |
| Three Marines selected (step 15,961) | `354446384e8ab1413cc0b5f938a58c1f64742e0c159bfcaf89cdc7b46c71dc0f` |

## Three Marines reach and damage a Hatchery (November 16)

Two fresh runs with a separate read-only observer reproduce the exact
15,962-step three-Marine input and all 18 visual anchors. Their six
sampled guest-state records agree byte for byte at the same command
boundaries. The last sample finds three selected owner-3 Marine
records whose HP values agree with the 40 HP Marine shown earlier.
The observer checks actor references against the owner chains and
uses a cache-aware reader, without guest writes, save restoration or
recompiler changes. No display corruption recurred in the pair, so it
does not classify the earlier differences in raw video. The observer
samples a bounded set of gameplay fields, and stops at group selection.

In a fresh replay the group takes two ordinary C-Right Move waypoints.
All three Marines are visible in the southeast Zerg base at observed
step 18,496. The attack launcher requires all 24 archived raw frames
to match, including the full frame showing the cursor on the hostile
structure and the native contextual `B KILL UNIT` prompt at 18,571.
It also requires the six prior guest-state records to match exactly.
B then registers at observed input step 18,574 and releases at 18,580.
The command and controller transition prefixes through the aim frame
are byte-identical to the archived aiming replay.

A subsequently selects the red Hatchery. Clean frames show 980,
719 and 468 HP with Marine muzzle flashes and human supply remaining
7/10. The run ends after 21,308 command steps and 28 frames, with the
pinned inputs unchanged. This is ordinary group targeting and observed
damage to one structure. It does not attribute individual shots,
establish combat timing parity, or reach building destruction, owner
elimination or a published result in the three-Marine run. The later
HP values are visual observations; the six guest samples all predate
the attack. Controller, file, wall-time, disk and isolation limits
were unchanged.

| Local receipt | SHA-256 |
| --- | --- |
| Paired precombat diagnostic report | `8d29a82cefa80dda2400dc07aa8dbce0a6aaa6344042f36326157a346cfbefed` |
| Six matching precombat guest samples | `b8bad8d20658a5b9ac140fd612df6842a4e242705f13026e6d43b271241a0094` |
| Three Marines in the hostile base (step 18,496) | `588f79ebca78d5d9507802637c1e9bc35cc717b7c6ea3519d12d02d6d2ad10db` |
| Contextual attack anchor (step 18,571) | `c55fcee8722672c2badeb5e7a6350fbebc5ae204154d228c2a17c39ea29bbf86` |
| Attack manifest | `58b26bffebb6ac500b398ef90e531c57f97a3be4eafc97de3d68337ad5b1c478` |
| Complete input tape | `1e3495e906065f107dfefa986d206aaf49851e7fd0e3630367609c3e0477b3c7` |
| Controller input log | `5df75b02382318d53d09ca0b151942488954dd1e13fb0ac49f63f494b1709a4a` |
| Hatchery 980 HP (step 19,507) | `56db04f84936a74a4baebbb700f847e22aeabd1cdb411b0c5c0d003391a7347c` |
| Hatchery 719 HP (step 20,407) | `918e3f31f2497b075c4ff624b8005c3bca8643f85a72db1ee2c6f234cd2be79e` |
| Hatchery 468 HP (step 21,307) | `48e43a9f2aed720a759e7f1002c0c47b3159ea57edbd9e78bb96248ccce7d7bf` |
