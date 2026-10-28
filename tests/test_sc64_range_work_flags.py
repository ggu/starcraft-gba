#!/usr/bin/env python3
"""Check the optional range worker reaches all canonical race products."""

import os
import subprocess


PRODUCTS = ("terran", "zerg", "protoss", "showcase")
FLAG = "-DSC64_RANGE_WORK"


def product_flags(extra_flag):
    env = os.environ.copy()
    if extra_flag:
        env["CANONICAL_RANGE_WORK_FLAGS"] = extra_flag
    else:
        env.pop("CANONICAL_RANGE_WORK_FLAGS", None)
    output = subprocess.check_output(
        ["make", "--no-print-directory", "-s", "range-work-print-flags"],
        env=env,
        text=True,
    )
    result = {}
    for line in output.splitlines():
        key, value = line.split("=", 1)
        result[key] = value.split()
    assert tuple(result) == PRODUCTS, result.keys()
    return result


def main():
    default = product_flags("")
    enabled = product_flags(FLAG)
    for product in PRODUCTS:
        assert FLAG not in default[product], (product, default[product])
        assert FLAG in enabled[product], (product, enabled[product])
        assert [x for x in enabled[product] if x != FLAG] == default[product], product
    print("SC64 range-work flag default/opt-in wiring passed for all four products")


if __name__ == "__main__":
    main()
