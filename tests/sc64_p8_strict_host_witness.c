/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-input P8 controller with a separate source-terminal witness.
 * The included controller retains its trace and pause-save behavior. */
#define main sc64_p8_controller_main
#include "sc64_p8_campaign_run.c"
#undef main

int main(int argc,char **argv)
{
    int controller_rc=sc64_p8_controller_main(argc,argv);
    unsigned victory_trigger=SC64_P8_TRIGGER_LIMIT, victory_count=0;
    int conditions_ok=1, completed=0, enabled=0;
    for(unsigned t=0;t<SC64_P8_SOURCE_TRIGGER_COUNT;++t) {
        const Sc64P8SourceTrigger *tr=&sc64_p8_source_triggers[t];
        for(unsigned i=0;i<tr->action_count;++i) {
            const Sc64P8SourceAction *action=&sc64_p8_source_actions[tr->first_action+i];
            if(action->opcode==1u && !(action->flags&2u)) {
                victory_trigger=t;++victory_count;
            }
        }
    }
    if(victory_count==1 && victory_trigger<SC64_P8_TRIGGER_LIMIT &&
       sc64_p8_source_metadata_valid() &&
       sc64_p8_source_triggers[victory_trigger].condition_count>0) {
        Sc64P8TriggerInfo info={0};
        if(sc64_p8_source_trigger_info(0,victory_trigger,&info))
            enabled=sc64_p8_runtime_trigger_enabled(&info,7,1)==1;
        completed=!!(race_p8.runtime[SC64_P8_RUNNER_HUMAN].done & (1u<<victory_trigger));
        for(unsigned i=0;i<sc64_p8_source_triggers[victory_trigger].condition_count;++i)
            if(!race_p8_ops.condition || race_p8_ops.condition(race_p8_ops.context,
                    victory_trigger,i,7)!=1) conditions_ok=0;
    } else conditions_ok=0;
    int faults_ok=1;
    for(unsigned i=0;i<SC64_P8_RUNNER_COUNT;++i)
        if(race_p8.runtime[i].fault!=SC64_P8_FAULT_NONE) faults_ok=0;
    int strict=controller_rc==0 && screen==SCREEN_RESULT && won && !lost &&
        !race_mode && race_selected_mission==SC64_P8_GLOBAL_ID &&
        race_campaign.mission==SC64_P8_GLOBAL_ID &&
        race_p8.runtime[SC64_P8_RUNNER_HUMAN].outcome==SC64_P8_OUTCOME_WON &&
        race_campaign.phase==SC64_RACE_PHASE_COMPLETE &&
        (race_campaign.objective_flags&SC64_RACE_OBJECTIVE_PRIMARY) &&
        ((unsigned)race_completed&(1u<<SC64_P8_GLOBAL_ID)) &&
        faults_ok && victory_count==1 && enabled && completed && conditions_ok;
    printf("P8_STRICT status=%s controller_rc=%d screen=%d won=%d lost=%d mode=%d mission=%d campaign_mission=%u outcome=%u phase=%u objectives=%u completed_mask=%u faults=%u,%u,%u victory_count=%u victory_trigger=%u enabled=%d trigger_done=%d conditions=%d frame=%d calls=%u\n",
        strict?"WIN":"NOT_WIN",controller_rc,screen,won,lost,race_mode,race_selected_mission,
        (unsigned)race_campaign.mission,(unsigned)race_p8.runtime[SC64_P8_RUNNER_HUMAN].outcome,
        (unsigned)race_campaign.phase,race_campaign.objective_flags,(unsigned)race_completed,
        (unsigned)race_p8.runtime[0].fault,(unsigned)race_p8.runtime[1].fault,
        (unsigned)race_p8.runtime[2].fault,victory_count,victory_trigger,
        enabled,completed,conditions_ok,frame,host_tick);
    return strict?0:1;
}
