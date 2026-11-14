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
