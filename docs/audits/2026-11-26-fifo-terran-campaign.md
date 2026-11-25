# Experimental gas-service Terran campaign segment

The opt-in `SC64_GAS_SERVICE_TRIAL` at `528d9082` was tested separately
from the default finite-gas release. The Terran trial ROM SHA-256 was
`be85a92d3554db7aaac0b2418cd4561c7f35e5af75d7304722bd37ef88a9a54d`;
the matching ELF was
`2e81ec6ed94e4921bdf85436454e0c699c42dad3c5148a08e6ab43daeadb408a`.

The T8 and T9 HOST controllers were rebuilt with the trial flag and used
ordinary input policies. T8's author trace was byte-identical to the
finite-gas release trace (`96e9ef4157fd53aadaaa32716d96bba328d6a3cc2555b1eb802ab37c72dd5cd4`);
it assigns one worker to its refinery. T9's new trace
(`6b0dc12d6edbe9ccf6776ee1a3b0a3371dceae33bf8d98f7c2dfa85e276a85a8`)
won at frame 48,586, compared with frame 49,486 for the independently
authored finite-gas release policy. Different inputs and economy mean this
is not a controlled measurement of a single action's duration.

Starting from an authentic retained T7 victory, mGBA replayed the trial
Terran cartridge through T8, T9 and T10, using each newly produced victory
SRAM for the next ordinary A advance. Every recorded checkpoint matched
without wallet or objective exceptions. Each victory loaded on a fresh HOME
boot and re-exported byte-identical SRAM.

| Mission | Checked ticks | Victory frame | Completion mask | Victory SRAM SHA-256 |
| --- | ---: | ---: | ---: | --- |
| T8 | 467 | 285,946 | `0xff0` | `e69975b517c0faff318a94c381ab356f4986e587ea84bd3d48708f0ea3629d74` |
| T9 | 53 | 48,586 | `0x1ff0` | `e79e3b0b4937a9af8937868fb436d6ae1de15df25195e6f9f48d1791e46dfaf3` |
| T10 | 6 | 3,361 | `0x3ff0` | `74dfc2d264dc18bb6fed40eaa2b34350911b73d77e24e515fbc98e20225be1ba` |

The T8 source controller used the longer test-only budget from #908. The
trial flag was added to HOST controller compilations in an isolated worktree;
the normal physical replay harness was privately adapted to the T9 trace's
new terminal and dialogue timing. These private adaptations and the generated
traces are not included as reproducible repository targets. The run is a
T7-victory-to-T10 segment, not a fresh ten-mission campaign or a physical
hardware measurement. It does not enable the experimental rule by default.

## Beginning a new campaign on the trial

A separate blank-SRAM run entered T1 through HOME, paused and reloaded
at input 6,222, matched 12 checkpoint ticks and won at frame 10,120.
The trial then loaded that exact T1 victory, advanced into T2 and won
at frame 29,366 with 31 matching checkpoint ticks. Both results
reloaded and re-exported identical SRAM in fresh processes. The T2 run
checks the result reload only. This opening pair has not yet been linked
through the middle of the campaign to the T7 victory used above.
