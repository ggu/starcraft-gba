# Z8 native AI program evidence

This note records the bounded static decode for the three AI script IDs used
by the Z8 scenario. It does not claim that the GBA runtime implements those
scripts or that every N64-side AI effect has been reconstructed.

## Provenance and method

The source is the owned StarCraft 64 USA ROM with normalized SHA-1
`a36d91b5aac7bb95df0d7de5f9814f5270e25c2b`. The existing pinned extractor
`references/sc64-maps/extract_sc64_maps.py` at
`09b57f787a247107d1de82b8c5eab2734667ea1d` reads BOLT leaf `012/000`; its
payload is 59,610 bytes with SHA-256
`96fd2408033e3c8ba8900fc719b1e90f9e828b932d382f51fe5b16cdff1af054`. Catalog
rows are bounded by the authenticated table header and terminator.

`tools/sc64_ai_programs.py` decodes only opcodes present in `Zer8`, `Ze8T`,
and `MvTe`. Opcode names and operand widths were cross-checked against the
pinned PyMS `AIBIN.py` revision
`bfc5d3aad0b5614a5aff72c223f8efa00afddfa4`. The decoder rejects unknown
opcodes, truncated arguments, out-of-leaf branches, and branches into operand
bytes. It allows an explicit `goto` to target an instruction in a later
script region; it does not equate the next catalog entry with a semantic
program end. The tool writes numeric command metadata only under `.local/`;
it does not save the ROM leaf or emit instruction bytes.

The report was generated with:

```sh
python3 tools/sc64_ai_programs.py \
  --rom /path/to/StarCraft64-USA.z64 \
  --extractor /path/to/references/sc64-maps/extract_sc64_maps.py \
  --out .local/build/z8-native-ai-policies/authenticated-report.json
```

The Z8 source descriptor at
`.local/build/z8-source-foundation-20261027/descriptor.json` has fingerprint
`f80b5f9e9419c324a6b86b9a11bbe3dab0ec545d20817fb0f26509e82c85a986`; it
contains the scenario's `RunAIScriptAtLocation` actions. Raw source fields are
preserved below where their semantics are not established.

The UnitType, UpgradeType, and TechType names below use the pinned BWAPI
headers at `d727fed68558c506163048ea889131d8cbb33915`. That crosswalk helps
interpret the source numeric IDs, but does not itself prove N64 enum identity.

## Decoded program facts

The three records are structurally valid under the referenced command layout:

| Script | Code offset | Bounded end | Decoded operations | Explicit `goto` |
| --- | ---: | ---: | ---: | ---: |
| `Zer8` | 17,824 | 18,110 | 108 | 18,015 |
| `Ze8T` | 17,636 | 17,824 | 65 | 17,769 |
| `MvTe` | 28,804 | 28,806 | 2 | none |

Both longer programs' only explicit jump targets are instruction boundaries
inside their own decoded regions. `MvTe` consists of the named `move_dt`
opcode followed by `stop`; that command has no encoded operands. Its attached
trigger action has source location index 9. The relationship between that
location and the N64 `move_dt` implementation has not been runtime traced.

`Zer8` begins with `start_campaign`, `wait(1)`, `start_town`,
`defaultbuild_off`, `default_min(0)`, and another `wait(1)`. Its five `build`
triples are `(1,154,150)`, `(1,64,130)`, `(4,64,80)`, `(1,160,80)`, and
`(1,167,80)`. The pinned BWAPI `UnitType.h` enum maps these type IDs to Nexus,
Probe, Probe, Gateway, and Stargate. `build(count, unit, priority)` is also the
syntax documented by PyMS's AI Basics tutorial; the numeric operands remain
the ground truth in the extracted report.

`Zer8` contains ground/air defense build/use pairs for type IDs 65, 83, 66,
and 70, followed by `max_force(1300)`. BWAPI maps those IDs to Zealot, Reaver,
Dragoon, and Scout. It then requests eight upgrades with operand triples
`(1,37,0)`, `(1,15,0)`, `(1,33,0)`, `(1,35,0)`, `(1,5,0)`, `(1,6,0)`,
`(1,13,0)`, `(1,14,0)`, each followed by `defenseclear_aa`. Later it repeatedly
builds attack groups from these decoded counts: 2 Zealots; 1 Scout; 4
Dragoons; 2 Scouts; 8 Zealots plus 1 Reaver; 2 Zealots plus 2 Dragoons plus 1
Reaver; 4 Scouts; 4 Dragoons plus 4 Zealots plus 2 Reavers; 3 Scouts; and 1
Reaver plus 4 Dragoons plus 1 Scout. These sequences use `attack_prepare`,
`attack_do`, and `attack_clear`; the program contains 24 `wait` instructions,
including encoded operands 500, 1,500, 6,000, 7,500, and 9,000. Its final
`goto` returns to offset 18,015. These are source command sequences and
operand values; completion or exact delay units on the N64 have not been
observed.

`Ze8T` has the same initial campaign/town setup. Its six `build` triples are
`(1,154,150)`, `(1,64,130)`, `(4,64,80)`, `(1,165,80)`, `(1,160,80)`, and
`(1,155,80)`, mapping to Nexus, Probe, Probe, Templar Archives, Gateway, and
Robotics Facility. Its defense build/use requests cover Zealot, Archon, Reaver,
and Dragoon. Upgrade/technology operands include `(1,15,0)`, `(1,5,0)`,
`(20,30)`, `(1,13,0)`, `(1,40,0)`, `(1,35,0)`, and `(19,30)`. BWAPI maps tech
IDs 20 and 19 to Hallucination and Psionic Storm; upgrade IDs 15, 5, 13, 40,
and 35 to Protoss Plasma Shields, Protoss Ground Armor, Protoss Ground
Weapons, Khaydarin Amulet, and Scarab Damage. Its four attack groups are one
Archon, two Archons, one Archon, and one Archon; the script uses wait operands
1, 500, 4,500, 6,000, 7,500, 18,000, and 19,500, then loops to offset 17,769.

The trigger descriptor associates `Zer8` with trigger 8, source location 4,
and enabled player slot 20; `Ze8T` with trigger 9, location 5, and slot 19.
`MvTe` is used by triggers 10, 11, 12, 19, 20, and 21 at locations 6, 7, and
9, all enabled for slot 19. Their associated conditions have source opcodes
and raw fields retained in the descriptor; this note does not assign an
unverified meaning to the source player indices or condition IDs.

## Limits and implementation consequence

This is a static decode of an AIScript-compatible data layout, not proof that
the N64 interpreter consumes these bytes with the PC interpreter's exact
semantics. In particular, command processing, attack group lifecycle,
request priority/resource spending, wait tick duration, script scheduling,
and `move_dt` location behavior still need native call-path or execution
evidence before they can be implemented as faithful game behavior.

The pinned OpenBW source does not close that gap: its scenario AI-script
action handler only implements the shared-vision `Kbi0` through `Kbi7` IDs and
reports other AI-script IDs as unsupported. It cannot establish `Zer8`,
`Ze8T`, or `MvTe` semantics. The Z8 engine callback must therefore remain
explicitly unsupported until those remaining runtime semantics are modeled;
these facts do not make Z8 playable or completable.
