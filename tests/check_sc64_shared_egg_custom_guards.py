#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify the custom service cannot silently activate old campaign stages."""
import subprocess
import sys

EXPECTED = "Custom Egg service cannot combine with the legacy HOST Egg stages"


def main():
    if len(sys.argv) < 2:
        raise SystemExit("usage: check_sc64_shared_egg_custom_guards.py compiler [flags ...]")
    base = [*sys.argv[1:], "-E", "starcraft/game.c", "-o", "/dev/null"]
    for name, flags in (
        ("old producer", ["-DSC64_SHARED_EGG_HOST_STAGE"]),
        ("old journal", ["-DSC64_SHARED_EGG_JOURNAL_HOST_STAGE"]),
        ("old producer and journal", ["-DSC64_SHARED_EGG_HOST_STAGE", "-DSC64_SHARED_EGG_JOURNAL_HOST_STAGE"]),
    ):
        result = subprocess.run(base + flags, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, check=False)
        if result.returncode == 0 or EXPECTED not in result.stderr:
            raise SystemExit(f"{name}: expected rejection absent\n{result.stderr}")
        print(f"{name}: rejected with named custom-service fence")


if __name__ == "__main__":
    main()
