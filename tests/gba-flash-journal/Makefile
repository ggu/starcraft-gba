# SPDX-License-Identifier: GPL-3.0-or-later
CLANG ?= clang
.PHONY: test-gba-flash-journal
test-gba-flash-journal:
	@mkdir -p build/flash-journal
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/gba_flash_journal.c -o build/flash-journal/model
	./build/flash-journal/model
