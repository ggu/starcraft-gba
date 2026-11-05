# SPDX-License-Identifier: GPL-3.0-or-later
# Optional only: callers provide an independently prepared private bank.
# No product enables this capability by including this fragment.
SC64_SHARED_COCOON_BANK_DIR ?= .local/sc64/shared-cocoon/generated
SC64_SHARED_COCOON_RENDER_FLAGS = -DSC64_SHARED_COCOON_RENDER -I$(SC64_SHARED_COCOON_BANK_DIR)
SC64_SHARED_COCOON_RENDER_INPUTS = $(SC64_SHARED_COCOON_BANK_DIR)/sc64_shared_cocoon_bank_generated.h
