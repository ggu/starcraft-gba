/* Journal tag byte allocation. All historical feature selectors operate in
 * bits 24..31. Research reserves byte 0x44 in bits 16..23; the original
 * family uses 0x43. Changing this contract requires a compatibility audit.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SAVE_TAG_NAMESPACE_H
#define SC64_SAVE_TAG_NAMESPACE_H
#define SC64_SAVE_TAG_NAMESPACE_MASK 0x00ff0000u
#define SC64_SAVE_TAG_BASE_NAMESPACE 0x00430000u
#define SC64_SAVE_TAG_RESEARCH_NAMESPACE 0x00440000u
/* Reserved solely for the explicit HOST active-job experiment. */
#define SC64_SAVE_TAG_RESEARCH_JOBS_NAMESPACE 0x00450000u
/* Reserved for the HOST-only campaign job experiment. 0x46 is used by the
 * independent Flash journal prototype and is deliberately left separate. */
#define SC64_SAVE_TAG_CAMPAIGN_JOBS_NAMESPACE 0x00470000u
/* Private HOST Flash/Q8 experiment; never accepted by ordinary SRAM builds. */
#define SC64_SAVE_TAG_Q8_FLASH_NAMESPACE 0x00480000u
/* Private P1 HOST controller/Flash experiment; no ordinary reader accepts it. */
#define SC64_SAVE_TAG_CONTROLLER_FLASH_NAMESPACE 0x00490000u
/* Compound HOST-only envelope. Existing 0x48 and 0x49 remain unchanged. */
#define SC64_SAVE_TAG_COMPOUND_FLASH_NAMESPACE 0x004a0000u
#define SC64_SAVE_TAG_COMPOUND_FLASH(tag) (((tag)&~SC64_SAVE_TAG_NAMESPACE_MASK)|SC64_SAVE_TAG_COMPOUND_FLASH_NAMESPACE)
#define SC64_SAVE_TAG_CONTROLLER_FLASH(tag) (((tag)&~SC64_SAVE_TAG_NAMESPACE_MASK)|SC64_SAVE_TAG_CONTROLLER_FLASH_NAMESPACE)
#define SC64_SAVE_TAG_BASE_VALID(tag) (((tag)&SC64_SAVE_TAG_NAMESPACE_MASK)==SC64_SAVE_TAG_BASE_NAMESPACE)
#define SC64_SAVE_TAG_RESEARCH(tag) (((tag)&~SC64_SAVE_TAG_NAMESPACE_MASK)|SC64_SAVE_TAG_RESEARCH_NAMESPACE)
#define SC64_SAVE_TAG_RESEARCH_JOBS(tag) (((tag)&~SC64_SAVE_TAG_NAMESPACE_MASK)|SC64_SAVE_TAG_RESEARCH_JOBS_NAMESPACE)
#define SC64_SAVE_TAG_CAMPAIGN_JOBS(tag) (((tag)&~SC64_SAVE_TAG_NAMESPACE_MASK)|SC64_SAVE_TAG_CAMPAIGN_JOBS_NAMESPACE)
#define SC64_SAVE_TAG_Q8_FLASH(tag) (((tag)&~SC64_SAVE_TAG_NAMESPACE_MASK)|SC64_SAVE_TAG_Q8_FLASH_NAMESPACE)
#endif
