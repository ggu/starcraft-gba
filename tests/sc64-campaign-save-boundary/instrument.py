# SPDX-License-Identifier: GPL-3.0-or-later
"""Copy a source view and add HOST-only rejection tracing; no gate is changed."""
import argparse
from pathlib import Path
import re
import shutil

FUNCTIONS = ('race_save_game', 'race_combined_egg_candidate_valid',
             'race_scarab_save_candidate_valid', 'race_load_preflight_candidate',
             'race_worker_cargo_prepare_selected', 'race_load_game')

def instrument(source):
    for name in FUNCTIONS:
        pattern = r'\b' + re.escape(name) + r'\s*\([^;]*?\)\s*\{'
        match = re.search(pattern, source, re.S)
        if not match:
            raise ValueError(f'cannot find definition: {name}')
        end = re.search(r'^}', source[match.end():], re.M)
        if not end:
            raise ValueError(f'cannot find end: {name}')
        stop = match.end() + end.end()
        body = source[match.start():stop]
        count = body.count('return 0;')
        if not count:
            raise ValueError(f'no reject branches: {name}')
        # Retain original newlines so __LINE__ points to the original source.
        body = body.replace('return 0;',
            'do { fprintf(stderr, "BOUNDARY REJECT ' + name + ':%d\\n", __LINE__); return 0; } while (0);')
        source = source[:match.start()] + body + source[stop:]
    return source

def main():
    p = argparse.ArgumentParser(); p.add_argument('out', type=Path); a = p.parse_args()
    root = Path(__file__).resolve().parents[2]
    out = a.out.resolve()
    if out.exists():
        raise FileExistsError(f'use a fresh ignored directory: {out}')
    (out / 'starcraft').mkdir(parents=True)
    (out / 'common').symlink_to(root / 'common', target_is_directory=True)
    for item in (root / 'starcraft').iterdir():
        target = out / 'starcraft' / item.name
        if item.name == 'race_save_adapter.h':
            target.write_text(instrument(item.read_text()))
        elif item.name == 'game.c':
            shutil.copyfile(item, target)
        else:
            target.symlink_to(item.resolve(), target_is_directory=item.is_dir())
    print(f'generated isolated traced source view: {out}')
if __name__ == '__main__': main()
