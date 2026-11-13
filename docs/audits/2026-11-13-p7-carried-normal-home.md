# P7 from HOME on normal Protoss P10

Homeland (P7) won on both the staged `c906058` and November 13 KODAK
`cbba1c5` normal Protoss P10 ROMs.
The replay selected mission 26 from fresh HOME, read all 11 briefing
pages, pressed START, and resumed the retained carried-P7 host tape at
zero-based update index 11, its first neutral gameplay update. The
replay acknowledged ten in-game dialogue pages through physical
press/release pairs while verifying that each pair left the game
frame unchanged. It neither injected SRAM nor wrote guest state.

The carried P8-flags host tape ended in a strict win at frame 55,892.
Both P10 runs reached the same frame on physical call 57,678
after replaying through tape index 57,615. The two trailing host calls
after the result were omitted. Every replayed key transition matched
the tape's post-update frame. The 59 selected snapshots were identical
across ROMs and tracked mission
identity, outcome and trigger fields, authored heroes, and original
Nexuses; those readbacks do not establish whole-world equality.

The game reported WIN with no defeat or runtime fault. All three
authored heroes (raw IDs 75, 78, 79) survived. Both hostile original
Nexuses (slots 34 and 74, source owner 0) were destroyed; the
source-owner-4 Nexus in slot 109 and neutral Nexus in slot 6 survived.
The victory switch was set, the victory cursor reset, and action timers
were clear. This verifies one ordinary route on existing binaries; it
does not qualify a save continuation or physical hardware performance.

| Input or receipt | SHA-256 |
| --- | --- |
| Carried P7 held-key tape | `d55d3a80681bb0186ec2971718c949937f25ceea6435ff33c14449b1fc06b775` |
| Port-free observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| Full replay harness | `8f60e3cee59a40428ca965d21307fc0308bcab2204e0cbc70b6545fa0a2db701` |
| P7 DWARF decoder and snapshot reader | `0618b79528455a7769e41c86d2d5245ed74686a0df0b7dfc5f2538d034e5c35b` |
| Staged P10 ROM | `b24184ee2b10b8c6f64153672a25d79a39ef6dc098b8c4cc7582ec8016538474` |
| Matching staged ELF | `863a74f69e2ada72129875a6e5558c526b98a60dd5e59685e4f5d97cab992e78` |
| Staged physical receipt | `57c023d073c674785bd00da2267415acd56b0071308b74a61df839619b9c416f` |
| KODAK P10 ROM | `7a8aa4da8bf156993db753b18a2fc2e02ad6bd880a58f014b6593b983a781310` |
| Matching KODAK ELF | `667bf7f4c435916c58c7715892485c9533fce4f9830011387bfbd8b6f8a3f73d` |
| KODAK physical receipt | `8ff96db01eaf51181cbc8116cef0985564cbddfaadfe51e10be48fae18465ed3` |
