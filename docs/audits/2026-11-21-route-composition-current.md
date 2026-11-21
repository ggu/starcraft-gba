# Current-source route composition: host and Z7 qualification

This opt-in composition combines the cold disconnected-component bypass
`6f836320`, warm blocked-goal gap retry `ec43748f`, and small-component fill
`c59e0963` with source release `5516dffa`. The tested composition is
`e91f735358c37d6ea867b5987a702935d14b8288`. No production flags were enabled.

`make test-sc64-route-composed` runs all three flags together under address and
undefined-behavior sanitizers. Its six route checks pass, including exact
384-cell and rejected 385-cell components, blocked roots, stripe escapes,
occupied reserves, stale owners, and canonical route comparisons. The
flags-off default ROM and a 321,445-query route trace match `5516dffa` byte for
byte. `python3 tools/check_source_tree.py`, `./dev doctor`, `./dev build`,
`./dev test`, and `make test-sc64-route-keep-fields` pass. The host suite's
exit status was observed directly; its complete output was not retained.

The current-source Z10 link uses the existing pinned input and toolchain vector.
It passes ROM and payload verification and RAM guards. The normal #671 ROM is
`ad2709458c154142fb776a096d42a444baffe98455832bc0e647a377633bb80a`; the
composed ROM is
`989eb1911da12ea69020c0410d5392b9f84e8c611e27808903c506518083b73c`.
The normal and composed IWRAM endpoints are `0x03004d60` and `0x03004ea8`;
EWRAM endpoints are `0x02039b30` and `0x0203a068`, respectively.

Both paired ordinary Z7 runs start from blank HOME, pause at frame 5,641,
restore in a fresh process, follow the same controller through call 231,031,
and win at frame 228,128. The pause and victory journals and fresh victory
readback match byte for byte. All 254 sampled world observations, six logical
cache cuts, 75 complete fields and 4,201 published partial directions match;
the latter use an independent BFS oracle. Nine of ten paired display cuts
match. At call 154,741, camera, palette and both framebuffer page hashes match,
but normal selects page 0 and composed selects page 1; the pages differ. This
is an observed display difference, not a demonstrated route or world-state
mismatch. Its cause and visual duration are unproven. Do not claim full display
equivalence or enable the flags based on this experiment alone.

| Window | Advancing calls | Normal mean / p95 / peak cycles | Composed mean / p95 / peak cycles |
| --- | ---: | ---: | ---: |
| Dense | 477 | 2,193,387 / 3,181,266 / 5,120,617 | 1,910,744 / 2,580,213 / 4,482,609 |
| Late peak | 926 | 2,449,609 / 3,733,549 / 5,811,026 | 2,355,215 / 3,330,589 / 5,244,636 |
| Late following | 905 | 2,381,249 / 3,624,506 / 5,365,242 | 2,280,739 / 3,183,649 / 4,498,748 |

These are emulator guest-cycle samples, not hardware performance. Other
campaigns, production build modes, full display equivalence and physical GBA
qualification remain open.

| Local evidence | SHA-256 |
| --- | --- |
| Normal Z7 result | `a7fc7bcbecb1ef79cdcd1990cdcd2d4bdef721215a5ef22aec9a87e7f2e67328` |
| Composed Z7 result | `648119ceaede4cfd9d110325ebb8b7ce14d59c5d4aa6efc1eee1162ff022d0fd` |
| Paired comparison including display mismatch | `8a06eedd3b8b264491dc0e757fe8ad6e8429101f1a8b7e6cd2b806d21885d392` |
| Pause journal | `4eaecbb90f5e923d73eb97a68012f5fb653d8c751fba7dc1e4d9d80ae54ca3ce` |
| Victory journal and fresh re-export | `1acce341f61b2a1e37cadc1e0b304dd63b5b31352789d128fa2d7bf20d6eb627` |
