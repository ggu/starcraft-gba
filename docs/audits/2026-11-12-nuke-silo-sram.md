# Nuclear Silo SRAM probe

The opt-in `SC64_NUKE_SILO_JOURNAL` saves a queued or armed missile in the
Silo's existing building row. Source ticks remaining use values 1–1,500;
1,501 denotes armed. The existing level field stores the paying player plus
one. On load, both fields go back to zero on the live building, and the
validated receipt restores the runtime sidecar. A detached neutral Silo can
retain an armed missile and the former owner's supply charge. An incomplete
queue needs a complete, grounded, same-owner Command Center and campaign
permission to build raw 14.

The outer journal tag uses namespace `0x4b` for these receipts and newly
introduced Ghost and building IDs. The inner version, row sizes, impact
queue and transport trailer stay unchanged. The reader validates checksum,
exact outer/inner pair, building identity, parent, payer and timer before
publishing the winning record. P10 custom saves in this namespace also
validate the existing research fields. A save without a new identity or
receipt still uses its prior tag. An active approach, painting or descent
refuses a save before touching SRAM; saving the missile actor itself needs
a separate authenticated codec.

`make test-sc64-nuke-silo-journal` uses the prepared private inputs and
runs the sanitized HOST fixture in separate writer and reader processes.
The observed custom game payloads have the same length as the fixtures'
predecessor records:

| Profile | Predecessor tag | Silo tag | Bytes |
| --- | --- | --- | ---: |
| Terran | `76435442` | `764b5442` | 15,832 |
| Z10 | `56435a61` | `564b5a61` | 16,312 |
| P10 | `7645505f` | `764b505f` | 16,308 |
| Showcase | `76435058` | `764b5058` | 14,204 |

The fixtures restore concurrent queued and neutral armed missiles, resume
the training clock, refund an unfinished missile, reconnect a detached
Silo, and preserve SRAM during all three unsaved strike phases. They also
verify that the checked journal falls back to the previous good record
when the newest has the wrong inner generation, legacy outer tag, or a
foreign payer on a queued Silo. P10 preserves an active Forge job and an
opponent's completed research level. Existing P10 and Z10 campaign SRAM
from earlier cartridge qualification also loaded in the new HOST readers.
The Terran T8 fixture uses the imported campaign roster, lands and captures
a neutral Silo, and resumes a paid missile and the campaign clock in a fresh
reader. This probe is not enabled in normal ROM targets.
