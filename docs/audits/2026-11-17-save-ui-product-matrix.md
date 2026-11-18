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

The November 17 fixed T1 Wasteland tape wins on the regular Terran
ROM at frame 10,120. On the opt-in image it previously reported 427
minerals instead of 377 at call 7,000. Both runs had already saved,
restarted and loaded through pause-menu L at zero-based tape tick 6,235.
The first audit overlooked this round trip. The fixed tape stopped at
the next differing named checkpoint, so it did not establish a
candidate victory or a changed game state at load.

On November 18 the regular host controller regenerated the historical
tape byte for byte. Rebuilding the controller with `SC64_SAVE_UI_CONTEXT`
made it choose Barracks after one Down press instead of two, because
the menu reopens on row 1. Both sanitized controllers win at game frame
10,120 with 10 Marines, five SCVs, 417 minerals and the same terminal
host state hash. The input counts differ by two, the extra Down press
and release. The paired ROMs were relinked from the same prepared
inputs and compiler vector, with only the UI-context flag toggled;
they are different binaries from the archived November 17 pair.

The [normal T1 replay driver](../../tools/sc64_t1_normal_replay.py)
independently verifies the ordinary save/restart/load using a fresh
boot, checks the restored semantic world, and records the saved and
loaded menu fields. The rebuilt regular ROM wins with all 11 named
checkpoints, nine briefing pages and eight in-field dialogue pages
observed. Its pause load preserves the world and resets the remembered
row from 1 to 0. The optional build still needs this ARM check against
its newly generated input tape before default enablement.

| Local evidence (SHA-256) | Digest |
| --- | --- |
| Terran Blood Bath first paired run | `67d8363eb9b3c03d4b0a24fa55ad4415ee69e2f89442ad58bef0870f62f3a106` |
| Other seven paired runs | `5e2311d410fa3003a48a3b1c1cc47e9e38d40f89737805b376cbca2ce7afb2ad` |
| Public-observer Showcase Blood Bath receipt | `b61c043efa28b3fbf5acc229a054bb166555229a1e95ddd34fd7a8d40b4c823c` |
| Opt-in Z1 campaign win receipt | `983527acfa151b01cd1697d804cc38f6e225b08eed900612acd77a07dbb41f63` |
| P3 corrupt-newer-bank fallback receipt | `0fc8a69a44cbd91568be0d8e6c9cc97951f5950894b013d6c91cedb7549faff5` |
| Regular T1 HOME win receipt | `d2b779185231c3dc0342bda474be094a094f484b893a0632ac50c0270f8205b9` |
| Opt-in T1 first divergence | `751738c2e63fdec33c925a7ddab7a7a17e9453c0e3b1b7b84e44b89ba416c58e` |
| Rebuilt regular T1 host tape | `5b6fc3b6af2ad1206f587f35de4587d26ed567a1e98e93d487b4ec758a29ee38` |
| Rebuilt opt-in T1 host tape | `27b78c4bcd6ee7076126a224c4a6fe362190fb7105631b446e19bde5689fc9ba` |
| Rebuilt physical observer | `724715d35416c4d202041fc0585fa4f1542f1b4d8ed6b53a6db373c9d8caa50d` |
| Rebuilt regular T1 ARM receipt | `541641b89e18d70843c4286aa503104f7c346536fa6bbade3d84761787385220` |
