# Bounded validated SRAM journal prerequisite

The experimental `SC64_SCARAB_RUNTIME` gate exposes a generic two-slot
selection API for at most three distinct, nonzero tags. It first checks each
candidate's tag, length, checksum and nonzero sequence, copies the payload to
caller-owned scratch, and asks a caller-supplied semantic validator to accept
it. The callback must not mutate SRAM or live state. Only accepted candidates
participate in ordering; the selected slot is revalidated and its original
payload copied back into scratch. Output selection is unchanged on failure.

Sequence comparison follows the existing wrapping signed-difference rule:
equal sequences choose slot zero. A difference of exactly half the sequence
range is ambiguous and causes failure instead of choosing either record. The
selected token records slot, tag, length, sequence and checksum. The companion
writer rechecks the token and record checksum before writing the other slot,
refuses to overwrite an unknown checksummed tag, then commits its tag last. A separate first-write operation refuses to write
if either slot contains any checksummed record, including an unknown tag.
Existing two-generation APIs and callers are unchanged.

`make test-scarab-save-selection` is an asset-free ASan/UBSan host fixture.
It covers all three-tag pairings in both slot orders, semantic fallback,
checksum failure, torn tag, sequence wrap, tie and half-range ambiguity,
stale/rechecksummed tokens, invalid parameters and first-write protection.

**This API is not registered with game saves.** It does not implement the
Scarab namespace, old Reaver initialization, BC/impact/Yamato migrations,
full game semantic validation, or a gameplay feature. The game adapter must
supply the complete side-effect-free preflight, use its token for the write,
and qualify active variable-length envelopes and fresh-process reload before
claiming compatibility. The checksum is the existing record integrity check,
not a cryptographic authentication mechanism.

## Combined-source qualification

Independent review of the unknown-tag protection child found no remaining
component blocker. Combined draft source `0517c99` passes the actual
asset-free journal Make target, and a separate clean indexed worktree passes
boundary, doctor, default build and test: 782 Python cases (23 optional
skips), eight ancillary cases and C fixtures. Boundary checks 1,233 indexed
text files. Its default ROM SHA-256 is
`577ad52c0652a4b57519dd11d1d96762ac289375a289115b8dd2c355f2271f98`,
byte-identical to the prior merged Scarab-foundation default at `52ee752`.
This is not a new four-profile ARM or gameplay qualification.

The generic API remains unregistered with game saves. Existing callers,
namespace and impact capacity are unchanged. Game-side preflight extraction,
three-generation migration and complete Reaver gameplay are separate work;
the component tests do not establish them. Private receipts are in the
`journal-default-qualification-20261031` worktree's `.local/qualification`
and the shared Scarab worktree's `.local/journal-generic-proof`.
