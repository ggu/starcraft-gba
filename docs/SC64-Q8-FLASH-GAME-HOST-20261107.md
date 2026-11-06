# Actual-game Q8 Flash HOST experiment

This is an unqualified, explicit HOST-only experiment. It does not change the
normal SRAM backend, enable an ARM ROM, establish hardware flash behavior, or
claim automatic save-type migration. `SC64_Q8_FLASH_HOST_STAGE` requires the
existing typed P10, Q8 and active-job flags, excludes the campaign journal, and
admits only a custom game with a Protoss human. The normal Q8 save fence remains
in place outside this capability.

The Flash-only outer namespace byte 0x48 wraps the unchanged 0x45 game payload
and a versioned tail containing one HP-debt byte per actor slot. It does not
retag predecessor data. A scoped adapter supplies the existing 0x45 decoder
only after it has checked the new outer tag. The shared game save serializer,
active-job checks, impact/transport/Yamato payload, and detached load preflight
remain in use; the 128 KiB model retains the first 32 KiB as a permanently
read-only 0x43/0x44/0x45 predecessor region. New journal writes use the
independent commit-sector model. The Flash device is explicitly supplied by
the HOST harness and does not alter `common/gba.c`'s normal SRAM path.

Predecessor selection reads the actual records and applies their original
version-specific game validation. This first scope rejects any live shielded
actor with nonzero legacy shield phase rather than assuming 320 or 1280 as its
unit. It preserves eligible Terran building burn debt and owner in their
existing field, and does not reinterpret non-shield aliases. Campaign/Z5
regeneration and fractional-only repair orders are not admitted. A
checksum-valid but unsupported sole predecessor cannot silently become a new
epoch. These are scope limits, not proof of a general migration algorithm.

The selector streams a candidate into the existing game scratch, verifies it
and its tail, retains only a 400-byte candidate-debt array, and rechecks the
selected full descriptor and checksum before publication. Before building a
new game payload, the writer selects and semantically validates candidates,
then snapshots their complete descriptors and acceptance. Its reselect callback
checks those descriptors without touching game scratch: the normal Flash
writer's reselect therefore cannot overwrite the newly serialized payload.
New or changed records abort reselect. This does not claim cryptographic
payment authenticity or solve concurrent FNV collisions.

Because checksum/length-invalid records are hidden from semantic selection,
the write path also reads raw commit and metadata tags before selection and
again immediately before the first sector erase. Unknown new-slot ownership,
including bad-length and bad-checksum records, fails closed; an unrecognized
or partially programmed tag is not assumed to belong to this experiment.
Raw committed 0x47 legacy history likewise prevents creating a new custom
epoch, even when that old record is not admissible to this custom reader.
Read selection may still fall back to a supported intact older record.

The authored HOST fixture builds a real no-feature predecessor and exercises
fresh-process migration, active paid Forge job plus actual shared damage packets
producing fractional Dragoon HP and shields, a further hit and completion
after load, read-only legacy bytes, checksum-valid bad-new
fallback and atomic rejection, and nonzero legacy phase rejection. Until the
strict native run succeeds, these are intended tests, not recorded results.
No private inputs or saved data are included in this source change. Full
profile coverage, current normal/ARM qualification, old 0x47 campaign
compatibility, persistent repair and Z5 aliases, all fault cuts, source
shield-timing parity, and real-device timing/geometry remain open.
