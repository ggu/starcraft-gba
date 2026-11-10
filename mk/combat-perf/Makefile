# Shared performance options of the race ROMs, and their optional checks.
# SPDX-License-Identifier: GPL-3.0-or-later

# Flip mode-4 pages from the VBlank interrupt so the next update overlaps the
# wait for VBlank. Drawing and simulation are unchanged; only the time at
# which a finished page appears moves. It takes effect in ROMs with the
# sampled-audio interrupt; host builds and other ROMs keep the polled gate.
SC64_PRESENT_FLAGS ?= -DGBA_ASYNC_PRESENT

# Size cached route fields to the active map, so the fixed route bank holds
# more of them on maps smaller than 128 by 128 (56 on the 64-by-64 custom
# maps instead of 14). Answers are unchanged; fewer fields are rebuilt.
# Empty this to compare a canonical ROM with the fixed 14-field layout.
CANONICAL_ROUTE_STRIDE_FLAGS ?= -DSC64_ROUTE_ACTIVE_STRIDE

# Optional differential of the per-profile tick cache, the 32-bit flight and
# position paths against retained earlier code, over every prepared profile.
.PHONY: test-sc64-weapon-source-ticks-bank
test-sc64-weapon-source-ticks-bank: | build
	@if test ! -f "$(WEAPON_SOURCE_HEADER)"; then \
		echo "SKIP: prepared weapon source bank not present at $(WEAPON_SOURCE_HEADER)"; \
	else \
		$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-unused-function -I"$(WEAPON_SOURCE_DIR)" -Istarcraft tests/sc64_weapon_source_ticks_bank.c -o build/test-sc64-weapon-source-ticks-bank && \
		./build/test-sc64-weapon-source-ticks-bank; \
	fi
