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

No standard build or reader uses the study tag; the optional runtime below
attaches it to the shared Carrier behavior.

| Local receipt | SHA-256 |
| --- | --- |
| Z10 sanitizer study | `f2f297125ea6d29f5e74ce4332797d78ca5e72121429a4807cea96ea6bb15e85` |
| ARM layout receipt | `03dbc28b8bb8856a64914d2fe3bdad75a93b52e14fce1cef76899302fdfdd41c` |

The optional Z10 runtime uses that envelope in custom games only. Carrier
retains its existing engine ID 45; Interceptor appends at 49. The shared
production, flight, projectile and Protoss bot services use the existing
actor facts and directional art. Campaign records still use the original
tag, base layout and eight-row journal limit. The probe keeps the live
32-row queue and allows all 32 rows in custom saves.

The new reader opens the compact record in the existing route/save scratch
and holds the projectile packet apart while it validates the reconstructed
world. It checks active impacts and projectile references against the
candidate world and validates each Carrier hangar before commit. Ordinary
predecessors retain their own tag and are loaded with an empty projectile
arena. The first compact upgrade retains the intact old bank.

```sh
make -f Makefile -f tests/sc64-zerg-carrier-runtime/Makefile \
  test-sc64-zerg-carrier-runtime build/StarCraft-Zerg-Z10-Carrier-Probe.gba
```

The ASan/UBSan runtime fixture paid for both actors through the real
queues and Carrier menu, restored a live shot and every one of 32 pending
impacts, then observed damage. A correctly checksummed record with an
invalid shooter or malformed packet fell back to the preserved ordinary
bank. A Zerg campaign save still opened under the original tag. The ARM
link passed the unchanged guards, leaving 14,888 bytes before the EWRAM
guard and 624 bytes before the IWRAM guard. With the probe flags off, the
packaged standard Z10 ROM is byte-identical to the envelope draft base
(`72e5c4f768de2cd163e4e82ac34885f8c15c25740406fa19d576b5e1684cb2a6`).

The ARM probe (`f4145128abbf64a1ef710baeea4278bf45debeeadd3196d2df33678dc5e5b47c`)
then ran a blank-journal Protoss-vs-Zerg match on authored Outpost. The
ordinary Stargate queue charged 350 minerals and 250 gas at frame 9,753;
the Carrier menu charged 25 minerals at 12,431. The Interceptor was in
flight by frame 12,811. A physical START/B save at 12,847 wrote a 14,100-byte
record: a 30-byte packet named the player's Interceptor, slot 12, as
shooter and a Hydralisk, slot 48, as target. A fresh process used title L
to restore the frame, wallets, supply and observed actors, and the
immediate re-save reproduced the full payload. The target was present at
40 HP after restore and absent 80 updates later; the replay does not
isolate the cause. Physical stack high-water, timing, and the full custom and campaign
matrix still need qualification.

A separate [withdrawal receipt](../../tools/sc64_z10_carrier_dock_receipt.py)
resumes that live-shot journal with ordinary title L. The player directs the
Carrier toward (101, 134) at frame 12,866. Interceptor slot 12 changes from
attack to return and docks with Carrier slot 5 by 12,881, restoring its
40 HP and 40 shields. START/B writes a 14,081-byte payload with no active
projectiles. A fresh title-L load restores the docked actor and re-saving
reproduces all 14,081 bytes.

After composing the Terran and Z10 drafts with release `e05a0e52`, both
separate probe builds pass their sanitizer fixtures and unchanged ARM
memory guards. The composed Z10 probe also reproduces the withdrawal
receipt from the original active-shot seed. With the probe disabled, the
composed standard Z10 ROM matches the current packaged release exactly
(SHA-256 `2b907cdecb5769dde471de04b280ffe7de79a3b43874b2eaabfc2a9b8be43eb8`).
