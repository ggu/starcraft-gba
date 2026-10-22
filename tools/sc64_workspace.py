#!/usr/bin/env python3
"""Local ROM verification and preparation for the optional StarCraft 64 mission."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PRIVATE = ROOT / '.local/sc64'
ROM = PRIVATE / 'rom/StarCraft64-USA.z64'
USA_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
SIZE = 32 * 1024 * 1024


def normalize(data: bytes) -> bytes:
    if len(data) != SIZE:
        raise ValueError(f'Expected a {SIZE}-byte StarCraft 64 USA ROM')
    if data[:4] == bytes.fromhex('80371240'):
        result = data
    elif data[:4] == bytes.fromhex('37804012'):
        result = bytearray(SIZE)
        result[0::2], result[1::2] = data[1::2], data[0::2]
        result = bytes(result)
    elif data[:4] == bytes.fromhex('40123780'):
        result = bytearray(SIZE)
        for i in range(4):
            result[i::4] = data[3-i::4]
        result = bytes(result)
    else:
        raise ValueError('Unknown N64 byte order')
    if hashlib.sha1(result).hexdigest() != USA_SHA1:
        raise ValueError('ROM does not match the supported StarCraft 64 USA revision')
    return result


def import_rom(path: Path) -> Path:
    member = None
    if path.suffix.lower() == '.zip':
        with zipfile.ZipFile(path) as archive:
            entries = [e for e in archive.infolist()
                       if Path(e.filename).suffix.lower() in ('.z64', '.v64', '.n64') and e.file_size == SIZE]
            if len(entries) != 1:
                raise ValueError('ZIP must contain exactly one 32 MiB N64 ROM')
            member = entries[0].filename
            data = archive.read(entries[0])
    else:
        if path.stat().st_size != SIZE:
            raise ValueError('Expected a 32 MiB ROM or ZIP containing one')
        data = path.read_bytes()
    normalized = normalize(data)
    ROM.parent.mkdir(parents=True, exist_ok=True)
    if not ROM.exists() or ROM.read_bytes() != normalized:
        temporary = ROM.with_suffix('.tmp')
        temporary.write_bytes(normalized)
        temporary.replace(ROM)
    metadata = {'input': str(path.resolve()), 'member': member, 'bytes': SIZE,
                'input_sha256': hashlib.sha256(data).hexdigest(), 'source_magic': data[:4].hex(),
                'normalized_sha1': USA_SHA1, 'normalized_sha256': hashlib.sha256(normalized).hexdigest()}
    (ROM.parent / 'verification.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print(f'Verified USA ROM: {ROM}')
    return ROM


def prepare(path: Path | None = None, profile: str = 'wasteland') -> None:
    if path:
        import_rom(path)
    else:
        normalize(ROM.read_bytes())
    subprocess.run([sys.executable, str(ROOT / 'tools/sc64_import.py'), '--rom', str(ROM), '--profile', profile], cwd=ROOT, check=True)
    subprocess.run([sys.executable, str(ROOT / 'tools/sc64_assets.py'), '--rom', str(ROM), '--profile', profile], cwd=ROOT, check=True)
    sprites = ROOT / 'tools/sc64_sprites.py'
    if sprites.exists():
        subprocess.run([sys.executable, str(sprites), '--rom', str(ROM), '--profile', profile], cwd=ROOT, check=True)


def native(seconds: int | None = None) -> None:
    metadata = json.loads((PRIVATE / 'native.json').read_text())
    executable = Path(metadata['executable'])
    storage = Path(metadata['storage'])
    if not executable.is_file():
        raise ValueError('Native reference app is missing; see docs/SC64.md for its pinned build workflow')
    normalize(ROM.read_bytes())
    command = [str(executable), f'--config=sc64.storage={storage}', f'--config=sc64.rom={ROM}']
    for line in subprocess.check_output(['ps', '-axo', 'pid=,command='], text=True).splitlines():
        if str(executable) in line and f'--config=sc64.storage={storage}' in line:
            print('The native reference is already running with this storage folder.')
            return
    if seconds is not None:
        if seconds < 1:
            raise ValueError('--seconds must be positive')
        command.append(f'--config=sc64.seconds={seconds}')
    if seconds is not None:
        subprocess.run(command, cwd=ROOT, check=True)
    else:
        log_path = PRIVATE / 'native-play.log'
        with log_path.open('ab') as log:
            process = subprocess.Popen(command, cwd=ROOT, stdout=log, stderr=log, start_new_session=True)
        print(f'Started StarCraft 64 native reference (PID {process.pid}); log: {log_path}')


def status(profile: str = 'wasteland') -> None:
    generated = PRIVATE / 'generated' if profile == 'wasteland' else PRIVATE / 'backwater/generated'
    extracted = PRIVATE / 'extracted' if profile == 'wasteland' else PRIVATE / 'backwater/extracted'
    mission_slug = profile
    for label, path in [('Verified ROM', ROM), ('Mission header', generated / 'sc64_mission_generated.h'),
                        ('Mission report', extracted / f'{mission_slug}-trigger-report.json'),
                        ('Terrain header', generated / 'sc64_assets_generated.h'),
                        ('Sprite header', generated / 'sc64_sprites_generated.h'),
                        ('GBA mission ROM', ROOT / 'build/StarCraft64-Wasteland-GBA.gba'),
                        ('Native app metadata', PRIVATE / 'native.json')]:
        print(('READY  ' if path.is_file() else 'MISSING') + f' {label}: {path}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['prepare', 'native', 'status'])
    parser.add_argument('--rom', type=Path)
    parser.add_argument('--profile', choices=['wasteland', 'backwater'], default='wasteland')
    parser.add_argument('--seconds', type=int)
    args = parser.parse_args()
    try:
        if args.action == 'prepare': prepare(args.rom, args.profile)
        elif args.action == 'native': native(args.seconds)
        else: status(args.profile)
    except (OSError, ValueError, zipfile.BadZipFile, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))
