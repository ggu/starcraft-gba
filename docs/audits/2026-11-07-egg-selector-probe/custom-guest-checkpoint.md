# Opt-in Z7 custom Egg guest checkpoint

At source `6bbc678f`, the selector remained opt-in and the frozen Z7 product
was driven through ordinary physical buttons in the pinned mGBA runner. The
real custom lobby selected Zerg against one Terran AI. The controller used
ordinary mining, built a Spawning Pool with a Drone, selected a Hatchery,
paid for a Zergling Egg, and used START/B to save. No guest RAM or registers
were injected and no HOST save was used as guest input. The fresh process
used the first guest's exported 32 KiB journal and the HOME-L load action.

The guest checkpoint passed the driver's actual payment assertions: a
completed friendly Pool existed, a friendly Hatchery lost one larva, and
minerals decreased by 50 across the training input. At the save, the guest
and the independently authenticated journal agreed on frame 1953, Egg slot
12, remaining timer 523, and supply 8/18; the journal recorded 448 minerals.
The game loader selected the Egg namespace (outer `16435a54`, inner
`12435a54`). The fresh load reproduced the saved named-world fingerprint
`fabe0825da7ea72ad8b8c7ab136d6a8392f28c7cce09293a4d2abbc68484aeba`.
At frame 2476 the original and reloaded continuations each had two friendly
Zerglings and no Egg, and both had fingerprint
`e02ced2894f07c678b9d134630e3dd781ba2883abed8dbf732c1df6231a1a40f`.
The comparison includes named live unit and building fields, owner banks,
mineral fields, frame, human race, map, bot count and supply; it is not full transient
state or pixel equality. The saved record does not preserve separate numeric
before/after payment snapshots; the successful executable assertions above
are the retained evidence for the debit and larva transition.

A subsequent read-only inspection of the same guest-exported journal closes
the saved-roster evidence gap without another guest run. Using the exact paired
ELF's DWARF layout, `RaceSave.state` begins at payload offset 15652 and its
five `players` integers at 15780. The selected valid journal has
`state[0..4] = {1,1,0,0,1}`, `state[21] = 0`, and
`players = {1,0,-1,-1,-1}`. The custom owner mapping and the candidate's
own roster validation associate the human with owner 0/Zerg (1) and its sole
bot with owner 1/Terran (0). The 16312-byte payload checksum independently
recomputes to `23e1f7fe`. This is authenticated saved-journal roster evidence,
not an additional live-register sample; the image hash below is unchanged.

The single job acquired the shared native lock, maintained the 4 GiB reserve,
used a 256 MiB per-file limit, a polled 64 MiB output threshold, and 600-second
per-guest and 1230-second overall deadlines. It completed in eight seconds;
the 594 pinned input hashes were unchanged. Strict HOST sanitizer and negative
contract tests preceded the guest run. The saved and restored comparisons do
not establish hardware behavior, full campaign coverage, ordinary Z1–Z6 guest
coverage, or 30 fps. No normal release flag was enabled.

Private local evidence (not committed):

* Physical result SHA-256 `90684dc1e60767baba3ce7d1419c4e0d8e9a7edc825ea9ebd013a2e178f4e555`.
* Guest-exported SRAM SHA-256 `3ba1cac0e9a488660e1bc7e95f37bf8e3f39562e564f8a499e1f04d4011ac5ab`.
* Supervisor receipt SHA-256 `99633848b0171ad691ed577df77cba7d2020e505e657c8a04ae3fdc5b23aab9c`.
* ROM `13059f71df0a8a0f1bfcd9e30fe9c6c54ec4a7f506585893e35b17bbaa84f608`; ELF `6c1d0bd83f072cfd6dc75b06d764b88711f7da6a37f6956e4a5c4e7df66e1e4d`.
* Physical runner `4199fe903b18c77e0833db1860c4968509cfc82e506bc6f2f9be7582b720fae2`; tape `bbe29731cabc528873bd5567335d0c03d048f5559aebca3b5a6ef67f4e325a25`.
* Offline reader `8966a33f43f0ad3fe155e3e8b0d05023244ef2a5c09967bea32bdbb29e415f62`; input manifest `6c467d390599bcf22778860aa2a2efbe2266cc123347c913983af48cafacaa46`.
