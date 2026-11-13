# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile. Run from the repository root.
#
# make check-all-roms builds and verifies every ROM that has a verify-* rule,
# continues past failures and prints one RAM-margin table (see
# tools/sc64_rom_check.py). It exits non-zero if any ROM fails; a ROM whose
# private inputs are absent is SKIPPED, not passed. The tool supplies the local
# -o and Z8_DESCRIPTOR arguments when those files exist.
#   make check-all-roms ROM_CHECK_ARGS="--only verify-zerg-z8,verify-showcase"
#   make check-all-roms ROM_CHECK_ARGS="--prune --require-all"
ROM_CHECK_ARGS ?=

.PHONY: check-all-roms
check-all-roms:
	$(PYTHON) tools/sc64_rom_check.py $(ROM_CHECK_ARGS)
