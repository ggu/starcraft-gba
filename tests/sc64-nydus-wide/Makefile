# Optional prepared-input composition check. No asset preparation is implied.
NYDUS_WIDE_OUT ?= build/nydus-wide
NYDUS_WIDE_COVER_DIR ?= .local/hardware-feedback/verified-cover
.PHONY: test-sc64-nydus-wide
test-sc64-nydus-wide:
	@mkdir -p $(NYDUS_WIDE_OUT)
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -I$(NYDUS_WIDE_COVER_DIR) \
	  $(BC_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) tests/sc64_nydus_wide_save.c tests/sc64_yamato_host_sram_shim.c \
	  $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(NYDUS_WIDE_OUT)/test
	$(NYDUS_WIDE_OUT)/test write $(NYDUS_WIDE_OUT)/wide.sav
	$(NYDUS_WIDE_OUT)/test read $(NYDUS_WIDE_OUT)/wide.sav
