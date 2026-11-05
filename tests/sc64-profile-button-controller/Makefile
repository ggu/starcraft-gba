# HOST-only; include the profile-journal Makefile for its qualified flag sets.
PROFILE_BUTTON_OUT ?= /tmp/sc64-profile-button-controller
PROFILE_BUTTON_FACT_ROOT ?= .local/profile-button-full-facts
PROFILE_BUTTON_P10 = -DSC64_GUARDIAN_FULL_FACTS_STAGE $(patsubst -I$(P10_FACT_V2_DIR),-I$(PROFILE_BUTTON_FACT_ROOT)/flight-p10,$(PROTOSS_P10_FLAGS))

define PROFILE_BUTTON_CONTROLLER
.PHONY: profile-button-$(1)
profile-button-$(1):
	mkdir -p $(PROFILE_BUTTON_OUT)/$(1)
	$(CLANG) $(PROFILE_HOST) $(PROFILE_BC) $(PROFILE_STAGE) -DSC64_SCARAB_TECH_PATH_HOST_PROBE $(2) tests/sc64_scarab_tech_controller.c $(PROFILE_EXTRA) -o $(PROFILE_BUTTON_OUT)/$(1)/controller
	$(PROFILE_BUTTON_OUT)/$(1)/controller write $(PROFILE_BUTTON_OUT)/$(1)/controller.sram
	$(PROFILE_BUTTON_OUT)/$(1)/controller read $(PROFILE_BUTTON_OUT)/$(1)/controller.sram
	$(PROFILE_BUTTON_OUT)/$(1)/controller launch-check $(PROFILE_BUTTON_OUT)/$(1)/controller.sram
	$(PROFILE_BUTTON_OUT)/$(1)/controller attack $(PROFILE_BUTTON_OUT)/$(1)/controller.sram $(PROFILE_BUTTON_OUT)/$(1)/impact.sram
	$(PROFILE_BUTTON_OUT)/$(1)/controller resume $(PROFILE_BUTTON_OUT)/$(1)/impact.sram
endef
$(eval $(call PROFILE_BUTTON_CONTROLLER,z7,$(PROFILE_Z7)))
$(eval $(call PROFILE_BUTTON_CONTROLLER,p10,$(PROFILE_BUTTON_P10)))
