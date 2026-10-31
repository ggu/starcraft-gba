# Asset-free typed wallet checks; campaign transactions need prepared inputs.
build/test-sc64-scarab-wallet: tests/sc64_scarab_wallet.c starcraft/sc64_scarab_wallet.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-scarab-wallet
test-sc64-scarab-wallet: build/test-sc64-scarab-wallet
	./build/test-sc64-scarab-wallet
test: test-sc64-scarab-wallet
