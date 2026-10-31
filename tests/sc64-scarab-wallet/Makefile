# Asset-free typed wallet checks; campaign transactions need prepared inputs.
build/test-sc64-scarab-wallet: tests/sc64_scarab_wallet.c starcraft/sc64_scarab_wallet.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-scarab-wallet
test-sc64-scarab-wallet: build/test-sc64-scarab-wallet
	./build/test-sc64-scarab-wallet
test: test-sc64-scarab-wallet

# These explicit fixtures consume prepared inputs; regenerate only their own
# actor-fact table to match the additional experimental Scarab enum.
SCARAB_WALLET_P8_FLAGS = $(filter-out -DSC64_FACT_TABLES -I$(BC_P7_FACT_DIR) -I$(P7_FACT_V2_DIR) -I$(P8_FACT_V2_DIR),$(PROTOSS_P8_FLAGS))
define SCARAB_SOURCE_WALLET_FIXTURE
.PHONY: test-scarab-source-wallet-$(1)
test-scarab-source-wallet-$(1): $$(CANONICAL_LOOKUP_INPUTS)
	@set -eu; \
	mkdir -p build/generated/scarab-source-wallet; \
	run=$$$$(mktemp -d build/generated/scarab-source-wallet/$(1).XXXXXX); \
	$$(CLANG) $$(SHARED_MECHANICS_HOST_FLAGS) -Wno-unused-function -Istarcraft -DSC64_SCARAB_RUNTIME $$($(2)) tools/sc64_fact_tables_gen.c -o "$$$$run/facts"; \
	"$$$$run/facts" > "$$$$run/sc64_fact_tables_generated.h"; \
	$$(CLANG) $$(SHARED_MECHANICS_HOST_FLAGS) -Wno-unused-function -Wno-unused-variable -DSC64_SCARAB_RUNTIME $$($(2)) -DSC64_FACT_TABLES -I"$$$$run" tests/sc64_scarab_source_wallet_game.c common/gba.c $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) -o "$$$$run/game"; \
	"$$$$run/game"
endef
$(eval $(call SCARAB_SOURCE_WALLET_FIXTURE,p7,SCARAB_P7_BASE_FLAGS))
$(eval $(call SCARAB_SOURCE_WALLET_FIXTURE,p8,SCARAB_WALLET_P8_FLAGS))
.PHONY: test-scarab-source-wallet
test-scarab-source-wallet: test-scarab-source-wallet-p7 test-scarab-source-wallet-p8
