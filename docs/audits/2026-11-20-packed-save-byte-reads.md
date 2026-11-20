# Packed save field reads

The 35-byte ZP unit and 28-byte packed building codecs read fields in
byte slices. They retain the existing least-significant-bit-first encoding,
field limits, padding checks, zero-row treatment and atomic decode. The
reader validates the entire span before reading and touches only its
covering bytes. Journal selection, candidate validators, encode and commit
order remain the same.

Z10 uses both readers. Terran uses the packed building reader. For this
first change, the P10 and Showcase typed compact codecs remain outside
the changed path. Their candidate ROMs are byte-identical to their controls.

## Cartridge comparison

The control is the `916bb897` runtime from PR #606, SHA-256
`d2633b10d0c890ec1a410d6899b35476f0aaa90787c2a0e4692c9274f034cdaa`.
The Z10 candidate SHA-256 is
`cff00c7f0d2af5bc60658474db376e72977da8e161c1286389c7c3b6d2fd30a3`.
Both use the same compiler arguments, imported data and normal ROM profile.
The Z8 ARM controller tape SHA-256 is
`b3d9f36b4f7a33b70dfbf12fc4437f7c5c42ca1c86876b5610cabf7324d5b4eb`.

The paired runs matched all 46,487 controller calls, 50 gameplay checkpoints,
34 page, palette and visibility samples, and 19 host checkpoints. Each run
exports the pause journal after call 17,121, reopens it with HOME L in a fresh
process at frame 16,659, and wins at frame 45,468. The exported pause and
victory SRAM journals match byte for byte. Both contain 32 KiB.

| Ordinary controller call | Control clock cycles | Candidate clock cycles | Change |
| --- | ---: | ---: | ---: |
| 17,120 pause save | 25,380,687 | 18,722,441 | −26.2% |
| 17,122 fresh HOME L load | 43,043,187 | 30,900,401 | −28.2% |
| 46,487 victory save | 60,095,088 | 39,865,784 | −33.7% |

The fallback check copies the victory journal and writes a noncanonical
`path_timer` into one packed unit in its newer slot. It recomputes the slot
checksum. Both normal cartridges load the newer record from the clean
journal and restore the exact older frame 16,659 from the checksum-valid
edited journal. This covers semantic rejection after checksum acceptance.

The normal Terran and Z10 controls and candidates also match on Blood Bath
with all three independent AI seats occupied. Each ordinary pause save at
frame 361 exports identical SRAM. HOME L restores the named world in both
versions, and a 240-update replay matches at frame 601. The custom smoke
receipt SHA-256 is
`1ae32fdd59054d617dc6272bc418b19697be1d7ca866e15d5f665c5daca03b62`.

The four normal ROMs pass the verifier and keep their control IWRAM and
EWRAM link endpoints. In the Z10 disassembly, the unit decoder plus field
reader uses eight fewer stack bytes and the building decoder plus reader
uses twenty more. Terran's building decoder plus reader uses twelve more.
The save and load entry-point stack frames remain the same in both builds.

## Host tests

The sanitized ZP unit fixture compares the byte reader with the per-bit
algorithm at every field offset and width through 32, including fields that
straddle five bytes and invalid tails. The tests cover the compact building
and typed codecs, legacy row fields, malformed decode without mutation,
prepared journal fallback and atomic predecessor preservation. The normal
Z10 host startup and save fixture passes 234 skirmish configurations.

## Typed building follow-up

P10 and Showcase now use the shared byte reader for their 25-byte typed
building rows. The existing fixed-offset reader continues to decode typed
unit rows. The typed building encoder and the checks for the row marker,
type range, padding and field values retain their existing behavior.

Both candidates use the normal compiler commands and pinned inputs from
the `06353a5d` runtime. They pass the ROM verifier and retain their IWRAM
and EWRAM endpoints. The typed building decoder and reader together use
12 fewer bytes of stack in both ARM builds. The save and load entry frames
are unchanged. The sanitized typed codec, packed codec and prepared journal
fixtures pass.

The Eye of the Storm A/B replay matches 47,266 physical controller calls,
10 briefing pages, 14 modal pages and 20 gameplay checkpoints. Both ROMs
restore the pause world with HOME L at frame 36,761 and finish at frame
46,129 with zero faults. Their 32 KiB pause, victory and victory readback
journals match the archived release journals byte for byte.

| P10 event | Control clock cycles | Candidate clock cycles | Change |
| --- | ---: | ---: | ---: |
| Pause B save at call 37,703 | 17,138,906 | 16,667,429 | −2.8% |
| Fresh HOME L from pause | 34,061,620 | 31,257,728 | −8.2% |
| Victory autosave at call 47,266 | 36,545,515 | 33,694,918 | −7.8% |
| Fresh HOME L from victory | 36,269,856 | 33,466,129 | −7.7% |

Clearing the typed building marker in the newer victory entry and
recomputing its checksum makes both normal P10 ROMs restore the same older
pause world. The unedited journal restores the newer victory on both.

Blood Bath with three occupied AI seats also produces byte-identical
pause SRAM and matching worlds 240 updates after fresh HOME L on both
P10 and Showcase. The respective save and load clock reductions are
1.1% and 1.5% for P10 and 0.6% and 1.5% for Showcase.

The P10 replay comparison receipt SHA-256 is
`a63416f8c98c75d49ad4990059f224b747ad2a39839b5dc4807495363e8b2625`.
The three-AI comparison receipt SHA-256 is
`9b86e75d2cc488fd196bb30fd4e7b920f8bf54c8f6d66df36681048fe78c266e`.

### Replay with the stationary-effect depth fix

Paired normal cartridges from release `f6869829` confirm all 47,266
controller calls and 20 gameplay checkpoints, including the fresh HOME L
load at frame 36,761 and victory at frame 46,129. Both builds reproduce
all three archived SRAM images byte for byte. Both also recover the
older pause entry from the checksum-valid journal with an invalid
typed-building marker in the newer entry.

| P10 event | Release cycles | Candidate cycles | Change |
| --- | ---: | ---: | ---: |
| Pause B save at call 37,703 | 17,140,329 | 16,668,832 | −2.8% |
| Fresh HOME L from pause | 34,061,620 | 31,257,728 | −8.2% |
| Victory autosave | 36,543,443 | 33,692,621 | −7.8% |
| Fresh HOME L from victory | 36,269,856 | 33,466,129 | −7.7% |

Mean across the full route stays at 10.52 emulated calls per second
rounded to two decimal places. The gain is concentrated at save and load.
Both current P10 and Showcase builds keep their RAM endpoints. Both match
the release save at frame 361 and the world at frame 601 after a fresh
HOME L in the three-AI Blood Bath comparison. Release `f5ea4cf3` then
updates the Unified HOST fixtures and keeps these two cartridge recipes
unchanged.

The campaign comparison receipt SHA-256 is
`1c1b4b4b5ddb043e04f674368b1905453aef892c8806b777a600e553d7b5538b`.
The three-AI receipt SHA-256 is
`e0ba425c919e495cdee46740636b9829acde9c07612a213e97837c669dedb00f`.
