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
