# SPDX-License-Identifier: GPL-3.0-or-later
"""Require the actual untouched historical game/common producer."""
import subprocess
import sys

root = sys.argv[1]
expected = "8127f7af1abe1ad9b5a911f7ea3554bcc691fba7"
head = subprocess.check_output(["git", "-C", root, "rev-parse", "HEAD"], text=True).strip()
if head != expected or subprocess.run(["git", "-C", root, "diff", "--quiet", "HEAD", "--"]).returncode:
    raise SystemExit("Flash fixture requires the clean exact 8127f7af baseline")
