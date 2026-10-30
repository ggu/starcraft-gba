# Scarab field codec prototype

`sc64_scarab_fields.h` is an inactive format experiment; it does not enable Reaver production, allocate actors, change a save generation, or establish raw Scarab identity. It adds no runtime storage by itself. The caller must enforce `MAX_UNITS <= 240` before using the eight-bit slot encoding.

A parent packs queued count (3 bits), active child slot (8 bits, 255 means none), and cooldown (6 bits) into 17 bits. The low 16 bits use signed-16 `carry` with a bias of 32768; `work_timer` bit 7 stores the seventeenth bit and bits 0–6 store progress 0–105. This keeps both fields representable by historical signed-16 rows as well as compact codecs; the decoder rejects reserved progress/slot/queue patterns before writing its output. A child packs parent slot (8 bits) and phase (2 bits) in nonnegative `carry`, and life (7 bits) plus no-collision timer (3 bits) in `work_timer`. The prototype leaves `build` at its existing value; the fixture uses −1. Packing is a representation, not validation of a whole world or a promise that these fields are free during gameplay.

The asset-free `tests/test_sc64_scarab_fields.py` extracts the current engine's compact and historical `Unit` definitions and the exact historical save/load loops, and compiles the real 38-byte compact, 35-byte Z/P, 29-byte derived and 28-byte P7 typed codec functions under ASan/UBSan. It exhaustively checks accepted parent and child field combinations; representative boundary values round-trip through all four codecs. The original prototype used a Reaver surrogate; the gated identity fixture now uses canonical raw 85 with 20 HP and 10 shields across the four enum capability sets. This proves bounded row representability, not candidate-world validation or saved-game compatibility. Each capability/layout run checks 7,759,566 parent shapes and 673,400 child shapes, including the later no-parent extension. Legacy loops and all four compact codecs also round-trip representative boundary rows. This is not a whole-world save/load or namespace/migration test.

Before any adapter uses this layout, audit generic field mutation (notably STOP clears `carry`), hidden-child participation, collision/target indexing, ownership transfer, parent/child death and slot reuse. Define phase-specific invariants, paid-queue and child identity checks, source transfer cancellation/refund before rebinding children, save versioning/migration or safe refusal, and fresh-process tests for all enabled codecs. Verify timer scheduling and memory guards on the actual four products. Existing legacy saves and unrelated unit fields must not silently acquire this interpretation.

Parent slot 255 is permitted only for an outside child (phase 2) or its
self-destruct signal (phase 3). Training and inside children still require a
live-slot-shaped parent in 0–239. The pinned reference's `destroy_unit_impl`
detaches outside fighters while destroying inside children; this sentinel
allows an outside Scarab to outlive its parent without retaining a reference
that might bind to a reused slot. Shape permission is not a world-lifecycle
validator. The expanded fixture checks 673,400 child shapes and round-trips
detached raw-85 boundary rows through each codec.

A separate local source-data audit reports raw 85 with 20 HP, 10 shields, movement/turning, invincible and mechanical flags. Its interpretation follows the pinned OpenBW data reader and is not native-execution evidence. A future identity must reject ordinary attacks against launched invincible Scarabs while retaining scripted death and expiration, and must not assign ordinary supply. The surrogate codec test above does not validate those facts or implement that identity.
