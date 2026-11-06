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
#define SC64_SAVE_TAG_BASE_VALID(tag) (((tag)&SC64_SAVE_TAG_NAMESPACE_MASK)==SC64_SAVE_TAG_BASE_NAMESPACE)
#define SC64_SAVE_TAG_RESEARCH(tag) (((tag)&~SC64_SAVE_TAG_NAMESPACE_MASK)|SC64_SAVE_TAG_RESEARCH_NAMESPACE)
#define SC64_SAVE_TAG_RESEARCH_JOBS(tag) (((tag)&~SC64_SAVE_TAG_NAMESPACE_MASK)|SC64_SAVE_TAG_RESEARCH_JOBS_NAMESPACE)
#endif
