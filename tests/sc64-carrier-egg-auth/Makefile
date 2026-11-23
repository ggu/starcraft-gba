# SPDX-License-Identifier: GPL-3.0-or-later
.PHONY: test-carrier-egg-auth
CC ?= cc
TEST_OUT ?= /tmp
TEST_FLAGS = -std=c11 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined

test-carrier-egg-auth:
	$(CC) $(TEST_FLAGS) tests/sc64_carrier_egg_callback_auth.c -o $(TEST_OUT)/carrier-auth-legacy
	$(CC) $(TEST_FLAGS) -DTEST_EGG_CONTEXT tests/sc64_carrier_egg_callback_auth.c -o $(TEST_OUT)/carrier-auth-egg
	$(TEST_OUT)/carrier-auth-legacy
	$(TEST_OUT)/carrier-auth-egg
