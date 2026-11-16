# T6 from HOME on the normal Terran cartridge

The retained T6 host route did not reproduce on the KODAK Terran ROM.
At its SCV queue checkpoint (tape call 1,790), the guest had no selected
building, the Command Center queue was empty, and the controller remained
in Build mode. The prior B menu helper directly reset `menu_cursor` in the
host process. That reset was absent from the tape, so the same B, Down, A
sequence selected a different command on the cartridge. Later the guest
never built the expected Refinery or Factory and did not win.

The host controller now uses ordinary Up or Down to return to Move after
opening the menu. Both path-grid planning helpers restore the route state
after their queries. An ASan/UBSan run of the production Terran flags plus
the host probe entry and `ROUTE_PURITY_DEBUG` verified the route hash
between updates and regenerated a win: call 23,849, frame 23,536, with
Raynor and two Dropships in the goal, Norad alive and no faults. No game
runtime or save format changes are needed.

The physical replay starts from fresh HOME on the unmodified KODAK
Terran cartridge (runtime `cbba1c5`, ROM SHA-256
`0f5e893037f71860c66112920e7195f5d7e2db5b4aa7f9e64e222689419de3d8`).
It selects Norad II, reads all 15 briefing pages, and deploys at call 69.
The 1,059 host gameplay transitions use only physical buttons. Thirteen
in-game dialogue pages take two additional calls each; each press and
release leaves the simulation frame unchanged. All 42 host checkpoints
match across 37 guest fields, including the paid SCV, refinery and factory,
the rescue mask, and the transport and goal counts. Two host-local
controller indices are not guest state and are excluded.

The cartridge wins at physical call 23,944, frame 23,536. Raynor and both
Dropships reach the goal, Norad survives, the primary objective and mission
completion bits are set, and there are zero trigger faults. Average cost
is 1,002,317 guest cycles per update (16.74 emulated updates per second);
the peak is 25,853,365 cycles. This qualifies the route from HOME, not
physical GBA speed or fresh SRAM continuation.

- Host tape SHA-256: `59e1aa7c6524c9cf1b17d31b57858ee0e2d28c8bd0b16ee10dc3233722556bfe`.
- Physical receipt SHA-256: `6bf6180b590954b836430cd23da435bcb7da4b76bfb3780a675fac4a49ef8ecd`.
