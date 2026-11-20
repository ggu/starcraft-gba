# Packed save field reads

The 35-byte ZP unit and 28-byte packed building codecs read fields in
byte slices. They retain the existing least-significant-bit-first encoding,
field limits, padding checks, zero-row treatment and atomic decode. The
reader validates the entire span before reading and touches only its
covering bytes. Journal selection, candidate validators, encode and commit
order remain the same.

Z10 uses both readers. Terran uses the packed building reader. P10 and
Showcase use the P7 typed compact unit and building codecs, and their
candidate ROMs are byte-identical to their controls.

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
