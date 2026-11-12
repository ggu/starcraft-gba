# T5 from HOME on the normal Terran cartridge

The retained T5 controller tape began after a compile-gated probe had already
started Revolution. The T10 test fixture can now record the ordinary HOME
route: open the mission menu, go right twice, select T5, and skip the briefing
with Start. It releases Start on the first gameplay update. The validator
checks the recorded menu inputs and accepts older probe tapes as well.

The controller's Supply Depot placement search asks the engine for reachable
sites. Those queries can warm route fields used by later game updates. The
fixture now restores the covered route state after scoring the candidate
sites. In the debug run, hashes also check that no observer changes that
state between game updates. Neither the mission scripts nor the runtime
were modified.

Two ASan/UBSan HOME runs built with the normal Terran T10 flag vector from
`6e843d814da131950c92da79901985fc2e05ff78` produced byte-identical winning
tapes. The run ends at game frame 122,281 after 123,792 calls, with Kerrigan
and Raynor alive, a player Command Center still standing, no enemy buildings,
the primary objective and completion bits set, and zero adapter faults.
The validator accepts its 7,208 held-key transitions and 150 observations.
The legacy probe fixture also still wins at its original frame 126,331.

```sh
make CLANG='clang -D_FORTIFY_SOURCE=0 -DROUTE_PURITY_DEBUG' build/test-sc64-full-t5-home-run
./build/test-sc64-full-t5-home-run --limit 200000 --trace .local/sc64/t5-home.json
```

The unmodified normal Terran T10 cartridge won from a fresh HOME boot in
mGBA at frame 122,281. All 150 observation records matched at 139 distinct
calls, including the paid Starport, Control Tower and Wraith wing. The
human trigger outcome was victory, the primary and completion bits were
set, both heroes were alive, and the player kept a Command Center. No
enemy buildings or trigger faults remained.

Fourteen authored text pages took ordinary A presses and releases. Each
left the simulation frame unchanged and preserved the held gameplay keys.
The replay therefore used 123,820 physical updates for the 123,792-call
HOST tape. The emulator runner made no guest-memory writes or SRAM loads.

The full replay averaged 792,269 guest cycles per main loop (21.18 emulated
updates per second), including rendering, UI and presentation waits. It
does not meet the near-30-fps target or measure physical GBA frame rate.
This qualifies one route from HOME; it does not test SRAM reload.

- ROM SHA-256: `a5375ee144fc105c072410385b2b2ed0a6f31d5abe883f4bbad6b4ff54c7f337`.
- ELF SHA-256: `d17801881a4badb0152f9169f5c7c0cadebb79832365f77fbddf54431c1b5913`.
- Winning HOME tape SHA-256: `5547d53fc2fccf6fa802f4fff9165525f78cc5e80a72f39de2963db312ae70d9`.
- Full physical win receipt SHA-256: `345efc2a25d17ebfb53f473770b256b3b3eec441a0924afd18db3f6943c1df85`.
