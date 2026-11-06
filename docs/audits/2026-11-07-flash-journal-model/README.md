# Experimental larger Flash journal model

This source-only HOST model is **not connected to the game save writer** and
normal ROMs continue to use their existing save paths. It is an investigation
into preserving old journals while making room for exact fractional combat
state. It does not establish cartridge support, physical power-cut safety,
Flash polling correctness, or runtime memory/stack qualification.

Runtime/model source: `3d529e7a`. Run from the repository root:

```sh
make -f tests/gba-flash-journal/Makefile test-gba-flash-journal
```

The transport-independent layout reserves the first 32 KiB permanently for
legacy read-only journals and provides two new 32 KiB slots. Each new slot
isolates its commit tag in a 4 KiB sector, outside the metadata and payload.
The maximum payload is 28,656 bytes. The last 32 KiB of the proposed 128 KiB
backing store is unused. A suitable backend must enforce bounds, banks,
one-to-zero programming, completion and sticky failure after uncertainty.
The assumed sector geometry and backend remain separate qualification gates.

The write path verifies the erased commit sector before changing metadata,
verifies metadata and payload before programming the commit tag last, and
publishes its descriptor only after read-back. A complete admissible new-layout
record outranks permanent legacy records, independent of counter wrap. Before
erasing a target, the writer repeats selection with the caller's semantic
acceptance policy and rejects a stale winner token. Checksum validity alone is
not product/context admissibility. The caller must use the same immutable
policy and serialize writes; this is not a concurrent writer API.

An independent Terra review found that merely rechecking the old token's slot
could erase a newer committed winner. Commit `3d529e7a` repairs that defect and
adds both old-new-layout and legacy stale-token cases, with no mutations on
rejection. The same review rechecked the repair and found no further concrete
defect in the reviewed model.

The final focused Clang AddressSanitizer/UndefinedBehaviorSanitizer run passed
under the shared native-job lock in 1.03 seconds, with all three selected inputs
unchanged. Tests cover full payload capacity, an old maximum payload plus a
400-byte tail, legacy/reserved-region preservation, 685 modeled byte/erase
interruption boundaries, 1,024 partial commit-tag erase patterns, erased-byte
tags, distinct successor data, counter wrap, checksum/semantic fallback,
stale winner rejection, and invalid argument/read errors. Private receipts are
`.local/flash-journal/reviewed/{model.log,result.json}`. Modeled interruptions
are not evidence of real-device bit-level erase or power-failure behavior.
