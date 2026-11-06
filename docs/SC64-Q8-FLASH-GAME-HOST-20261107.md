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

Before writing, the serialized base and staged fractions also pass the same
detached game preflight as the reader. That preflight clears research padding
and rearranges the variable shot/impact area, so the writer saves the bounded
suffix, restores it and the checked research bits, then compares the whole
payload's FNV checksum before allowing erase. No second complete payload is
allocated; FNV consistency is not collision-resistant authentication.

Because checksum/length-invalid records are hidden from semantic selection,
the write path also reads raw commit and metadata tags before selection and
again immediately before the first sector erase. Unknown new-slot ownership,
including bad-length and bad-checksum records, fails closed; an unrecognized
or partially programmed tag is not assumed to belong to this experiment.
Raw committed 0x47 legacy history likewise prevents creating a new custom
epoch, even when that old record is not admissible to this custom reader.
Read selection may still fall back to a supported intact older record.

At source/test commit `3982a8f81101fefe7d3ebf88c44451b759ecaf7e`, the
authored HOST fixture passed with ASan and UBSan configured to halt and with
diagnostic output scanned. A separate producer compiled exact baseline
`8127f7af1abe1ad9b5a911f7ea3554bcc691fba7` without research or Q8 flags and
exported a genuine 0x43 SRAM record. Fresh processes exercised migration,
an active paid Forge job plus actual shared damage packets producing
fractional Dragoon HP and shields, a further hit and job completion after
load, and preservation of the first 32 KiB. The same run passed checksum-valid
bad-new fallback and sole-record rejection, an encodable invalid mining target,
nonzero legacy shield phase rejection, unknown committed tags with bad checksum
or length, mixed 0x47 write refusal, and a pre-erase ownership-change negative.

The migrated 128 KiB model image hashes to
`35f4c1c1378fa080846a201ee44a0479a8b962b42ecb7a791252f73b49ac4e5a`;
its preserved first 32 KiB hash is
`7aed3701fedf0d3c47b73d40db686925edfd6eeac20c7309f99796bc84d95fe7`.
The separately written new image hashes to
`1d06a637435acba27d05e7ec1821bdf8332dc1ed79cdbdd1d9000fa80e519e13`.
These private test artifacts are not committed. The 593 verified inputs and
baseline source were unchanged; the exact commands and results are in local
`.local/q8-flash-game/final-results.json` and `final-*.log`. The asset-free
P10 scope test, source boundary, and exhaustive current tag inventory also
passed. With the experiment disabled, a normal guarded P10 ARM link and fixed
header ROM matched the reference SHA-256
`317cc67d8f48b6771b3ebf4497c52b290466de49d097133b7b1bfdea65b34cd9`.

Earlier checkpoint `52b8d0c9` was cancelled while queued after a raw ownership
defect was identified; no child ran. The later `5e31fdce` strict build failed
because P10 does not define the unguarded `ORDER_REPAIR` identifier; the
failure is retained in `strict-v2.log` and was corrected before the final run.
No private inputs or saved data are included in this source change. Full
profile coverage, current normal/ARM qualification, old 0x47 campaign
compatibility, persistent repair and Z5 aliases, all fault cuts, source
shield-timing parity, and real-device timing/geometry remain open.
