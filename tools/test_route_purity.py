#!/usr/bin/env python3
"""Run the observer fixture using an already-prepared Z10 ARM args JSON."""
import json
import pathlib
import subprocess
import sys

if len(sys.argv) != 2:
    sys.exit('usage: python3 tools/test_route_purity.py PREPARED_Z10_ARGS.json')
args = json.loads(pathlib.Path(sys.argv[1]).read_text())['args']
flags = []
for i, arg in enumerate(args):
    if arg.startswith(('-D', '-I')) and not arg.startswith('-DSC64_AUDIO_BLOB'):
        flags.append(arg)
    elif arg == '-include':
        flags.extend(args[i:i + 2])
sources = list(dict.fromkeys(arg for arg in args if arg.endswith('.c') and arg not in
                             ('common/main.c', 'common/memory.c', 'starcraft/game.c')))
out = pathlib.Path('build/route-purity-test')
out.parent.mkdir(parents=True, exist_ok=True)
subprocess.run(['clang', '-std=c11', '-O1', '-g0', '-Wall', '-Wextra', '-Werror',
                '-Wno-unused-function', '-Wno-unused-variable', '-Wno-unused-parameter',
                '-fsanitize=address,undefined', '-DGBA_HOST', '-DROUTE_PURITY_DEBUG',
                '-D_FORTIFY_SOURCE=0', *flags, 'tests/sc64_route_purity_prefix.c',
                *sources, '-o', str(out)], check=True)
subprocess.run([str(out)], check=True)
