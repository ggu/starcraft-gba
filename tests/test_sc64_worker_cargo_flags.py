#!/usr/bin/env python3
"""Check shared cargo activation and baseline opt-out in current products."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

PRODUCTS = ("terran", "zerg", "protoss", "showcase")
FLAG = "-DSC64_WORKER_CARGO_KIND"


def product_flags(value):
    env = os.environ.copy()
    if value is None:
        env.pop("CANONICAL_WORKER_CARGO_FLAGS", None)
    else:
        env["CANONICAL_WORKER_CARGO_FLAGS"] = value
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "vectors.mk"
        path.write_text(
            "$(info terran=$(TERRAN_ROM_FLAGS))\n"
            "$(info zerg=$(ZERG_Z7_CANONICAL_FLAGS))\n"
            "$(info protoss=$(PROTOSS_P7_CANONICAL_FLAGS))\n"
            "$(info showcase=$(SHOWCASE_FLAGS))\n"
            ".PHONY: cargo-print-flags\ncargo-print-flags:\n\t@:\n"
        )
        output = subprocess.check_output(
            ["make", "--no-print-directory", "-s", "-f", "Makefile",
             "-f", str(path), "cargo-print-flags"], env=env, text=True,
        )
    return dict((key, value.split()) for key, value in
                (line.split("=", 1) for line in output.splitlines()))


class WorkerCargoFlagTests(unittest.TestCase):
    def test_shared_default_and_opt_out(self):
        default, disabled, enabled = (product_flags(value)
                                      for value in (None, "", FLAG))
        self.assertEqual(tuple(default), PRODUCTS)
        for product in PRODUCTS:
            with self.subTest(product=product):
                self.assertIn(FLAG, default[product])
                self.assertEqual(default[product], enabled[product])
                self.assertNotIn(FLAG, disabled[product])
                self.assertEqual([x for x in enabled[product] if x != FLAG],
                                 disabled[product])


if __name__ == "__main__":
    unittest.main()
