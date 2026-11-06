# SPDX-License-Identifier: GPL-3.0-or-later
CLANG ?= clang
.PHONY: test-gba-flash1m-integrated
test-gba-flash1m-integrated:
	@mkdir -p build/flash1m-integrated
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/gba_flash1m_model.c common/gba_flash1m.c -o build/flash1m-integrated/transport
	./build/flash1m-integrated/transport
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/gba_flash1m_journal_model.c common/gba_flash1m.c -o build/flash1m-integrated/journal
	./build/flash1m-integrated/journal
