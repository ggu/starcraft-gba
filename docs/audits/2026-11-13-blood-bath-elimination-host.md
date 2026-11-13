# Blood Bath CPU elimination: host preview

The native Blood Bath audit traced the final building loss to individual
player defeat and a transfer of remaining actors to the neutral owner,
where they leave their old AI. This preview exercises that transition in
the GBA engine when a CPU loses its last live building and the human still
has a building. It covers Blood Bath launches that use the resolved start
word, in both Melee and Free for All. Ordinary builds, campaigns, other
maps and historical Blood Bath launches without a resolved start word
keep their current rules.

The optional `SC64_NATIVE_SKIRMISH_ELIMINATION` flag is HOST-only. It
adds a three-bit mask indexed by the original occupied CPU rank. The AI
scheduler skips those ranks without changing the other CPU phases.
Supported surviving units retain identity, health and location, change
to the existing neutral owner, and clear their old orders and targeting
references. The saved neutral unit order is idle; the preview does not
issue it future orders. A Depot under construction is a live building
for this check, while a worker walking to place an unpaid foundation
does not count as a structure.

Loaded Dropship, Shuttle and researched Overlord passengers transfer with
the live transport.
They keep their slot reference and transported order, so they stay out
of collision and targeting. Pending board and unload orders are cleared,
and a retired transport cannot receive new ones. This models the native
code path that relinks a loaded actor without taking the unload or death
path. The native passive Free for All run sampled all CPUs still active
at observer step 9,201 and did not reach a defeat.

A live Overlord from a CPU that completed Ventral Sacs also sets the
neutral owner's capability. That bit applies to the neutral owner as a
whole, remains set after carrier death, and clears on restart. It does
not enable neutral orders. A retired CPU with research but no live
Overlord cannot set it. This follows the native transfer's upgrade copy
to owner 11.

Actors with coupled Carrier, Scarab, Egg, Ghost or Battlecruiser state
still defer the owner transition in profiles that enable them. Victory waits until all occupied CPU ranks retire, even when no CPU has
buildings. The match keeps updating while a fenced CPU remains active.
The host prototype retains the existing human defeat rule; this does
not establish native precedence for simultaneous final losses. A release needs linked survivor evidence
from an ordinary native match and ARM and hardware tests, including the
cost of a large transfer and persistence of relevant sidecars.

The mask occupies reserved bits 13–15 of the existing resolved-start
word in `RaceSave.state[23]`; it does not change save size or move any
record. The sparse-seat, race, start-order and game-type fields stay
intact. Zero mask leaves the previous word unchanged. The neutral capability uses bit 7 of
byte 3 in the existing 16-byte transport trailer. Bit 0 of that byte
keeps the human's research level. Only a marked Blood Bath record with
a retired, researched Zerg CPU may set the neutral bit. Older readers
reject a marked record rather than silently reactivating those CPU
ranks; they can still fall back to any earlier valid save in the other
SRAM bank. The preview loader rejects marked records with a live actor
belonging to an eliminated rank, and rejects marked neutral survivors
with active orders or targets except for a validated transported passenger.
The transport and passenger must both carry the neutral source owner;
the usual cargo type, same-owner, position and capacity checks still apply. Restart resets the mask and
reuses the saved opening. Reading a historical record does not apply
elimination during load; the normal outcome check can do so on the
next completed update.

Run `make test-sc64-native-skirmish-elimination` locally with the
ordinary Terran profile inputs already prepared. The sanitizer fixture
uses physical menu keys, real building creation/destruction, the
ordinary simulation, SRAM save/load and pause restart. It covers both
modes, a surviving CPU still taking turns, two owners eliminated in one
update, victory after a final CPU's fence clears, human defeat while a
CPU is still fenced, Dropship and Shuttle cargo through SRAM, rejected
neutral boarding/unload commands and malformed cargo, Overlord retirement,
SRAM, death and restart, the neutral research bit's prior reader rejection,
previous and marked record reads, a changed launch clock, and a malformed
marked record that tries to eliminate a still-live owner. It checks the
previous reader's exact word policy directly. It is a host fixture and
does not qualify an ARM ROM or establish late-game N64 observation.

The queued impact fixture covers two distinct boundaries. A packet from a
CPU whose last building falls first still arrives on the next update while
other CPUs keep the match active, and can defeat the human. If both last
buildings fall in the same update, both due packets resolve in either queue
order before the outcome check; the existing human defeat branch wins that
tie. This is a host ordering check, not a native precedence claim. The
native defeat action at `0x80080690` preserves pending result 3 or 4, but
that alone does not show whether the victory condition is met in the same
trigger pass as the human loses its final building.

The inverse direction remains unresolved: retirement currently clears
incoming pending impact target references for live neutral transfers with
the same helper used on death. The owned finalizer establishes that the
actor itself survives and is relinked to owner 11; it does not establish
whether a pending projectile may hit that actor afterward. Keep the policy
unchanged until the native projectile target path or a qualified runtime
witness distinguishes transfer from death.
