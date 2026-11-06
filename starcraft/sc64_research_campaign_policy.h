/* SPDX-License-Identifier: GPL-3.0-or-later
 * Inactive P1-P10 human policy groundwork. Caller authenticates canonical
 * live or detached context and the origin/integrity of the private table. */
#ifndef SC64_RESEARCH_CAMPAIGN_POLICY_H
#define SC64_RESEARCH_CAMPAIGN_POLICY_H
#include "sc64_research_human_binding.h"
#include "sc64_protoss_research.h"

typedef struct {
    const unsigned char (*upgrades)[8][6][2]; /* P1..P10, source owner */
    const unsigned char (*cost_supported)[6];
} Sc64PrCampaignPolicyView;
typedef struct {
    Sc64ResearchHumanBinding binding;
    uint8_t initial, maximum, cost_supported, family;
} Sc64PrCampaignRule;

/* Define this only when an authenticated, generated private policy header has
 * already been included. No generated data means no implicit defaults. */
static inline Sc64PrCampaignPolicyView sc64_pr_campaign_generated_view(void)
{
#ifdef SC64_PR_AUTHENTICATED_CAMPAIGN_POLICY
    Sc64PrCampaignPolicyView v = {sc64_pr_campaign_upgrades[2],
                                   sc64_pr_campaign_cost_supported[2]};
#else
    Sc64PrCampaignPolicyView v = {0, 0};
#endif
    return v;
}

/* roles must be derived from the authenticated context's runtime role map,
 * not actor origin or a reverse mapping from compact enemy. This is a read-only
 * policy query; it does not establish physical producer, prerequisites, power,
 * wallet sufficiency, duplicate jobs, or journal validity. */
static inline int sc64_pr_campaign_rule(const Sc64PrCampaignPolicyView *view,
    int custom_mode, const char *mission, unsigned human_slot,
    const int8_t roles[8], int current_owner, unsigned family,
    Sc64PrCampaignRule *out)
{
    Sc64PrCampaignRule r; unsigned max, initial, supported, hard_max;
    if (!view || !view->upgrades || !view->cost_supported || !out ||
        family >= SC64_PR_FAMILY_COUNT ||
        !sc64_research_bind_protoss_human(custom_mode,mission,human_slot,
                                           roles,current_owner,&r.binding)) return 0;
    max=view->upgrades[r.binding.chapter-1][r.binding.current_controller_key][family][0];
    initial=view->upgrades[r.binding.chapter-1][r.binding.current_controller_key][family][1];
    supported=view->cost_supported[r.binding.chapter-1][family];
    hard_max=family==SC64_PR_SINGULARITY ? 1u : 3u;
    if (max>hard_max || initial>max || supported>1) return 0;
    r.initial=(uint8_t)initial; r.maximum=(uint8_t)max;
    r.cost_supported=(uint8_t)supported; r.family=(uint8_t)family; *out=r; return 1;
}

/* Source-default quote is supported only for a well-formed scenario rule and
 * level. The quote's time scaling is project policy, not observed N64 timing. */
static inline int sc64_pr_campaign_quote(const Sc64PrCampaignRule *rule,
    unsigned family, unsigned current, Sc64ProtossResearchQuote *out)
{
    Sc64ProtossResearchQuote q;
    if (!rule || !out || family>=SC64_PR_FAMILY_COUNT || rule->family!=family ||
        rule->binding.chapter<1 || rule->binding.chapter>10 ||
        rule->binding.current_controller_key != (rule->binding.chapter==10?1:7) ||
        rule->binding.payer_key != rule->binding.current_controller_key ||
        rule->binding.wallet_slot != SC64_PR_BINDING_HUMAN_BANK ||
        rule->binding.compact_owner != SC64_PR_BINDING_HUMAN_OWNER ||
        rule->cost_supported!=1 || rule->initial>rule->maximum ||
        current<rule->initial || current>=rule->maximum ||
        !sc64_pr_quote(family,current,&q) || rule->maximum>q.max_level) return 0;
    q.max_level=rule->maximum; *out=q; return 1;
}
#endif
