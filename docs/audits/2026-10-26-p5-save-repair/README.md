# P5 save payload repair

Current-source qualification of the staged P5 Shuttle profile found that its
fifth normal-menu mission, global ID 24, could not save after its source
triggers started. The first four entries passed their round trips. The same
failure occurred with both the staged flags and the current canonical P4
renderer options, so it was not caused by the rendering changes.

The P5 save block copied the live mission payload and then cleared it. Its
preprocessor guard enclosed the `else if` opening but not the clear itself.
With P6/P7 disabled, the clear remained in the active P5 branch. The candidate
validator correctly rejected the resulting mismatch between objective stage
and trigger state before writing SRAM.

The active P5 branch now closes unconditionally. Only the inactive shared-
union clear is conditional on the P6/P7 union users. P5-only builds already
zero the entire candidate before packing, so inactive bytes remain canonical
without erasing the copied payload. No validation is relaxed, and no save
layout, generation, pool limit or mission rule changes.

The profile regression now checks nonzero live trigger state against the
serialized payload, including an in-flight action cursor and its post-load
value. Focused local checks pass for:

- P5 Shuttle runtime, all five normal-menu mission save/load round trips,
  234 skirmish startup/save configurations and framebuffer coverage.
- The P6 profile's shared-payload saves and 234 skirmish configurations.
- P7's immediate-save/first-tick equivalence fixture.
- The P5 profile under the current canonical P4 rendering flags, including
  the same five mission round trips and 234 skirmish configurations.

The current P5 normal-menu host controller also reaches victory at host tick
5,561 / game frame 5,496. This is separate from profile setup/save coverage.
Private logs, maps, sprite headers and controller traces stay ignored.

This repair does not promote P5 to the canonical Protoss ROM. A promotion
still needs current-source ARM playthrough, fresh-process SRAM continuity,
carried-mission qualification and performance evidence. Canonical coverage
remains T1–T10, Z1–Z4 and P1–P4 (18/30); the 30-fps goal remains unfinished.
