# The Big Push from HOME on the normal Terran cartridge

The exact November 13 KODAK Terran ROM (`cbba1c5`) boots fresh at HOME,
selects The Big Push through the ordinary campaign picker, and reads nine
briefing pages. It then replays the retained probe-entry HOST tape using
physical buttons only. Seven authored gameplay pages are acknowledged
without advancing simulation. Guest observations are read through the
matching ELF; no guest memory is written and no SRAM is supplied.

All 272 retained observations matched. They include the Command Center
relocation, first combat contact, paid production, supply growth, and
attack orders. At tape call 143,260 (physical call 143,321, game frame
139,216) the mission won: no hostile buildings, the hero alive, completion
bit 11 set and zero trigger faults. The host tape has a single verified
route; this does not establish other routes or fresh SRAM continuation.

Mean main-loop cost was 842,000 guest cycles (19.93 emulated updates/s),
with a peak of 22,546,637 cycles. Those include rendering, interface and
display waits. No physical GBA timing was measured.

| Pinned input or receipt | SHA-256 |
| --- | --- |
| Terran ROM | `0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8` |
| Matching ELF | `00951a012354bf97ccd3d81682f8e4e7d2321381195c589b0d2d705b1abb8ab8` |
| Physical observer | `4f65560119fa9e29df216f24efce6da417572304e2c39f02447023159435a549` |
| HOST tape | `5aee3cbc5125c79ec99414386d6a49122aff0e5fbe465e968b905aaf2f62d182` |
| Physical replay harness | `52dd306b60fd14ebc08904c8dcb447a4ed8208ae14f4d85af743f539b42a35a3` |
| Full replay receipt | `89b89813b394f4b813f0c33d1bd3b9526396cc4841e9572b7e888bb72ea2cc9d` |
