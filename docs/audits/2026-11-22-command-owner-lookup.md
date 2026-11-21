# Query-local Command owner lookup

The direct census resolves each source owner's selector result once per
query. A live row still validates its raw type, actor kind, owner and
coordinates before using that result. An inactive or missing owner
does not surface a selector error. The tests include a malformed force
value on an unused owner, then on the same owner with a live unit both
inside and outside the active player mask.

On top of the persistent raw-type memo, the prepared P10 full saved
tail kept all 68 world, display and fog cuts, all 9,489 frame and route
rows, and victory SRAM. Mean main-loop guest cycles fell from 1,554,111
to 1,549,430. Mean game-update cycles fell from 880,116 to 875,408.
Emulator stack and physical GBA timing still need separate checks.

Z8's 1,200-update saved continuation matched all observed state,
frame and route rows, and SRAM. Mean guest cycles fell from 908,600
to 907,047. The ordinary Z10 pause, fresh continuation and victory
readback match the persistent-cache build. The 3,600-update busy
Blood Bath replay matches both sampled pages, world, continuation,
SRAM and all timing counters.

The normal P10 and Z10 ARM links pass ROM and static RAM guards.
The prepared sanitizer census checks pass on both. Both sanitizer checks also pass with the raw-type memo disabled.
Rebuilding all four trial profiles on release `7c463fab` reproduces
the tested binaries and keeps the RAM endpoints. Terran and Showcase
are byte-identical to that release.

The three race builds pass 378 Custom Game launches each, covering
both maps, all human races and every one-to-three-CPU seat configuration.
All 54 sampled pause and fresh HOME continuations pass. Campaign and
Showcase openings match at frames 61 and 121, and all 164 Custom Game
camera cuts match. The ordinary Z7 replay matches all 254 world
checkpoints, ten display cuts and six route-cache cuts over 231,031
controller calls. Its pause, victory and third-process readback SRAM
match the release byte for byte. The local source-boundary, doctor,
default build and host suite pass.

| Local evidence | SHA-256 |
| --- | --- |
| Full P10 trial | `b5ba3e4dca06ba2d3eededfafaf873b3d1e8a5951e7312618ba15f75e07f6a0e` |
| Z8 continuation | `6b19cf44c919f254090e0ebf0d43a0bd7f9d395bb310ac8c43d0f78559e5899c` |
| Z10 fresh campaign continuation | `8eff8d7c7b24f7596c97b416cb3083b42e274c7e7dc6b2412ab3920965d3bd36` |
| Busy Blood Bath comparison | `81d4d3f0b7decb0842fb7498d3b35176c1147d25facc4b276b15ebe461ff5ff1` |
| Z7 pause, fresh replay and victory | `b244dd8951b3aab46507c1b98ca6fe664118e08dd39ee2aae9d5af2a292e35ec` |
| Custom Game launch matrix, Terran | `59c80259608ab7e65ab137590e20f62ed7d6b1d4d0bbd50f27e5ca995c4941ad` |
| Custom Game launch matrix, Z10 | `a70c16261985a76507f88aa78d16ace4496af249667e4108c4c63aabcb37d1ed` |
| Custom Game launch matrix, P10 | `b5ce40163885f174b7eecc6fff206773f1a715db1e80522819dfebc9e36c5691` |
| Custom Game save and fresh HOME matrix | `150238825c33f8839692647b172d98c7d897ff09e36f2ded8ba035c8845a0531` |
