# SPDX-License-Identifier: GPL-3.0-or-later
# GBA side of the N64 parity harness (WP2): host-only command injector and
# trace emitter (tests/sc64_parity_inject.h, tests/sc64_parity_trace.h) and a
# runner that proves them on two prepared profiles. Nothing here builds a ROM.
#
#   make test-sc64-parity         four self-gates on both profiles (ASan)
#   make bench-sc64-parity        host throughput, -O2 unsanitized and ASan
#
# Zerg Z10 needs Z8_DESCRIPTOR=.local/build/z9-inputs/z8-descriptor.json, as
# its ROM does.
#
# Each profile compiles exactly the simulation its shipped ROM runs. The
# preprocessor options (-D, -U, -I, -include) come from the ROM rule itself:
# <profile>.romflags is parsed from a dry run of the ROM ELF's recipe, so the
# ROM's flag vector, its target-specific CFLAGS additions and its probe flags
# all carry over, and nothing has to be kept in step by hand. Host flags then
# replace the ARM code-generation options.
PARITY_DIR := build/parity
# Not $(MAKE): a recipe naming that variable runs even under an outer make -n.
PARITY_SUBMAKE := $(MAKE)
PARITY_RUN_DEPS := tests/sc64_parity_run.c tests/sc64_parity_inject.h tests/sc64_parity_trace.h \
	tests/sc64_route_purity.h $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c $(COMMON_HEADERS) \
	mk/parity/Makefile
PARITY_WARN := -std=c11 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DGBA_HOST -Icommon
PARITY_CFLAGS_asan := $(PARITY_WARN) -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined
PARITY_CFLAGS_fast := -std=c11 -O2 -w -D_FORTIFY_SOURCE=0 -DGBA_HOST -Icommon
PARITY_CFLAGS_trace := -DSC64_PARITY_TRACE
PARITY_CFLAGS_plain :=

# Generated prerequisites of the ROM ELFs, including those that rules in
# Makefile, mk/combat-perf and mk/fog add, so a clean tree generates them
# before the first compile.
PARITY_INPUTS.terran = $(TERRAN_FULL_INPUTS) $(TERRAN_BUILDING_WORD_INPUTS) $(TANK_SOURCE_HEADER) \
	$(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) $(BC_INPUTS) build/canonical-target-index-config \
	$(CANONICAL_TARGET_INDEX_CONFIG) build/terrain-dirty-capture-config build/terran-normal-config \
	$(TERRAN_NORMAL_CONFIG) $(SHARED_SIGHT_HEADERS) \
	$(if $(filter 1,$(SC64_STATIC_MINERAL_WORDS)),$(if $(filter build/starcraft-terran.elf,$(SC64_STATIC_MINERAL_WORDS_ELFS)),$(SC64_STATIC_MINERAL_WORDS_HEADER)))
PARITY_INPUTS.zerg-z10 = $(ZERG_Z10_CANONICAL_INPUTS) $(if $(Z10_FACT_FLAGS),$(Z10_FACT_HEADER)) \
	$(if $(Z10_BUILDING_WORDS_FLAGS),$(Z10_SELECTED_BUILDING_WORD_HEADER)) \
	build/z10-building-words-config $(Z10_BUILDING_WORDS_CONFIG) build/z10-facts-config $(Z10_FACT_CONFIG) \
	build/canonical-target-index-config $(CANONICAL_TARGET_INDEX_CONFIG) build/terrain-dirty-capture-config \
	$(if $(filter 1,$(SC64_STATIC_MINERAL_WORDS)),$(if $(filter build/starcraft-zerg-z10.elf,$(SC64_STATIC_MINERAL_WORDS_ELFS)),$(SC64_STATIC_MINERAL_WORDS_HEADER)))
PARITY_SOURCES.terran = $(filter-out starcraft/game.c,$(TERRAN_ARM_SOURCES))
PARITY_SOURCES.zerg-z10 = $(filter-out starcraft/game.c,$(Z10_ARM_SOURCES))

# $(1) profile, $(2) build kind (asan|fast), $(3) emitter (trace|plain)
#
# <binary>.cmd holds the exact compile command as shell text and is rewritten
# only when that text changes. A -M pass lists every dependency of every
# translation unit in <binary>.d, which is loaded below, so a changed generated
# header rebuilds the binary too, whichever include option found it; a -MM pass
# (<binary>.mmd) tells system headers apart. <binary>.inputs holds the SHA-256
# of each generated input, taken before and again after the compile; the build
# fails if any input or the dependency list itself changed in between. After turning an opt-in generated input off
# whose generator source is gone (SC64_STATIC_MINERAL_WORDS), delete the stale
# build/parity/*.d files: they still name that header. The binary is
# published last, so <binary>.d, .inputs and .flags (a copy of the command)
# always describe the executable beside them, and a missing one rebuilds it.
define PARITY_BINARY
$(PARITY_DIR)/$(1)-$(2)-$(3).cmd: $(PARITY_DIR)/$(1).romflags parity-force | build
	@printf '%s %s %s %s\n' '$$(CLANG) $$(PARITY_CFLAGS_$(2))' "$$$$(cat $$<)" '$$(PARITY_CFLAGS_$(3))' \
	  'tests/sc64_parity_run.c common/gba.c $$(PARITY_SOURCES.$(1))' > $$@.new
	@cmp -s $$@.new $$@ && rm $$@.new || mv $$@.new $$@
$(PARITY_DIR)/$(1)-$(2)-$(3): $(PARITY_DIR)/$(1)-$(2)-$(3).cmd $$(PARITY_RUN_DEPS) $$(PARITY_INPUTS.$(1)) \
	$$(PARITY_SOURCES.$(1)) $$(if $$(filter 4,$$(words $$(wildcard $(PARITY_DIR)/$(1)-$(2)-$(3).d \
	  $(PARITY_DIR)/$(1)-$(2)-$(3).mmd $(PARITY_DIR)/$(1)-$(2)-$(3).flags \
	  $(PARITY_DIR)/$(1)-$(2)-$(3).inputs))),,parity-force) | build
	eval "$$$$(cat $$<) -M -MP -MT $$@" > $$@.d.tmp
	eval "$$$$(cat $$<) -MM -MT $$@" > $$@.mmd.tmp
	$$(PYTHON) tools/sc64_parity_selfgate.py inputs $$@.d.tmp --user-deps $$@.mmd.tmp > $$@.inputs.pre
	eval "$$$$(cat $$<) -o $$@.tmp"
	eval "$$$$(cat $$<) -M -MP -MT $$@" > $$@.d.post
	cmp -s $$@.d.tmp $$@.d.post || { echo "$$@: dependencies changed while compiling; rebuild" >&2; exit 1; }
	$$(PYTHON) tools/sc64_parity_selfgate.py inputs $$@.d.tmp --user-deps $$@.mmd.tmp --binary $$@.tmp \
	  --expect $$@.inputs.pre > $$@.inputs.tmp
	rm $$@.inputs.pre $$@.d.post
	mv $$@.d.tmp $$@.d
	mv $$@.mmd.tmp $$@.mmd
	mv $$@.inputs.tmp $$@.inputs
	cp $$< $$@.flags
	mv $$@.tmp $$@
PARITY_BINARIES += $(PARITY_DIR)/$(1)-$(2)-$(3)
endef

# $(1) profile, $(2) ROM ELF, $(3) scenario
#
# <profile>.romflags is rewritten only when the ROM's options change, so a ROM
# flag change rebuilds the parity binaries.
define PARITY_PROFILE
$(PARITY_DIR)/$(1).romflags: parity-force | build
	@mkdir -p $(PARITY_DIR)
	@$$(PYTHON) tools/sc64_parity_selfgate.py rom-flags $(2) --make '$$(PARITY_SUBMAKE)' --out $$@
$(call PARITY_BINARY,$(1),asan,trace)
$(call PARITY_BINARY,$(1),asan,plain)
$(call PARITY_BINARY,$(1),fast,trace)
$(call PARITY_BINARY,$(1),fast,plain)
PARITY_PROFILES += $(1):$(3)
endef

$(eval $(call PARITY_PROFILE,terran,build/starcraft-terran.elf,t1))
$(eval $(call PARITY_PROFILE,zerg-z10,build/starcraft-zerg-z10.elf,skirmish))

-include $(wildcard $(PARITY_DIR)/*.d)

.PHONY: test-sc64-parity bench-sc64-parity parity-force
parity-force:
test-sc64-parity: $(filter %-asan-trace %-asan-plain,$(PARITY_BINARIES))
	$(PYTHON) tools/sc64_parity_selfgate.py gate --dir $(PARITY_DIR) --kind asan $(addprefix --profile ,$(PARITY_PROFILES))
bench-sc64-parity: $(PARITY_BINARIES)
	$(PYTHON) tools/sc64_parity_selfgate.py bench --dir $(PARITY_DIR) $(addprefix --profile ,$(PARITY_PROFILES))
