# Asset-free route-dispatch oracle for the exact monotone shortest-step probe.
ROUTE_MONOTONE_TEST_INPUTS = tests/sc64_route_monotone.c starcraft/sc64_routes.h \
	starcraft/sc64_depot_goal.h starcraft/sc64_route_monotone_probe.h starcraft/sc64_map_math.h \
	Makefile mk/monotone-routes/Makefile
ROUTE_MONOTONE_TEST_BASE = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -I.
build/test-sc64-route-monotone: $(ROUTE_MONOTONE_TEST_INPUTS) | build
	$(CLANG) $(ROUTE_MONOTONE_TEST_BASE) $< -o $@
build/test-sc64-route-monotone-active: $(ROUTE_MONOTONE_TEST_INPUTS) | build
	$(CLANG) $(ROUTE_MONOTONE_TEST_BASE) -DTEST_ACTIVE $< -o $@
build/test-sc64-route-monotone-partial: $(ROUTE_MONOTONE_TEST_INPUTS) | build
	$(CLANG) $(ROUTE_MONOTONE_TEST_BASE) -DTEST_PARTIAL $< -o $@
build/test-sc64-route-witness: $(ROUTE_MONOTONE_TEST_INPUTS) | build
	$(CLANG) $(ROUTE_MONOTONE_TEST_BASE) -DTEST_WITNESS $< -o $@
build/test-sc64-route-witness-active: $(ROUTE_MONOTONE_TEST_INPUTS) | build
	$(CLANG) $(ROUTE_MONOTONE_TEST_BASE) -DTEST_WITNESS -DTEST_ACTIVE $< -o $@
.PHONY: test-sc64-route-monotone
test-sc64-route-monotone: build/test-sc64-route-monotone build/test-sc64-route-monotone-active build/test-sc64-route-monotone-partial build/test-sc64-route-witness build/test-sc64-route-witness-active
	./build/test-sc64-route-monotone
	./build/test-sc64-route-monotone-active
	./build/test-sc64-route-monotone-partial
	./build/test-sc64-route-witness
	./build/test-sc64-route-witness-active
test: test-sc64-route-monotone
