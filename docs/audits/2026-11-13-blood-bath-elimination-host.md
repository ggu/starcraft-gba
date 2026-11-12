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

Loaded passengers and actors with coupled Carrier, Scarab, Egg, Ghost
or Battlecruiser state may need a coordinated transfer across other
systems. If a survivor of the defeated owner has such a state in a
profile that enables it, this preview defers the whole owner transition.
Until that transition can run, the port still uses its existing final
win/loss result logic. Ordinary human defeat and simultaneous final
losses also keep the existing result precedence. This is a prototype
constraint; a released version needs a native witness for linked units
and complete ARM and hardware tests, including the frame cost of a
large transfer and the persistence of relevant sidecars.

The mask occupies reserved bits 13–15 of the existing resolved-start
word in `RaceSave.state[23]`; it does not change save size or move any
record. The sparse-seat, race, start-order and game-type fields stay
intact. Zero mask leaves the previous word unchanged. Older readers
reject a marked record rather than silently reactivating those CPU
ranks; they can still fall back to any earlier valid save in the other
SRAM bank. The preview loader rejects marked records with a live actor
belonging to an eliminated rank, and rejects marked neutral survivors
that retain active orders or targets. Restart resets the mask and
reuses the saved opening. Reading a historical record does not apply
elimination during load; the normal outcome check can do so on the
next completed update.

Run `make test-sc64-native-skirmish-elimination` locally with the
ordinary Terran profile inputs already prepared. The sanitizer fixture
uses physical menu keys, real building creation/destruction, the
ordinary simulation, SRAM save/load and pause restart. It covers both
modes, a surviving CPU still taking turns, two owners eliminated in one
update, final victory, a loaded passenger that defers an entire transfer,
previous and marked record reads, a changed launch clock, and a malformed
marked record that tries to eliminate a still-live owner. It checks the
previous reader's exact word policy directly. It is a host fixture and
does not qualify an ARM ROM or establish late-game N64 observation.
