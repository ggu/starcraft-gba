# P10 cursor query cost after packed sight

Sixteen matched, ordinary packed-sight P10 updates average 1,713,563
guest cycles. The renderer accounts for 768,097; its explicit flip wait
takes 25 cycles on each sampled update, and `gfx_end` takes 93. The
samples exclude trigger sweeps and depot stalls, and do not estimate
display waits on other workloads.

The render-only cursor action query takes 61,459 cycles on average
(3.59% of the whole update), rising to 87,863 in the five late samples.
Its busiest direct child is the hostile unit/building scan
(26,660 cycles, or 42,730 late); it evaluates the mission hostility
policy for every eligible actor before checking cursor distance.
The selection scan and mineral helper add 14,434 and 11,597 cycles
under this query. The worker scan adds 7,504 directly, and the
mineral helper repeats worker/selection work. These are nested scopes
and must not be summed twice. The larger `draw_unit_pass` takes 156,085,
but spans culling, sprite lookup, several blitters and drawing; the
cursor path is a more direct opportunity with a tighter equivalence test.

The optional `SC64_FIELD_CURSOR_QUERY_PROBE` filters nearby geometry
before expensive owner or selection predicates, keeps exact distance
and slot tie order, and reuses the worker and selection results in the
race-profile HUD. It defers selection until needed, preserving the
double-tap, gas, building, attack and mineral priorities. It changes
no render cache, actor lists, saved data or rendering order. Gameplay
uses the same hostile, selection and mineral rules.

The asset-free ASan/UBSan differential compares the cursor outcomes
over 1,200 deterministic worlds and the full framebuffer for 60 of
them. The established field UI regression also passes with the option
on. A prepared P10 flag-vector host pair matches query results with
the option on and off. No cartridge was compiled or linked, so no
cycle savings or ARM pixel equivalence are established yet.
