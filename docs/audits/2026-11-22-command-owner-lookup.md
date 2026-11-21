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
The prepared sanitizer census checks pass on both. Repeat the
campaign and Custom Game entry-point matrix before promoting.
Rebuilding all four trial profiles on release `7c463fab` reproduces
the tested binaries. Both sanitizer checks also pass with the
raw-type memo disabled.

| Local evidence | SHA-256 |
| --- | --- |
| Full P10 trial | `b5ba3e4dca06ba2d3eededfafaf873b3d1e8a5951e7312618ba15f75e07f6a0e` |
| Z8 continuation | `6b19cf44c919f254090e0ebf0d43a0bd7f9d395bb310ac8c43d0f78559e5899c` |
| Z10 fresh campaign continuation | `8eff8d7c7b24f7596c97b416cb3083b42e274c7e7dc6b2412ab3920965d3bd36` |
| Busy Blood Bath comparison | `81d4d3f0b7decb0842fb7498d3b35176c1147d25facc4b276b15ebe461ff5ff1` |
