.PHONY: test-stage-save-auth
CC ?= cc
EGG_TEST_BIN_DIR ?= /tmp
test-stage-save-auth:
	$(CC) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/sc64_stage_save_auth.c -o $(EGG_TEST_BIN_DIR)/sc64-stage-save-auth
	$(EGG_TEST_BIN_DIR)/sc64-stage-save-auth
