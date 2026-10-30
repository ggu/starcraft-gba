# Reaver parent field conversion prototype

The historical engine initializes generic `carry` and `work_timer` to zero
when spawning a Reaver (`spawn_unit` in `starcraft/game.c`). The experimental
parent initializer instead packs an empty queue, no active child, zero cooldown
and zero progress into those same fields. Those facts do not establish that
every historical saved Reaver still has zero in both fields, nor do they prove
the entire saved actor graph is migratable.

`sc64_scarab_parent_convert` is an unregistered, field-only conversion. Its
caller must determine the record format from an authenticated, collision-safe
namespace, not from the fields. It accepts a historical raw-83 row only when
both fields are zero, or a raw-83 row explicitly identified as packed after
validating that packed representation. Raw 85, other identities, unknown
formats and unsupported historical values are rejected without changing the
outputs. No actor or record is modified by the helper itself.

The focused sanitizer fixture tests empty conversion, packed round-trip,
cross-format rejection, raw-85 rejection and unchanged outputs on failure.
It is not an actual historical envelope, whole-save migration, transactional
commit, or evidence of N64 behavior. Before use, the caller must validate all
identities and child links, preserve every other field and tail, convert all
parents in candidate scratch, and preflight the complete candidate before
mutating live state. Namespace registration, historical save fixtures, and
fresh-process continuation remain open.
