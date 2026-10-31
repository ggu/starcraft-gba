# Experimental hidden-child save stage

The normally disabled Terran namespace stage now encodes authenticated
raw-85 training and inside children using the existing bridge before the
compact codec, and decodes them before candidate graph checks and live
restore. Old namespaces still reject raw-85 rows. Outside and self-destruct
phases remain rejected; no logical flight order is registered as a native
state. The existing graph validator checks parent, ownership, paid queue and
training health. This is a staged format, not product admission.

The synthetic host fixture spawns a Reaver, pays through the actual Scarab
enqueue adapter and advances the actual production tick. On both maps, fresh
processes save a child in training, load and finish training, then save and
reload the inside child. A deliberately checksum-correct newer inside row
with invalid health falls back to the earlier training record. With that
older record removed, rejection preserves sampled units, buildings, resources,
frame, RNG and SRAM. This is not ordinary paid building/gameplay or full-world
atomicity. It does not exercise flight, hardware or other save codecs.

The existing seeded BC fixture also passed matching writer/read continuation
output for both old BC and staged records with two Yamato shots and one impact;
that tail fixture does not contain a Scarab child. Compile and run receipts
are in `.local/decoder-proof/hidden-stage-final-{compile,runs}.json` and
`tail-hidden-{compile,runs}.json`. The host build reused the prepared v6
source bank (`0e03fcc54e860ab7d7a9904bcacdd47077045754294193ea7759dc9c949d9845`)
at its durable path; the earlier vector's generated-bank path was unavailable.
No complete profile matrix, ARM, stack or performance qualification is claimed.

An independent review found that dead raw-85 rows could be written with an
outside phase even though the staged reader rejects them. The writer now
applies its hidden-phase restriction to every Scarab row, including dead rows.
Six actual writer cases reject live and dead outside/self-destruct rows, dead
type/raw mismatch and malformed carry without writing SRAM. Seven checksummed
candidate cases cover invalid inside HP, hidden target/timer, owner and raw
identity mismatches, orphan parent and impossible parent queue count; each
falls back to the prior training save and rejects with sampled state and SRAM
unchanged when that fallback is removed. These are selected corruptions, not
an exhaustive capacity or whole-world rejection proof.

One staged record also contains a genuinely in-training child at the same
time as two active Yamato shots and a pending impact; its fresh-process
continuation output matches the writer. The run and compile receipts are
`.local/decoder-proof/hidden-combined-{compile,runs}.json`; negative case
receipts are `hidden-boundary-runs.json`.

The retained gate-off writer's BC record and a staged training-child record
were also placed in the same journal. The stage selected the newer valid
record for each tag order. With a checksummed invalid newer staged child row,
it selected the older BC record and preserved that entire predecessor slot
when writing a replacement. With a checksummed invalid newer BC parent row,
it selected the older staged child record and preserved its entire slot when
writing a replacement. The reversed ordering was made by editing the journal
sequence in a fixture; corruptions were likewise injected and rechecksummed.
The old BC input itself came from the retained gate-off writer, not a simulated
past release. Latest-source compile and 10 fresh-process receipts are in
`.local/decoder-proof/hidden-mixed-{compile,runs}.json`.
