/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary current-field-control transitions; no world-state writes.
 * START launches the briefing; an L tap cycles the unit selection. */
#ifndef P7_GEN43_INPUT_TAPE_H
#define P7_GEN43_INPUT_TAPE_H
typedef struct { unsigned tick; unsigned short held; } P7ReplayInput;
static const P7ReplayInput p7_replay_inputs[] = {
    {1u,1u},
    {2u,0u},
    {3u,16u},
    {4u,0u},
    {5u,16u},
    {6u,0u},
    {7u,16u},
    {8u,0u},
    {9u,1u},
    {10u,0u},
    {11u,8u},
    {12u,0u},
    {612u,512u},
    {613u,0u},
    {614u,2u},
    {615u,0u},
    {616u,128u},
    {617u,0u},
    {618u,1u},
    {619u,0u},
    {620u,80u},
    {662u,16u},
    {713u,0u},
    {714u,1u},
    {715u,0u},
};
#define P7_REPLAY_INPUT_COUNT (sizeof(p7_replay_inputs)/sizeof(p7_replay_inputs[0]))
#define P7_REPLAY_TICKS 2115u
#endif
