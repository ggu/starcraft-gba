# Save-validation diagnostic

At source `3e67e0e1`, a probe-only `SC64_DIAGNOSTIC_SAVE_VALIDATE` hook measured
semantic validation during a genuine Z7 HOME load. The normal probe first
matched the previously qualified ROM byte-for-byte:
`6e191f20f5b6acf0e6f2fe7f207ff93d633d91fb420c362bc78775f0b2861686`.
All 1,800 prepared input hashes stayed unchanged and both linker guards passed.

The genuine prefix seed was
`182407d91e782c4e879e65047223a70a0ec5f83bbec0131d4e205debddbd0d39`.
The selector inspected its valid slot 0 and accepted its callback. Within that
callback, the complete mission preflight took 50,263,869 guest cycles. After
selection reloaded the winner, its direct preflight took another 50,264,771.
These two non-overlapping preflights total roughly 5.99 emulated seconds;
the full instrumented load cost 119,306,055 cycles (7.11 seconds). The generic
callback scope (50,299,233 cycles) contains the first preflight and must not be
added to it. Code placement and diagnostic writes affect these measurements.

The diagnostic ROM hash was
`0c4f69e14d5a3b66a0ef43b0a7da478937ab4ac005dd0ed7536e6922d9fd7fa5`.
Its flags were `-DZ7_PEAK_DIAGNOSTIC -DZ7_PEAK_FRAME_LIMIT=1
-DSC64_DIAGNOSTIC_SAVE_VALIDATE`. It loaded through ordinary HOME input and
stopped at `Z7D STOP`; this load-only observation does not qualify a mission
completion, isolate every load cost, or establish physical-hardware timings.
No save-selection or validation semantics have been optimized in this commit.

Layer 0 of each `Z7S VALIDATE` row wraps the generic selector callback. Layer 1
times the complete cargo candidate preflight, including the direct call after
the winning raw bytes are reloaded. Normal products omit the diagnostic hooks.
The local logs, raw journal, input manifest and receipt remain private.
