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

The Z10 candidate also wins Z1 from HOME at frame 50,669. All 51
observations and all ten authored pages matched the archived route. The
Chrysalis survived and the full replay averaged 23.71 emulated updates/s.
In P3, a valid two-bank save selected its latest victory checkpoint, at
frame 45,951. Changing only the newest bank's UI tag and recomputing its
checksum fell back to the genuine older checkpoint at frame 23,993 with
menu row 1 preserved. A newer checksum alone does not publish malformed
UI context over a valid older bank.

Do not enable the setting yet. The archived T1 Wasteland input route
still wins on the current regular Terran ROM at frame 10,120. On the
opt-in Terran image the same route matches through call 6,000, then has
427 minerals instead of 377 at call 7,000, with frame, actor counts and
the captured objective fields unchanged. Neither leg has loaded a save
at that point. The candidate replay stops at the first named difference;
the test does not establish a cause or a candidate victory. Isolate the
first command, queue or resource difference before enabling the setting.

| Local evidence (SHA-256) | Digest |
| --- | --- |
| Terran Blood Bath first paired run | `67d8363eb9b3c03d4b0a24fa55ad4415ee69e2f89442ad58bef0870f62f3a106` |
| Other seven paired runs | `5e2311d410fa3003a48a3b1c1cc47e9e38d40f89737805b376cbca2ce7afb2ad` |
| Public-observer Showcase Blood Bath receipt | `b61c043efa28b3fbf5acc229a054bb166555229a1e95ddd34fd7a8d40b4c823c` |
| Opt-in Z1 campaign win receipt | `983527acfa151b01cd1697d804cc38f6e225b08eed900612acd77a07dbb41f63` |
| P3 corrupt-newer-bank fallback receipt | `0fc8a69a44cbd91568be0d8e6c9cc97951f5950894b013d6c91cedb7549faff5` |
| Regular T1 HOME win receipt | `d2b779185231c3dc0342bda474be094a094f484b893a0632ac50c0270f8205b9` |
| Opt-in T1 first divergence | `751738c2e63fdec33c925a7ddab7a7a17e9453c0e3b1b7b84e44b89ba416c58e` |
