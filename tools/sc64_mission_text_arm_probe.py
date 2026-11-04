#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run a mission text ARM probe in an isolated mGBA and rebuild its box frames.

One mGBA process gets its own HOME and save directory; no debugger port is
opened. Frames are dumped by tests/sc64_mission_text_arm_probe.c over the debug
log and written as PNG next to the log. They hold ROM-derived art and text, so
keep --out inside an ignored folder. Emulator evidence only.
"""
from __future__ import annotations

import argparse
import json
import os
import pathlib
import pty
import select
import struct
import subprocess
import time
import zlib

from emulator_smoke import ROOT, require_mgba


def png(path: pathlib.Path, width: int, height: int, rgb: bytes) -> None:
    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body))
    raw = b''.join(b'\0' + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
                     + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b''))


def decode_images(lines: list[str], out: pathlib.Path) -> list[str]:
    """Turn the probe's hex lines into PNG frames; return the rest of the log."""
    rest: list[str] = []
    name, palette, pixels = None, bytearray(), bytearray()
    for line in lines:
        if line.startswith('MT IMAGE END'):
            if len(palette) != 512 or len(pixels) != 38400:
                raise ValueError(f'{name}: truncated frame dump')
            colours = struct.unpack('<256H', bytes(palette))
            lut = [bytes(((c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3)) for c in colours]
            png(out / f'{name}.png', 240, 160, b''.join(lut[p] for p in pixels))
            name = None
        elif line.startswith('MT IMAGE '):
            mission, box = line.split()[2:4]
            name, palette, pixels = f'mission-{int(mission):02d}-box-{box}', bytearray(), bytearray()
        elif line.startswith('MTP ') and name:
            palette.extend(bytes.fromhex(line[4:]))
        elif line.startswith('MTI ') and name:
            pixels.extend(bytes.fromhex(line[4:]))
        else:
            rest.append(line)
    return rest


def run(mgba: str, rom: pathlib.Path, out: pathlib.Path, timeout: float) -> list[str]:
    home = out / 'home'
    saves = out / 'saves'
    config = home / '.config' / 'mgba'
    config.mkdir(parents=True, exist_ok=True)
    saves.mkdir(parents=True, exist_ok=True)
    (config / 'config.ini').write_text('[ports.qt]\nlogToStdout=1\nlogLevel=7\n')
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home / '.config'))
    master, slave = pty.openpty()
    proc = subprocess.Popen(
        [mgba, '-l', '7', '-C', 'mute=1', '-C', 'audioSync=0', '-C', 'videoSync=0',
         '-C', f'savegamePath={saves}', '-C', f'savestatePath={saves}',
         '-C', f'screenshotPath={out}', str(rom)],
        stdin=subprocess.DEVNULL, stdout=slave, stderr=slave, env=env, cwd=out)
    os.close(slave)
    captured = bytearray()
    try:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline and b'MT DONE' not in captured and proc.poll() is None:
            ready, _, _ = select.select([master], [], [], 1)
            if ready:
                try:
                    chunk = os.read(master, 1 << 20)
                except OSError:
                    break
                if not chunk:
                    break
                captured.extend(chunk)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        os.close(master)
    (out / 'probe.log').write_bytes(bytes(captured))
    keep = ('MT ', 'MTP ', 'MTI ')
    lines = []
    for text in captured.decode(errors='replace').splitlines():
        hits = [text.index(k) for k in keep if k in text]
        if hits:
            lines.append(text[min(hits):].strip())
    return lines


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('rom', type=pathlib.Path)
    parser.add_argument('--out', type=pathlib.Path, required=True)
    parser.add_argument('--timeout', type=float, default=900)
    parser.add_argument('--require', default='',
                        help='comma list of global missions that must open a box with no input')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    lines = decode_images(run(require_mgba(), args.rom.resolve(), out, args.timeout), out)
    (out / 'result.txt').write_text('\n'.join(lines) + '\n')
    print('\n'.join(lines))
    shown = {int(l.split('mission=')[1].split()[0]) for l in lines if l.startswith('MT BOX ')}
    required = {int(m) for m in args.require.split(',') if m}
    ok = (bool(lines) and lines[-1] == 'MT DONE ok' and required <= shown and
          not any(l.startswith('MT FAIL') for l in lines))
    print(json.dumps({'ok': ok, 'rom': args.rom.name}))
    if not ok:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
