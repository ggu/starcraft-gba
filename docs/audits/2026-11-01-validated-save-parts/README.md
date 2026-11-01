# Selected two-part journal writer

The proposed shared HP-regeneration version needs a validated predecessor
and a short trailer outside the large route/save scratch. The existing
selected-slot writer accepts only a contiguous payload. These component APIs
write two nonempty RAM spans as one checksummed journal record without a new
payload buffer:

- `save_write_parts_selected` rechecks the selected predecessor's tag,
  length, sequence and checksum, protects unrelated valid alternate records,
  and writes the opposite slot with the next nonzero sequence.
- `save_write_parts_first` refuses to overwrite any checksummed record in
  either slot, including another namespace.

The destination tag is cleared before metadata/payload stores and committed
last. Both spans contribute to the same checksum and payload length. Existing
contiguous writers and normal Transport1 paths are unchanged. Existing
semantic-selection APIs become available under `SC64_SHARED_HP_REGEN` as
well as the Scarab stage; only the HP gate adds the split selected/fresh
writers. No product gate is enabled and no save namespace is assigned here.
The caller still needs an isolated semantic preparation callback and an
infallible live commit; this component does not make the game loader atomic.

The source-only host fixture compares complete SRAM output against the
existing contiguous APIs at multiple splits, both slot orders, wrap and the
maximum payload. It includes rejected/stale tokens, invalid lengths and
pointers, duplicate/zero/unknown tags, semantic rejection of newer records,
preserved predecessors and a tag-last interruption model around the span
boundary. The existing selection fixture is also compiled under the HP gate.
These tests are unrun; native work is queued behind the active physical
replay. No hardware power-loss or game-level save/load qualification is
claimed.
