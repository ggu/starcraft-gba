# P10 optical owner-mask early check trial

This default-off experiment checks an already-built optical-list owner's mask
before coordinate/axis work when every bit in that mask is already visible.
The visible-owner result only accumulates bits by OR, the skipped operations
are reads, and the scalar/no-list path is unchanged. The existing source-owner
mask, collapsed-owner authentication, actor filtering, target order, detector
geometry, update cadence and fallback remain authoritative. No default product
flag, actor cap, route/cache size or VRAM allocation is changed.

The 94bc P10 package ROM `ff94a2ea9f98404c97f44a8b6b7bf557b3572c761de530d867032eeb8f76b10b`
is byte-identical to the current merged c27bb48c P10 production build using
the same 392 package dependencies. It was independently reproduced as the
reference in this b4b0000c-based trial. The guarded normal candidate ROM was
`01347dd4edd6103f7d822f3ff6cdfb405bba6a199363770bcbfa1441c64bd1f4`.
Both linked under unchanged guards with identical BSS/IWRAM ends
`020391e0/03004b20` (link bounds, not stack high-water proof). The 392
package inputs, isolated copies and source hashes matched before and after.

A diagnostic-only eligibility census replayed the same P7 ordinary-button
window under the actual P10 vector. Of 702,037 eligible-live optical-unit-list
visits, 302,833 (43.1%) had no unseen owner bits at the proposed branch; of
445,505 building-list visits, 177,360 (39.8%) did. The diagnostic matched
11 selected-world/fog/displayed indexed framebuffer/palette samples, all
frame/screen/mode transitions, 958 advancing and 42 nonadvancing play calls,
and three ordinary text acknowledgments against the unmodified package. Its
net 0.626% loop overhead means it is a census, not a speed result.

The independent per-source scalar owner-mask oracle passed 2,000 randomized
worlds in both normal and candidate builds, with 1,978 positive masks. The
P10-aware independent Arbiter full-state lifecycle differential also passed;
its complete 40,931,363-byte control/candidate streams both hashed
`288bc2b1a8bc6c901cb7395ab2785677e5babf4180e40142c316373fa89d0a29`.
These fixtures exercise raw owner identity, collapsed owners, malformed
fallback, geometry, list/index lifecycle and slot changes within their tested
scope. A separate older cache fixture failed on the control before candidate
execution because it hardcodes P7 save generation 40 while P10 uses 47; its
failure log is retained, and the P10-aware fixture did not alter that assertion.

A separate normal-ROM physical-button pair measured tape ticks 8020–9019 from
fresh boot. Both had 958 advancing and 42 nonadvancing play calls and three
text acknowledgments. Every timeline transition and 11 sampled selected-world,
raw unit/building/P7-state, fog, full displayed indexed framebuffer and palette
fingerprints matched. The uninstrumented control averaged 2,253,117.469 guest
cycles per advancing call and the candidate 2,238,993.523, a 14,124-cycle
(0.627%) mean reduction. Their respective maxima were 3,955,382 and 3,954,098,
a 0.032% change. This is a nonterminal single P7-input/P10-product emulator
window, not every-frame/full-state equality, hardware FPS, a full campaign,
or proof of near-30-fps performance.

The observed mean gain is small relative to the multi-million-cycle busy loop
and does not materially improve its movement-dominated tail. Keep this trial
off and do not promote it on this evidence alone. A more consequential next
target is exact attribution of the roughly 2.67-million-cycle movement scope
on peak call 8347, preserving route/cache behavior, source order, actors and
cadence. Receipts and private logs remain in the isolated
`sc64-p10-optical-mask-nov6/.local/qualification/` directory.
