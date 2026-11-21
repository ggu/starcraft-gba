# Target cell occupancy trial

The current P10 zero-new-route-build attribution puts target searching and
index construction at about 8.15% of guest cycles; late Z7 attribution puts
target searching near 490,000 cycles on three route bursts. The current
index keeps conservative independent owner row and column occupancy. This
trial added a 640-byte per-owner, per-row column mask, maintained on first
cell insertion and cleared on index rebuild. Movement/removal retained stale
bits until rebuild, preserving the original order and never hiding a target.

On the combined armed-table and persistent-Command-cache cartridge, the
P10 ordinary saved tail calls 37,705–38,953 matched four world, both-page,
palette and fog cuts; every frame, screen, route-build and route-hit row;
and SRAM. A cell-bit variant tested every candidate cell against the new
mask. Its mean update cycles rose 819,393 to 819,860 (0.057%); mean orders
rose 526,143 to 526,593 (0.086%). Its peak update rose 3,359,158 to
3,359,533. A row-only variant skipped rows with no intersecting occupied
columns and left the cell loop unchanged. Mean update cycles rose to
820,218 (0.101%); mean orders rose to 526,847 (0.134%). Neither should be
promoted on this evidence. These are emulator cycles, not physical pacing.

The cell-bit variant passed the prepared P10 ASan/UBSan real-engine target
fixture: 4,000 live queries over 500 updates, 24,000 synthetic-world
comparisons and 500 fallback checks against the scalar reference. The
control, cell-bit and row-only bounded replay result SHA-256 values are
`9934f3a27991d87482c21351af3b0dbc468628e89b4ce0c1f3c7a14d214b13a7`,
`0cadc2abe0c05a65176ea8d7f7a100c686fa77b91135287fe20b66ad602ce5c8`,
and `633a91edc12f017b369a603961525a9190dc78e00dd442a4c4ff097c3d798efb`.
Artifacts and scripts are local under
`/Volumes/Storage/sc64-route-next-20261122/build/target-row`. No runtime
change is retained. P10 does not establish the outcome on denser late Z7.
