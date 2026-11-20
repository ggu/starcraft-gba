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

## Default release switch

[PR #393](https://github.com/ggu/starcraft-gba/pull/393) selects Z10 as the
default Zerg product. The Z10 build and explicit Z9 build match their
previously qualified ROMs byte-for-byte. The existing Z9 filename, code
`SCZ9`, and `.local/play/zerg-z9` are unchanged. Z10 uses `SCZA` and
`.local/play/zerg-z10`.

- Ordinary host controllers for carried Z1–Z9 all win under the same Z10
  profile flags, as does the Z10 controller above. The carried Z9 controller
  wins at frame 68,520 with zero faults.
- All 18 full custom controller runs on the Z10 flags, covering both maps,
  three human races and 1–3 rotated AI slots, reach ordinary defeat results.
  This adds complete outcomes to the separate 234 startup/save cases.
- Physical button input to the normal ROMs loads a 32 KiB Z9 save at Z8
  frame 16,660 on Z9, and a Z10 save at frame 2,527 on Z10. Z10 declines the
  Z9 journal and remains at the title. Neither seed file was modified.
- On the normal Z10 ROM, physical buttons start Blood Bath as Protoss
  against three AIs and reproduce the host result at frame 18,986
  (19,121 input calls). This sample tests the normal entry point and a
  complete custom match, not the entire ARM lineup matrix.
- The matched 1,000-update Zerg ARM battle window changes mean loop cycles
  from 1,369,965 on Z9 to 1,368,916 on Z10 (−0.08%). Sampled workloads
  match. The window remains above the 561,792-cycle budget for 30 fps.

## Normal sparse-route build and fresh victory

The Z10 ROM `ebc0d0f4…` from [#659](https://github.com/ggu/starcraft-gba/pull/659)
runs the current renderer, Carrier service and sparse route packing.
Ordinary controller input from blank SRAM wins the final Zerg mission
at frame 50,193. The replay writes its pause journal at frame 2,526
with START and B. The complete 32 KiB export is byte-identical to the
previous qualified normal replay. A separate mGBA process loads this new
journal with L on HOME and wins at the same frame.

All 22 historical control observations match in the HOME run. The 20
observations after fresh load also match the uninterrupted run's named
state, world fingerprint, campaign state, and crystal carrier. The
replay retains the crystal Drone at the objective, clears the countdown,
and publishes victory without a mission fault. The whole uninterrupted
run averages 12.08 emulated main-loop updates per second. Its figure
includes menus, presentation and waits. Dense campaign performance and
physical-device timing remain open.

| Private receipt | SHA-256 |
| --- | --- |
| HOME and pause save | `e1e632861aadbc8cbebd230544125bda2177ca541d635a5731db223ce504a14f` |
| Fresh HOME load and victory | `6eb1e4ea58ac6f73659ba289927ac90c01afc45f1d4ae87a2b56326ef5dab7ca` |
| Paired comparison | `4642cd7387339634bd530baea0989f50b90f9447a4e01514299acfba03751a58` |
