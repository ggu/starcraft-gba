/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Include after sc64_weapon_source_vfx_render.h. */
#ifndef SC64_YAMATO_RASTER_ADAPTER_H
#define SC64_YAMATO_RASTER_ADAPTER_H
#include "yamato_source_generated.h"
static inline const Sc64YamatoFrame *sc64_yamato_raster_frame(uint16_t image,unsigned frame) {
 for(unsigned i=0;i<SC64_YAMATO_FRAME_COUNT;i++) if(sc64_yamato_frames[i].image==image && sc64_yamato_frames[i].frame==frame)return &sc64_yamato_frames[i];
 return 0;
}
static inline int sc64_yamato_raster_charge_frame(unsigned tick) {if(tick<42)return 4+(tick%14)/2;if(tick<50)return (tick-42)/2;return -1;}
static inline int sc64_yamato_raster_impact_frame(unsigned tick) {return tick<28?(int)(tick/2):-1;}
#define SC64_YAMATO_RASTER_CAST_SOURCE_TICK 44u
/* world coordinates use the existing source-VFX renderer convention. */
static inline void sc64_yamato_raster_draw(uint16_t image,unsigned frame,unsigned heading,int world_x,int world_y) {
 int mirror=0;if(image==541||image==542){if(heading>=32)return;mirror=heading>16;frame+=mirror?32-heading:heading;}
 const Sc64YamatoFrame *f=sc64_yamato_raster_frame(image,frame);if(!f)return;
 Sc64WeaponSourceVfxRaster r={sc64_yamato_pixels+f->offset,0,f->modifier==9?sc64_yamato_light_lut:0,f->image,f->x,f->y,(int16_t)(f->cw-f->x-f->w),f->w,f->h,f->cw,f->ch,f->modifier,f->modifier==9?SC64_YAMATO_LIGHT_ROWS:0};
 sc64_weapon_source_vfx_draw_raster(&r,world_x,world_y,mirror);
}
#endif
