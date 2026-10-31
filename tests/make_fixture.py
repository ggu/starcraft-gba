# SPDX-License-Identifier: GPL-3.0-or-later
"""Copy a production Makefile and link its literal, repository-owned includes."""
from pathlib import Path
import re
import shlex

_INCLUDE = re.compile(r'^\s*(?:-?include|sinclude)\s+(.+)$')


def copy_makefile_with_includes(source_root: Path, fixture_root: Path) -> None:
    source_root = Path(source_root).resolve()
    fixture_root = Path(fixture_root)
    (fixture_root / 'Makefile').write_bytes((source_root / 'Makefile').read_bytes())
    pending = [Path('Makefile')]
    seen = set(pending)
    while pending:
        source = source_root / pending.pop()
        for line in source.read_text().splitlines():
            match = _INCLUDE.match(line)
            if not match or line.startswith('\t'):
                continue
            for name in shlex.split(match.group(1), comments=True):
                if any(ch in name for ch in '$*?['):
                    continue  # Make resolves dynamic includes from fixture inputs.
                relative = Path(name)
                if relative.is_absolute() or '..' in relative.parts:
                    raise ValueError(f'non-repository Make include: {name}')
                if relative in seen:
                    continue
                target = source_root / relative
                if not target.is_file():
                    raise FileNotFoundError(target)
                destination = fixture_root / relative
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.symlink_to(target)
                seen.add(relative)
                pending.append(relative)
