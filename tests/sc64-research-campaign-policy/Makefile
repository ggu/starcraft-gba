# SPDX-License-Identifier: GPL-3.0-or-later
build/test-sc64-research-campaign-policy: tests/sc64_research_campaign_policy.c starcraft/sc64_research_campaign_policy.h starcraft/sc64_research_human_binding.h starcraft/sc64_protoss_research.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-research-campaign-policy
test-sc64-research-campaign-policy: build/test-sc64-research-campaign-policy
	./build/test-sc64-research-campaign-policy
test: test-sc64-research-campaign-policy

# Optional: SC64_PR_POLICY_DIR must contain the authenticated private generated
# header; prepared P10 production inputs are also required.
build/test-sc64-research-campaign-policy-game: tests/sc64_research_campaign_policy_game.c starcraft/sc64_research_campaign_policy.h $(STARCRAFT) $(wildcard starcraft/*.h) | build
	$(CLANG) $(P8_HOST_FLAGS) -I$(SC64_PR_COVER_DIR) -I$(SC64_PR_POLICY_DIR) $(PROTOSS_P10_BASE_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-research-campaign-policy-game
test-sc64-research-campaign-policy-game: build/test-sc64-research-campaign-policy-game
	./build/test-sc64-research-campaign-policy-game
