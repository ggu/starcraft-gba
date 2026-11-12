# Z10 ordinary-input ARM win and save continuation

The current Z10 probe uses the production generation-49 Z10 flags and the
normal update, audio and render loop, with a test entry point that feeds only
controller inputs. Its tape begins at the title and enters Full Circle
through the campaign picker. The controller holds the opening with Sunken
Colonies and Hydralisks, then takes Mutalisks to the Temple and escorts the
crystal carrier. The run has no direct world writes.

- Full mGBA replay: 61 host/ARM checkpoints match, including victory at
  frame 58,795, call 60,834, zero faults, and digest 2,990,057,405.
- Prefix: an ordinary pause-menu save at frame 2,527, call 2,666, writes
  a 32 KiB SRAM image while three buildings are under construction. A
  separate emulator process quick-loads it. All 59 comparable continuation
  checkpoints match the uninterrupted ARM run, and 60 including the resume
  observation match the host resume.
- [#390](https://github.com/ggu/starcraft-gba/pull/390) shortened only the
  Z10 probe title to fit the GBA header. All three probe headers, sizes and
  checksums pass. The normal Z10 cartridge is unaffected by the title fix.
- The carried-profile sanitizer test launches and save-round-trips all ten
  Zerg missions and all 234 custom startup lineups on Z10: two maps, three
  human races, and every assignment of 1–3 AI slots. These are early save
  points, not 234 complete games.

The explicit `zerg-z10` tooling authenticates its additional descriptor and
trigger program. On a workspace with prepared inputs:

```sh
./dev sc64 status --profile zerg-z10
make test-sc64-z10-carried-profile test-sc64-z10-replay sc64-z10-arm-replay
```

At qualification time Z9 was still the default. Z10 uses its own save
directory and save generation. This work does not migrate Z9 saves, establish every Z10 save
point, verify original-N64 timing, or measure physical hardware.
