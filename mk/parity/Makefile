# SPDX-License-Identifier: GPL-3.0-or-later
# Parity harness rules (see tools/sc64_parity_tables.py). Included from
# mk/padded-impact-save/Makefile because the top-level Makefile is at its size
# limit. These rules read private ROM inputs when present and write derived
# output only under $(SC64_PARITY_HOME) (default: .local/parity in the main
# checkout); they print a skip line and succeed when the inputs are absent.

.PHONY: parity-tables parity parity-content parity-l0
parity-tables:
	$(PYTHON) tools/sc64_parity_tables.py

# The developer loop (./dev parity): the table differ plus every captured N64
# scenario with a golden file under $(SC64_PARITY_HOME)/golden, judged on the
# GBA (tools/sc64_parity.py, docs/PARITY-HARNESS.md). Skips cleanly with no
# private inputs.
parity:
	$(PYTHON) tools/sc64_parity.py


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
PARITY_INPUTS.protoss-p10 = $(PROTOSS_P10_INPUTS) $(P10_HATCHERY_CONFIG) $(P10_BUILDING_WORD_HEADER) \
	build/canonical-target-index-config $(CANONICAL_TARGET_INDEX_CONFIG) build/terrain-dirty-capture-config \
	$(SHARED_SIGHT_HEADERS) \
	$(if $(filter 1,$(SC64_STATIC_MINERAL_WORDS)),$(if $(filter build/starcraft-protoss-p10.elf,$(SC64_STATIC_MINERAL_WORDS_ELFS)),$(SC64_STATIC_MINERAL_WORDS_HEADER)))
PARITY_SOURCES.terran = $(filter-out starcraft/game.c,$(TERRAN_ARM_SOURCES))
PARITY_SOURCES.zerg-z10 = $(filter-out starcraft/game.c,$(Z10_ARM_SOURCES))
PARITY_SOURCES.protoss-p10 = $(filter-out starcraft/game.c,$(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES))

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
# The command is read once per build (<binary>.cmd.snap), used for every pass,
# and recorded in .inputs together with the executable's SHA-256.
define PARITY_BINARY
$(PARITY_DIR)/$(1)-$(2)-$(3).cmd: $(PARITY_DIR)/$(1).romflags parity-force | build
	@printf '%s %s %s %s\n' '$$(CLANG) $$(PARITY_CFLAGS_$(2))' "$$$$(cat $$<)" '$$(PARITY_CFLAGS_$(3))' \
	  'tests/sc64_parity_run.c common/gba.c $$(PARITY_SOURCES.$(1))' > $$@.new
	@cmp -s $$@.new $$@ && rm $$@.new || mv $$@.new $$@
$(PARITY_DIR)/$(1)-$(2)-$(3): $(PARITY_DIR)/$(1)-$(2)-$(3).cmd $$(PARITY_RUN_DEPS) $$(PARITY_INPUTS.$(1)) \
	$$(PARITY_SOURCES.$(1)) $$(if $$(filter 4,$$(words $$(wildcard $(PARITY_DIR)/$(1)-$(2)-$(3).d \
	  $(PARITY_DIR)/$(1)-$(2)-$(3).mmd $(PARITY_DIR)/$(1)-$(2)-$(3).flags \
	  $(PARITY_DIR)/$(1)-$(2)-$(3).inputs))),,parity-force) | build
	cp $$< $$@.cmd.snap
	eval "$$$$(cat $$@.cmd.snap) -M -MP -MT $$@" > $$@.d.tmp
	eval "$$$$(cat $$@.cmd.snap) -MM -MT $$@" > $$@.mmd.tmp
	$$(PYTHON) tools/sc64_parity_selfgate.py inputs $$@.d.tmp --user-deps $$@.mmd.tmp \
	  --cmd-file $$@.cmd.snap > $$@.inputs.pre
	eval "$$$$(cat $$@.cmd.snap) -o $$@.tmp"
	eval "$$$$(cat $$@.cmd.snap) -M -MP -MT $$@" > $$@.d.post
	cmp -s $$@.d.tmp $$@.d.post || { echo "$$@: dependencies changed while compiling; rebuild" >&2; exit 1; }
	$$(PYTHON) tools/sc64_parity_selfgate.py inputs $$@.d.tmp --user-deps $$@.mmd.tmp \
	  --cmd-file $$@.cmd.snap --binary $$@.tmp --expect $$@.inputs.pre > $$@.inputs.tmp
	rm $$@.inputs.pre $$@.d.post
	mv $$@.d.tmp $$@.d
	mv $$@.mmd.tmp $$@.mmd
	mv $$@.inputs.tmp $$@.inputs
	mv $$@.cmd.snap $$@.flags
	mv $$@.tmp $$@
PARITY_BINARIES += $(PARITY_DIR)/$(1)-$(2)-$(3)
endef

# $(1) profile, $(2) ROM ELF
#
# <profile>.romflags is rewritten only when the ROM's options change, so a ROM
# flag change rebuilds the parity binaries.
define PARITY_ROMFLAGS
$(PARITY_DIR)/$(1).romflags: parity-force | build
	@mkdir -p $(PARITY_DIR)
	@$$(PYTHON) tools/sc64_parity_selfgate.py rom-flags $(2) --make '$$(PARITY_SUBMAKE)' --out $$@
endef

# $(1) profile, $(2) ROM ELF, $(3) scenario
define PARITY_PROFILE
$(call PARITY_ROMFLAGS,$(1),$(2))
$(call PARITY_BINARY,$(1),asan,trace)
$(call PARITY_BINARY,$(1),asan,plain)
$(call PARITY_BINARY,$(1),fast,trace)
$(call PARITY_BINARY,$(1),fast,plain)
PARITY_PROFILES += $(1):$(3)
endef

$(eval $(call PARITY_PROFILE,terran,build/starcraft-terran.elf,t1))
$(eval $(call PARITY_PROFILE,zerg-z10,build/starcraft-zerg-z10.elf,skirmish))
# The self-gates and the bench cover the two profiles above. Protoss P10 has
# only the census binary (the census needs one release profile per race).
PARITY_GATE_BINARIES := $(PARITY_BINARIES)
$(eval $(call PARITY_ROMFLAGS,protoss-p10,build/starcraft-protoss-p10.elf))
$(eval $(call PARITY_BINARY,protoss-p10,fast,trace))
PARITY_CENSUS_BINARIES := $(PARITY_DIR)/terran-fast-trace $(PARITY_DIR)/zerg-z10-fast-trace \
	$(PARITY_DIR)/protoss-p10-fast-trace

-include $(wildcard $(PARITY_DIR)/*.d)

.PHONY: test-sc64-parity bench-sc64-parity parity-force
parity-force:
test-sc64-parity: $(filter %-asan-trace %-asan-plain,$(PARITY_BINARIES))
	$(PYTHON) tools/sc64_parity_selfgate.py gate --dir $(PARITY_DIR) --kind asan $(addprefix --profile ,$(PARITY_PROFILES))
bench-sc64-parity: $(PARITY_GATE_BINARIES)
	$(PYTHON) tools/sc64_parity_selfgate.py bench --dir $(PARITY_DIR) $(addprefix --profile ,$(PARITY_PROFILES))

# Opening censuses, iscript sequences and the sound id map
# (tools/sc64_parity_content.py). The censuses launch every catalog mission in
# the host build of its release profile; those binaries are prerequisites only
# when this checkout has the private ROM and extractor (it also needs its own
# prepared inputs to build them), and the tool reports a census as absent
# when its binary is. parity-l0 prints the whole offline layer.
# The census binaries are built by a sub-make, as tools/sc64_parity.py does,
# with the prepared Z10 descriptor when the caller gave no other, so a plain
# `make parity-content` works in a prepared checkout. Not $(MAKE): a recipe
# naming it runs even under an outer make -n.
# The ROM and extractor are found as the tool finds them: in this checkout or
# in the main one (recursive variables, so git runs only for this target).
PARITY_MAIN = $(patsubst %/.git,%,$(shell git rev-parse --path-format=absolute --git-common-dir 2>/dev/null))
PARITY_FIND = $(firstword $(wildcard $(1) $(PARITY_MAIN)/$(1)))
PARITY_CENSUS_READY = $(and $(call PARITY_FIND,.local/sc64/rom/StarCraft64-USA.z64),$(call PARITY_FIND,references/sc64-maps/extract_sc64_maps.py))
PARITY_Z8_PREPARED := .local/build/z9-inputs/z8-descriptor.json
PARITY_CENSUS_Z8 := $(if $(and $(filter file default,$(origin Z8_DESCRIPTOR)),$(wildcard $(PARITY_Z8_PREPARED))),Z8_DESCRIPTOR=$(PARITY_Z8_PREPARED),Z8_DESCRIPTOR=$(Z8_DESCRIPTOR))
# Each profile is built on its own; one that fails to build is passed as
# --absent-profile, so its censuses read absent and the rest of the report is
# still written (never from an older binary).
PARITY_CENSUS_PROFILES := terran zerg-z10 protoss-p10
parity-content:
	@failed=; \
	$(if $(PARITY_CENSUS_READY),for p in $(PARITY_CENSUS_PROFILES); do \
	  $(PARITY_SUBMAKE) -s $(PARITY_CENSUS_Z8) $(PARITY_DIR)/$$p-fast-trace || failed="$$failed --absent-profile $$p"; \
	done;) \
	$(PYTHON) tools/sc64_parity_content.py $(if $(PARITY_CENSUS_READY),--binaries $(PARITY_DIR)) $$failed
parity-l0: parity-tables parity-content
