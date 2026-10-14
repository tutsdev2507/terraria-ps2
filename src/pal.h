/* Camada de plataforma: o nucleo (game.c) so usa isto. Backends: pal_ps2.c (PS2/gsKit), pal_web.c (Android/Termux/PC via navegador), pal_test.c (headless). */
#ifndef PAL_H
#define PAL_H
#include <stdint.h>
#define SCR_W 320
#define SCR_H 224
#define RGBA(r,g,b,a) (((uint32_t)(r)<<24)|((uint32_t)(g)<<16)|((uint32_t)(b)<<8)|(uint32_t)(a))
enum { B_UP=1,B_DOWN=2,B_LEFT=4,B_RIGHT=8,B_CROSS=16,B_CIRCLE=32,B_SQUARE=64,B_TRIANGLE=128,
       B_L1=256,B_R1=512,B_L2=1024,B_R2=2048,B_START=4096,B_SELECT=8192 };
typedef struct { uint32_t btn; int lx,ly,rx,ry;      /* sticks -127..127 */
                 int ptr_down, ptr_x, ptr_y; } Input; /* toque/mouse (coords logicas) */
typedef struct Tex { int w,h; uint8_t *px; void *gs; int id; char *path; } Tex;
Tex *pal_tex_load(const char *relpath);          /* .tex (TEX0,w,h,RGBA8); NULL se faltar */
void pal_rect(int x,int y,int w,int h,uint32_t rgba);
void pal_blit(Tex *t,int sx,int sy,int sw,int sh,int dx,int dy,int dw,int dh,uint32_t tint,int flipx);
int  pal_file_read(const char *rel, void **buf, long *len);  /* malloc'd */
int  pal_file_write(const char *rel, const void *buf, long len);
void pal_log(const char *s);
/* nucleo */
void game_init(void);
void game_tick(const Input *in);   /* 1/60 s */
void game_draw(void);
#endif
