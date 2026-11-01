#include "gba.h"
#include "audio_mixer.h"
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(GBA_PRESENT_DEADLINE)
#include "gba_present.h"
#endif
#ifdef GBA_HOST
#include <stdio.h>
static u16 host_fb[2][19200];
u8 gba_vram_scratch[GBA_VRAM_SCRATCH_BYTES] __attribute__((aligned(4)));
static u8 host_save[32768];
static u16 host_palette[256];
#endif
#define REG16(a) (*(volatile u16 *)(a))
#define REG32(a) (*(volatile u32 *)(a))
#ifdef GBA_HOST
#define HOT
#else
#define HOT __attribute__((section(".iwram"),long_call,target("arm")))
#endif
#if defined(SC64_TILE_RENDERER) && !defined(GBA_HOST)
#define HOT_MODE4
#else
#define HOT_MODE4 HOT
#endif
#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
/* Direct GBA mode-4 double buffering. All framebuffer stores are halfwords. */
#ifdef GBA_HOST
static volatile u16 *pixels=host_fb[1];
#else
static volatile u16 *pixels=(volatile u16*)0x0600a000;
#endif
static u16 front;
static u8 backbuffer_index;
static u32 clear_generation[2];
#ifdef GBA_SCENE_CLIP
static u8 gfx_clip_top, gfx_clip_bottom;
#endif
static int tone_frames, noise_frames;
volatile u32 gfx_draw_cycles;
static u32 draw_start;
#ifdef SC64_TILE_RENDERER
#ifdef GBA_HOST
u16 gfx_host_vram[49152];
u16 gfx_host_io[64];
static u16 host_obj_palette[256];
static u16 host_oam[512];
#define GFX_OAM16 ((volatile u16 *)host_oam)
#define GFX_OBJ_PAL16 ((volatile u16 *)host_obj_palette)
#define GFX_BG_PAL16 ((volatile u16 *)host_palette)
#else
#define GFX_OAM16 ((volatile u16 *)0x07000000)
#define GFX_OBJ_PAL16 ((volatile u16 *)0x05000200)
#define GFX_BG_PAL16 ((volatile u16 *)0x05000000)
#endif
static u8 gfx_tile_buffer[240*160] __attribute__((aligned(4)));
u32 gfx_tile_dirty[20], gfx_tile_drawn[20], gfx_tile_art_marks[20];
u8 gfx_tile_band_top, gfx_tile_band_bottom;
void (*gfx_tile_present_hook)(void);
static u8 gfx_tile_on, gfx_tile_zero, gfx_tile_capture;
#define GFX_TILE_COLOR(c) ((c) ? (c) : gfx_tile_zero)
#define GFX_TILE_TOUCH(x0,y0,x1,y1) do { if (gfx_tile_on) gfx_tile_touch((x0),(y0),(x1),(y1)); } while (0)
/* Mark the 8x8 tiles covered by [x0,x1) x [y0,y1). Art capture records a
 * separate map so the static-art pass never reaches the sprite overlay. */
static HOT void gfx_tile_touch(int x0,int y0,int x1,int y1) {
    if(x0<0)x0=0; if(y0<0)y0=0; if(x1>240)x1=240; if(y1>160)y1=160;
    if(x0>=x1||y0>=y1)return;
    if(y0<GFX_TILE_WORLD_TOP)gfx_tile_band_top=1;
    if(y1>GFX_TILE_WORLD_BOTTOM)gfx_tile_band_bottom=1;
    u32 mask=((2u<<((x1-1)>>3))-1u)&~((1u<<(x0>>3))-1u);
    for(int ty=y0>>3;ty<=(y1-1)>>3;ty++) {
        if(gfx_tile_capture)gfx_tile_art_marks[ty]|=mask;
        else {gfx_tile_dirty[ty]|=mask;gfx_tile_drawn[ty]|=mask;}
    }
}
int gfx_tile_active(void){return gfx_tile_on;}
u8 *gfx_tile_buffer_pixels(void){return gfx_tile_buffer;}
void gfx_tile_art_capture(int enable){
    gfx_tile_capture=(u8)(enable!=0);
    if(enable)for(int i=0;i<20;i++)gfx_tile_art_marks[i]=0;
}
/* Clear last frame's dynamic pixels in the world rows only; the HUD bands
 * persist and are redrawn when something drew over them. */
HOT void gfx_tile_clear_world(void) {
    for(int ty=GFX_TILE_WORLD_TOP>>3;ty<GFX_TILE_WORLD_BOTTOM>>3;ty++) {
        u32 m=gfx_tile_drawn[ty];
        if(!m)continue;
        int y0=ty*8<GFX_TILE_WORLD_TOP?GFX_TILE_WORLD_TOP:ty*8;
        u32 *base=(u32*)(gfx_tile_buffer+y0*240);
        int rows=ty*8+8-y0;
        for(u32 bits=m,tx=0;bits;bits>>=1,tx++) if(bits&1) {
            u32 *row=base+2*tx;
            for(int y=0;y<rows;y++,row+=60){row[0]=0;row[1]=0;}
        }
        gfx_tile_dirty[ty]|=m;
        gfx_tile_drawn[ty]=0;
    }
}
static void gfx_tile_palette_sync(void) {
    for(int i=0;i<256;i++)GFX_OBJ_PAL16[i]=GFX_BG_PAL16[i];
    GFX_BG_PAL16[GFX_TILE_BLACK_INDEX]=GFX_BG_PAL16[0];
    GFX_OBJ_PAL16[GFX_TILE_BLACK_INDEX]=GFX_BG_PAL16[0];
}
void gfx_tile_enter(void) {
    volatile u16 *vram=GFX_VRAM16;
    for(int i=0;i<49152;i++)vram[i]=0;
    for(int i=0;i<32;i++){vram[GFX_TILE_FOG*32+i]=0;}
    /* Solid fog and opaque-black tiles for the fog layer. */
    for(int i=0;i<32;i++){
        vram[GFX_TILE_FOG*32+i]=(u16)(C_FOG|(C_FOG<<8));
        vram[GFX_TILE_BLACK*32+i]=(u16)(GFX_TILE_BLACK_INDEX|(GFX_TILE_BLACK_INDEX<<8));
    }
    for(int ty=15;ty<20;ty++)for(int tx=0;tx<30;tx++)
        vram[GFX_TILE_SBB_HUD*1024+ty*32+tx]=(u16)(GFX_TILE_HUD_FIRST+(ty-15)*30+tx);
    for(int i=0;i<128;i++){GFX_OAM16[i*4]=0x0200;GFX_OAM16[i*4+1]=0;GFX_OAM16[i*4+2]=0;}
    for(int k=0;k<8;k++){
        GFX_OAM16[k*4]=(u16)(((k>>2)*64)|0x2000);
        GFX_OAM16[k*4+1]=(u16)(((k&3)*64)|0xC000);
        GFX_OAM16[k*4+2]=(u16)(k*128);
    }
    for(int i=0;i<240*160/4;i++)((u32*)gfx_tile_buffer)[i]=0;
    for(int i=0;i<20;i++)gfx_tile_dirty[i]=gfx_tile_drawn[i]=gfx_tile_art_marks[i]=0;
    gfx_tile_band_top=gfx_tile_band_bottom=1;
    gfx_tile_palette_sync();
    GFX_IO16(0x08)=(u16)(0x0080|(GFX_TILE_SBB_HUD<<8));
    GFX_IO16(0x0a)=(u16)(0x0081|(GFX_TILE_SBB_ART<<8));
    GFX_IO16(0x0c)=(u16)(0x0082|(GFX_TILE_SBB_FOG<<8));
    GFX_IO16(0x0e)=(u16)(0x0083|(GFX_TILE_SBB_TERRAIN<<8));
    for(int r=0x10;r<0x20;r+=2)GFX_IO16(r)=0;
    GFX_IO16(0x40)=240; /* WIN0H: x 0..240 */
    GFX_IO16(0x44)=(u16)((GFX_TILE_WORLD_TOP<<8)|GFX_TILE_WORLD_BOTTOM);
    GFX_IO16(0x48)=0x001f; /* inside WIN0: BG0-3 and OBJ */
    GFX_IO16(0x4a)=0x0011; /* HUD bands: BG0 and OBJ over the backdrop */
    GFX_IO16(0x00)=0x3f40; /* mode 0, BG0-3, OBJ 1D, WIN0 */
    gfx_tile_on=1;gfx_tile_zero=GFX_TILE_BLACK_INDEX;gfx_tile_capture=0;
    pixels=(volatile u16*)gfx_tile_buffer;
}
void gfx_tile_leave(void) {
    gfx_tile_on=0;gfx_tile_zero=0;gfx_tile_capture=0;
    GFX_IO16(0x00)=(u16)(0x0404|(front?16:0));
#ifdef GBA_HOST
    pixels=host_fb[front^1];
#else
    pixels=(volatile u16*)(front?0x06000000:0x0600a000);
#endif
}
/* Copy changed tiles top to bottom; rows convert faster than they scan out. */
static HOT void gfx_tile_present_tiles(void) {
    volatile u16 *vram=GFX_VRAM16;
    for(int ty=0;ty<20;ty++) {
        u32 m=gfx_tile_dirty[ty];
        if(!m)continue;
        gfx_tile_dirty[ty]=0;
        for(int tx=0;m;m>>=1,tx++) if(m&1) {
            volatile u32 *dst;
            if(ty<15) {
                int sprite=(ty>>3)*4+(tx>>3);
                dst=(volatile u32*)(vram+0x8000+(sprite*64+(ty&7)*8+(tx&7))*32);
            } else dst=(volatile u32*)(vram+(GFX_TILE_HUD_FIRST+(ty-15)*30+tx)*32);
            const u32 *src=(const u32*)(gfx_tile_buffer+ty*8*240+tx*8);
            for(int r=0;r<8;r++){dst[r*2]=src[r*60];dst[r*2+1]=src[r*60+1];}
        }
    }
}
#ifdef GBA_HOST
static u16 host_rgb[2][38400];
static u8 host_rgb_valid[2];
static int host_bg_pixel(int bg,int x,int y,u8 *out) {
    u16 cnt=GFX_IO16(0x08+bg*2);
    int bx=(x+GFX_IO16(0x10+bg*4))&255,by=(y+GFX_IO16(0x12+bg*4))&255;
    u16 e=gfx_host_vram[((cnt>>8)&31)*1024+(by>>3)*32+(bx>>3)];
    int px=bx&7,py=by&7;
    if(e&0x400)px=7-px;
    if(e&0x800)py=7-py;
    int byte=((cnt>>2)&3)*16384+(e&1023)*64+py*8+px;
    u16 pair=gfx_host_vram[byte>>1];
    *out=(u8)(byte&1?pair>>8:pair);
    return *out!=0;
}
static u8 host_obj_list[128],host_obj_count;
static int host_obj_pixel(int x,int y,u8 *out) {
    for(int n=0;n<host_obj_count;n++) {
        int k=host_obj_list[n];
        u16 a0=host_oam[k*4],a1=host_oam[k*4+1],a2=host_oam[k*4+2];
        int oy=a0&255,ox=a1&511;
        if(x<ox||x>=ox+64||y<oy||y>=oy+64)continue;
        int sx=x-ox,sy=y-oy;
        int byte=0x10000+(a2&1023)*32+((sy>>3)*8+(sx>>3))*64+(sy&7)*8+(sx&7);
        u16 pair=gfx_host_vram[byte>>1];
        *out=(u8)(byte&1?pair>>8:pair);
        if(*out)return 1;
    }
    return 0;
}
/* Compose the emulated mode-0 layers exactly as configured above. */
static void host_tile_compose(u16 *rgb) {
    host_obj_count=0;
    for(int k=0;k<128;k++)if(!(host_oam[k*4]&0x0200))host_obj_list[host_obj_count++]=(u8)k;
    for(int y=0;y<160;y++)for(int x=0;x<240;x++) {
        int inside=x>=(GFX_IO16(0x40)>>8)&&x<(GFX_IO16(0x40)&255)&&
                   y>=(GFX_IO16(0x44)>>8)&&y<(GFX_IO16(0x44)&255);
        u16 enable=(inside?GFX_IO16(0x48):GFX_IO16(0x4a))&(GFX_IO16(0x00)>>8);
        u16 color=GFX_BG_PAL16[0];
        u8 index;
        int done=0;
        for(int prio=0;prio<4&&!done;prio++) {
            if(prio==0&&(enable&0x10)&&host_obj_pixel(x,y,&index)){color=host_obj_palette[index];done=1;break;}
            for(int bg=0;bg<4;bg++)
                if((enable&(1<<bg))&&(GFX_IO16(0x08+bg*2)&3)==prio&&host_bg_pixel(bg,x,y,&index)){
                    color=host_palette[index];done=1;break;
                }
        }
        rgb[y*240+x]=color;
    }
}
#endif
#else
#define GFX_TILE_COLOR(c) (c)
#define GFX_TILE_TOUCH(x0,y0,x1,y1) ((void)0)
#endif
#ifdef GBA_HOST
/* RGB555 of the frame currently on screen, in either presentation mode. */
void gfx_host_frame_rgb(u16 *rgb) {
#ifdef SC64_TILE_RENDERER
    if(host_rgb_valid[front]){for(int i=0;i<38400;i++)rgb[i]=host_rgb[front][i];return;}
#endif
    for(int i=0;i<38400;i++){u16 a=host_fb[front][i/2];rgb[i]=host_palette[(i&1)?a>>8:a&255];}
}
#endif
static u32 previous_present;
#ifdef SC64_UNIT_WORD_SPRITE_DIAGNOSTICS
volatile u32 gfx_sprite_words_calls;
#endif
static const u16 colors[32]={
 RGB(1,1,2),RGB(30,30,28),RGB(13,15,17),RGB(3,5,8),RGB(8,12,15),
 RGB(30,5,5),RGB(12,2,3),RGB(7,28,12),RGB(2,10,5),RGB(6,14,30),RGB(2,5,15),
 RGB(31,25,9),RGB(14,10,3),RGB(9,28,29),RGB(22,10,28),RGB(31,15,4),
 RGB(14,9,5),RGB(8,7,5),RGB(12,13,13),RGB(5,10,6),RGB(20,18,11),RGB(21,23,24),
 RGB(27,19,13),RGB(2,3,3),RGB(9,17,26),RGB(19,8,17),RGB(29,23,12),RGB(28,8,1),
 RGB(12,1,2),RGB(2,3,5),RGB(8,11,12),RGB(31,31,18)
};
/* Cartridge backup signature used by emulator/flash-cart save-type detection. */
__attribute__((used)) const char gba_save_signature[]="SRAM_V113";
#if defined(SC64_AUDIO_IMPORTED) && !defined(GBA_HOST)
static void audio_hw_init(void);
#endif
void gba_init(void) {
#ifdef GBA_SCENE_CLIP
 gfx_clip_top=0; gfx_clip_bottom=160;
#endif
#ifdef GBA_HOST
 front=0; backbuffer_index=1; clear_generation[0]=clear_generation[1]=0;
 pixels=host_fb[1];
 for(int i=0;i<32;i++)host_palette[i]=colors[i];
#else
 REG16(0x04000208)=0;
 REG16(0x04000000)=0x0404;
 REG16(0x04000204)=0x4317;
 volatile u16 *pal=(volatile u16*)0x05000000;
 for(int i=0;i<32;i++)pal[i]=colors[i];
 REG16(0x04000084)=0x0080;
 REG16(0x04000080)=0x9977;
 REG16(0x04000082)=0x0002;
 REG16(0x04000108)=0; REG16(0x0400010c)=0;
 REG16(0x0400010e)=0x0084; REG16(0x0400010a)=0x0080;
 front=0; backbuffer_index=1; clear_generation[0]=clear_generation[1]=0;
#ifdef SC64_AUDIO_IMPORTED
 audio_hw_init();
#endif
#endif
}
#ifdef GBA_SCENE_CLIP
void gfx_set_clip_y(int top,int bottom) {
 if(top<0||top>bottom||bottom>160)return;
 gfx_clip_top=(u8)top; gfx_clip_bottom=(u8)bottom;
}
u16 gfx_get_clip_y(void) {
 return (u16)(gfx_clip_top|((u16)gfx_clip_bottom<<8));
}
#endif
u8 gfx_backbuffer_index(void) { return backbuffer_index; }
u32 gfx_clear_generation(u8 buffer_index) {
 return buffer_index < 2 ? clear_generation[buffer_index] : 0;
}
void gfx_set_palette(int first,int count,const u16 *rgb555) {
 if(first<0||count<0||first+count>256||!rgb555)return;
#ifdef GBA_HOST
 for(int i=0;i<count;i++)host_palette[first+i]=rgb555[i];
#else
 volatile u16 *pal=(volatile u16*)0x05000000;
 for(int i=0;i<count;i++)pal[first+i]=rgb555[i];
#endif
#ifdef SC64_TILE_RENDERER
 if(gfx_tile_on)gfx_tile_palette_sync();
#endif
}
u32 gba_clock(void){
#ifdef GBA_HOST
return 0;
#else
u16 hi,lo;do{hi=REG16(0x0400010c);lo=REG16(0x04000108);}while(hi!=REG16(0x0400010c));return ((u32)hi<<16)|lo;
#endif
}
u16 gba_keys(void){
#ifdef GBA_HOST
return 0;
#else
return (~REG16(0x04000130))&1023;
#endif
}
#if !defined(GBA_HOST) && !defined(SC64_LATE_TERRAN_STORAGE) && !defined(GBA_PRESENT_DEADLINE)
static void vblank(void){while(REG16(0x04000006)>=160){}while(REG16(0x04000006)<160){}}
#endif
void gfx_begin(u8 color) {
#ifdef GBA_SCENE_CLIP
 gfx_clip_top=0; gfx_clip_bottom=160;
#endif
 draw_start=gba_clock();
#ifdef SC64_TILE_RENDERER
 if(gfx_tile_on){gfx_rect(0,0,240,160,color);return;}
#endif
 backbuffer_index=(u8)(front^1);
 clear_generation[backbuffer_index]++;
#ifdef GBA_HOST
 pixels=host_fb[backbuffer_index];
 for(int i=0;i<19200;i++)pixels[i]=color|((u16)color<<8);
#else
 pixels=(volatile u16*)(front?0x06000000:0x0600a000);
 volatile u32 fill=color|((u32)color<<8)|((u32)color<<16)|((u32)color<<24);
 REG32(0x040000d4)=(u32)&fill;
 REG32(0x040000d8)=(u32)pixels;
 /* A 32-bit transfer clears four Mode 4 pixels per DMA beat. */
 REG32(0x040000dc)=0x85000000|9600;
 while(REG32(0x040000dc)&0x80000000){}
#endif
}
/* Select the next backbuffer without clearing it. Only renderers proven to
 * overwrite every pixel may use this; partial scenes need gfx_begin(). */
#if defined(SC64_NO_FRAME_CLEAR) || defined(GBA_HOST)
void gfx_begin_preserve(void) {
#ifdef GBA_SCENE_CLIP
 gfx_clip_top=0; gfx_clip_bottom=160;
#endif
 draw_start=gba_clock();
#ifdef SC64_TILE_RENDERER
 if(gfx_tile_on)return;
#endif
 backbuffer_index=(u8)(front^1);
#ifdef GBA_HOST
 pixels=host_fb[backbuffer_index];
#else
 pixels=(volatile u16*)(front?0x06000000:0x0600a000);
#endif
}
#endif
/* Copy an exact full-width band between Mode 4 pixels and aligned linear
   indexed-pixel storage. The GBA DMA path uses whole 32-bit words only. */
static int gfx_band_valid(int y,int height,const void *buffer) {
 return buffer&&(((uintptr_t)buffer&3)==0)&&y>=0&&height>0&&height<=160&&y<=160-height;
}
HOT_MODE4 int gfx_read_band(int y,int height,u8 *dst) {
 if(!gfx_band_valid(y,height,dst))return 0;
#ifdef GBA_HOST
 for(int row=0;row<height;row++) {
  const volatile u16 *src=pixels+(y+row)*120;
  u8 *out=dst+row*240;
  for(int x=0;x<120;x++) {
   u16 pair=src[x];out[x*2]=(u8)pair;out[x*2+1]=(u8)(pair>>8);
  }
 }
#else
 REG32(0x040000d4)=(u32)(pixels+y*120);
 REG32(0x040000d8)=(u32)dst;
 REG32(0x040000dc)=0x84000000|(240*height/4);
 while(REG32(0x040000dc)&0x80000000){}
#endif
 return 1;
}
HOT_MODE4 int gfx_blit_band(int y,int height,const u8 *src) {
 if(!gfx_band_valid(y,height,src))return 0;
 GFX_TILE_TOUCH(0,y,240,y+height);
#ifdef GBA_HOST
 for(int row=0;row<height;row++) {
  volatile u16 *out=pixels+(y+row)*120;
  const u8 *in=src+row*240;
  for(int x=0;x<120;x++)out[x]=in[x*2]|((u16)in[x*2+1]<<8);
 }
#else
 REG32(0x040000d4)=(u32)src;
 REG32(0x040000d8)=(u32)(pixels+y*120);
 REG32(0x040000dc)=0x84000000|(240*height/4);
 while(REG32(0x040000dc)&0x80000000){}
#endif
 return 1;
}
/* Both helpers stay in ROM: the canonical profiles have no IWRAM reserve left,
   and they move few pixels per frame. */
int gfx_copy_band_from_front(int y,int height) {
 if(y<0||height<=0||height>160||y>160-height)return 0;
 GFX_TILE_TOUCH(0,y,240,y+height);
#ifdef GBA_HOST
 const u16 *src=host_fb[backbuffer_index^1]+y*120;
 volatile u16 *out=pixels+y*120;
 for(int i=0;i<120*height;i++)out[i]=src[i];
#else
 /* pixels addresses the backbuffer; the other page is on screen. */
 REG32(0x040000d4)=(u32)((const u16*)(front?0x0600a000:0x06000000)+y*120);
 REG32(0x040000d8)=(u32)(pixels+y*120);
 REG32(0x040000dc)=0x84000000|(240*height/4);
 while(REG32(0x040000dc)&0x80000000){}
#endif
 return 1;
}
void gfx_blit_opaque(int x,int y,int width,int height,const u8 *src) {
 if(!src||width<=0||height<=0||width>256||height>256||x>=240||x+width<=0)return;
#ifdef GBA_SCENE_CLIP
 if(y>=gfx_clip_bottom||y+height<=gfx_clip_top)return;
 int top=y<gfx_clip_top?gfx_clip_top-y:0;
 int bottom=y+height>gfx_clip_bottom?gfx_clip_bottom-y:height;
#else
 if(y>=160||y+height<=0)return;
 int top=y<0?-y:0,bottom=y+height>160?160-y:height;
#endif
 GFX_TILE_TOUCH(x,y,x+width,y+height);
 int left=x<0?-x:0,right=x+width>240?240-x:width;
 for(int row=top;row<bottom;row++) {
  int col=left;const u8*s=src+row*width;
  volatile u16*d=pixels+(y+row)*120+((x+col)>>1);
  if((x+col)&1){*d=(*d&255)|((u16)s[col]<<8);d++;col++;}
  for(;col+1<right;col+=2,d++)*d=s[col]|((u16)s[col+1]<<8);
  if(col<right)*d=(*d&0xff00)|s[col];
 }
}
/* Opaque clipped 240-pixel window from aligned, padded source rows. Areas
   outside the source keep the framebuffer's current background. The shifted
   word path preserves every camera pixel without unaligned ARM7 loads. */
HOT_MODE4 int gfx_blit_window(int y,int height,const u8 *source,int stride,int rows,int left,int top) {
 if(!source||((uintptr_t)source&3)||stride<=0||(stride&3)||rows<=0||
    y<0||height<=0||height>160||y>160-height)return 0;
 GFX_TILE_TOUCH(0,y,240,y+height);
#ifdef GBA_WINDOW_BANDS
 /* A vertical scroll of a full-width cache is one contiguous transfer. Keep
    out-of-source rows untouched, just as the generic per-row path does. */
 if(stride==240 && left==0) {
  if(top>=rows || top<=-height)return 1;
  int first=top<0?-top:0,last=height;
  if(top>rows-height)last=rows-top;
  if(first<last)
   return gfx_blit_band(y+first,last-first,source+(top+first)*240);
  return 1;
 }
#endif
 for(int row=0;row<height;row++) {
  int sy=top+row;if(sy<0||sy>=rows)continue;
  int begin=left<0?-left:0,end=left+240>stride?stride-left:240;
  if(begin>=end||begin>=240||end<=0)continue;
  int col=begin;
  const u8 *in=source+sy*stride+left+col;
  volatile u16 *out=pixels+(y+row)*120+(col>>1);
  if(col&1){*out=(*out&255)|((u16)*in++<<8);out++;col++;}
  if(col+2<=end&&((uintptr_t)out&2)){
   *out++=in[0]|((u16)in[1]<<8);in+=2;col+=2;
  }
  int words=(end-col)/4;
  volatile u32 *wide=(volatile u32*)out;
  if(words) {
   const u32 *aligned=(const u32*)((uintptr_t)in&~(uintptr_t)3);
   int shift=((uintptr_t)in&3)*8;
   if(!shift) {
#ifdef GBA_HOST
    for(int n=0;n<words;n++)wide[n]=aligned[n];
#else
    /* Opaque, aligned source rows can move directly from EWRAM to VRAM.
       The edge pairs and byte-shifted rows retain their exact CPU path. */
    REG32(0x040000d4)=(u32)aligned;
    REG32(0x040000d8)=(u32)wide;
    REG32(0x040000dc)=0x84000000|(u32)words;
    while(REG32(0x040000dc)&0x80000000){}
#endif
   } else {
    u32 a=*aligned++;
    for(int n=0;n<words;n++){u32 b=*aligned++;wide[n]=(a>>shift)|(b<<(32-shift));a=b;}
   }
   in+=words*4;col+=words*4;out=(volatile u16*)(wide+words);
  }
  if(col+2<=end){*out++=in[0]|((u16)in[1]<<8);in+=2;col+=2;}
  if(col<end)*out=(*out&0xff00)|*in;
 }
 return 1;
}
/* Internal terrain-only window: caller must redraw exposed edge tiles before
 * presentation. Unlike gfx_blit_window, this may overwrite that dirty edge. */
int gfx_blit_terrain_window(int y,int height,const u8 *source,int stride,int rows,int left,int top) {
 if(!source||((uintptr_t)source&3)||stride!=240||rows<=0||y<0||height<=0||height>160||y>160-height||!left||(left&3)||left<=-240||left>=240)
  return gfx_blit_window(y,height,source,stride,rows,left,top);
#ifdef GBA_SCENE_CLIP
 /* Edge redraw must cover every row that the bulk copy may touch. */
 if(gfx_clip_top>y || gfx_clip_bottom<y+height)
  return gfx_blit_window(y,height,source,stride,rows,left,top);
#endif
 if(top>=rows||top<=-height)return 1;
 int first=top<0?-top:0,last=height;if(top>rows-height)last=rows-top;
 if(first>=last)return 1;
 int lo=first,hi=last;
 if((top+lo)*240+left<0)lo++;
 if((top+hi-1)*240+left+240>rows*240)hi--;
 if(lo>=hi)return gfx_blit_window(y,height,source,stride,rows,left,top);
 if(lo>first&&!gfx_blit_window(y+first,lo-first,source,stride,rows,left,top+first))return 0;
 if(hi<last&&!gfx_blit_window(y+hi,last-hi,source,stride,rows,left,top+hi))return 0;
 GFX_TILE_TOUCH(0,y+lo,240,y+hi);
 const u8 *in=source+(top+lo)*240+left;
#ifdef GBA_HOST
 volatile u16 *out=pixels+(y+lo)*120;
 for(int i=0;i<(hi-lo)*120;i++)out[i]=(u16)in[2*i]|((u16)in[2*i+1]<<8);
#else
 REG32(0x040000d4)=(u32)in;
 REG32(0x040000d8)=(u32)(pixels+(y+lo)*120);
 REG32(0x040000dc)=0x84000000|(u32)((hi-lo)*60);
 while(REG32(0x040000dc)&0x80000000){}
#endif
 return 1;
}
#if defined(D2_DEN_OF_EVIL) || defined(GBA_HOST)
/* Nearest-neighbour 2x opaque cache window. Each source pixel becomes a
   2x2 block; the caller's framebuffer fill supplies pixels beyond cache edges. */
HOT int gfx_blit_window_2x(int y,int height,const u8 *source,int stride,int rows,int left,int top) {
 if(!source||((uintptr_t)source&3)||stride<=0||(stride&3)||rows<=0||
    y<0||height<=0||(height&1)||height>160||y>160-height)return 0;
 int out_rows=height>>1;
 for(int row=0;row<out_rows;row++) {
  int sy=top+row;if(sy<0||sy>=rows)continue;
  int begin=left<0?-left:0,end=left+120>stride?stride-left:120;
  if(begin>=end||begin>=120||end<=0)continue;
  const u8 *in=source+sy*stride+left+begin;
  int col=begin;
  volatile u16 *out=pixels+(y+row*2)*120+col;
  volatile u16 *out2=out+120;
  for(;col<end;col++,in++,out++,out2++) {
   u16 doubled=*in|((u16)*in<<8);
   *out=doubled;*out2=doubled;
  }
 }
 return 1;
}
#endif
void gfx_end(void) {
    gfx_draw_cycles=gba_clock()-draw_start;
#ifndef GBA_HOST
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(GBA_PRESENT_DEADLINE)
    while (!gba_present_ready(gba_clock()-previous_present,
                              REG16(0x04000006))) {}
#else
    /* Target 30 updates/s without adding a third video frame when drawing
       already consumed more than one 59.73 Hz hardware frame. */
    u32 elapsed=gba_clock()-previous_present;
    vblank();
#endif
#endif
#ifdef SC64_TILE_RENDERER
    if(gfx_tile_on) {
#ifdef SC64_DIAGNOSTIC_TILES
        u32 present_start=gba_clock();
        extern volatile u32 sc64_tile_diag_present,sc64_tile_diag_tiles_out;
        for(int ty=0;ty<20;ty++)for(u32 m=gfx_tile_dirty[ty];m;m&=m-1)sc64_tile_diag_tiles_out++;
#endif
        if(gfx_tile_present_hook)gfx_tile_present_hook();
        gfx_tile_present_tiles();
#ifdef SC64_DIAGNOSTIC_TILES
        sc64_tile_diag_present+=gba_clock()-present_start;
#endif
        front^=1;
#ifdef GBA_HOST
        host_tile_compose(host_rgb[front]);host_rgb_valid[front]=1;
#else
        previous_present=gba_clock();
#endif
        return;
    }
#ifdef GBA_HOST
    host_rgb_valid[front^1]=0;
#endif
#endif
    front^=1;
#ifndef GBA_HOST
    REG16(0x04000000)=0x0404|(front?16:0);
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(GBA_PRESENT_DEADLINE)
    previous_present=gba_clock();
#else
    if(elapsed<280896u)vblank();
    previous_present=gba_clock();
#endif
#endif
}
HOT void gfx_pixel(int x,int y,u8 c){if((unsigned)x>=240)return;
#ifdef GBA_SCENE_CLIP
 if((unsigned)y-(unsigned)gfx_clip_top>=(unsigned)(gfx_clip_bottom-gfx_clip_top))return;
#else
 if((unsigned)y>=160)return;
#endif
 c=GFX_TILE_COLOR(c);GFX_TILE_TOUCH(x,y,x+1,y+1);
 volatile u16*p=pixels+y*120+(x>>1);u16 old=*p;*p=(x&1)?((old&255)|((u16)c<<8)):((old&0xff00)|c);}
HOT_MODE4 void gfx_blit16(int x,int y,const u8 *src) {
 if(!src||x>=240||x<=-16)return;
#ifdef GBA_SCENE_CLIP
 if(y>=gfx_clip_bottom||y+16<=gfx_clip_top)return;
 int top=y<gfx_clip_top?gfx_clip_top-y:0;
 int bottom=y+16>gfx_clip_bottom?gfx_clip_bottom-y:16;
#else
 if(y>=160||y<=-16)return;
 int top=y<0?-y:0,bottom=y+16>160?160-y:16;
#endif
 GFX_TILE_TOUCH(x,y,x+16,y+16);
 if(x>=0&&x<=224&&!(x&3)&&!((uintptr_t)src&3)) {
  for(int row=top;row<bottom;row++) {
   const u32 *s=(const u32*)(src+row*16);
   volatile u32 *d=(volatile u32*)(pixels+(y+row)*120+x/2);
   d[0]=s[0];d[1]=s[1];d[2]=s[2];d[3]=s[3];
  }
 } else if(x>=0&&x<=224&&(x&3)==2&&!((uintptr_t)src&3)) {
  /* Camera offsets are commonly two pixels off a word boundary. Align the
     destination with a leading pixel pair, then join adjacent source words
     so the 12 middle pixels still use aligned 32-bit VRAM stores. */
  for(int row=top;row<bottom;row++) {
   const u32 *s=(const u32*)(src+row*16);
   u32 a=s[0],b=s[1],c=s[2],e=s[3];
   volatile u16 *d=pixels+(y+row)*120+x/2;
   *d++=(u16)a;
   volatile u32 *wide=(volatile u32*)d;
   wide[0]=(a>>16)|(b<<16);
   wide[1]=(b>>16)|(c<<16);
   wide[2]=(c>>16)|(e<<16);
   d=(volatile u16*)(wide+3);
   *d=(u16)(e>>16);
  }
 } else {
  int left=x<0?-x:0,right=x+16>240?240-x:16;
  for(int row=top;row<bottom;row++) {
   int col=left;
   volatile u16 *d=pixels+(y+row)*120+((x+col)>>1);
   if((x+col)&1){*d=(*d&255)|((u16)src[row*16+col]<<8);d++;col++;}
   for(;col+1<right;col+=2)*d++=src[row*16+col]|((u16)src[row*16+col+1]<<8);
   if(col<right)*d=(*d&0xff00)|src[row*16+col];
  }
 }
}
HOT void gfx_blit_sprite(int x,int y,int width,int height,const u8 *src) {
 if(!src||width<=0||height<=0||width>256||height>256||x>=240||x+width<=0)return;
#ifdef GBA_SCENE_CLIP
 if(y>=gfx_clip_bottom||y+height<=gfx_clip_top)return;
 int top=y<gfx_clip_top?gfx_clip_top-y:0;
 int bottom=y+height>gfx_clip_bottom?gfx_clip_bottom-y:height;
#else
 if(y>=160||y+height<=0)return;
 int top=y<0?-y:0,bottom=y+height>160?160-y:height;
#endif
 GFX_TILE_TOUCH(x,y,x+width,y+height);
 int left=x<0?-x:0,right=x+width>240?240-x:width;
 for(int row=top;row<bottom;row++) {
  int col=left;const u8*s=src+row*width;
  volatile u16*d=pixels+(y+row)*120+((x+col)>>1);
  if((x+col)&1){if(s[col])*d=(*d&255)|((u16)s[col]<<8);d++;col++;}
#ifdef GBA_SPRITE_WORDS
  /* Advancing both pointers by two preserves their alignment difference. */
  int word_right=(((uintptr_t)d^(uintptr_t)(s+col))&3u)?0:right;
#endif
  for(;col+1<right;col+=2,d++){
#ifdef GBA_SPRITE_WORDS
   /* A quad is word-copied only when ROM and VRAM are both aligned. Mixed
      quads fall through to the exact pair compositor; after one pair, both
      alignments are off and that fallback remains in use until re-aligned. */
   if(col+3<word_right && !((uintptr_t)d&3u)) {
    u32 pixels32=*(const u32 *)(const void *)(s+col);
    if(!pixels32 || !(((pixels32-0x01010101u)&~pixels32&0x80808080u))) {
     if(pixels32) *(volatile u32 *)(void *)d=pixels32;
     col+=2;
     ++d;
     continue;
    }
   }
#endif
   u8 a=s[col],b=s[col+1];
   if(a&&b)*d=a|((u16)b<<8);
   else if(a)*d=(*d&0xff00)|a;
   else if(b)*d=(*d&255)|((u16)b<<8);
  }
  if(col<right&&s[col])*d=(*d&0xff00)|s[col];
 }
}
/* dst[i] = src[n-1-i]. Source art lives in ROM, so the aligned middle is read
 * a word at a time from the end. Showcase runs it from IWRAM. */
#if defined(GBA_MAPPED_BLIT_IWRAM) && !defined(GBA_HOST)
HOT
#endif
void gfx_reverse_bytes(u8 *dst,const u8 *src,unsigned n) {
 typedef u32 __attribute__((may_alias)) word;
 const u8 *s=src+n;
 while(n&&((uintptr_t)s&3u)){*dst++=*--s;--n;}
 for(;n>=4;n-=4,dst+=4) {
  s-=4;
  u32 w=*(const word*)(const void*)s;
  dst[0]=(u8)(w>>24);dst[1]=(u8)(w>>16);dst[2]=(u8)(w>>8);dst[3]=(u8)w;
 }
 while(n){*dst++=*--s;--n;}
}
/* Keep this optional compositor in ROM: prepared profiles have little spare
 * IWRAM. Showcase, which draws many source weapon effects and has room, opts
 * into IWRAM with GBA_MAPPED_BLIT_IWRAM. Mode-4 VRAM requires halfword
 * stores, including odd clipped edges. */
#if defined(GBA_MAPPED_BLIT_IWRAM) && !defined(GBA_HOST)
HOT
#endif
void gfx_blit_sprite_mapped(int x,int y,int width,int height,const u8 *src,
                            const u8 *lookup,unsigned rows) {
 if(!src||!lookup||!rows||rows>255||width<=0||height<=0||width>256||height>256||
    x>=240||x<=-width)return;
#ifdef GBA_SCENE_CLIP
 int clip_top=gfx_clip_top,clip_bottom=gfx_clip_bottom;
#else
 int clip_top=0,clip_bottom=160;
#endif
 if(y>=clip_bottom||y<=clip_top-height)return;
 int top=y<clip_top?clip_top-y:0;
 int bottom=y>clip_bottom-height?clip_bottom-y:height;
 int left=x<0?-x:0,right=x>240-width?240-x:width;
 for(int row=top;row<bottom;++row) {
  const u8 *s=src+row*width;
  int col=left;
  volatile u16 *d=pixels+(y+row)*120+((x+col)>>1);
  if((x+col)&1) {
   unsigned a=s[col];
   if(a&&a<=rows) {u16 old=*d;*d=(old&255)|((u16)lookup[(a-1)*256+(old>>8)]<<8);}
   ++col;++d;
  }
  for(;col+1<right;col+=2,++d) {
   unsigned a=s[col],b=s[col+1];
   if((a&&a<=rows)||(b&&b<=rows)) {
    u16 old=*d;
    u8 lo=(u8)old,hi=(u8)(old>>8);
    if(a&&a<=rows)lo=lookup[(a-1)*256+lo];
    if(b&&b<=rows)hi=lookup[(b-1)*256+hi];
    *d=(u16)(lo|((u16)hi<<8));
   }
  }
  if(col<right) {
   unsigned a=s[col];
   if(a&&a<=rows) {u16 old=*d;*d=(old&0xff00)|lookup[(a-1)*256+(old&255)];}
  }
 }
}
#ifdef SC64_UNIT_WORD_SPRITES
/* Phase-aligned four-pixel raster used only for generated unit frames.
 * The sidecar's transparent lanes are already zero-padded at frame edges. */
HOT void gfx_blit_sprite_words(int x,int y,const GbaSpriteWordFrame *frame) {
 if(!frame||!frame->width||!frame->height||frame->width>256||
    frame->height>256||x>=240||x<=-(int)frame->width||
    y>=160||y<=-(int)frame->height)return;
 if(!frame->phase_words[0]||!frame->phase_words[1]||
    !frame->phase_words[2]||!frame->phase_words[3])return;
#ifdef SC64_UNIT_WORD_SPRITE_DIAGNOSTICS
 ++gfx_sprite_words_calls;
#endif
 int width=frame->width,height=frame->height;
#ifdef GBA_SCENE_CLIP
 if(y>=gfx_clip_bottom||y<=gfx_clip_top-height)return;
 int top=y<gfx_clip_top?gfx_clip_top-y:0;
 int bottom=y+height>gfx_clip_bottom?gfx_clip_bottom-y:height;
#else
 int top=y<0?-y:0;
 int bottom=y+height>160?160-y:height;
#endif
 int left=x<0?0:x,right=x+width>240?240:x+width;
 if(left>=right||top>=bottom)return;
 int phase=x%4;if(phase<0)phase+=4;
 int origin=x-phase;
 int first=left&~3,last=(right+3)&~3;
 int groups=(phase+width+3)/4;
 const u32 *row=frame->phase_words[phase]+top*groups*2+(first-origin)/4*2;
 for(int dy=y+top;dy<y+bottom;dy++) {
  volatile u32 *dst=(volatile u32 *)(void *)(pixels+dy*120+first/2);
  const u32 *src=row;
  for(int dx=first;dx<last;dx+=4,dst++,src+=2) {
   u32 mask=src[1];
   if(!mask)continue;
   u32 color=src[0];
   if(mask==0xffffffffu)*dst=color;
   else *dst=(*dst&~mask)|color;
  }
  row+=groups*2;
 }
}
#endif
#if defined(D2_DEN_OF_EVIL) || defined(GBA_HOST)
/* Transparent nearest-neighbour 2x sprite expansion for the Den scene. */
HOT void gfx_blit_sprite_2x(int x,int y,int width,int height,const u8 *src) {
 if(!src||width<=0||height<=0||width>256||height>256||x>=240||y>=160||x+width*2<=0||y+height*2<=0)return;
 int top=y<0?(-y)/2:0,bottom=(y+height*2>160)?(160-y+1)/2:height;
 int left=x<0?(-x)/2:0,right=(x+width*2>240)?(240-x+1)/2:width;
 if(bottom>height)bottom=height;
 if(right>width)right=width;
 for(int row=top;row<bottom;row++) {
  const u8 *s=src+row*width;
  for(int col=left;col<right;col++)if(s[col]) {
   u8 c=s[col];int dx=x+col*2,dy=y+row*2;
   for(int yy=0;yy<2;yy++)if((unsigned)(dy+yy)<160
#ifdef GBA_SCENE_CLIP
                              &&dy+yy>=gfx_clip_top&&dy+yy<gfx_clip_bottom
#endif
                              ) {
    volatile u16 *d=pixels+(dy+yy)*120;
    if(dx>=0&&dx+1<240&&!(dx&1))d[dx>>1]=c|((u16)c<<8);
    else {if(dx>=0&&dx<240)gfx_pixel(dx,dy+yy,c);if(dx+1>=0&&dx+1<240)gfx_pixel(dx+1,dy+yy,c);}
   }
  }
 }
}
#endif
#ifdef SC64_UNIT_FACING
/* gfx_blit_sprite with each source row mirrored horizontally. */
void gfx_blit_sprite_hflip(int x,int y,int width,int height,const u8 *src) {
 if(!src||width<=0||height<=0||width>256||height>256||x>=240||x+width<=0)return;
#ifdef GBA_SCENE_CLIP
 if(y>=gfx_clip_bottom||y+height<=gfx_clip_top)return;
 int top=y<gfx_clip_top?gfx_clip_top-y:0;
 int bottom=y+height>gfx_clip_bottom?gfx_clip_bottom-y:height;
#else
 if(y>=160||y+height<=0)return;
 int top=y<0?-y:0,bottom=y+height>160?160-y:height;
#endif
 int left=x<0?-x:0,right=x+width>240?240-x:width;
 for(int row=top;row<bottom;row++) {
  const u8*s=src+row*width+width-1;
  int col=left;
  volatile u16*d=pixels+(y+row)*120+((x+col)>>1);
  if((x+col)&1){u8 a=s[-col];if(a)*d=(*d&255)|((u16)a<<8);d++;col++;}
  for(;col+1<right;col+=2,d++){
   u8 a=s[-col],b=s[-(col+1)];
   if(a&&b)*d=a|((u16)b<<8);
   else if(a)*d=(*d&0xff00)|a;
   else if(b)*d=(*d&255)|((u16)b<<8);
  }
  if(col<right){u8 a=s[-col];if(a)*d=(*d&0xff00)|a;}
 }
}
#endif
HOT void gfx_rect(int x,int y,int w,int h,u8 color) {
    if(w<=0||h<=0||x>=240||x+w<=0)return;
#ifdef GBA_SCENE_CLIP
    if(y>=gfx_clip_bottom||y+h<=gfx_clip_top)return;
    if(y<gfx_clip_top){h-=gfx_clip_top-y;y=gfx_clip_top;}
    if(y+h>gfx_clip_bottom)h=gfx_clip_bottom-y;
    if(h<=0)return;
#else
    if(y>=160||y+h<=0)return;
#endif
    if(x<0){w+=x;x=0;}
#ifndef GBA_SCENE_CLIP
    if(y<0){h+=y;y=0;}
#endif
    if(x+w>240)w=240-x;
#ifndef GBA_SCENE_CLIP
    if(y+h>160)h=160-y;
#endif
    color=GFX_TILE_COLOR(color);
    GFX_TILE_TOUCH(x,y,x+w,y+h);
    const u16 pair=color|((u16)color<<8);
    const u32 quad=(u32)pair|((u32)pair<<16);
    for(int row=y;row<y+h;row++) {
        int left=w,xx=x;
        volatile u16 *dst=pixels+row*120+(xx>>1);
        if(xx&1){*dst=(*dst&255)|((u16)color<<8);dst++;left--;xx++;}
        if((xx&2)&&left>=2){*dst++=pair;left-=2;}
        volatile u32 *wide=(volatile u32*)dst;
        while(left>=16){wide[0]=quad;wide[1]=quad;wide[2]=quad;wide[3]=quad;wide+=4;left-=16;}
        while(left>=4){*wide++=quad;left-=4;}
        dst=(volatile u16*)wide;
        if(left>=2){*dst++=pair;left-=2;}
        if(left)*dst=(*dst&0xff00)|color;
    }
}
#if defined(GBA_SHAPES_ROM_ARM) && !defined(GBA_HOST)
#define SHAPE_HOT __attribute__((long_call,target("arm")))
#else
#define SHAPE_HOT HOT
#endif
SHAPE_HOT void gfx_frame(int x,int y,int w,int h,u8 c){gfx_rect(x,y,w,1,c);gfx_rect(x,y+h-1,w,1,c);gfx_rect(x,y,1,h,c);gfx_rect(x+w-1,y,1,h,c);}
int absi(int x){return x<0?-x:x;}
int clampi(int x,int lo,int hi){return x<lo?lo:(x>hi?hi:x);}
SHAPE_HOT void gfx_line(int x0,int y0,int x1,int y1,u8 c){
#ifdef GBA_SHAPES_ROM_ARM
 if(y0==y1){
    if((unsigned)y0>=160)return;
    int lo=x0<x1?x0:x1,hi=x0<x1?x1:x0;
    if(hi<0||lo>=240)return;
    if(lo<0)lo=0;
    if(hi>239)hi=239;
    gfx_rect(lo,y0,hi-lo+1,1,c);return;
 }
 if(x0==x1){
    if((unsigned)x0>=240)return;
    int lo=y0<y1?y0:y1,hi=y0<y1?y1:y0;
    if(hi<0||lo>=160)return;
    if(lo<0)lo=0;
    if(hi>159)hi=159;
    gfx_rect(x0,lo,1,hi-lo+1,c);return;
 }
#endif
 int dx=absi(x1-x0),sx=x0<x1?1:-1,dy=-absi(y1-y0),sy=y0<y1?1:-1,err=dx+dy;for(;;){gfx_pixel(x0,y0,c);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}}
#if (defined(SC64_P3_RUNTIME) && defined(SC64_VISIBILITY_SPANS)) || defined(SC64_T4_RUNTIME) || defined(SC64_Z5_CONSUME_ENABLED)
/* These imported profiles render from source sprites and do not call this
 * fallback. Keep it in ROM to reserve IWRAM for visibility updates. */
void gfx_circle(int x,int y,int r,u8 c){if(r<0||r>240)return;for(int yy=-r;yy<=r;yy++)for(int xx=-r;xx<=r;xx++)if(xx*xx+yy*yy<=r*r)gfx_pixel(x+xx,y+yy,c);}
#else
HOT void gfx_circle(int x,int y,int r,u8 c){if(r<0||r>240)return;for(int yy=-r;yy<=r;yy++)for(int xx=-r;xx<=r;xx++)if(xx*xx+yy*yy<=r*r)gfx_pixel(x+xx,y+yy,c);}
#endif
/* Sprite-heavy profiles can leave this fallback in ROM to make room for
 * clipping in the sprite compositors without reducing the stack reserve. */
#ifndef GBA_DIAMOND_ROM
HOT
#endif
void gfx_diamond(int x,int y,int rx,int ry,u8 color) {
    if(rx<0||ry<0||rx>480||ry>320)return;
    if(!ry){gfx_rect(x-rx,y,rx*2+1,1,color);return;}
    color=GFX_TILE_COLOR(color);
    GFX_TILE_TOUCH(x-rx,y-ry,x+rx+1,y+ry+1);
    const u16 pair=color|((u16)color<<8);
    for(int yy=-ry;yy<=ry;yy++) {
        int span=ry-(yy<0?-yy:yy);
        int half=rx==2*ry?span*2:(rx==ry?span:rx*span/ry);
        int left=x-half,right=x+half,sy=y+yy;
        if(right<0||left>=240
#ifdef GBA_SCENE_CLIP
           ||(unsigned)sy-(unsigned)gfx_clip_top>=(unsigned)(gfx_clip_bottom-gfx_clip_top)
#else
           ||(unsigned)sy>=160
#endif
           )continue;
        if(left<0)left=0;
        if(right>239)right=239;
        volatile u16 *dst=pixels+sy*120+(left>>1);
        if(left&1){*dst=(*dst&255)|((u16)color<<8);dst++;left++;}
        for(;left+1<=right;left+=2)*dst++=pair;
        if(left<=right)*dst=(*dst&0xff00)|color;
    }
}
static const u8 font[96][7]={
 [33]={14,17,17,31,17,17,17},
 [34]={30,17,17,30,17,17,30},
 [35]={15,16,16,16,16,16,15},
 [36]={30,17,17,17,17,17,30},
 [37]={31,16,16,30,16,16,31},
 [38]={31,16,16,30,16,16,16},
 [39]={15,16,16,23,17,17,15},
 [40]={17,17,17,31,17,17,17},
 [41]={31,4,4,4,4,4,31},
 [42]={7,2,2,2,18,18,12},
 [43]={17,18,20,24,20,18,17},
 [44]={16,16,16,16,16,16,31},
 [45]={17,27,21,21,17,17,17},
 [46]={17,25,25,21,19,19,17},
 [47]={14,17,17,17,17,17,14},
 [48]={30,17,17,30,16,16,16},
 [49]={14,17,17,17,21,18,13},
 [50]={30,17,17,30,20,18,17},
 [51]={15,16,16,14,1,1,30},
 [52]={31,4,4,4,4,4,4},
 [53]={17,17,17,17,17,17,14},
 [54]={17,17,17,17,17,10,4},
 [55]={17,17,17,21,21,21,10},
 [56]={17,17,10,4,10,17,17},
 [57]={17,17,10,4,4,4,4},
 [58]={31,1,2,4,8,16,31},
 [16]={14,17,19,21,25,17,14},
 [17]={4,12,4,4,4,4,14},
 [18]={14,17,1,2,4,8,31},
 [19]={30,1,1,14,1,1,30},
 [20]={2,6,10,18,31,2,2},
 [21]={31,16,16,30,1,1,30},
 [22]={14,16,16,30,17,17,14},
 [23]={31,1,2,4,8,8,8},
 [24]={14,17,17,14,17,17,14},
 [25]={14,17,17,15,1,1,14},
 [26]={0,4,4,0,4,4,0},
 [15]={1,2,2,4,8,8,16},
 [13]={0,0,0,31,0,0,0},
 [11]={0,4,4,31,4,4,0},
 [14]={0,0,0,0,0,6,6},
 [12]={0,0,0,0,6,6,4},
 [1]={4,4,4,4,4,0,4},
 [31]={14,17,1,2,4,0,4},
 [29]={0,0,31,0,31,0,0},
 [59]={14,8,8,8,8,8,14},
 [61]={14,2,2,2,2,2,14},
 [8]={2,4,8,8,8,4,2},
 [9]={8,4,2,2,2,4,8},
 [5]={25,26,2,4,8,11,19},
 [30]={16,8,4,2,4,8,16},
 [28]={1,2,4,8,4,2,1},
 [10]={0,21,14,31,14,21,0},
 [63]={0,0,0,0,0,0,31},
 [7]={4,4,0,0,0,0,0},
};

static HOT void glyph_row(int x,int y,u8 bits,u8 color) {
#ifdef GBA_SCENE_CLIP
    if ((unsigned)y-(unsigned)gfx_clip_top>=(unsigned)(gfx_clip_bottom-gfx_clip_top)) return;
#else
    if ((unsigned)y>=160) return;
#endif
    int col=0;
    /* Start on a halfword boundary, then update pairs with a single store. */
    if (x%2) {
        int px=x;
        if ((unsigned)px<240 && (bits&16)) {
            volatile u16 *dst=pixels+y*120+(px>>1);
            *dst=(*dst&255)|((u16)color<<8);
        }
        col=1;
    }
    for (;col+1<5;col+=2) {
        int px=x+col;
        if ((unsigned)px>=240 || px+1>=240) continue;
        int first=(bits&(16>>col))!=0;
        int second=(bits&(16>>(col+1)))!=0;
        if (!first&&!second) continue;
        volatile u16 *dst=pixels+y*120+(px>>1);
        if (first&&second) *dst=color|((u16)color<<8);
        else if (first) *dst=(*dst&0xff00)|color;
        else *dst=(*dst&255)|((u16)color<<8);
    }
    if (col<5) {
        int px=x+col;
        if ((unsigned)px<240 && (bits&(16>>col))) {
            volatile u16 *dst=pixels+y*120+(px>>1);
            *dst=(*dst&0xff00)|color;
        }
    }
}
HOT void gfx_text(int x,int y,const char*t,u8 c){int origin=x;
#ifdef SC64_TILE_RENDERER
if(gfx_tile_on){c=GFX_TILE_COLOR(c);int w=0,h=1,line=0;for(const char*q=t;*q;q++){if(*q=='\n'){h++;line=0;}else if(++line>w)w=line;}gfx_tile_touch(x,y,x+w*6,y+(h-1)*9+7);}
#endif
for(;*t;t++){u8 ch=(u8)*t;if(ch=='\n'){x=origin;y+=9;continue;}if(ch>='a'&&ch<='z')ch-=32;if(ch>=32&&ch<128&&x>-6&&x<240)for(int row=0;row<7;row++)glyph_row(x,y+row,font[ch-32][row],c);x+=6;}}
#if defined(SC64_ZP_FULL_STORAGE) || defined(GBA_SCENE_CLIP)
/* Later campaign actors need the hot-code reserve; number formatting can
 * run from ROM while glyph drawing stays in IWRAM. Scene clipping also uses
 * this cold formatter placement to stay within the canonical IWRAM reserve. */
void gfx_num(
#else
HOT void gfx_num(
#endif
int x,int y,int n,u8 c){char b[12];int i=0;u32 v;if(n<0){gfx_text(x,y,"-",c);x+=6;v=0u-(u32)n;}else v=(u32)n;do{b[i++]='0'+v%10;v/=10;}while(v&&i<11);for(int j=0;j<i;j++){char z[2]={b[i-j-1],0};gfx_text(x+j*6,y,z,c);}}
#if defined(SC64_ZP_CAMPAIGN_STORAGE) && defined(SC64_TARGET_OCCUPANCY)
/* Reserve IWRAM for the expanded actor targeting loop in this profile. */
void gfx_bar(
#else
HOT void gfx_bar(
#endif
int x,int y,int w,int h,int v,int max,u8 c,u8 empty){
    if(max<=0||v<=0){gfx_rect(x,y,w,h,empty);return;}
    /* A full bar is just the foreground rectangle; the empty fill is entirely
       covered, so skip both the overdraw and the old width multiplication. */
    if(v>=max){gfx_rect(x,y,w,h,c);return;}
    gfx_rect(x,y,w,h,empty);
    gfx_rect(x,y,w*clampi(v,0,max)/max,h,c);
}
u32 rng_next(u32*s){u32 x=*s;if(!x)x=0x6d2b79f5u;x^=x<<13;x^=x>>17;x^=x<<5;*s=x;return x;}
static u32 hash_bytes(const u8*p,u32 n){u32 h=2166136261u;while(n--){h^=*p++;h*=16777619u;}return h;}
#ifdef GBA_HOST
#define SRAM_BASE host_save
#else
#define SRAM_BASE ((u8*)0x0e000000)
#endif
static void sram_put32(volatile u8*p,u32 v){for(int i=0;i<4;i++)p[i]=(u8)(v>>(8*i));}
static u32 sram_get32(volatile const u8*p){u32 v=0;for(int i=0;i<4;i++)v|=(u32)p[i]<<(8*i);return v;}
/* Two alternating 16 KiB records preserve the previous save if power is
   lost during a write. Commit the tag last; a checksum covers the payload. */
#define SAVE_SLOT_SIZE 16384u
#define SAVE_PAYLOAD_MAX (SAVE_SLOT_SIZE-16u)
/* A zero len accepts any stored payload length; slot alternation and the
   sequence clock are shared by every record with the same tag. */
static int valid_record(volatile const u8 *p,u32 len,u32 tag) {
    u32 stored=sram_get32(p+4);
    if(sram_get32(p)!=tag||!stored||stored>SAVE_PAYLOAD_MAX||(len&&stored!=len)||
       !sram_get32(p+12))return 0;
    u32 h=2166136261u;
    for(u32 i=0;i<stored;i++){h^=p[16+i];h*=16777619u;}
    return h==sram_get32(p+8);
}
static int newest_record(u32 len,u32 tag) {
    volatile const u8 *base=(volatile const u8*)SRAM_BASE;
    int a=valid_record(base,len,tag),b=valid_record(base+SAVE_SLOT_SIZE,len,tag);
    if(!a&&!b)return -1;
    if(!a)return 1;
    if(!b)return 0;
    return (s32)(sram_get32(base+SAVE_SLOT_SIZE+12)-sram_get32(base+12))>0?1:0;
}
void save_write(const void *data,u32 len,u32 tag) {
    if(!len||len>SAVE_PAYLOAD_MAX||!tag)return;
    volatile u8 *base=(volatile u8*)SRAM_BASE;
    int newest=newest_record(0,tag);
    u32 sequence=newest<0?1:sram_get32(base+newest*SAVE_SLOT_SIZE+12)+1;
    if(!sequence)sequence=1;
    volatile u8 *p=base+(newest==0?SAVE_SLOT_SIZE:0);
    const u8 *src=data;
    sram_put32(p,0);
    sram_put32(p+4,len);
    sram_put32(p+8,hash_bytes(src,len));
    sram_put32(p+12,sequence);
    for(u32 i=0;i<len;i++)p[16+i]=src[i];
    sram_put32(p,tag);
}
#ifdef SC64_SHARED_TRANSPORT
/* The normal journal protocol, with a short trailer held outside its large
 * shared save scratch. Both spans are covered by one checksum and tag. */
int save_write_parts(const void *data,u32 len,const void *tail,u32 tail_len,u32 tag) {
    if(!data||!tail||!len||!tail_len||len>SAVE_PAYLOAD_MAX||
       tail_len>SAVE_PAYLOAD_MAX-len||!tag)return 0;
    const u8 *a=data,*b=tail;u32 h=2166136261u;
    for(u32 i=0;i<len;i++){h^=a[i];h*=16777619u;}
    for(u32 i=0;i<tail_len;i++){h^=b[i];h*=16777619u;}
    volatile u8 *base=(volatile u8*)SRAM_BASE;
    int newest=newest_record(0,tag);
    u32 seq=newest<0?1:sram_get32(base+newest*SAVE_SLOT_SIZE+12)+1;
    if(!seq)seq=1;
    volatile u8 *p=base+(newest==0?SAVE_SLOT_SIZE:0);
    sram_put32(p,0);sram_put32(p+4,len+tail_len);sram_put32(p+8,h);sram_put32(p+12,seq);
    for(u32 i=0;i<len;i++)p[16+i]=a[i];
    for(u32 i=0;i<tail_len;i++)p[16+len+i]=b[i];
    sram_put32(p,tag);return 1;
}
int save_read_parts(void *data,u32 len,void *tail,u32 tail_len,u32 tag) {
    if(!data||!tail||!len||!tail_len||len>SAVE_PAYLOAD_MAX||
       tail_len>SAVE_PAYLOAD_MAX-len||!tag)return 0;
    int newest=newest_record(len+tail_len,tag);if(newest<0)return 0;
    volatile const u8 *p=(volatile const u8*)SRAM_BASE+newest*SAVE_SLOT_SIZE+16;
    u8 *a=data,*b=tail;
    for(u32 i=0;i<len;i++)a[i]=p[i];
    for(u32 i=0;i<tail_len;i++)b[i]=p[len+i];
    return 1;
}
#endif

/* Preserve the newest readable predecessor when writing the first record of
 * a new format. Tags share the same physical two-slot SRAM journal. */
void save_write_migrating(const void *data,u32 len,u32 tag,u32 legacy_len,u32 legacy_tag) {
    if(len>SAVE_PAYLOAD_MAX||legacy_len>SAVE_PAYLOAD_MAX||!tag||!legacy_tag)return;
    volatile u8 *base=(volatile u8*)SRAM_BASE;
    int current=newest_record(0,tag), legacy=newest_record(legacy_len,legacy_tag), newest;
    if(current<0)newest=legacy;
    else if(legacy<0)newest=current;
    else newest=(s32)(sram_get32(base+current*SAVE_SLOT_SIZE+12)-
                      sram_get32(base+legacy*SAVE_SLOT_SIZE+12))>0?current:legacy;
    u32 sequence=newest<0?1:sram_get32(base+newest*SAVE_SLOT_SIZE+12)+1;
    if(!sequence)sequence=1;
    volatile u8 *p=base+(newest==0?SAVE_SLOT_SIZE:0);
    const u8 *src=data;
    sram_put32(p,0);sram_put32(p+4,len);sram_put32(p+8,hash_bytes(src,len));
    sram_put32(p+12,sequence);
    for(u32 i=0;i<len;i++)p[16+i]=src[i];
    sram_put32(p,tag);
}
/* An incompatible experimental record may share a tag. It must not displace
 * the readable legacy predecessor during the first committed feature write. */
void save_write_migrating_feature(const void *data,u32 len,u32 tag,u32 legacy_tag,
                                  u32 marker_offset,u32 marker_mask,u32 marker_value) {
    volatile u8 *base=(volatile u8*)SRAM_BASE;
    int current=-1, legacy, newest;
    if (!len || len>SAVE_PAYLOAD_MAX || !tag || !legacy_tag ||
        marker_offset>SAVE_PAYLOAD_MAX-4u) return;
    for (int i=0;i<2;++i) {
        volatile const u8 *p=base+i*SAVE_SLOT_SIZE;
        if (!valid_record(p,0,tag) || sram_get32(p+4)<marker_offset+4u ||
            (sram_get32(p+16+marker_offset)&marker_mask)!=marker_value) continue;
        if (current<0 || (s32)(sram_get32(p+12)-
            sram_get32(base+current*SAVE_SLOT_SIZE+12))>0) current=i;
    }
    legacy=newest_record(0,legacy_tag);
    if(current<0)newest=legacy;
    else if(legacy<0)newest=current;
    else newest=(s32)(sram_get32(base+current*SAVE_SLOT_SIZE+12)-
                      sram_get32(base+legacy*SAVE_SLOT_SIZE+12))>0?current:legacy;
    u32 sequence=newest<0?1:sram_get32(base+newest*SAVE_SLOT_SIZE+12)+1;
    if(!sequence)sequence=1;
    volatile u8 *p=base+(newest==0?SAVE_SLOT_SIZE:0);
    const u8 *src=data;
    sram_put32(p,0);sram_put32(p+4,len);sram_put32(p+8,hash_bytes(src,len));
    sram_put32(p+12,sequence);
    for(u32 i=0;i<len;i++)p[16+i]=src[i];
    sram_put32(p,tag);
}

int save_read(void *data,u32 len,u32 tag) {
    if(len>SAVE_PAYLOAD_MAX||!tag)return 0;
    int newest=newest_record(len,tag);
    if(newest<0)return 0;
    volatile const u8 *src=(volatile const u8*)SRAM_BASE+newest*SAVE_SLOT_SIZE+16;
    u8 *dst=data;
    for(u32 i=0;i<len;i++)dst[i]=src[i];
    return 1;
}
/* Newest valid record for tag, whatever its length, if it fits capacity. */
int save_read_latest(void *data,u32 capacity,u32 tag,u32 *len) {
    if(!tag||!len)return 0;
    int newest=newest_record(0,tag);
    if(newest<0)return 0;
    volatile const u8 *p=(volatile const u8*)SRAM_BASE+newest*SAVE_SLOT_SIZE;
    u32 stored=sram_get32(p+4);
    if(stored>capacity)return 0;
    u8 *dst=data;
    for(u32 i=0;i<stored;i++)dst[i]=p[16+i];
    *len=stored;
    return 1;
}
#if defined(SC64_SCARAB_RUNTIME) || defined(SC64_SHARED_HP_REGEN) || defined(SC64_WORKER_CARGO_KIND)
#ifdef SC64_SHARED_HP_REGEN
/* SRAM tag stores are bytewise. A nonzero high byte commits this namespace;
 * clear it before changing any other byte and restore it only after the
 * complete record. Partial tags remain retryable, not unrelated records. */
#define SAVE_VALIDATED_TAG_VALID(tag) (((tag)&0xff000000u)!=0)
#if defined(GBA_HOST) && defined(GBA_SAVE_TAG_STORE_TEST)
#define SAVE_VALIDATED_TAG_BYTE(p,i,value) GBA_SAVE_TAG_STORE_TEST(p,i,(u8)(value))
#else
#define SAVE_VALIDATED_TAG_BYTE(p,i,value) ((p)[i]=(u8)(value))
#endif
static void save_validated_invalidate(volatile u8 *p) {
    SAVE_VALIDATED_TAG_BYTE(p,3,0); SAVE_VALIDATED_TAG_BYTE(p,0,0);
    SAVE_VALIDATED_TAG_BYTE(p,1,0); SAVE_VALIDATED_TAG_BYTE(p,2,0);
}
static void save_validated_commit_tag(volatile u8 *p,u32 tag) {
    SAVE_VALIDATED_TAG_BYTE(p,0,tag); SAVE_VALIDATED_TAG_BYTE(p,1,tag>>8);
    SAVE_VALIDATED_TAG_BYTE(p,2,tag>>16); SAVE_VALIDATED_TAG_BYTE(p,3,tag>>24);
}
#define SAVE_VALIDATED_INVALIDATE(p) save_validated_invalidate(p)
#define SAVE_VALIDATED_COMMIT_TAG(p,tag) save_validated_commit_tag(p,tag)
#else
#define SAVE_VALIDATED_TAG_VALID(tag) ((tag)!=0)
#define SAVE_VALIDATED_INVALIDATE(p) sram_put32(p,0)
#define SAVE_VALIDATED_COMMIT_TAG(p,tag) sram_put32(p,tag)
#endif
#define SAVE_VALIDATED_IS_COMMITTED(p,tag) \
    (SAVE_VALIDATED_TAG_VALID(tag)&&valid_record(p,0,tag))
int save_select_validated(void *scratch,u32 capacity,const u32 *tags,unsigned tag_count,
                          GbaSaveValidator validate,void *context,GbaSaveSelection *selection) {
    volatile const u8 *base=(volatile const u8*)SRAM_BASE;
    GbaSaveSelection best={0,0,0,0,-1};
    if(!scratch||!capacity||!tags||!tag_count||tag_count>3||!validate||!selection)return 0;
    for(unsigned i=0;i<tag_count;i++) {
        if(!SAVE_VALIDATED_TAG_VALID(tags[i]))return 0;
        for(unsigned j=0;j<i;j++)if(tags[i]==tags[j])return 0;
    }
    for(int slot=0;slot<2;slot++) {
        volatile const u8 *p=base+(unsigned)slot*SAVE_SLOT_SIZE;
        u32 tag=sram_get32(p),len=sram_get32(p+4),seq=sram_get32(p+12);int allowed=0;
        for(unsigned i=0;i<tag_count;i++)if(tags[i]==tag)allowed=1;
        if(!allowed||len>capacity||!valid_record(p,0,tag))continue;
        for(u32 i=0;i<len;i++)((u8*)scratch)[i]=p[16+i];
        if(!validate(scratch,len,tag,context))continue;
        if(best.slot>=0 && seq-best.sequence==0x80000000u)return 0;
        if(best.slot<0||(s32)(seq-best.sequence)>0)
            best=(GbaSaveSelection){tag,len,seq,sram_get32(p+8),slot};
    }
    if(best.slot<0)return 0;
    volatile const u8 *p=base+(unsigned)best.slot*SAVE_SLOT_SIZE;
    if(!valid_record(p,best.length,best.tag)||sram_get32(p+12)!=best.sequence||
       sram_get32(p+8)!=best.checksum)return 0;
    for(u32 i=0;i<best.length;i++)((u8*)scratch)[i]=p[16+i];
    *selection=best;return 1;
}
int save_select_transaction(void *scratch,u32 capacity,const u32 *tags,unsigned tag_count,
                            GbaSaveValidator prepare,GbaSaveCommit commit,void *context,
                            GbaSaveSelection *selection) {
    GbaSaveSelection chosen;
    if(!commit || !selection ||
       !save_select_validated(scratch,capacity,tags,tag_count,prepare,context,&chosen))return 0;
    if(!prepare(scratch,chosen.length,chosen.tag,context))return 0;
    commit(context);
    *selection=chosen;
    return 1;
}
int save_write_selected(const void *data,u32 len,u32 tag,const GbaSaveSelection *previous,
                        const u32 *tags,unsigned tag_count) {
    volatile u8 *base=(volatile u8*)SRAM_BASE;
    int new_allowed=0,old_allowed=0;
    if(!data||!len||len>SAVE_PAYLOAD_MAX||!SAVE_VALIDATED_TAG_VALID(tag)||!previous||!tags||!tag_count||tag_count>3||
       previous->slot<0||previous->slot>1||!previous->tag||!previous->length||!previous->sequence)return 0;
    for(unsigned i=0;i<tag_count;i++) {
        if(!SAVE_VALIDATED_TAG_VALID(tags[i]))return 0;
        for(unsigned j=0;j<i;j++)if(tags[i]==tags[j])return 0;
        if(tags[i]==tag)new_allowed=1;
        if(tags[i]==previous->tag)old_allowed=1;
    }
    if(!new_allowed||!old_allowed)return 0;
    volatile const u8 *old=base+(unsigned)previous->slot*SAVE_SLOT_SIZE;
    if(!valid_record(old,previous->length,previous->tag)||sram_get32(old+12)!=previous->sequence||
       sram_get32(old+8)!=previous->checksum)return 0;
    volatile u8 *p=base+(unsigned)(1-previous->slot)*SAVE_SLOT_SIZE;
    u32 other=sram_get32(p);int other_allowed=0;
    for(unsigned i=0;i<tag_count;i++)if(tags[i]==other)other_allowed=1;
    if(!other_allowed && SAVE_VALIDATED_IS_COMMITTED(p,other))return 0;
    u32 sequence=previous->sequence+1;if(!sequence)sequence=1;
    const u8 *src=data;
    SAVE_VALIDATED_INVALIDATE(p);sram_put32(p+4,len);sram_put32(p+8,hash_bytes(src,len));sram_put32(p+12,sequence);
    for(u32 i=0;i<len;i++)p[16+i]=src[i];
    SAVE_VALIDATED_COMMIT_TAG(p,tag);return 1;
}
int save_write_first(const void *data,u32 len,u32 tag) {
    volatile u8 *base=(volatile u8*)SRAM_BASE;
    if(!data||!len||len>SAVE_PAYLOAD_MAX||!SAVE_VALIDATED_TAG_VALID(tag))return 0;
    for(unsigned slot=0;slot<2;slot++) {
        volatile const u8 *p=base+slot*SAVE_SLOT_SIZE;u32 existing=sram_get32(p);
        if(SAVE_VALIDATED_IS_COMMITTED(p,existing))return 0;
    }
    const u8 *src=data;volatile u8 *p=base;
    SAVE_VALIDATED_INVALIDATE(p);sram_put32(p+4,len);sram_put32(p+8,hash_bytes(src,len));sram_put32(p+12,1);
    for(u32 i=0;i<len;i++)p[16+i]=src[i];
    SAVE_VALIDATED_COMMIT_TAG(p,tag);return 1;
}
#ifdef SC64_SHARED_HP_REGEN
static u32 save_parts_checksum(const u8 *a,u32 len,const u8 *b,u32 tail_len) {
    u32 h=2166136261u;
    for(u32 i=0;i<len;i++){h^=a[i];h*=16777619u;}
    for(u32 i=0;i<tail_len;i++){h^=b[i];h*=16777619u;}
    return h;
}
static void save_parts_commit(volatile u8 *p,const u8 *a,u32 len,
                               const u8 *b,u32 tail_len,u32 tag,u32 sequence) {
    u32 checksum=save_parts_checksum(a,len,b,tail_len);
    SAVE_VALIDATED_INVALIDATE(p);sram_put32(p+4,len+tail_len);
    sram_put32(p+8,checksum);sram_put32(p+12,sequence);
    for(u32 i=0;i<len;i++)p[16+i]=a[i];
    for(u32 i=0;i<tail_len;i++)p[16+len+i]=b[i];
    SAVE_VALIDATED_COMMIT_TAG(p,tag);
}
int save_write_parts_selected(const void *data,u32 len,const void *tail,u32 tail_len,
                              u32 tag,const GbaSaveSelection *previous,
                              const u32 *tags,unsigned tag_count) {
    volatile u8 *base=(volatile u8*)SRAM_BASE;
    int new_allowed=0,old_allowed=0;
    if(!data||!tail||!len||!tail_len||len>SAVE_PAYLOAD_MAX||tail_len>SAVE_PAYLOAD_MAX-len||
       !SAVE_VALIDATED_TAG_VALID(tag)||!previous||!tags||!tag_count||tag_count>3||previous->slot<0||previous->slot>1||
       !previous->tag||!previous->length||!previous->sequence)return 0;
    for(unsigned i=0;i<tag_count;i++) {
        if(!SAVE_VALIDATED_TAG_VALID(tags[i]))return 0;
        for(unsigned j=0;j<i;j++)if(tags[i]==tags[j])return 0;
        if(tags[i]==tag)new_allowed=1;
        if(tags[i]==previous->tag)old_allowed=1;
    }
    if(!new_allowed||!old_allowed)return 0;
    volatile const u8 *old=base+(unsigned)previous->slot*SAVE_SLOT_SIZE;
    if(!valid_record(old,previous->length,previous->tag)||sram_get32(old+12)!=previous->sequence||
       sram_get32(old+8)!=previous->checksum)return 0;
    volatile u8 *p=base+(unsigned)(1-previous->slot)*SAVE_SLOT_SIZE;
    u32 other=sram_get32(p);int other_allowed=0;
    for(unsigned i=0;i<tag_count;i++)if(tags[i]==other)other_allowed=1;
    if(!other_allowed&&SAVE_VALIDATED_IS_COMMITTED(p,other))return 0;
    u32 sequence=previous->sequence+1;if(!sequence)sequence=1;
    save_parts_commit(p,data,len,tail,tail_len,tag,sequence);
    return 1;
}
int save_write_parts_first(const void *data,u32 len,const void *tail,u32 tail_len,u32 tag) {
    volatile u8 *base=(volatile u8*)SRAM_BASE;
    if(!data||!tail||!len||!tail_len||len>SAVE_PAYLOAD_MAX||tail_len>SAVE_PAYLOAD_MAX-len||!SAVE_VALIDATED_TAG_VALID(tag))return 0;
    for(unsigned slot=0;slot<2;slot++) {
        volatile const u8 *p=base+slot*SAVE_SLOT_SIZE;u32 existing=sram_get32(p);
        if(SAVE_VALIDATED_IS_COMMITTED(p,existing))return 0;
    }
    save_parts_commit(base,data,len,tail,tail_len,tag,1);
    return 1;
}
#endif
#undef SAVE_VALIDATED_TAG_VALID
#undef SAVE_VALIDATED_INVALIDATE
#undef SAVE_VALIDATED_COMMIT_TAG
#undef SAVE_VALIDATED_IS_COMMITTED
#undef SAVE_VALIDATED_TAG_BYTE
#endif
#ifdef GBA_HOST
void save_test_corrupt_latest(u32 len,u32 tag) {
    int newest=newest_record(len,tag);
    if(newest>=0)host_save[newest*SAVE_SLOT_SIZE+16]^=0x80;
}
#endif
#ifdef GBA_HOST
u32 audio_host_tones, audio_host_noises, audio_host_samples;
int audio_host_last_sample=-1, audio_host_music=-1;
#endif
void audio_tone(int hz,int frames,int volume){
#ifdef GBA_HOST
(void)hz; (void)frames; (void)volume; ++audio_host_tones;
#else
hz=clampi(hz,64,2048);int f=2048-131072/hz;REG16(0x04000068)=((clampi(volume,0,15)<<12)|0x0080);REG16(0x0400006c)=0x8000|f;tone_frames=frames;
#endif
}
/* PSG noise placeholder for builds without a sampled bank. */
void audio_noise(int frames,int volume){
#ifdef GBA_HOST
(void)frames; (void)volume; ++audio_host_noises;
#else
REG16(0x04000078)=(u16)((clampi(volume,0,15)<<12)|0x0200);REG16(0x0400007c)=0x8000|0x0024;noise_frames=frames;
#endif
}
void audio_update(void){
#ifndef GBA_HOST
if(tone_frames>0&&!--tone_frames)REG16(0x04000068)=0;
if(noise_frames>0&&!--noise_frames)REG16(0x04000078)=0;
#endif
}
#if defined(SC64_AUDIO_IMPORTED)
#include "sc64_audio_generated.h"
#ifdef GBA_HOST
/* Host builds queue requests but never mix the bank. */
static const u8 sc64_audio_blob[4];
#else
/* Linked from common/audio_bank.S; private data under .local/ only. */
extern const u8 sc64_audio_blob[];
#endif
static const AudioBank audio_bank={
 sc64_audio_blob,sc64_audio_sample_offset,sc64_audio_sample_length,SC64_AUDIO_SAMPLE_COUNT,
 sc64_audio_music_offset,sc64_audio_music_frames,SC64_AUDIO_MUSIC_TRACKS
};
#ifndef SC64_AUDIO_FORMAT
#error "sc64_audio_generated.h predates codec format 2; rerun tools/sc64_audio_pack.py"
#else
_Static_assert(SC64_AUDIO_FORMAT==AUDIO_CODEC_FORMAT,"audio bank and decoder formats differ");
#endif
_Static_assert(SC64_AUDIO_SAMPLES_PER_FRAME==AUDIO_FRAME_SAMPLES,"bank and mixer frame sizes differ");
/* Mix and music double buffers with their guards, the decode scratch, then
 * the decoder states (see the arena layout in gba.h). */
#define AUDIO_DOUBLE_BYTES (2*AUDIO_FRAME_SAMPLES+64)
#define AUDIO_DECODER_OFFSET (2*AUDIO_DOUBLE_BYTES+AUDIO_VOICES*AUDIO_FRAME_SAMPLES)
#ifndef GBA_HOST
_Static_assert(sizeof(AudioDecoder)==12,"decoder state is not 12 bytes");
_Static_assert(GBA_AUDIO_MIX_BYTES>=AUDIO_DECODER_OFFSET+AUDIO_DECODERS*sizeof(AudioDecoder),"audio arena lacks a buffer, guard or decoder");
#endif
static AudioMixer audio_mixer;
int audio_sampled_available(void){return 1;}
void audio_play_sample(u16 sample,u8 priority,u8 kind){
#ifdef GBA_HOST
 ++audio_host_samples; audio_host_last_sample=sample;
#endif
 audio_mixer_request(&audio_mixer,sample,priority,kind);
}
void audio_play_music(int track,int loop){
#ifdef GBA_HOST
 audio_host_music=track;
#endif
 audio_mixer_music(&audio_mixer,track<0?AUDIO_MUSIC_STOP:(u8)track,(u8)(loop!=0));
}
int audio_music_track(void){
 u8 request=audio_mixer.music_request;
 if(request!=AUDIO_MUSIC_NONE)return request==AUDIO_MUSIC_STOP?-1:request;
 return audio_mixer.music_track==AUDIO_MUSIC_NONE?-1:audio_mixer.music_track;
}
#ifndef GBA_HOST
#ifdef SC64_TILE_RENDERER
#error "The audio buffers use the gap between the two mode-4 pages"
#endif
/* Two double buffers between the mode-4 pages (VRAM 0x06009600-0x06009FFF
 * is unused), each two contiguous 176-sample halves: DMA1 plays the mix on
 * Direct Sound A, DMA2 the music on Direct Sound B. Both DMAs run back to
 * back and are re-armed at their first half every second frame; each VBlank
 * mixes and decodes into the halves that just finished. Everything stays
 * halfword-written; the zero guard after each pair is never written, so a
 * delayed re-arm reads silence. Voices decode into the scratch slots. */
#define AUDIO_MIX_BUFFER ((volatile u16*)GBA_AUDIO_ARENA)
#define AUDIO_MUSIC_BUFFER ((volatile u16*)(GBA_AUDIO_ARENA+AUDIO_DOUBLE_BYTES))
#define AUDIO_SCRATCH ((volatile u16*)(GBA_AUDIO_ARENA+2*AUDIO_DOUBLE_BYTES))
/* Word and halfword fields only, so VRAM takes every store. */
#define AUDIO_DECODER_STATES ((AudioDecoder*)(GBA_AUDIO_ARENA+AUDIO_DECODER_OFFSET))
volatile u32 audio_irq_cycles,audio_irq_peak,audio_irq_frames;
/* audio_music_clear: the mixer zeroed one music half when music stopped;
 * the other half still holds the last frame and is zeroed next VBlank. */
static u8 audio_half,audio_music_clear;
/* Thumb, from ROM: the IWRAM reserve is too small for the mixer. */
static void __attribute__((noinline)) audio_vblank(void){
 u32 start=gba_clock();
 u32 half=0;
 if(audio_half){
  REG32(0x040000c4)=0;REG32(0x040000bc)=(u32)AUDIO_MIX_BUFFER;REG32(0x040000c4)=0xb6400000u;
  REG32(0x040000d0)=0;REG32(0x040000c8)=(u32)AUDIO_MUSIC_BUFFER;REG32(0x040000d0)=0xb6400000u;
  half=AUDIO_FRAME_SAMPLES/2;
 }
 audio_half^=1;
 int music=audio_mixer_frame(&audio_mixer,&audio_bank,AUDIO_MIX_BUFFER+half,AUDIO_SCRATCH,AUDIO_MUSIC_BUFFER+half);
 if(audio_music_clear&&audio_mixer.music_track==AUDIO_MUSIC_NONE){
  volatile u32 *z=(volatile u32*)(volatile void*)(AUDIO_MUSIC_BUFFER+half);
  for(u32 i=0;i<AUDIO_FRAME_SAMPLES/4;i++)z[i]=0;
 }
 audio_music_clear=music==AUDIO_MUSIC_SILENCE;
 u32 spent=gba_clock()-start;
 audio_irq_cycles+=spent;++audio_irq_frames;
 if(spent>audio_irq_peak)audio_irq_peak=spent;
}
/* The BIOS dispatcher enters this with an ARM-state load of 0x03007FFC. */
__attribute__((target("arm"),noinline)) static void audio_irq(void){
 u16 flags=REG16(0x04000202)&REG16(0x04000200);
 if(flags&1)audio_vblank();
 REG16(0x04000202)=flags;
 *(volatile u16*)0x03007ff8|=flags;
}
static void audio_hw_init(void){
 audio_mixer_reset(&audio_mixer,AUDIO_DECODER_STATES);
 for(u32 i=0;i<GBA_AUDIO_MIX_BYTES/2;i++)AUDIO_MIX_BUFFER[i]=0;
 audio_half=0;audio_music_clear=0;
 /* PSG 100%, Direct Sound A 100% and B 50%, both on timer 0, both FIFOs
  * reset, both output to left and right; silent music is a zero buffer. */
 REG16(0x04000082)=0x0002|0x0004|0x0300|0x3000|0x0800|0x8000;
 REG16(0x04000102)=0;REG16(0x04000100)=(u16)(65536u-280896u/AUDIO_FRAME_SAMPLES);REG16(0x04000102)=0x0080;
 REG32(0x040000c4)=0;REG32(0x040000bc)=(u32)AUDIO_MIX_BUFFER;REG32(0x040000c0)=0x040000a0;
 REG32(0x040000c4)=0xb6400000u;
 REG32(0x040000d0)=0;REG32(0x040000c8)=(u32)AUDIO_MUSIC_BUFFER;REG32(0x040000cc)=0x040000a4;
 REG32(0x040000d0)=0xb6400000u;
 *(volatile u32*)0x03007ffc=(u32)audio_irq;
 REG16(0x04000004)|=0x0008;
 REG16(0x04000200)=0x0001;
 REG16(0x04000208)=1;
}
#endif
#else
int audio_sampled_available(void){return 0;}
void audio_play_sample(u16 sample,u8 priority,u8 kind){(void)sample;(void)priority;(void)kind;}
void audio_play_music(int track,int loop){(void)track;(void)loop;}
int audio_music_track(void){return -1;}
#endif

#ifdef GBA_HOST
u8 gfx_test_pixel(int x,int y){if((unsigned)x>=240||(unsigned)y>=160)return 0;u16 v=pixels[y*120+x/2];return (u8)(x&1?v>>8:v);}
void gfx_dump_ppm(const char* path){FILE*f=fopen(path,"wb");if(!f)return;fprintf(f,"P6\n240 160\n255\n");for(int i=0;i<38400;i++){u16 a=host_fb[front][i/2];u8 idx=(i&1)?a>>8:a&255;u16 v=host_palette[idx];u8 rgb[3]={(u8)((v&31)*255/31),(u8)(((v>>5)&31)*255/31),(u8)(((v>>10)&31)*255/31)};fwrite(rgb,1,3,f);}fclose(f);}
#endif
