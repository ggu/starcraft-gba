/* Private experimental BC heading helpers. SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BC_HEADING_H
#define SC64_BC_HEADING_H
#include <stdint.h>
#include "sc64_battlecruiser_yamato_motion.h"
/* Source flingy 70 turn rate, raw direction units per source update. */
#define SC64_BC_SOURCE_TURN_RATE 20u
#define SC64_BC_RECOVERY_MAX 6u
/* work_timer encoding is used ONLY for raw-12 actors under this experiment. */
static inline int sc64_bc_state_pack(unsigned heading,unsigned recovery,uint16_t *out) {
 if(!out||heading>255u||recovery>SC64_BC_RECOVERY_MAX)return 0;
 *out=(uint16_t)((heading<<3)|recovery);return 1;
}
static inline int sc64_bc_state_unpack(unsigned value,uint8_t *heading,uint8_t *recovery) {
 if(value>2047u||(value&7u)>SC64_BC_RECOVERY_MAX||!heading||!recovery)return 0;
 *heading=(uint8_t)(value>>3);*recovery=(uint8_t)(value&7u);return 1;
}
static inline int sc64_bc_state_recovery(unsigned state,unsigned recovery,uint16_t *out){uint8_t h,r;if(!sc64_bc_state_unpack(state,&h,&r))return 0;return sc64_bc_state_pack(h,recovery,out);}
/* OpenBW direction_t subtraction wraps to signed [-128,127], so an exact
 * half-turn chooses -128. One call corresponds to one source movement update. */
static inline uint8_t sc64_bc_heading_turn(uint8_t current,uint8_t target){int d=(uint8_t)(target-current);if(d>=128)d-=256;if(d>20)d=20;else if(d< -20)d= -20;return (uint8_t)(current+d);}
/* Zero displacement preserves the source heading. Caller converts coordinates
 * to source units; the shared integer direction kernel validates its domain. */
static inline int sc64_bc_heading_toward(uint8_t current,int32_t dx,int32_t dy,uint8_t *out){uint8_t target;if(!out)return 0;if(!dx&&!dy){*out=current;return 1;}if(dx < -8190 || dx > 8190 || dy < -8190 || dy > 8190)return 0;target=sc64_yamato_direction(dx,dy);*out=sc64_bc_heading_turn(current,target);return 1;}
/* OpenBW set_image_heading: round 256-way heading to 32 directions. */
static inline uint8_t sc64_bc_heading_face(uint8_t heading){return (uint8_t)(((unsigned)heading+4u)/8u&31u);}
#endif
