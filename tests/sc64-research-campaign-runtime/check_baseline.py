#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Require the genuine historical producer for the optional journal matrix."""
import pathlib
import subprocess
import sys

EXPECTED = "847dbaaebf6454b0cb45577ce1e7ddd91d81e7d2"
root = pathlib.Path(sys.argv[1])
if not root.is_dir():
    raise SystemExit("Set SC64_CAMPAIGN_BASELINE_ROOT to a clean 847dbaae checkout")
head = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
dirty = subprocess.run(["git", "-C", str(root), "diff", "--quiet", "HEAD", "--", "common", "starcraft"]).returncode
if head != EXPECTED or dirty:
    raise SystemExit("Historical producer must have clean 847dbaae game/common sources")
print("historical campaign writer source: 847dbaae verified")
