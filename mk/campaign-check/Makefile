# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile; not a standalone makefile. Run from the
# repository root.
#
# The one rule that wins each of the 30 campaign missions with a scripted
# controller that presses ordinary buttons through game_update() (HOST only).
# `make check-campaigns` runs all 30 and prints a table (tools/sc64_campaign_check.py);
# `make check-campaign-<id>` runs one (t1..t10, z1..z10, p1..p10).
#
#   Terran   T1..T10  the full-profile (TERRAN_FULL_INPUTS) controllers.
#   Zerg     Z1..Z8   the controllers linked with the Z9 canonical flags;
#            Z9, Z10  their own flag vectors and controllers.
#   Protoss  P1..P10  the P10 flags the P10 ROM ships: controller, key tape and a
#                     controller-free host replay of that tape, which must win.
#
# The older Terran, Zerg and Protoss rules that run these sources under earlier
# stage flags are listed in docs/VALIDATION.md; they are not what proves a win.
CAMPAIGN_CHECK_ARGS ?=
CC_DIR := .local/sc64/campaign-check
CC_TERRAN_DIR := .local/sc64/terran-late
CC_ZERG_DIR := .local/sc64/z9-carried

.PHONY: check-campaigns
check-campaigns:
	$(PYTHON) tools/sc64_campaign_check.py $(CAMPAIGN_CHECK_ARGS)

# Terran: $(1) mission id, $(2) binary, $(3) arguments.
define CC_TERRAN
.PHONY: check-campaign-$(1)
check-campaign-$(1): build/$(2)
	mkdir -p $(CC_TERRAN_DIR)
	./build/$(2) $(3) --trace $(CC_TERRAN_DIR)/full-$(1)-controller.json
endef
$(eval $(call CC_TERRAN,t1,test-sc64-full-t1-run,))
$(eval $(call CC_TERRAN,t2,test-sc64-full-t2-run,))
$(eval $(call CC_TERRAN,t4,test-sc64-full-t4-run,--limit 30000))
$(eval $(call CC_TERRAN,t5,test-sc64-full-t5-run,--limit 200000))
$(eval $(call CC_TERRAN,t6,test-sc64-full-t6-run,--limit 30000))
$(eval $(call CC_TERRAN,t7,test-sc64-t7-campaign-run,--limit 60000))
$(eval $(call CC_TERRAN,t8,test-sc64-t8-campaign-run,--limit 200000))
$(eval $(call CC_TERRAN,t9,test-sc64-t9-campaign-run,--limit 500000))
$(eval $(call CC_TERRAN,t10,test-sc64-t10-campaign-run,--limit 100000))
# T3 takes no trace argument.
.PHONY: check-campaign-t3
check-campaign-t3: build/test-sc64-full-t3-run
	./build/test-sc64-full-t3-run

# Zerg Z1..Z8 under the Z9 vector. $(1) id, $(2) binary suffix, $(3) arguments.
define CC_ZERG
.PHONY: check-campaign-$(1)
check-campaign-$(1): build/test-sc64-z9-carried-$(2)
	mkdir -p $(CC_ZERG_DIR)
	./build/test-sc64-z9-carried-$(2) $(3)
endef
$(eval $(call CC_ZERG,z1,z1,--trace $(CC_ZERG_DIR)/z1.json))
$(eval $(call CC_ZERG,z2,z1z2,--controller 1 --trace $(CC_ZERG_DIR)/z2.json))
$(eval $(call CC_ZERG,z3,z3,--policy hybrid --trace $(CC_ZERG_DIR)/z3.json))
$(eval $(call CC_ZERG,z4,z4,--limit 150000 --trace $(CC_ZERG_DIR)/z4.json))
$(eval $(call CC_ZERG,z5,z5,--limit 60000 --trace $(CC_ZERG_DIR)/z5-diagnostic.json --attempt $(CC_ZERG_DIR)/z5-attempt.json --verified $(CC_ZERG_DIR)/z5.json))
$(eval $(call CC_ZERG,z7,z7,$(ZERG_Z7_CONTROLLER_POLICY) --limit $(ZERG_Z7_CONTROLLER_LIMIT) --trace $(CC_ZERG_DIR)/z7.json))
$(eval $(call CC_ZERG,z8,z8,$(CC_ZERG_DIR)/z8.jsonl))
# Z6 prints its verdict only to the trace; show the winning event.
.PHONY: check-campaign-z6
check-campaign-z6: build/test-sc64-z9-carried-z6
	mkdir -p $(CC_ZERG_DIR) $(CC_DIR)
	./build/test-sc64-z9-carried-z6 --trace $(CC_ZERG_DIR)/z6.json > $(CC_DIR)/z6.log 2>&1 || { tail -n 20 $(CC_DIR)/z6.log; exit 1; }
	grep -o '"kind":"won","host_tick":[0-9]*,"frame":[0-9]*' $(CC_ZERG_DIR)/z6.json

# Z9 and Z10: write the controller trace beside the others, always run.
define CC_ZLATE
.PHONY: check-campaign-$(1)
check-campaign-$(1): build/test-sc64-$(1)-campaign-run
	mkdir -p $(CC_DIR)
	./build/test-sc64-$(1)-campaign-run $(CC_DIR)/$(1)-controller.jsonl
endef
$(eval $(call CC_ZLATE,z9))
$(eval $(call CC_ZLATE,z10))

# Protoss P1..P10: the result is the controller-free replay of the controller's
# own key tape under the P10 flags (build/protoss-px/<id>/host-full.txt, from
# mk/protoss-carried/Makefile); it must report won=1.
define CC_PX
.PHONY: check-campaign-$(1)
check-campaign-$(1): $(PX_DIR)/$(1)/host-full.txt
	grep ' won=1 ' $(PX_DIR)/$(1)/host-full.txt | tail -n 1
endef
$(foreach p,p1 p2 p3 p4 p5 p6 p7 p8 p9 p10,$(eval $(call CC_PX,$(p))))
