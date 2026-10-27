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
bytes. It follows `goto` targets through every reached region, including
shared code, and validates each reached region's further branches. It does not
equate the next catalog entry with a semantic program end. The tool writes
decoded program metadata only under `.local/`; that output is private program
data, not a redistributable asset. The ROM leaf and instruction bytes are never
saved.

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

Numeric type, upgrade, and tech IDs were cross-checked against the pinned BWAPI
headers at `d727fed68558c506163048ea889131d8cbb33915`; those per-command values
and names are not reproduced here. The crosswalk helps interpret source IDs,
but does not itself prove N64 enum identity.

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

`Zer8` begins with the campaign/town initialization commands, disables
default building, and sets its default minimum. It has five build requests,
several ground/air defense build/use categories, a force-size setting, and
eight upgrade requests. Its repeating body assembles and executes ten attack
groups, with a `goto` back into the script body. The exact unit IDs, request
arguments, group compositions, and wait operands are retained only in the
ignored metadata report.

`Ze8T` has the same broad initialization categories, six build requests,
ground/air defense build/use categories, upgrade and technology requests, and
four attack-group executions before its `goto` loop. The exact unit IDs,
request arguments, group compositions, and wait operands are retained only in
the ignored metadata report.

The trigger descriptor associates the two longer scripts with distinct
trigger/player/location rows, and uses `MvTe` in six more rows. Their source
conditions are preserved in the private descriptor; this note does not
reinterpret the source player indices or condition IDs.

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
