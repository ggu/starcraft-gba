# Asset-free route-cache policy and full-field fixtures.
# Optional route policies stay off in ROM defaults. Exercise each independently
# and together, with and without the borrowed visitation grid.
ROUTE_POLICY_TEST_FLAGS = -Icommon -DGBA_HOST -DSC64_RACES -DSC64_LARGE_MAPS -DSC64_ROUTE_PACK3
define route_policy_fixture
build/test-sc64-route-policy-$(1): tests/race_routes_partial.c starcraft/sc64_routes.h starcraft/sc64_depot_goal.h starcraft/sc64_bitgrid.h common/gba.h Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -I. $$(ROUTE_POLICY_TEST_FLAGS) $(2) $$< -o $$@
endef
$(eval $(call route_policy_fixture,baseline,))
$(eval $(call route_policy_fixture,visit,-DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,partial,-DSC64_ROUTE_PARTIAL_FLOOD))
$(eval $(call route_policy_fixture,frequency,-DSC64_ROUTE_DECAY_FREQ))
$(eval $(call route_policy_fixture,partial-frequency,-DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ))
$(eval $(call route_policy_fixture,partial-visit,-DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,frequency-visit,-DSC64_ROUTE_DECAY_FREQ -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,partial-frequency-visit,-DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,split-baseline,-DSC64_ROUTE_FLOOD_SPLIT))
$(eval $(call route_policy_fixture,split-visit,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,split-visit-newcell,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_VISIT_GRID -DSC64_ROUTE_WORD_CLEAR -DSC64_ROUTE_NEW_CELL))
$(eval $(call route_policy_fixture,split-partial,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_PARTIAL_FLOOD))
$(eval $(call route_policy_fixture,split-frequency,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_DECAY_FREQ))
$(eval $(call route_policy_fixture,split-partial-frequency,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ))
$(eval $(call route_policy_fixture,split-partial-visit,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,split-frequency-visit,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_DECAY_FREQ -DSC64_ROUTE_VISIT_GRID))
$(eval $(call route_policy_fixture,split-partial-frequency-visit,-DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_PARTIAL_FLOOD -DSC64_ROUTE_DECAY_FREQ -DSC64_ROUTE_VISIT_GRID))
ROUTE_POLICY_VARIANTS = baseline visit partial frequency partial-frequency partial-visit frequency-visit partial-frequency-visit
.PHONY: test-sc64-route-policies
test-sc64-route-policies: $(addprefix build/test-sc64-route-policy-,$(ROUTE_POLICY_VARIANTS))
	@$(foreach variant,$(ROUTE_POLICY_VARIANTS),./build/test-sc64-route-policy-$(variant) &&) true
test-starcraft: test-sc64-route-policies
.PHONY: test-sc64-route-flood-split
test-sc64-route-flood-split: $(addprefix build/test-sc64-route-policy-split-,$(ROUTE_POLICY_VARIANTS)) build/test-sc64-route-policy-split-visit-newcell
	@$(foreach variant,$(ROUTE_POLICY_VARIANTS),./build/test-sc64-route-policy-split-$(variant) &&) ./build/test-sc64-route-policy-split-visit-newcell
test-starcraft: test-sc64-route-flood-split

