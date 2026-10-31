#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run actual-game southeast fixtures against retained private source logs."""
import argparse
import subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--game',type=Path,required=True)
p.add_argument('--source-dir',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
for distance in (280,300,320,384,448):
    result=subprocess.run([str(a.game.resolve()),str(distance)],capture_output=True,check=True)
    log=a.out/f'{distance}-game.log';log.write_bytes(result.stdout)
    command=['python3','tests/sc64_compare_defiler_se.py','--actual',str(log),
             '--source',str(a.source_dir/f'{distance}-source.log')]
    if distance==280:command+=['--source-q8',str(a.source_dir/'280-source-q8.log')]
    subprocess.run(command,check=True)
