# Parity harness, offline layer (L0)

The offline layer compares the engine with StarCraft 64's own data without
running the N64. It needs the private, authenticated ROM and the pinned BOLT
extractor (`references/sc64-maps`); without them every command prints a skip
line and succeeds. Reports go to `$SC64_PARITY_HOME` (default: `.local/parity`
in the main checkout) through a writer that refuses any path git would track.
Every row is tagged `provenance: rom-table`, and anything the engine lacks is
reported as absent, never skipped.

```sh
make parity-l0
```

`parity-l0` runs two tools:

| Command | Tool | Report |
| --- | --- | --- |
| `make parity-tables` | `tools/sc64_parity_tables.py` | `tables/rom-table-diff.{json,txt}`: units, weapons, flingy, upgrades and techdata cells |
| `make parity-content` | `tools/sc64_parity_content.py` | `content/l0-content.{json,txt}`: opening censuses, iscript sequences, sound ids |

## Opening censuses

For the 30 campaign missions and the Blood Bath skirmish map, the CHK's
placements (UNIT rows, and THG2 rows that place a unit rather than a sprite,
such as installation doors and traps) are joined with the world the release
profile builds at launch, on raw type, raw owner and the source position
halved. The GBA side comes from `--scenario census` of
`tests/sc64_parity_run.c` in the host build of the mission's profile
(`build/parity/{terran,zerg-z10,protoss-p10}-fast-trace`, built from each
ROM's own flags; see [PARITY-TRACE.md](PARITY-TRACE.md)). `parity-content`
builds them in a sub-make with the prepared Z10 descriptor
(`.local/build/z9-inputs/z8-descriptor.json`) unless `Z8_DESCRIPTOR` is given.
A profile whose binary fails to build has its censuses reported absent; the
rest of the report is still written. Before a census binary runs, it is
checked against the build record make wrote beside it (`<binary>.inputs`: the
executable's SHA-256 and every input's SHA-256 at build time) and must sit in
this checkout's `build/` tree; the verified copy is what runs, and its digest
is recorded per mission. A binary that fails the check is never run.
Do not run it in parallel with `parity` (`./dev parity`), which builds the
same Terran and Zerg binaries.

| Verdict | Meaning |
| --- | --- |
| `exact` | every placement has a GBA actor at the halved position, and there are no others |
| `collision` | the only differences are stacked placements (within 8 GBA px) the engine displaced |
| `cap-dropped` | placements are missing while the GBA unit or building table is full |
| `differs` | anything else: moved, missing below the cap, or extra GBA actors |

Minerals (position and amount) and geysers are reported beside the verdict.
Starting banks: SetResources rows whose trigger holds on its first pass from
the CHK alone (Always, ElapsedTime at most N or at least 0-1 s, CountdownTimer
at least 0, Command conditions counted on the placements), evaluated for each
human or computer slot that runs the trigger, are compared with the GBA bank
over the launch and 120 input-free steps: the grant must be the launch value
or the first change of the granted balance. A computer's grant that the
engine-owner bank does not show is reported as not compared, because
campaign computers keep mission-specific wallets per source slot that the
trace does not carry. Blood Bath's CHK holds only start locations and
resources: its census needs one town hall for the human and one for the
computer, each exactly on its own start location (halved), plus the minerals
and geysers.

## Iscript sequences

For each unit in the engine's baked walk, attack, death and construction
banks (`.local/sc64/{walk,attack-poses,death,construction}/generated`; a bank
is used only when its manifest pins the header beside it), the ROM iscript is
walked linearly: playfram, wait (N ticks, as OpenBW runs it) and waitrand,
unconditional goto, ignorerest and end, with the sound opcodes on the way.

- Walking: the steady loop's frames (pose = frame / 17 for directional
  images) and waits against the baked cycle.
- Ground and air attack: per-tick frames of heading 0 of the root image,
  compared up to relabelling because the engine bank numbers its own frames;
  the engine's recovery ticks are compared with Gnd/AirAttkToIdle (up to its
  first goto) as if the order ended with the path. A path that never plays a
  frame is reported as not compared.
- Death: each burst layer's frame set and duration (the death record lists
  frames in index order, so play order is not checked).
- Construction: frames, waits and how the program ends (loop entry or held
  frame) per stage.

ROM animations with no baked table for a unit are listed as not modelled.

A row reads `match` only when everything it names was compared. When part of
it could not be, the row reads `not compared` with the reason: a Walking path
with no steady loop, a death path that never ends or a remnant layer (its
duration is the engine's cap), a construction program whose end the linear
walk cannot follow, or an attack with any tick whose frame the previous
animation left (the ROM does not define it). A manual `--binaries` is checked
against the recorded inputs only; `make parity-content` always rebuilds first,
with make's own dependency tracking. A death burst is
timed from its first framed wait, and a waitrand is a range the engine's value
must fall in. The banks are read from this checkout's `.local/sc64` only
(never the main checkout's: this checkout's builds do not read those). A bank
that is missing, stale or malformed gives every unit whose ROM image carries
its animation an `absent` row, so no unit drops out of the report.

## Sound ids

`tools/sc64_audio_pack.py` writes `sc64_audio_sound_ids.json` beside its bank:
the sfxdata ids behind every bank index and, per raw unit and kind, the ids
the bank plays, pinned to the header's SHA-256. The header, blob and manifest
are unchanged by it. An older bank has no id map; regenerate it with the same
options to get one (the bank itself comes out byte-identical). The pack
stages each file inside the bank folder and renames them into place in order
(blob, header, manifest, then the id map) under a lock in that folder, so two
packers never interleave. The id map pins the header's and the blob's SHA-256
and is renamed last, so it is the generation's commit record: after an
interrupted publish, or for a reader looking mid-publish, its digests do not
match and the report refuses the bank as stale (the manifest also pins the
blob). A ROM build reads the header and the blob separately and does not
check the id map, so do not run the pack while building, and after an
interrupted pack run it again (or check with `--verify-bank`) before building. The report
compares, per unit the ROM voices or the animation report names, and per kind
(ready, yes, what, annoyed, attack, death),
the ROM's ids from units.dat and the iscript opcodes (the unit's attack
animations, its weapons' bullet images, Death and the images it spawns) with
the bank's, after checking the id map against the header's own indices. Ids
whose ROM clip is empty are reported as silent in the ROM. The engine has no
annoyed kind, so those rows read absent.
