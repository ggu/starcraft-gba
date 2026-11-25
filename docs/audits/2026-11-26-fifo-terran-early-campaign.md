# Experimental gas-service Terran T3–T5 continuation

This is a trial of the opt-in `SC64_GAS_SERVICE_TRIAL` at `528d9082`, not
qualification of the default release. The tested Terran ROM SHA-256 was
`be85a92d3554db7aaac0b2418cd4561c7f35e5af75d7304722bd37ef88a9a54d`.
The run continued the actual T2 victory save produced by the blank-start
T1–T2 trial, SHA-256
`24ce34a1b0ea5063e5e94ff20093e6db0a292be12a1ef244ea1a5b5d9a94b1b1`.
Each new result save became the next mission's input via ordinary A advance.

T3's neutral HOST controller was rebuilt with the trial flag. On the trial
cartridge in mGBA, all 54 periodic observations matched the trial HOST trace,
including the recorded world counts, both wallets, countdown and objectives;
no historical battle-field exceptions were used. T3 won at frame 54,000
with completion mask `0x70`. The HOST terminal record does not include its
wallet or actor counts, so those fields were compared through frame 53,993;
the final outcome and objective were checked separately. The result save
reloaded from HOME and re-exported byte for byte (SHA-256
`eef6f3bb1bab0125dc56e2e669924e08db3eafed1e1702caa388614a9da2fd64`).

T4's trial HOST controller matched all ten recorded observations and the
cartridge won at frame 8,911 with mask `0xf0`. Its fresh result reload also
re-exported identical SRAM (SHA-256
`859c0b58b1cd86a84cedc77ae59a3314b8a391741fae151a59ad2558659ad6ab`).

T5's trial HOST paid-air controller won, but its uninterrupted tape did not
match the cartridge: at tick 22,000 the cartridge was back in T5 at frame
1,295 while the HOST expected frame 21,668. No matching-run claim is made for
that tape. The actual T4 save instead entered T5, matched the opening
checkpoints and produced a fresh-reloaded ordinary pause save at frame 19,419
(SHA-256 `fd0f450b1068cb6155ff007fd4c09efe4c96098e5217240fc9e696d331a9b44b`).
The previously retained ordinary-input paid-production and attack stages
continued from that save. A later fresh-reloaded cut at frame 88,779 had seven
air units and eleven enemy buildings (SHA-256
`456c65411f81fb5bf05fa0f21050ccad3247a6c81510e3c27a1a6fa6882bbd7f`).
After the retained continuation and three adaptive ordinary-input attacks,
T5 won at frame 145,471 with both heroes alive, zero enemy buildings and mask
`0x1f0`. A fresh HOME load re-exported the victory SRAM byte for byte
(SHA-256 `8b45fe3cf4bc286f048c85ccf74f12d9feb643e3c4907d9b9025621ded082266`).

The T3 and T4 HOST runs used the candidate source and trial flag; the HOST
trace hashes were `b212efeeaa935c800b1ac66004da915d37b17bbba31d84e8cac647cb2c626724`
and `f10192aab83c53c616888b0430e354d9dc49f163d7e3a94796ff7ca362df7b7a`,
respectively. The private stage scripts and generated traces are not supplied
as repository replay targets. This is emulator evidence for the trial ROM,
not a physical-hardware measurement or a completed T1–T10 run.

## Carried T6 and T7

The actual T5 victory save then entered T6. Its trial HOST trace was
byte-identical to the earlier finite-gas reference. On the candidate cartridge
all 42 recorded checkpoint ticks matched without exceptions; T6 won at frame
23,536 with Norad and Raynor alive, both Dropships at the goal and completion
mask `0x3f0`. A fresh HOME result reload re-exported identical SRAM (SHA-256
`0e4296e2f29afdb4c67f4cc841fa485874a207c69274c92327a9fc77f298c028`).

That actual T6 result entered T7. The FIFO HOST controller produced a new
winning trace, SHA-256
`04ebbaeaa567cdb3262862a37092fe44e7b96c2112a70d8b03a0315da843b713`.
The cartridge matched all 12 distinct checkpoint ticks without wallet or
objective exceptions and won at frame 4,081 with Kerrigan alive, the emitter
at the goal, no faults and mask `0x7f0`. A fresh HOME load re-exported
identical SRAM (SHA-256
`2e6d17a6771abde6da2508f0e0efffa2c0c77f67be1a3ebb9c7f178dd1576d77`).
The private T7 replay reduced its generic free-disk reserve from 256 to
48 MiB after two reserve stops; its log was redirected to `/dev/null` and
its save outputs are small. No gameplay or comparison rule was changed.
The continuation from this newly produced T7 save through T8–T10 is ongoing.
