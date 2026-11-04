#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Convert a generated PX held-key tape into the setup probe's private tape.h."""
import argparse
import re
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
text = args.source.read_text()
match = re.search(r'static const u32 px_tape\[\d+\]\s*=\s*\{(.*?)\};', text, re.S)
if not match:
    parser.error('missing px_tape array')
words = [int(x, 16) for x in re.findall(r'0x([0-9a-fA-F]+)u', match.group(1))]
if not words or any(a >> 10 >= b >> 10 for a, b in zip(words, words[1:])):
    parser.error('tape must contain strictly increasing held-key transitions')
with args.output.open('x') as out:
    out.write('static const struct { unsigned t; unsigned short h; } tape[] = {\n')
    for value in words:
        out.write('{%d,%d},\n' % (value >> 10, value & 1023))
    out.write('};\n')
