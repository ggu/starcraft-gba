# Z10 custom Carrier save budget

The normal Z10 record consumes a 16,296-byte base before its transport
and impact tail. Only eight of the 32 live impact rows fit in the
original journal. This detached format study keeps all original actors,
full active fog, all 32 live impacts, and a full compact 100-projectile
Carrier packet within the unchanged 16,368-byte journal.

| Wire region | Bytes | Preservation rule |
| --- | ---: | --- |
| Version and 240 unit records | 8,194 | Keep every field; the two coordinate high bits and five raw-owner high bits must be zero |
| 160 building records | 4,200 | Same coordinate/owner checks and seven reserved zero bits per record |
| Minerals, active fog and remaining world fields | 1,656 | All minerals, 512 bytes of active map fog, wallets, map choices, campaign/backwater and invincibility |
| Packet and tail lengths | 4 | Two little-endian lengths checked against total input size |
| Full Carrier packet | 2,015 | All 100 projectile rows and allocator state |
| Full live impact and transport tail | 216 | 32 impact rows, header and unchanged transport trailer |
| **Worst-case record** | **16,285** | **83 bytes below the existing limit** |

Both current custom maps are 64×64. The old encoder already checks
actor positions against the live map. Custom spawns leave source owner
unset, and the narrow raw-owner code still preserves -1 through 14,
including source-neutral owner 11. The writer explicitly rejects
records with any omitted bit set. The other omitted regions are the
1,536 fog bytes beyond active geometry and 220 bytes of Zerg mission
storage and padding; all their bytes must be zero before encoding.

The same record can be packed forward and unpacked backward using one
18,527-byte workspace. One partial-byte-safe row at a time is staged
on the stack, so packed adjacency never overwrites a future source row.
After opening, the compact packet is preserved above the reconstructed
ordinary bytes for later actor-level validation. This fits the existing
32 KiB Z10 route/save scratch union without a second full record.
Production integration still needs the existing route/save ownership
fence and a complete call-stack qualification.

The ASan/UBSan test restores both maps' actual predecessor bytes and
combines an authentic save base and transport trailer with 32 actual
queued impacts. It writes the full augmented envelope through the
normal SRAM journal primitives and runs the real detached game
preflight on the restored base and transport. The additional impact
rows are separately validated against the candidate world; the existing
preflight's eight-row buffer is intentionally unchanged. Corrupt but
checksummed compact records and semantically invalid lobby fields fall
back to the intact predecessor. All 18 prepared custom configurations
also round-trip after 90 updates, and the in-place codec matches the
disjoint codec at full capacity, with empty packets, and on arbitrary
allowed bit patterns across all slots. A freestanding ARM compile
independently confirms the layout and workspace sizes.

```sh
make -f Makefile -f tests/sc64-zerg-carrier-envelope/Makefile test-sc64-zerg-carrier-envelope
```

No normal build or reader uses the study tag. Next: overlay the shared
Carrier service and append its actor ID without altering old IDs, validate
full live projectile references before publishing world or arena, and
qualify a dedicated, guarded ARM preview with ordinary input and game
saves, then integrate across cartridges.

| Local receipt | SHA-256 |
| --- | --- |
| Z10 sanitizer study | `f2f297125ea6d29f5e74ce4332797d78ca5e72121429a4807cea96ea6bb15e85` |
| ARM layout receipt | `03dbc28b8bb8856a64914d2fe3bdad75a93b52e14fce1cef76899302fdfdd41c` |
