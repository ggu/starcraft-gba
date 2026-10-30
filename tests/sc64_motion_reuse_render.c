/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdint.h>
/* Hand-authored moving projectile, flight trail and death child overlays.
 * The draw key aliases projectile id 100 to image 200, exercising keyed
 * image/frame resolution as well as motion reuse. */
#define GBA_HOST 1
#define MAP_COLS 32
#define MAP_ROWS 32
#define SC64_WEAPON_SOURCE_BANK_VERSION 1
#define SC64_WEAPON_SOURCE_SPROLUSELO_LO 1
#define SC64_WEAPON_SOURCE_METADATA 1
#define SC64_WEAPON_SOURCE_BANK_HEADER <stdint.h>
#define SC64_WEAPON_SOURCE_PROFILE_COUNT 1
#define SC64_WEAPON_SOURCE_IMAGE_COUNT 2
#define SC64_WEAPON_SOURCE_FRAME_COUNT 2
#define SC64_WEAPON_SOURCE_ANIMATION_COUNT 3
#define SC64_WEAPON_SOURCE_EVENT_COUNT 2
#define SC64_WEAPON_SOURCE_OVERLAY_COUNT 1
#define SC64_WEAPON_SOURCE_LUT_COUNT 1
#define SC64_WEAPON_SOURCE_LO_TABLE_COUNT 0
#define SC64_WEAPON_SOURCE_LO_OFFSET_COUNT 0
#define SC64_SOURCE_BULLET_FLY 0u

typedef struct {
    uint8_t actor_raw, weapon_owner_raw, weapon_id, mode, variant;
    uint8_t attack_anim, projectile_ticks_per_frame, flags;
    uint8_t weapon_bullet_type, weapon_count;
    uint16_t attackwith_tick, attack_image, projectile_image, projectile_flingy;
    uint16_t projectile_lifetime, projectile_first_frame, projectile_frame_count;
    uint16_t impact_source_tick, attack_event_first, attack_event_count;
    uint16_t projectile_event_first, projectile_event_count, overlay_first, overlay_count;
    int32_t speed_raw, halt_raw;
    int16_t accel_raw;
    int8_t forward_offset, upward_offset;
} Sc64WeaponSourceAttackProfile;
typedef struct {
    uint16_t image_id, event_first, event_count, overlay_first, overlay_count;
    uint32_t duration_ticks, loop_pc, loop_source_tick;
    uint8_t animation_id, flags;
} Sc64WeaponSourceAnimation;
typedef struct {
    uint16_t image_id, frame_first, frame_count, canvas_w, canvas_h;
    uint16_t source_frame_count, frame_directory_first;
    uint8_t directional, modifier, color_shift, lut_index, flags;
} Sc64WeaponSourceImage;
typedef struct {
    uint16_t image_id;
    uint32_t source_tick, pc;
    uint16_t source_frame;
    uint8_t opcode, flags;
} Sc64WeaponSourceEvent;
typedef struct {
    uint16_t parent_image, image_id;
    uint32_t source_tick;
    int32_t relative_tick;
    int16_t x, y;
    uint8_t lo_index, aux, opcode, flags;
    uint16_t sprite_id;
} Sc64WeaponSourceOverlay;
typedef struct {
    uint32_t pixel_offset, mirror_pixel_offset;
    int16_t x, y, mirror_x;
    uint16_t w, h, canvas_w, canvas_h, source_frame_id, image_id;
} Sc64WeaponSourceFrame;
typedef struct { uint32_t data_offset; uint16_t rows; uint8_t modifier, color_shift; } Sc64WeaponSourceLut;
typedef struct { uint16_t image_id, frame_count, offset_count; uint8_t lo_kind, flags; uint32_t offset_first; } Sc64WeaponSourceLoTable;
typedef struct { int8_t x, y; } Sc64WeaponSourceLoOffset;

static Sc64WeaponSourceAttackProfile test_profile;
static Sc64WeaponSourceImage test_request_image, test_keyed_image;
static Sc64WeaponSourceAnimation sc64_weapon_source_animations[3];
#define test_init sc64_weapon_source_animations[0]
#define test_death sc64_weapon_source_animations[1]
#define test_flight sc64_weapon_source_animations[2]
static Sc64WeaponSourceEvent sc64_weapon_source_events[2];
static Sc64WeaponSourceOverlay sc64_weapon_source_overlays[1];
static const uint8_t test_pixels[4] = {11, 12, 77, 78};
static const Sc64WeaponSourceFrame sc64_weapon_source_frames[2] = {
    {0, UINT32_MAX, 0, 0, 0, 2, 1, 2, 1, 0, 200},
    {2, UINT32_MAX, 0, 0, 0, 2, 1, 2, 1, 1, 200}
};
static const uint16_t sc64_weapon_source_frame_directory[2] = {0, 1};
static const Sc64WeaponSourceLut sc64_weapon_source_luts[1] = {{0, 0, 0, 0}};
static const uint8_t sc64_weapon_source_lut_bytes[1] = {0};
static const Sc64WeaponSourceLoTable sc64_weapon_source_lo_tables[1] = {{0}};
static const Sc64WeaponSourceLoOffset sc64_weapon_source_lo_offsets[1] = {{0, 0}};

static inline const Sc64WeaponSourceAttackProfile *
sc64_weapon_source_profile_lookup(uint8_t actor, uint8_t weapon,
                                  uint8_t mode, uint8_t variant)
{
    return actor == test_profile.actor_raw && weapon == test_profile.weapon_id &&
           mode == test_profile.mode && variant == test_profile.variant ?
           &test_profile : 0;
}
static inline const Sc64WeaponSourceImage *
sc64_weapon_source_image_lookup(uint16_t id)
{
    return id == 100u ? &test_request_image :
           id == 200u ? &test_keyed_image : 0;
}
static inline const Sc64WeaponSourceAnimation *
sc64_weapon_source_animation_lookup(uint16_t id, uint8_t animation)
{
    if (id != 100u && id != 200u) return 0;
    return animation == 0u ? &test_init : animation == 1u ? &test_death : animation == 2u ? &test_flight : 0;
}
static inline const Sc64WeaponSourceFrame *
sc64_weapon_source_frame_lookup(uint16_t id, uint16_t source_frame)
{
    if (id != 200u || source_frame > 1u) return 0;
    return &sc64_weapon_source_frames[source_frame];
}
static inline const uint8_t *
sc64_weapon_source_frame_pixels(const Sc64WeaponSourceFrame *frame, int mirror)
{
    if (!frame || mirror) return 0;
    return test_pixels + frame->pixel_offset;
}
static inline const Sc64WeaponSourceLut *
sc64_weapon_source_lut_for_image(uint16_t id)
{
    (void)id;
    return 0;
}
static inline int
sc64_weapon_source_lo_lookup(uint16_t image, uint8_t lo, uint16_t frame,
                             uint8_t offset, int16_t *x, int16_t *y)
{
    (void)sc64_weapon_source_lo_tables;
    if (image != 100 || lo != 0 || frame != 2 || offset != 0) return 0;
    *x = 4; *y = -8;
    return 1;
}

static int camx, camy;
static int currently_visible(int x, int y) { (void)x; (void)y; return 1; }
#include "../starcraft/sc64_weapon_source_runtime.h"
#include "../starcraft/sc64_weapon_source_vfx_render.h"
#include <assert.h>
#include <stdio.h>

typedef struct { unsigned age; int x,y,w,h,offset; } ExpectedDraw;
/* Original 5d output for this hand-authored moving projectile and children. */
static const ExpectedDraw expected_draws[]={
    {0,49,62,2,1,2},
    {1,49,62,2,1,2},
    {2,49,62,2,1,2},
    {3,49,62,2,1,2},
    {4,59,62,2,1,2},
    {4,59,62,2,1,2},
    {5,69,62,2,1,2},
    {5,59,62,2,1,2},
    {6,69,62,2,1,2},
    {6,59,62,2,1,2},
    {7,79,62,2,1,2},
    {7,59,62,2,1,2},
    {8,89,62,2,1,2},
    {9,99,62,2,1,2},
    {10,109,62,2,1,2},
    {11,109,62,2,1,2},
    {12,119,62,2,1,2},
    {13,129,62,2,1,2},
    {14,139,62,2,1,2},
    {15,149,62,2,1,2},
    {16,149,62,2,1,2},
    {17,149,62,2,1,2},
    {17,149,62,2,1,2},
    {18,149,62,2,1,2},
    {18,149,62,2,1,2},
    {19,149,62,2,1,2},
    {19,149,62,2,1,2},
};
static unsigned active_age;
static unsigned draw_count;
static int lo_test, lo_expect_x, lo_pixel_offset;
void gfx_reverse_bytes(u8 *dst, const u8 *src, unsigned n)
{ for (unsigned i = 0; i < n; ++i) dst[i] = src[n - i - 1u]; }
void gfx_blit_sprite(int x, int y, int w, int h, const u8 *src)
{
 if (lo_test) {
   assert(x == lo_expect_x && y == 58 && w == 2 && h == 1);
   assert(src == test_pixels + lo_pixel_offset); ++draw_count; return;
 }
 assert(draw_count<sizeof(expected_draws)/sizeof(expected_draws[0]));
 const ExpectedDraw *e=&expected_draws[draw_count++];
 assert(active_age==e->age && x==e->x && y==e->y && w==e->w && h==e->h);
 assert(src==test_pixels+e->offset);
}
void gfx_blit_sprite_mapped(int x, int y, int w, int h, const u8 *src,
                            const u8 *lut, unsigned rows)
{ (void)lut; (void)rows; gfx_blit_sprite(x, y, w, h, src); }

int main(void)
{
 test_profile=(Sc64WeaponSourceAttackProfile){.actor_raw=5,.weapon_id=11,.projectile_image=100,
   .projectile_lifetime=20,.projectile_frame_count=1,.speed_raw=5120,.weapon_bullet_type=SC64_SOURCE_BULLET_FLY};
 test_request_image=(Sc64WeaponSourceImage){.image_id=200,.frame_first=0,.frame_count=1,.canvas_w=2,.canvas_h=1,.source_frame_count=2};
 test_keyed_image=(Sc64WeaponSourceImage){.image_id=200,.frame_first=1,.frame_count=1,.canvas_w=2,.canvas_h=1,.source_frame_count=2};
 /* Compare exact pointers with the original lookup, including missing images
  * and every out-of-range 16-bit frame ID, before checking draw events. */
 for (unsigned frame=0;frame<=UINT16_MAX;++frame) {
   assert(sc64_weapon_source_vfx_frame_resolved(&test_keyed_image,(uint16_t)frame)==
          sc64_weapon_source_frame_lookup(200,(uint16_t)frame));
   assert(!sc64_weapon_source_vfx_frame_resolved(0,(uint16_t)frame));
 }
 test_init=(Sc64WeaponSourceAnimation){.image_id=200,.event_count=2,.duration_ticks=3,.loop_pc=UINT32_MAX};
 test_flight=(Sc64WeaponSourceAnimation){.image_id=200,.overlay_count=1,.duration_ticks=20,.loop_pc=UINT32_MAX,.animation_id=2};
 test_death=(Sc64WeaponSourceAnimation){.image_id=200,.overlay_count=1,.duration_ticks=3,.loop_pc=UINT32_MAX,.animation_id=1};
 sc64_weapon_source_events[0]=(Sc64WeaponSourceEvent){.image_id=200,.source_tick=0,.source_frame=1,.opcode=0};
 sc64_weapon_source_events[1]=(Sc64WeaponSourceEvent){.image_id=200,.source_tick=2,.source_frame=1,.opcode=36};
 sc64_weapon_source_overlays[0]=(Sc64WeaponSourceOverlay){.parent_image=200,.image_id=200,.source_tick=1,.opcode=15};
 Sc64WeaponSourceVfxKey key={5,1,11,8,0,0};uint32_t packed;assert(sc64_weapon_source_vfx_pack(key,&packed));
 for(uint32_t age=0;age<40;age++){active_age=age;sc64_weapon_source_vfx_draw_trace(packed,50,50,150,50,1,age);}
 assert(draw_count==sizeof(expected_draws)/sizeof(expected_draws[0]));
 /* Image 17 source frame 2 offset is (4,-8). A later parent frame must
  * not move an already spawned sprite; mirrored direction reverses x. */
 test_request_image.directional=1;
 test_flight=(Sc64WeaponSourceAnimation){.image_id=100,.event_count=1,
   .overlay_count=1,.duration_ticks=20,.loop_pc=UINT32_MAX};
 sc64_weapon_source_events[0]=(Sc64WeaponSourceEvent){.image_id=100,.source_frame=0,.opcode=0};
 sc64_weapon_source_overlays[0]=(Sc64WeaponSourceOverlay){.parent_image=100,.image_id=200,
   .source_tick=0,.lo_index=0,.opcode=21};
 lo_test=1; draw_count=0; lo_expect_x=51;
 sc64_weapon_source_vfx_draw_overlays(&test_flight,1,1,100,3,50,50,2,0,0,0,0,0,0,0,0);
 assert(draw_count==1);
 lo_expect_x=47;
 sc64_weapon_source_vfx_draw_overlays(&test_flight,1,1,100,3,50,50,30,1,0,0,0,0,0,0,0);
 assert(draw_count==2);
 sc64_weapon_source_vfx_draw_overlays(&test_flight,1,1,100,3,50,50,3,0,0,0,0,0,0,0,0);
 assert(draw_count==2); /* Missing source frame fails closed. */
 sc64_weapon_source_vfx_draw_overlays(&test_flight,3,3,100,2,50,50,2,0,0,0,0,0,0,0,0);
 assert(draw_count==2); /* Child Init has ended at age three. */
 /* Relative display time is a different clock: birth at source tick 5
  * samples frame 0 + heading 2 even when its relative tick is 2 or -2.
  * At source tick 2 the frame is 1, which would select an absent LO. */
 test_flight.event_count=2;
 sc64_weapon_source_events[0]=(Sc64WeaponSourceEvent){.image_id=100,.source_tick=2,.source_frame=1,.opcode=0};
 sc64_weapon_source_events[1]=(Sc64WeaponSourceEvent){.image_id=100,.source_tick=5,.source_frame=0,.opcode=0};
 sc64_weapon_source_overlays[0].source_tick=5;
 sc64_weapon_source_overlays[0].relative_tick=2;
 lo_expect_x=51; lo_pixel_offset=2; draw_count=0;
 sc64_weapon_source_vfx_draw_overlays(&test_flight,2,2,100,3,50,50,2,0,1,0,0,0,0,0,0);
 assert(draw_count==1);
 sc64_weapon_source_overlays[0].relative_tick=-2; draw_count=0;
 sc64_weapon_source_vfx_draw_overlays(&test_flight,0,0,100,3,50,50,2,0,1,0,0,0,0,0,0);
 assert(draw_count==1);
 puts("moving projectile and child draw events match fixture");return 0;
}
