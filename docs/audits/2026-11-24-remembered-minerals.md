# Remembered mineral patches

A mineral patch enters the player's memory when its center has live
sight. That memory clears when the player sees the patch depleted.
The observation runs after the sight update, including scripted
Showcase reveals. If a patch depletes in the same update that the last
scout leaves, the previous observation remains until the player
returns. Terrain exploration alone grants no knowledge of a patch. The incremental
sight path scans the mineral roster after a sight source changes or
collection exhausts a remembered patch. A stationary sight roster
with no pending depletion skips the scan.

The shared world painter draws a blue diamond for remembered minerals.
The minimap uses a cyan mark for each remembered patch and draws its
unit and building markers over that layer. The depth queue and tile
cache distinguish live art, remembered markers and hidden slots.
Hidden depletion leaves both marks in place. Worker orders and mineral
amounts still come from the simulation. The native reference for this
behavior is the [scout-and-return audit](2026-11-07-native-fog-memory/README.md#remote-mineral-patches).

## Save format

Standalone saves pack the bitmap into the high bits of the selection
word. The race save codec stores up to 144 observation bits in six
bounded state words with an in-band marker. The loaders restore the
original state words in the candidate before gameplay validation and
publish the bitmap with the selected world. Record lengths and SRAM
journal tags are identical to their existing formats. Carrier
envelopes copy the state words in fixed-size spans.

Legacy records start with empty mineral memory and observe the patches
in live sight after loading. The legacy race readers used by older
builds can reject the marked state words. Keep a copy of the earlier
save generation when comparing builds.

## Verification

`make test-sc64-mineral-memory` covers the standalone game with and
without the mineral FIFO, and race wire records with 32, 48, 80 and
144 slots. The gameplay test uses real sight and checks both framebuffer
pages, hidden depletion through a save, the FIFO record tag, migration
to the remembered-mineral reader from a legacy record, and restart.
The wire test compares every position and the empty and full bitmaps
against an independent little-endian byte fixture. It also checks
legacy state and rejects malformed markers or padding.

The Terran and P6 host witnesses use the authored skirmish map and
existing prepared profile inputs. The witnesses exercise the production
collection helper, including last-load depletion in stationary sight.
Hidden depletion survives save/load and clears on return. Terran also
loads a legacy record through the worker-cargo and transport save paths.
P6 covers the 144-slot save layout with no pending impacts.
The canonical Terran ARM syntax check passes with the prepared inputs.

The Z10, P10 and Showcase release checks need profile-specific
generated headers. Campaign replay, populated P6 impact saves and
on-hardware frame pacing still need qualification.

## Observer cost

The ARM7 observation probe uses 144 slots and a 128-by-128 packed
sight grid. Grouping the bitmap work by byte reduces the isolated
scan in all four measured cases.

| Patches present | Initial memory | Per-slot loop cycles | Byte-grouped cycles |
| --- | --- | ---: | ---: |
| Sparse | Empty | 14,972 | 12,734 |
| Sparse | Full | 30,332 | 23,072 |
| All | Empty | 31,868 | 24,104 |
| All | Full | 13,436 | 11,666 |

These measurements cover the isolated scan when invoked. The
incremental visibility gate skips that scan during unchanged sight
unless collection exhausting a remembered patch requests an update.
Full-frame timings also include draw admission, minimap markers and
any additional visible work.
