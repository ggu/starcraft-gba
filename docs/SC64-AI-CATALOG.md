# Private SC64 AI catalog index

`tools/sc64_ai_catalog.py` reads a user-supplied, already verified StarCraft 64
USA ROM through the pinned BOLT extractor and audits only leaf `012/000`'s
bounded script-catalog envelope. Its JSON output is restricted to ignored
`.local/`; it contains catalog metadata and hashes, never script-program bytes.
The checked-in tests use synthetic rows only.

First prepare the pinned local extractor as described in
[the optional unified setup](DEVELOPMENT.md#optional-unified-profile). The
auditor uses that existing checkout and accepts an uncompressed `.z64`,
`.v64` or `.n64` image; it does not fetch a ROM or install dependencies.

Run it with:

```sh
python3 tools/sc64_ai_catalog.py --rom "/path/to/your/verified StarCraft 64 ROM"
```

The auditor pins the normalized ROM SHA-1, the BOLT leaf SHA-256, and the
extractor revision and source bytes. It bounds the little-endian header offset,
16-byte rows, IDs, offsets and duplicates; the null-ID terminator is four
bytes, so the parser checks the ID before requiring the rest of a row. A zero
code offset is recorded as a zero pointer without asserting how the runtime
resolves it. A nonzero in-range offset is reported only as a pointer fact; the
auditor does not claim it points to executable code.

The candidate table layout is consistent with the PC `AIScript.bin` table
parser in [PyMS `AIBIN.py` at pinned revision `bfc5d3a`](https://github.com/poiuyqwert/PyMS/blob/bfc5d3aad0b5614a5aff72c223f8efa00afddfa4/PyMS/FileFormats/AIBIN.py#L500-L532),
which reads a little-endian table pointer, then 16-byte `<4s3L` rows until a
four-byte zero ID. This is format compatibility evidence only. It does not
establish that the N64 executable uses the same dispatch, program encoding,
or timing; the output marks execution semantics unaudited. This catalog tool
does not decode opcodes or change campaign runtime behavior.
