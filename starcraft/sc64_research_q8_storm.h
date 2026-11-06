/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RESEARCH_Q8_STORM_H
#define SC64_RESEARCH_Q8_STORM_H
/* Keep the mission adapters' existing eligibility, overlap and death hooks.
 * Only their integer/Q8 state boundary changes in this fenced experiment. */
static int sc64_pr_q8_storm_read(int target,int *hp,int *shield)
{
    unsigned h,s;
    if(!hp||!shield||!sc64_pr_q8_actor_hp(target,&h)||
       !sc64_pr_q8_actor_shield(target,&s)||h>INT_MAX||s>INT_MAX)return 0;
    *hp=(int)h;*shield=(int)s;return 1;
}
static int sc64_pr_q8_storm_commit(int target,int hp,int shield)
{
    if(hp<0||shield<0)return 0;
    /* Snapshot validation and the bounded pure Storm subtraction establish
     * both commit preconditions before either live field is published. */
    return sc64_pr_q8_commit_hp(target,(unsigned)hp)&&
           sc64_pr_q8_commit_shield(target,(unsigned)shield);
}
#endif
