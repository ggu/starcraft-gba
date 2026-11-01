# Selected two-part journal writer

The proposed shared HP-regeneration version needs a validated predecessor
and a short trailer outside the large route/save scratch. The existing
selected-slot writer accepts only a contiguous payload. These component APIs
write two nonempty RAM spans as one checksummed journal record without a new
payload buffer:

- `save_write_parts_selected` rechecks the selected predecessor's tag,
  length, sequence and checksum, protects unrelated valid alternate records,
  and writes the opposite slot with the next nonzero sequence.
- `save_write_parts_first` refuses to overwrite any committed checksummed
  record in either slot, including another namespace.

The HP protocol requires a nonzero high tag byte as a one-byte commit marker.
That byte is cleared first, before the remaining tag/metadata/payload stores,
and restored last after the three low tag bytes. Zero-high-byte partial tags
are uncommitted and retryable, even when their payload checksum is valid.
They are not supported foreign namespaces; allowed tag lists reject them.
Both spans contribute to the same FNV-1a checksum and payload length. Existing
HP-off contiguous writers and normal Transport1 paths are unchanged. The HP
contiguous selected/fresh writers use the same bytewise marker protocol.
Existing
semantic-selection APIs become available under `SC64_SHARED_HP_REGEN` as
well as the Scarab stage; only the HP gate adds the split selected/fresh
writers. No product gate is enabled and no save namespace is assigned here.
The caller still needs an isolated semantic preparation callback and an
infallible live commit; this component does not make the game loader atomic.

The source-only host fixture compares complete SRAM output against the
existing contiguous APIs at multiple splits, both slot orders, wrap and the
maximum payload. It includes rejected/stale tokens, invalid lengths and
pointers, duplicate/zero/unknown tags, semantic rejection of newer records,
preserved predecessors, every byte of tag invalidation/commit and interrupted
payload stores around the span boundary, including successful retry from
checksum-valid partial tags. A host-only observer also checks the actual
writers at each tag-byte store, including the contiguous HP writers; the
default ARM stores have no observer. Both fixtures are built under HP alone
and the combined HP/Scarab gates; this does not qualify a game envelope.
Qualification on frozen component `35f5ed42e4fca14ff8e2c90692453d1ed899b361`
used ASan/UBSan host builds under the shared native lock. The four focused
HP-only and combined HP/Scarab fixtures and the existing Scarab-only selection
regression passed. The current fixture against isolated prototype `ea8b0de`
compiled and its `partial-tag-retry` case failed at the expected
`save_write_parts_selected` assertion. An initial baseline compile omitted
`audio_codec.h`; that failed log was retained, the unchanged header was added
to the isolated closure, and the corrected compile produced the assertion.
No guest SRAM was injected.

An unprepared clean source boundary, doctor, default build and full default
tests passed: 932 tests (26 skipped), plus groups of 5 and 8. Fresh default
ROMs from the component and base `0ff3ffb` matched byte for byte, SHA-256
`73c0deeeb36cca4cb8614248a7e60cadac11a0ec542f2ece31c37b171e2c56b8`.
Sources and isolated test inputs were checked unchanged. This component is
still default-off: no game namespace or loader integration is qualified. The
zero-high-byte rule does not support foreign committed namespaces with that
tag shape. No hardware power-loss or game-level save/load qualification is
claimed.
