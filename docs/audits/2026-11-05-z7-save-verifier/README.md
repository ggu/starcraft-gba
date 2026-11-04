# Z7 replay save profiles

The Z7 replay verifier defaults to its historical `legacy` fixed-length journal.
Current canonical Z7 uses full storage (240 units and 160 buildings), a 16-byte
Transport1 trailer and may include a bounded `IMP1` pending-impact block. Use
`--save-profile transport1` for the pre-cargo transport namespace and
`--save-profile cargo-transport1` for the cargo namespace. The probe's `SAVED`
record reports the inner RaceSave tag and size; the SRAM journal tag and length
include the transport namespace and trailer.

For example, with a retained trace, mGBA log and the SRAM produced by that run:

```sh
python3 tools/sc64_z7_arm_replay.py compare TRACE LOG --sav SRAM --save-profile cargo-transport1
```

Prefix and resume use the same option; resume also needs its original prefix log,
seed SRAM and cut. The verifier checks envelope structure, tags, checksums,
sequence and transport/impact bounds. It does not replace the game's full
semantic save validator. Synthetic test records exercise the verifier only; they
are not guest SRAM qualification evidence.
