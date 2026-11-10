# Z2 shared Egg HOST stage

This is an opt-in HOST experiment, not a release flag, an ordinary campaign
completion, or N64 parity evidence. Source checkpoint `2ac2eabe` uses the
existing Zerg Egg service for the constructed Z2 campaign state. Z2 admission
authenticates the producer's raw source slot against the mission descriptor and
current owner, then checks that source's PUNI permission before charging. It
fails closed when that mapping cannot authenticate ownership. A tagged Egg's
detached candidate is checked against the same source/product permission; old
tag rows continue through their existing decoder. Source PUNI is an admission
rule, not a filter on pre-existing map actors.

The historical permission bank was recreated in ignored storage using the
repository's pinned importer, the owned ROM and pinned extractor. The ROM SHA-256
was `e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020`,
the extractor SHA-256 was
`6a84761a9a8074fcc393527a7b17cea194084e8ab49176ff82fa3bef8e1b1dc2`, and
the generated header SHA-256 matched the prior receipt:
`c9ff17c083b5f9cb699aad753da5b496931dd818f5b50d315954926632457a03`.
The private bank, ROM and save bytes are not included here.

Under the shared native lock and with over 4 GiB free, the actual-bank fixture
passed for source slot 3: a paid Zergling Egg, source-owner checks, a
checksum-valid new-tag row changed to a PUNI-denied Queen which independently
passes the ordinary Egg shape/owner test, atomic rejection, fresh-process
save/load and paired birth. A separate unmodified `aa8f43c5` engine, with the
same 592 hash-verified prepared dependencies and no Egg-stage flags, produced
a Z2 initial-world old-tag save. The new stage loaded that save and selected it
again after a checksum-valid, PUNI-denied newer Egg record was written. The
private old save hash was
`90747d6b16add25fb58b5e684a018f744b6ee78adc1ac751775faaa50ab69a08`.
These are constructed HOST fixtures: minerals, larva and prerequisites are
set for the test; they are not ordinary-input gameplay or a win.

At `23713c68`, the synthetic-permission fixture and fresh-process pair passed,
along with existing Z1, Z8, Z9 and capacity fixtures. At `01effac6` the focused
synthetic denied-product regression passed. The initial actual-bank attempt at
`efe0ddb5` failed its independent shape assertion: the chosen Ultralisk is not
an admitted larva product in this profile. The corrected Queen witness passed;
the failed run remains part of the local receipt. The final `2ac2eabe` actual
fixture, writer, fresh reader and genuine-old fallback all passed; input and
permission hashes were checked before and after. Logs and receipts remain in
the isolated checkout's ignored `.local/egg-early-*` directories.

The authored Z2 campaign adapter has no Z2-specific `spawn_unit` call after
initialization; this observation does not establish the behavior of all
original CHK triggers or CreateUnit actions. Transferred ownership, further
missions, trigger-driven reservations, broader old-save variants, ARM links,
stack and hardware behavior remain unqualified. Do not enable this stage in a
normal ROM on this evidence.
