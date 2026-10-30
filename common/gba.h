#ifndef GBA_H
#define GBA_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
#if defined(GBA_HOST)
#define GBA_HOT
#else
#define GBA_HOT __attribute__((section(".iwram"),long_call,target("arm")))
#endif
enum { KEY_A=1,KEY_B=2,KEY_SELECT=4,KEY_START=8,KEY_RIGHT=16,KEY_LEFT=32,KEY_UP=64,KEY_DOWN=128,KEY_R=256,KEY_L=512 };
enum { C_BLACK=0,C_WHITE,C_DIM,C_PANEL,C_EDGE,C_RED,C_RED_DARK,C_GREEN,C_GREEN_DARK,C_BLUE,C_BLUE_DARK,C_GOLD,C_GOLD_DARK,C_CYAN,C_PURPLE,C_ORANGE,C_BROWN,C_DIRT,C_STONE,C_GRASS,C_SAND,C_SILVER,C_SKIN,C_SHADOW,C_TERRAN,C_ZERG,C_PROTOSS,C_LAVA,C_BLOOD,C_FOG,C_UI,C_HIGHLIGHT };
void gba_init(void);
u16 gba_keys(void);
u32 gba_clock(void);
void gfx_begin(u8 background);
#ifdef GBA_SCENE_CLIP
/* Restrict drawing primitives to [top,bottom). Invalid ranges are ignored.
 * Explicit band/window copy helpers keep their validated spans and do
 * not consult this scene clip. Every begin call restores the full screen. */
void gfx_set_clip_y(int top,int bottom);
/* Current valid range: top in the low byte, exclusive bottom in the high. */
u16 gfx_get_clip_y(void);
#endif
/* The buffer selected by the last begin call, and its explicit-clear count.
 * Preserve-mode begin leaves that count unchanged for reusable pixel bands. */
u8 gfx_backbuffer_index(void);
u32 gfx_clear_generation(u8 buffer_index);
#if defined(SC64_NO_FRAME_CLEAR) || defined(GBA_HOST)
void gfx_begin_preserve(void);
#endif
void gfx_end(void);
int gfx_read_band(int y,int height,u8 *aligned_pixels);
int gfx_blit_band(int y,int height,const u8 *aligned_pixels);
/* Copy a full-width band from the page being displayed into the backbuffer.
 * The displayed page is never written while it is shown, so its bands can
 * stand in for an EWRAM copy of pixels that both pages must share. */
int gfx_copy_band_from_front(int y,int height);
/* Opaque indexed rows at any screen position; every source pixel is stored,
 * including color 0. Uses the current scene clip. */
void gfx_blit_opaque(int x,int y,int width,int height,const u8 *indexed_pixels);
/* The two mode-4 pages end at 0x06013600 and bitmap products keep objects
 * off, so the OBJ tile area 0x06014000-0x06017FFF holds data instead. It is
 * shared, not free: starcraft/sc64_obj_vram.h assigns its ranges (the P7
 * projectile arena first, pixel caches after), and the host backs the whole
 * area with one array so overlaps surface in fixtures. VRAM takes no byte
 * stores: write it with gfx_read_band or word stores. */
#define GBA_VRAM_SCRATCH_BYTES 16384u
#ifdef GBA_HOST
extern u8 gba_vram_scratch[GBA_VRAM_SCRATCH_BYTES];
#define GBA_VRAM_SCRATCH gba_vram_scratch
#else
#define GBA_VRAM_SCRATCH ((u8 *)0x06014000)
#endif
int gfx_blit_window(int y,int height,const u8 *source,int stride,int rows,int left,int top);
int gfx_blit_terrain_window(int y,int height,const u8 *source,int stride,int rows,int left,int top);
#if defined(D2_DEN_OF_EVIL) || defined(GBA_HOST)
int gfx_blit_window_2x(int y,int height,const u8 *source,int stride,int rows,int left,int top);
#endif
void gfx_set_palette(int first,int count,const u16 *rgb555);
void gfx_blit16(int x,int y,const u8 *indexed_pixels);
void gfx_blit_sprite(int x,int y,int width,int height,const u8 *indexed_pixels);
/* Destination-dependent indexed effect. Source zero is transparent; source
 * 1..rows selects a 256-byte lookup row indexed by the previous destination
 * colour. Out-of-range source values preserve the destination. rows is 1..255.
 * The caller supplies rows*256 bytes. Uses the current scene clip. */
void gfx_reverse_bytes(u8 *dst,const u8 *src,unsigned n);
void gfx_blit_sprite_mapped(int x,int y,int width,int height,const u8 *strengths,
                            const u8 *lookup,unsigned rows);
#ifdef SC64_UNIT_WORD_SPRITES
/* Optional phase-aligned unit raster metadata. Each phase contains one
 * {color, alpha-byte-mask} word per four destination pixels, row-major.
 * Transparent lanes in each color word must be zero. */
typedef struct {
    u16 width, height;
    const u32 *phase_words[4];
} GbaSpriteWordFrame;
void gfx_blit_sprite_words(int x,int y,const GbaSpriteWordFrame *frame);
#endif
#if defined(D2_DEN_OF_EVIL) || defined(GBA_HOST)
void gfx_blit_sprite_2x(int x,int y,int width,int height,const u8 *indexed_pixels);
#endif
void gfx_pixel(int x,int y,u8 color);
#ifdef SC64_UNIT_FACING
void gfx_blit_sprite_hflip(int x,int y,int width,int height,const u8 *indexed_pixels);
#endif
void gfx_rect(int x,int y,int w,int h,u8 color);
void gfx_frame(int x,int y,int w,int h,u8 color);
void gfx_line(int x0,int y0,int x1,int y1,u8 color);
void gfx_circle(int x,int y,int radius,u8 color);
void gfx_diamond(int x,int y,int rx,int ry,u8 color);
void gfx_text(int x,int y,const char* text,u8 color);
void gfx_num(int x,int y,int value,u8 color);
void gfx_bar(int x,int y,int w,int h,int value,int maximum,u8 color,u8 empty);
u32 rng_next(u32 *seed);
int clampi(int x,int lo,int hi);
int absi(int x);
void save_write(const void *data,u32 length,u32 tag);
void save_write_migrating(const void *data,u32 length,u32 tag,u32 legacy_length,u32 legacy_tag);
void save_write_migrating_feature(const void *data,u32 length,u32 tag,u32 legacy_tag,
                                  u32 marker_offset,u32 marker_mask,u32 marker_value);
int save_read(void *data,u32 length,u32 tag);
int save_read_latest(void *data,u32 capacity,u32 tag,u32 *length);
void audio_tone(int frequency,int frames,int volume);
void audio_noise(int frames,int volume);
void audio_update(void);
/* Optional sampled audio from a private imported bank (SC64_AUDIO_IMPORTED).
 * Presentation only: these never read or write game state. Without a bank
 * they do nothing and audio_sampled_available() returns 0. */
int audio_sampled_available(void);
/* kind: an effect, a unit response (the speech voice) or an advisor line,
 * which ignores priority and plays one line at a time (audio_mixer.h). */
enum { AUDIO_CLASS_EFFECT, AUDIO_CLASS_SPEECH, AUDIO_CLASS_ADVISOR };
void audio_play_sample(u16 sample,u8 priority,u8 kind);
void audio_play_music(int track,int loop);
/* Current or pending music track, or -1 when silent. */
int audio_music_track(void);
/* Unused VRAM between the two mode-4 pages: 0x06009600-0x06009FFF. The
 * first GBA_AUDIO_MIX_BYTES belong to the sampled-audio mixer:
 *   +0    mix double buffer, 2 x 176 bytes + 64-byte zero guard (DMA1, Direct Sound A)
 *   +416  music double buffer, 2 x 176 bytes + 64-byte zero guard (DMA2, Direct Sound B)
 *   +832  voice decode scratch, 4 x 176 bytes
 *   +1536 stream decoder states, 5 x 12 bytes (4 voices, then music), + 4 spare
 * A late VBlank interrupt lets DMA run into a guard, which is never written
 * and reads as silence. The rest is free for halfword-sized presentation
 * state. */
#define GBA_AUDIO_ARENA 0x06009600u
#define GBA_AUDIO_ARENA_BYTES 2560u
#define GBA_AUDIO_MIX_BYTES 1600u
void game_init(void);
void game_update(u16 held,u16 pressed);
void game_render(void);
int game_self_test(void);
#ifdef SC64_TILE_RENDERER
/* Experimental mode-0 presentation. Software primitives keep drawing into a
 * persistent linear 240x160 buffer; changed 8x8 tiles are copied at VBlank to
 * eight 64x64 sprites (rows 0-119) or a HUD background (rows 120-159).
 * Terrain, fog and static art live on hardware backgrounds behind WIN0. */
#define GFX_TILE_BLACK_INDEX 255 /* opaque copy of palette color 0 */
#define GFX_TILE_FOG 1
#define GFX_TILE_BLACK 2
#define GFX_TILE_TERRAIN_FIRST 3
#define GFX_TILE_TERRAIN_SLOTS 464
#define GFX_TILE_ART_FIRST (GFX_TILE_TERRAIN_FIRST + GFX_TILE_TERRAIN_SLOTS)
#define GFX_TILE_ART_SLOTS (GFX_TILE_HUD_FIRST - GFX_TILE_ART_FIRST)
#define GFX_TILE_HUD_FIRST 650
#define GFX_TILE_SBB_TERRAIN 25
#define GFX_TILE_SBB_FOG 26
#define GFX_TILE_SBB_ART 27 /* and 28 */
#define GFX_TILE_SBB_HUD 29
#define GFX_TILE_TABLES 0xF000u /* VRAM byte offset of renderer tables */
#define GFX_TILE_WORLD_TOP 12
#define GFX_TILE_WORLD_BOTTOM 120
#ifdef GBA_HOST
extern u16 gfx_host_vram[49152];
extern u16 gfx_host_io[64];
#define GFX_VRAM16 ((volatile u16 *)gfx_host_vram)
#define GFX_IO16(offset) (gfx_host_io[(offset) >> 1])
#else
#define GFX_VRAM16 ((volatile u16 *)0x06000000)
#define GFX_IO16(offset) (*(volatile u16 *)(0x04000000 + (offset)))
#endif
extern u32 gfx_tile_dirty[20], gfx_tile_drawn[20], gfx_tile_art_marks[20];
extern u8 gfx_tile_band_top, gfx_tile_band_bottom;
extern void (*gfx_tile_present_hook)(void);
int gfx_tile_active(void);
void gfx_tile_enter(void);
void gfx_tile_leave(void);
void gfx_tile_clear_world(void);
void gfx_tile_art_capture(int enable);
u8 *gfx_tile_buffer_pixels(void);
#endif
#ifdef GBA_HOST
void gfx_host_frame_rgb(u16 *rgb38400);
void gfx_dump_ppm(const char *path);
void save_test_corrupt_latest(u32 length,u32 tag);
u8 gfx_test_pixel(int x,int y);
#endif
#endif
