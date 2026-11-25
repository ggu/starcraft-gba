# SPDX-License-Identifier: GPL-3.0-or-later
# Asset-free check for the ares capture spike helpers
# (experiments/ares-headless/capture_view.h); no ROM or emulator needed.
ARES_CAPTURE_CXX ?= clang++
.PHONY: test-ares-capture-view
build/test-ares-capture-view: tests/sc64-ares-observer/capture_view.cpp experiments/ares-headless/capture_view.h mk/ares-capture/Makefile | build
	$(ARES_CAPTURE_CXX) -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
test-ares-capture-view: build/test-ares-capture-view
	./build/test-ares-capture-view
test: test-ares-capture-view
