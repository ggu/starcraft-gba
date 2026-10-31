#!/usr/bin/env python3
"""Export the two BC facility body frames to an opt-in private sidecar.

Uses the existing authenticated sprite importer in a temporary directory; the
only retained output is a minimal generated header. No shared bank is changed.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_race_sprites  # noqa: E402

FORMAT_VERSION = 1
NAMES = tuple(f"sc64_race_sprite_{name}_frame_0{suffix}"
              for name in ("science_facility", "physics_lab")
              for suffix in ("_pixels", ""))


def extract_declarations(text: str) -> list[str]:
    """Fail closed if a required independent frame declaration is absent/ambiguous."""
    result = []
    for name in NAMES:
        pattern = (r"static const (?:unsigned char|Sc64SpriteFrame)\s+" +
                   re.escape(name) + r"(?:\[[^\]]*\])?\s*=\s*\{.*?\};")
        found = re.findall(pattern, text, flags=re.S)
        if len(found) != 1:
            raise ValueError(f"expected exactly one declaration for {name}")
        result.append(found[0])
    return result


def render_header(declarations: list[str]) -> str:
    if len(declarations) != len(NAMES):
        raise ValueError("incomplete BC body frame set")
    return ("/* Generated from verified USA ROM. Private local data; do not commit. */\n"
            "#ifndef SC64_BC_BODY_ART_GENERATED_H\n#define SC64_BC_BODY_ART_GENERATED_H\n"
            f"#define SC64_BC_BODY_ART_FORMAT_VERSION {FORMAT_VERSION}u\n"
            "typedef struct { short offset_x, offset_y; unsigned short w, h, canvas_w, canvas_h; "
            "const unsigned char *pixels; } Sc64SpriteFrame;\n" +
            "\n".join(declarations) + "\n#endif\n")


def export(rom: Path, remap: Path, header: Path) -> str:
    target = header.resolve()
    if target.is_relative_to(ROOT) and not target.is_relative_to(ROOT / ".local"):
        raise ValueError("private generated art must not be written into the source tree")
    with tempfile.TemporaryDirectory(prefix="sc64-bc-body-") as temp:
        tmp = Path(temp)
        source_header = tmp / "race.h"
        # include_late reuses the authenticated source image compositor for the
        # two raw buildings; no output in the shared generated banks is touched.
        sc64_race_sprites.generate(rom=rom, remap_path=remap, out=tmp / "previews",
                                   header=source_header, include_t4=True,
                                   include_t6=True, include_late=True)
        content = render_header(extract_declarations(source_header.read_text(encoding="ascii")))
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content, encoding="ascii")
    return hashlib.sha256(content.encode("ascii")).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--remap", required=True, type=Path)
    parser.add_argument("--header", required=True, type=Path)
    args = parser.parse_args()
    try:
        digest = export(args.rom, args.remap, args.header)
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f"sc64_bc_body_art: {exc}", file=sys.stderr)
        return 1
    print(f"BC body sidecar v{FORMAT_VERSION}: {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
