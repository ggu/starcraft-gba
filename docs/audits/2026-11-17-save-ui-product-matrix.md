# Command context save qualification on current cartridges

The candidate changes only `SC64_SAVE_UI_CONTEXT` in the current regular
ROM compile vectors. Terran T10, Zerg Z10, Protoss P10 and Showcase all
link with the same working RAM bounds as their November 17 controls.
No normal build setting changes in this qualification.

Each pair played both writers from HOME using real buttons. After 600
neutral updates the controller chose a non-default command, selected its
town hall, then used START/B at frame 606. Every reader started a fresh
process and used title L. Each exact cartridge therefore read and
continued both its own journal and the other cartridge's journal.

| Product | Outpost: human / CPU seats | Blood Bath: human / CPU seats |
| --- | --- | --- |
| Terran T10 | P / `--z` | T / `zpr` |
| Zerg Z10 | T / `zpr` | Z / `t-p` |
| Protoss P10 | Z / `p--` | P / `-tz` |
| Showcase | Z / `ptz` | T / `pz-` |

In all 32 fresh-process loads, the world fields (including named unit and
building state, queues, mineral fields, lobby and resolved starts) matched
at load and after 180 more updates. No input journal changed. Each marked
journal on its enabled reader also restored selected building 0 and
command row 1 with the menu closed. An unmarked journal still opens in
the default UI context. A regular reader accepts a marked journal and
resets the context to its own default. This tests live custom-game
continuation on the two maps and the lineups listed here.

The [public observer](../../tools/sc64_save_ui_context_pair.py) independently
repeated the four Showcase Blood Bath combinations with the same results.
It takes exact ROM, ELF and observer hashes and only drives ordinary
buttons. The archived ROMs, saves and raw trace files stay local.

The Races and Unified optional host vectors also pass the full shared
mechanics fixture with ASan/UBSan and the UI setting enabled. Sixteen
prepared headers matched the input manifest before and after. This run
used the exact compiler vectors with those prepared headers: full Make
dependency traversal still requires a palette remap absent from that
archive. It is not a fresh ARM replay of those older profiles.

| Local evidence (SHA-256) | Digest |
| --- | --- |
| Terran Blood Bath first paired run | `67d8363eb9b3c03d4b0a24fa55ad4415ee69e2f89442ad58bef0870f62f3a106` |
| Other seven paired runs | `5e2311d410fa3003a48a3b1c1cc47e9e38d40f89737805b376cbca2ce7afb2ad` |
| Public-observer Showcase Blood Bath receipt | `b61c043efa28b3fbf5acc229a054bb166555229a1e95ddd34fd7a8d40b4c823c` |
