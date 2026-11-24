ROUTE_CLEAR_PREFIX_INPUTS = tests/sc64_route_monotone.c starcraft/sc64_routes.h \
	starcraft/sc64_route_monotone_probe.h starcraft/sc64_depot_goal.h \
	starcraft/sc64_map_math.h Makefile mk/monotone-clear-prefix/Makefile

build/test-sc64-route-monotone-clear-prefix: $(ROUTE_CLEAR_PREFIX_INPUTS) | build
	$(CLANG) $(ROUTE_MONOTONE_TEST_BASE) -DTEST_ACTIVE -DTEST_CLEAR_PREFIX $< -o $@

.PHONY: test-sc64-route-monotone-clear-prefix
test-sc64-route-monotone-clear-prefix: build/test-sc64-route-monotone-clear-prefix
	./build/test-sc64-route-monotone-clear-prefix
test: test-sc64-route-monotone-clear-prefix
