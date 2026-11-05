# Z7 and Z8 carried replays under Z9 flags

These input-only emulator replays use production source at `0997e2fc` (the Z9
candidate subsequently merged as `01d78012`) and the Z9 canonical Make flags,
including worker cargo and transport. Replay instrumentation and recipes are
separate from game production. The Z7 tape was generated from the retained Z7
controller trace (SHA-256 `42e9a86021b3a16f6f67d2e19f3e475c4c98096799ce1abdc652f9117359696a`);
the Z8 trace SHA-256 is `d704678fdff73287902f0763c19c4cc3e5512906663587ae6a5f400a6057f1fc`.
Private inputs derive from the host qualification snapshot described in
[the host audit](../2026-11-05-z9-carried-host/README.md); its five
noncompiled metadata changes remain disclosed there. ROMs, saves and logs are
private local artifacts. Native execution used the shared lock and 2 GiB guard.

Z7 reached a win at input tick 175455. The full ARM replay matched 307/307
host trace observations. An ordinary menu save at tick 136058 was loaded in a
fresh emulator process and continued to the same win. The explicit Z9 cargo
transport verifier profile checks generation 48's inner `0x52435a60` and outer
`0x56435a60` identities with the existing strict payload checks. Across host
and ARM, respectively, all 308 full, 259 prefix and 50 resume cargo digest
records matched; each matched one unique scalar CHECK kind and tick, including
cut or win. This digest includes worker flags, work timer and build fields;
it is a sampled check rather than an exhaustive world comparison.

The Z7 resumed journal retained the original pause slot and added a winning
sequence 2; the uninterrupted ARM run instead wrote a winning sequence 1.
The committed winning payload SHA-256 was
`5c697b168ef5e3844f3b636256cd4c2d7942b65aab30541993610ae7afff9f09`
in ARM full and both resumed backends. The pause payload was
`e460664b0be20101100ff927086a164acb6c08e3b58b04eb5a103f6c32ca4f82`
in both backends. Committed header and payload bytes matched across backends;
unused filler differs. The host full probe did not export its journal, so no
whole-journal equality is asserted for that run.

Z8 won at tick 40335; the full, menu-save prefix, and fresh-process resume
matched 41/41, 18/18, and 24/24 host/ARM checkpoints respectively, with cargo
digest fields included. The ordinary input tape saved at tick 17374. The ARM
full and resumed 32 KiB journals were byte-identical, SHA-256
`2c8a1151c7214fb00f66bf3c3132bab71f35f7aca1bd6085fde48fea425cc4bb`.
The resumed HOST and ARM committed header/payload records matched; unused
filler differs between backends. Each preserved the original pause slot.

These checks establish the recorded routes and sampled states, not arbitrary
save compatibility, all carried missions on ARM, physical hardware timing, or
N64 gameplay parity. Z9 remains staged rather than a default promotion on
this evidence alone.
