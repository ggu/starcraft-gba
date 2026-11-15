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
