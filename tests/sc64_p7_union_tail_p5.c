/* Exact P5+P7 persistent-union tail guard without the P6 member. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

int main(void)
{
    RaceSave candidate;
    memset(&candidate,0,sizeof(candidate));
    printf("P5=%lu P7=%lu\n",(unsigned long)sizeof(candidate.p5),
           (unsigned long)sizeof(candidate.p7));
    assert(sizeof(candidate.p7)==sizeof(candidate.p5));
    candidate.state[0]=0;
    candidate.state[2]=SC64_P5_GLOBAL_ID;
    assert(race_p7_union_tail_valid(&candidate));
    assert(race_p7_union_tail_valid(&candidate));
    return 0;
}
