/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHARED_COCOON_RENDER_H
#define SC64_SHARED_COCOON_RENDER_H
#if !defined(SC64_SHARED_COCOON_RENDER)
#error "Shared Cocoon renderer requires explicit opt-in"
#endif
/* Each frame retains distinct primary and destination-dependent underlay.
 * Timelines are precomputed; no source interpreter or gameplay RNG on draw. */
typedef struct {
    int x,y,w,h,canvas_w,canvas_h;
    const unsigned char *primary,*shadow;
} Sc64CocoonFrame;
typedef struct {
    const Sc64CocoonFrame *frames;
    unsigned frame_count;
    const unsigned char *timeline;
    unsigned variants,ticks;
    const unsigned char *shadow_lut;
    int shadow_x,shadow_y;
} Sc64CocoonBank;
typedef int (*Sc64CocoonVisible)(int world_x,int world_y);

static const Sc64CocoonFrame *
sc64_cocoon_resolve(const Sc64CocoonBank *b, unsigned slot, int remaining)
{
    if (!b || !b->frames || !b->timeline || !b->shadow_lut ||
        !b->frame_count || !b->variants || b->variants>8 || b->ticks!=751 ||
        remaining<0 || remaining>750) return 0;
    unsigned n=b->timeline[(slot%b->variants)*b->ticks+(unsigned)(750-remaining)];
    if (n>=b->frame_count) return 0;
    const Sc64CocoonFrame *f=&b->frames[n];
    if (!f->primary || !f->shadow || f->w<=0 || f->h<=0 ||
        f->canvas_w<=0 || f->canvas_h<=0 || f->canvas_w>256 || f->canvas_h>256 ||
        f->x<0 || f->y<0 || f->w>f->canvas_w-f->x || f->h>f->canvas_h-f->y) return 0;
    return f;
}

/* Visibility is evaluated in world coordinates for each raster run; the
 * existing blitters enforce the active scene clip and screen boundaries. */
static void
sc64_cocoon_layer(const Sc64CocoonFrame *f,const unsigned char *pixels,
                  const unsigned char *lut,int screen_x,int screen_y,
                  int world_x,int world_y,Sc64CocoonVisible visible)
{
    int left=screen_x-f->canvas_w/2+f->x, top=screen_y-f->canvas_h/2+f->y;
    int wleft=world_x-f->canvas_w/2+f->x, wtop=world_y-f->canvas_h/2+f->y;
    for (int y=0;y<f->h;++y) {
        int x=0;
        while (x<f->w) {
            if (visible && !visible(wleft+x,wtop+y)) { ++x;continue; }
            int first=x++;
            while (x<f->w && (!visible || visible(wleft+x,wtop+y))) ++x;
            const unsigned char *row=pixels+(unsigned)y*(unsigned)f->w+(unsigned)first;
            if (lut) gfx_blit_sprite_mapped(left+first,top+y,x-first,1,row,lut,1);
            else gfx_blit_sprite(left+first,top+y,x-first,1,row);
        }
    }
}
static int
sc64_cocoon_draw(const Sc64CocoonBank *b,unsigned slot,int remaining,
                 int screen_x,int screen_y,int world_x,int world_y,Sc64CocoonVisible visible)
{
    const Sc64CocoonFrame *f=sc64_cocoon_resolve(b,slot,remaining);
    if (!f) return 0;
    sc64_cocoon_layer(f,f->shadow,b->shadow_lut,screen_x+b->shadow_x,screen_y+b->shadow_y,
                      world_x+b->shadow_x,world_y+b->shadow_y,visible);
    sc64_cocoon_layer(f,f->primary,0,screen_x,screen_y,world_x,world_y,visible);
    return 1;
}
#endif
