# Standalone Protoss P6 impact saves

The standalone P6 cartridge has a 16,348-byte legacy save payload and a
16,368-byte SRAM payload limit. Its 20-byte tail holds the shared eight-byte
impact header and two six-byte rows. The shared journal requires room for
at least eight rows.

P6 keeps the historical layout for saves with zero, one or two pending impacts.
For three to 22, a four-byte `P6I1` suffix identifies the same impact packet
stored in seven reserved bits per building row. The 160 building rows supply
140 bytes for that packet. Loading validates the packet and clears the
reserved bits before ordinary world validation. Gameplay uses the shared
32-row queue. With 23 or more pending impacts, saving reports `SAVE FAILED`
and leaves the previous SRAM record intact. [Impact save format](../IMPACT-EVENTS.md#save-and-load)

## Local qualification

The normal `verify-protoss-p6` target passed on `964ef269` with frozen
prepared inputs. The standalone profile uses the existing ROM placement
for the P4 validator. The cartridge ends IWRAM at `0x03004e88` and EWRAM
at `0x02036ca8`, leaving 376 and 29,528 bytes before the respective link
guards. The ROM SHA-256 is
`a3976eef37f78daee282f2a25a1d4e51c82f3d05d03b514096930a6f0000bab4`.

The P6 ASan/UBSan save fixture passed the legacy lengths, 22-row restore,
23-row refusal, malformed marker, and nonzero unused-bit cases. The shared
impact contract fixture also passed.

The full `test-protoss-p6` target still fails at its controller. The current
delayed-impact build and an immediate-damage diagnostic produce the same
4,347-event trace ending in a loss at frame 53,613. The trace SHA-256 is
`c75ab9d2aec1b1d7e2e0d170dde934d783ede5ab4891388d1a1850a74482b71d`.
The same loss was recorded on the earlier composition profile. [Earlier controller audit](2026-11-03-p4-p6-controllers/README.md)
