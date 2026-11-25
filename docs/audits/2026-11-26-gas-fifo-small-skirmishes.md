# Gas FIFO with one and two opponents

These cases extend the [three-opponent audit](2026-11-26-gas-fifo-skirmishes.md). They use the existing Custom Game HOST controller and FIFO cartridges built from `528d9082` with `SC64_GAS_SERVICE_TRIAL` and `SC64_GAS_SERVICE_QUEUE_SAVE`. The release matrix plans are the starting point; adjusted plans are identified below. They are distinct evolving matches, not identical-input comparisons or proof of balance or native fidelity.

For each successful win, the ordinary physical-button ARM replay starts from HOME, checks the actual lobby start order, owner and race for every CPU, compares the recorded checkpoints, saves at a busy pause, resumes from that save in a fresh process, checks all later observations, compares victory SRAM, and reloads the result. A loss replay checks the same lobby and physical-button checkpoints through the loss RESULT screen, but does not establish fresh-save continuation or result reload. Blood Bath start orders are independently probed on ARM before HOST authoring and checked again in the full replay. Outpost uses order zero, also independently checked in the full replay.

## Matrix

| Human / map / CPUs | Plan | Start order | HOST result / frame | ARM result and continuation checks |
| --- | --- | ---: | --- | --- |
| terran / Outpost / Z | release | 0 | won, 26,912 | win, 73 after reload |
| terran / Outpost / TZ | release | 0 | won, 31,701 | win, 108 after reload |
| terran / Blood Bath / Z | release | 13 | won, 22,713 | win, 60 after reload |
| terran / Blood Bath / PZ | release | 11 | won, 39,414 | win, 405 after reload |
| zerg / Outpost / P | release | 0 | lost, 19,513 | loss, 183 checks |
| zerg / Outpost / P | adjusted | 0 | won, 18,407 | win, 60 after reload |
| zerg / Outpost / PZ | release | 0 | lost, 26,777 | loss, 188 checks |
| zerg / Outpost / PZ | adjusted | 0 | won, 23,432 | win, 104 after reload |
| zerg / Blood Bath / P | release | 7 | won, 11,073 | win, 28 after reload |
| zerg / Blood Bath / TZ | release | 6 | won, 38,190 | win, 267 after reload |
| protoss / Outpost / T | release | 0 | lost, 18,090 | loss, 132 checks |
| protoss / Outpost / T | adjusted | 0 | won, 23,732 | win, 73 after reload |
| protoss / Outpost / TZ | release | 0 | lost, 26,607 | loss, 180 checks |
| protoss / Outpost / TZ | adjusted | 0 | won, 29,622 | win, 81 after reload |
| protoss / Blood Bath / T | release | 8 | lost, 26,343 | loss, 267 checks |
| protoss / Blood Bath / T | adjusted | 2 | won, 16,795 | win, 52 after reload |
| protoss / Blood Bath / TZ | release | 24 | won, 16,117 | win, 103 after reload |

The adjusted plan that ended before a pause is excluded from the qualified wins. The one-opponent Protoss Outpost release plan loss was confirmed on ARM. Its adjusted all-Zealot plan opts into `GLOBAL_ATTACK_RATIO=0`, removing the controller’s extra non-melee requirement that its army be at least twice the total enemy army before attacking. The default remains 2. In the observed loss, the two home mineral patches were depleted before the decisive counterattack and the Terran army had built a substantial lead. The adjusted controller committed earlier and won; this is one strategy outcome, not a claim that the ratio is generally optimal. Its ARM replay matched 169 uninterrupted checks and 73 fresh-save continuation checks, with victory SRAM and result reload verified.

## Reproduction inputs

For each row, build the FIFO-enabled HOST author and matching FIFO ARM cartridge/ELF from the source revision above. Invoke `tools/sc64_custom_full_match.py author` with `--controller`, `--rom`, `--elf`, `--runner`, `--output`, `--map`, `--human`, `--cpus`, `--limit 160000`, and the plan below. For wins with a pause, invoke its `replay` subcommand on the generated trace and a fresh output directory. For losses, use `tools/sc64_custom_loss_replay.py` with the same replay arguments; the victory replay rejects loss traces. Input trace hashes below pin these particular authored tapes; ROM, ELF, runner and detailed replay receipts remain local and contain no distributable game assets.

| Case and plan | Trace SHA-256 |
| --- | --- |
| terran / 0 / z / release: `PRODUCERS=3` | `5b0ed7a9de1393aeb00f4060d5db4ddea6aa032b50e9ead4beab65f32bd5ef74` |
| terran / 0 / tz / release: `(defaults)` | `2b2f3e34759bcf36c20c9b0678271fb97eeba5c55985be4fae0b9605d49ec94a` |
| terran / 1 / z / release: `LOBBY_WAIT=8,FIRST_ATTACK=7000,PRODUCERS=3,WORKERS=14,TECH_AFTER=2,LAB_AFTER=3,ATTACK_MIN=6,ATTACK_RATIO=3,ATTACK_MARGIN=8` | `8b4ea466fef7bf51595fb97c6950e60552046d10fcd11e913fbb06cc09404d08` |
| terran / 1 / pz / release: `LOBBY_WAIT=8,FIRST_ATTACK=7000` | `9329a9c552e787f0a9b98e1ec1bd28c3e7a5399828963d2cecfac7ea1e862ea3` |
| zerg / 0 / p / release: `LINE=38,WORKERS=9,DEFENSES=2,PRODUCERS=2,FIRST_ATTACK=7000,DEFENSE=146,DEFENSE_FRAME=2500,ATTACK_RATIO=1,ATTACK_MIN=6,ATTACK_MARGIN=0` | `45f54f34065eb89b903603d8c6a942096653a75d9d879f34d74b72093361298f` |
| zerg / 0 / p / adjusted: `LINE=37,WORKERS=9,DEFENSES=2,PRODUCERS=2,FIRST_ATTACK=7000,ATTACK_RATIO=1,ATTACK_MIN=6,ATTACK_MARGIN=0,DEFENSE=146,DEFENSE_FRAME=2500` | `9c18030e550f511e1c0fe7d44867a5166df617ceaad652fed5aaac7777113649` |
| zerg / 0 / pz / release: `WORKERS=12,LINE=38` | `8afe2231fee01e65feb738c61aa24a2a559360d86fc409980ebb8c4f45e7d16d` |
| zerg / 0 / pz / adjusted: `LINE=37,WORKERS=9,DEFENSES=2,PRODUCERS=2,FIRST_ATTACK=7000,ATTACK_RATIO=1,ATTACK_MIN=6,ATTACK_MARGIN=0,DEFENSE=146,DEFENSE_FRAME=2500` | `88f04d33b70ce0de3f53b7db0bc1e712a6def0daf39eec211cb9160227d392f6` |
| zerg / 1 / p / release: `LINE=38,WORKERS=12,PRODUCERS=1,FIRST_ATTACK=7000,LOBBY_WAIT=16` | `ca1a3ee51df897a1f8d8728025ed47882d94ef10e53a7fe3f3c2d06853d00d62` |
| zerg / 1 / tz / release: `LINE=38,WORKERS=12,PRODUCERS=1,FIRST_ATTACK=7000,LOBBY_WAIT=22` | `a5229297c214c3d920607c3b93a65c410546663ddbd4923e1abda0cadc93466e` |
| protoss / 0 / t / release: `LINE=66,WORKERS=12,DEFENSES=4,PRODUCERS=2,FIRST_ATTACK=7000,DEFENSE=162` | `fe7c829d0bdb7f2bed76793016ed7adca8d611e3999911bd5af737b8916615d1` |
| protoss / 0 / t / adjusted: `LINE=65,WORKERS=9,PRODUCERS=3,MAX_PRODUCERS=3,FIRST_ATTACK=3000,ATTACK_MIN=5,ATTACK_RATIO=0,ATTACK_MARGIN=0,GLOBAL_ATTACK_RATIO=0,TECH_FRAME=30000,RECALL_THREAT=5,RETREAT=0,DEFENSES=0,VERBOSE=1` | `a24e0cf5070b544160d34617b35663e679694f8508ccaa51db13dfd62af983e5` |
| protoss / 0 / tz / release: `LINE=65,WORKERS=10,DEFENSE=162,DEFENSES=4,PRODUCERS=2,FIRST_ATTACK=7000` | `4105b8dfd2383e834c1ddb081bd98264e50bdbb5c4f1e0c25897c1ba6a948153` |
| protoss / 0 / tz / adjusted: `LINE=65,WORKERS=10,DEFENSES=4,PRODUCERS=3,FIRST_ATTACK=3000,ATTACK_RATIO=1,ATTACK_MIN=6,ATTACK_MARGIN=0,DEFENSE=162,TECH_FRAME=30000` | `787933a5f208ce04749fd7c6831fb6460404841dc1d9e8e8584a0b211f8898a0` |
| protoss / 1 / t / release: `LINE=66,TECH_FRAME=8000,LOBBY_WAIT=18` | `4f7c5b5c9864a2ce6cf2358ec1fbe4cdc8d1db929bb704464595e4a9ef413333` |
| protoss / 1 / t / adjusted: `LINE=66,TECH_FRAME=8000,LOBBY_WAIT=0,FIRST_ATTACK=4500,WORKERS=14,PRODUCERS=3,ATTACK_MIN=6,ATTACK_RATIO=1,ATTACK_MARGIN=0` | `9351e881351168eda9a182622e2e92a03659a558e39b36d10329070deda311cc` |
| protoss / 1 / tz / release: `LINE=65,TECH_FRAME=30000,WORKERS=12,PRODUCERS=3,FIRST_ATTACK=3000,ATTACK_MIN=8,ATTACK_RATIO=1,ATTACK_MARGIN=0` | `7d9e4f2137fc2df0779f8e4e1f4474f693d08a239e4312febf8f088a3ec19a9d` |
