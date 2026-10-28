# P7 owner-validity mask hoist

This P7 cache optimization replaces an eight-slot scan and up to eight
per-row source-owner lookups with one `sc64_p7_valid_source_mask()` calculation
for each full-world validation. It passes that immutable mask to the existing
row validator; the single-row save check still computes its own mask once per
call. The prior merged zero-byte helper remains unchanged.

The mask includes source bit `i` exactly when `race_p7_source_owner(i) >= 0`.
The validator rejects detected-owner bits outside that mask, preserving the
old predicate. Owner 0 is valid. The resolver reads the canonical
`static const` P7 race-map descriptor, its const `owner_modes` table, and the
const active-slot table; it does not use mutable `race_p7_map`, world binding,
or menu state. Hoisting therefore observes the same owner mapping for every
row in a single world validation. Cases where the resolver returns negative
(including a missing descriptor, noncanonical human slot, null owner-mode
table, or inactive owner slot) have their bits clear and remain rejected.
The optimization preserves the old `>= 0` predicate; it adds no new range
validation for arbitrary positive resolver results.

No tracked test file was changed for this optimization. The existing P7
Arbiter cache fixture remains the current public regression. A separate
private sanitizer proof compared the hoisted predicate with the former
per-row rule across arbitrary 16-bit weapon words, alive/dead states, and
controlled allowed-source masks, including invalid owners. Its controlled
resolver and differential harness are not shipped.

Private paired ARM evidence compares the merged zero-only helper against the
combined zero-helper plus mask candidate on the same 600-frame opening and
1,400-frame attack replays. All 21 state records match the corresponding host
replays and each other on ARM. Mean cycles fell 2.05% in opening and 2.36% in
attack. This is a bounded P7 replay result, not a full-campaign or hardware
frame-rate qualification. The optimization is active wherever the existing
P7 Arbiter cache flag is enabled; no new feature flag was introduced.

Qualification on this candidate source: `./dev test` exited 0 (649 Python
tests passed, 20 skipped; all native targets also completed). The four direct
normal ARM links passed linker bounds: BSS ends at 0x203708c/0x20370f8/
0x20382b8/0x203df38 against 0x203e000, and IWRAM ends at
0x3004db8/0x3004f58/0x3004b70/0x3004fd0 against 0x3005000. The source tree
boundary check passed. Terran and Zerg ROM bytes are unchanged; Protoss P7 and
Showcase are expected to change because both enable the P7 Arbiter cache flag.
Prepared-input P7 cache-generation-43 and immediate-save host fixtures passed; the
private cache fixture uses generation-43 assertions matching this runtime.

The approved real P7 campaign RGB replay passed 21/21 world records, 21/21
Mode4 RGB555 captures, and was cleaned up after the owned emulator reached its
terminal marker. This checks raster/state parity only; its timing records are
excluded. Its ROM and preflight hashes, resolved include paths, and isolated
emulator terminal evidence are in the ignored qualification output.

The paired 2.05%/2.36% timing result remains private bounded replay evidence,
not a full-campaign or device claim. Root’s token audit independently found
the measured and final header token streams identical after renaming only the
local parameter `allowed` to `allowed_sources`; formatting/comment differences
are nonsemantic.
