# Experimental Scarab packets in the shared impact pool

This private, default-OFF stage follows `617c53a`. It replaces the separate
four-entry Scarab packet array with tagged rows in the existing 32-entry live
impact queue. The normal build does not compile the tagged path. This reduces
BSS by 32 bytes; it adds no per-actor state. It does not claim the source's
100-bullet capacity or release readiness.

Pinned OpenBW `4b046d5f65302b10cb0a745f0fecd37ec85b20a8` ignores a failed
`create_bullet` in IScript `useweapon`, while `sigorder` still leads to the
child's Die order. Ten bounded, synthetic flat-world runs (five start phases
for each of a full bullet container and failed sprite/image initialization)
observed no weapon-82 packet or victim damage and zero remaining child counts.
That is source/reference behavior, not a hardware observation. On a full GBA
32-row shared queue this stage likewise consumes the child without fabricating
an impact; its smaller capacity is an explicit remaining limitation. The
alternative four-row policy that paused the child is retained only in the
separate rejected experiment `1422ee4` and is not part of this branch.

Marker `0xfe` is outside the ordinary source-type range. Its five payload bytes
hold source X12, Y12, explicit target9 (511 means absent), owner3, countdown3
and one reserved zero bit. Supported world geometry and the `MAX_UNITS +
MAX_BUILDINGS < 511` assertion keep coordinates and target sentinel distinct.
The packet countdown starts at 6 because the shared queue advances after orders
on the creation update; only source-due updates advance tagged rows. Ordinary
rows retain their existing per-game-update timing. Due rows remain in the
shared queue until resolved so cleanup caused by an earlier impact can clear a
later Scarab's explicit target without losing its positional splash. The
strict packet helpers validate bounds and reserved bits.

These packets have no registered save representation. A save refuses while a
marker is live, before writing SRAM; the ordinary impact decoder rejects the
marker for both historical and new Scarab tags. Synthetic tests inject a
checksum-valid marker into a new-tag candidate and verify fallback to an older
valid record, then rejection with sampled live state and SRAM unchanged after
the older record is removed. Pending-save refusal also leaves SRAM unchanged.
These checks do not establish whole-load atomicity.

A separate production gap remains: after an actual paid attack, once its
packet has drained, the dead raw-85 outside row is still rejected by the
namespace writer until that slot is reused. A focused test observes failure
with unchanged SRAM and success after the same slot is reused. It does not
normalize dead or live outside rows; outside-child persistence and post-impact
save readiness remain closed.

Sanitized actual game-update checks cover five simultaneously pending paid
Scarabs, mixed ordinary/Scarab same-update resolution, target death and slot
reuse, owner change, 31 ordinary rows plus a Scarab, and a full 32-row pool.
The existing ordinary impact combat/cleanup/overflow cases also pass with the
probe enabled. An initial invocation of the broad ordinary fixture failed its
unrelated save-length assertion because it expects a non-Yamato envelope;
the focused wrapper runs its nine non-save cases, and the new-tag save tests
above run separately. Earlier test attempts and all argv/exits remain in
ignored `.local/shared-packet`.

All four full-probe ARM links pass existing guards: Terran
`020370d0/03004e68`, Zerg `02037144/03004f68`, P7 `020382fc/03004c38`,
and Showcase `0203df50/03004ff0`. IWRAM matches the preceding probe and
Showcase has 16 bytes of headroom. The existing prepared P7 fact table was
used; no source assets are committed. No new full default qualification was
run for the gated runtime change.

With prepared inputs, `make test-scarab-impact-packet` runs the asset-free
packet bounds/marker regression; `make test-scarab-shared-impact-terran` runs
the actual shared-queue and SRAM checks; and
`make test-scarab-postimpact-gap-terran` reproduces the known fail-closed save
gap. The opt-in `make -j1 test-scarab-cross-codecs` runs the existing Zerg,
P7 and Showcase cross-codec fixtures for both maps, generating P7 facts in an
isolated ignored directory. These recipes have no private extraction or
asset-generation prerequisites. They are scoped synthetic fixtures, not
native-play or historical-release save qualification.

Full source concurrency, bullet/image allocation capacity equivalence,
precise pathing, repeated fractional residue, research/persistence, runtime
performance and outside-child or pending-packet saves remain unproven.

After the four ARM links and focused host checks, all 104 retained input hashes
still matched. Zerg, P7 and Showcase each also passed gated actual-update
cases for the ordinary phase, one-free-row boundary, full pool, owner change,
and same-slot reuse. The reproduced post-impact save gap remains a blocker to
admitting the feature; its failed save did not change SRAM.
