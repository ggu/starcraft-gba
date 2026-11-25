# SPDX-License-Identifier: GPL-3.0-or-later
# Parity harness rules (see tools/sc64_parity_tables.py). Included from
# mk/padded-impact-save/Makefile because the top-level Makefile is at its size
# limit. These rules read private ROM inputs when present and write derived
# output only under $(SC64_PARITY_HOME) (default: .local/parity in the main
# checkout); they print a skip line and succeed when the inputs are absent.

.PHONY: parity-tables
parity-tables:
	$(PYTHON) tools/sc64_parity_tables.py
