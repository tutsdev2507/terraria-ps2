/* Backend PS2 (gsKit). ESCRITO, NAO COMPILADO/TESTADO (sem PS2SDK no ambiente de desenvolvimento).
 * VRAM: framebuffer CT16 640x448 x2 (~1.1MB) + 16 chunks 256x256 CT16 (2MB) com LRU (texcache.c, testado no host). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <libpad.h>
#include "pal.h"
#include "texcache.h"
#ifndef DATA_ROOT
#define DATA_ROOT "host:data"
#endif
static GSGLOBAL *gs; static char padBuf[256] __attribute__((aligned(64)));
#define SX 2
#define COL(c) GS_SETREG_RGBAQ((c)>>24,((c)>>16)&255,((c)>>8)&255,(((c)&255)>>1),0)
/* ---- Texturas: chunks 256x256 CT16, cache LRU na VRAM, dados lidos do arquivo sob demanda (nada residente na RAM) ---- */
#define CH 256
#define NSLOT 16
static TexCache tcache; static TcSlot tcslots[NSLOT]; static GSTEXTURE slotg[NSLOT]; static int slot_frame[NSLOT]; static int cur_frame, next_tex_id=1;
static uint8_t chunk_rgba[CH*CH*4] __attribute__((aligned(64)));
static uint16_t chunk_ct16[CH*CH] __attribute__((aligned(64)));
static void tex_cache_init(void){
    tc_init(&tcache,tcslots,NSLOT);
    for(int i=0;i<NSLOT;i++){ GSTEXTURE *g=&slotg[i]; g->Width=CH; g->Height=CH; g->PSM=GS_PSM_CT16; g->Filter=GS_FILTER_NEAREST; g->Mem=(void*)chunk_ct16;
        g->Vram=gsKit_vram_alloc(gs,gsKit_texture_size(CH,CH,GS_PSM_CT16),GSKIT_ALLOC_USERBUFFER); }
}
Tex *pal_tex_load(const char *rel){
    char p[256]; snprintf(p,sizeof p,"%s/%s",DATA_ROOT,rel); FILE *f=fopen(p,"rb"); if(!f) return NULL;
    uint8_t h[8]; if(fread(h,1,8,f)!=8||memcmp(h,"TEX0",4)){fclose(f);return NULL;} fclose(f);
    Tex *t=calloc(1,sizeof *t); t->w=h[4]|h[5]<<8; t->h=h[6]|h[7]<<8; t->id=next_tex_id++; t->path=strdup(p); return t;   /* so metadados */
}
static int chunk_slot(Tex *t,int cx,int cy){
    int miss; int s=tc_acquire(&tcache,t->id*256+cy*16+cx,&miss);
    if(miss){
        if(slot_frame[s]==cur_frame){ gsKit_queue_exec(gs); gsKit_queue_reset(gs->Os_Queue); }  /* slot ainda referenciado por prims pendentes deste quadro */
        int x0=cx*CH,y0=cy*CH,cw=t->w-x0<CH?t->w-x0:CH,chh=t->h-y0<CH?t->h-y0:CH;
        memset(chunk_rgba,0,sizeof chunk_rgba); FILE *f=fopen(t->path,"rb");
        if(f){ for(int r=0;r<chh;r++){ fseek(f,8+((long)(y0+r)*t->w+x0)*4,SEEK_SET); if(fread(chunk_rgba+r*CH*4,1,cw*4,f)!=(size_t)cw*4) break; } fclose(f); }
        tc_rgba_to_ct16(chunk_rgba,chunk_ct16,CH*CH); FlushCache(0);
        gsKit_texture_upload(gs,&slotg[s]);
    }
    slot_frame[s]=cur_frame; return s;
}
void pal_rect(int x,int y,int w,int h,uint32_t c){ gsKit_set_primalpha(gs,GS_SETREG_ALPHA(0,1,0,1,0),0); gsKit_prim_sprite(gs,x*SX,y*SX,(x+w)*SX,(y+h)*SX,1,COL(c)); }
void pal_blit(Tex *t,int sx,int sy,int sw,int sh,int dx,int dy,int dw,int dh,uint32_t tint,int flipx){
    if(!t||sw<=0||sh<=0) return;
    for(int cy=sy/CH;cy<=(sy+sh-1)/CH;cy++) for(int cx=sx/CH;cx<=(sx+sw-1)/CH;cx++){
        int x0=sx>cx*CH?sx:cx*CH, x1=sx+sw<(cx+1)*CH?sx+sw:(cx+1)*CH, y0=sy>cy*CH?sy:cy*CH, y1=sy+sh<(cy+1)*CH?sy+sh:(cy+1)*CH;
        int s=chunk_slot(t,cx,cy);
        float fx0=(float)(x0-sx)/sw, fx1=(float)(x1-sx)/sw, fy0=(float)(y0-sy)/sh, fy1=(float)(y1-sy)/sh;
        if(flipx){ float a=fx0; fx0=1.f-fx1; fx1=1.f-a; }
        float u0=x0-cx*CH,u1=x1-cx*CH; if(flipx){ float tt=u0; u0=u1; u1=tt; }
        gsKit_prim_sprite_texture(gs,&slotg[s],(dx+dw*fx0)*SX,(dy+dh*fy0)*SX,u0,y0-cy*CH,(dx+dw*fx1)*SX,(dy+dh*fy1)*SX,u1,y1-cy*CH,1,COL(tint));
    }
}
int pal_file_read(const char *rel,void **buf,long *len){ char p[256]; snprintf(p,sizeof p,"%s/%s",DATA_ROOT,rel); FILE *f=fopen(p,"rb"); if(!f) return 0;
    fseek(f,0,SEEK_END); *len=ftell(f); fseek(f,0,SEEK_SET); *buf=malloc(*len); fread(*buf,1,*len,f); fclose(f); return 1; }
int pal_file_write(const char *rel,const void *buf,long len){ char p[256]; snprintf(p,sizeof p,"%s/%s",DATA_ROOT,rel); FILE *f=fopen(p,"wb"); if(!f) return 0; fwrite(buf,1,len,f); fclose(f); return 1; }
void pal_log(const char *s){ printf("%s\n",s); }
int main(void){
    SifInitRpc(0); SifLoadModule("rom0:SIO2MAN",0,NULL); SifLoadModule("rom0:PADMAN",0,NULL);
    gs=gsKit_init_global(); gs->PSM=GS_PSM_CT16; gs->ZBuffering=0; /* 2D: desliga Z para economizar ~0.6MB VRAM */
    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF); dmaKit_chan_init(DMA_CHANNEL_GIF);
    gsKit_init_screen(gs); tex_cache_init(); gsKit_mode_switch(gs,GS_ONESHOT); gsKit_set_test(gs,GS_ATEST_OFF);
    padInit(0); padPortOpen(0,0,padBuf); game_init();
    for(;;){
        Input in; memset(&in,0,sizeof in); struct padButtonStatus b;
        if(padGetState(0,0)==PAD_STATE_STABLE&&padRead(0,0,&b)){ uint16_t k=0xffff^b.btns;
            if(k&PAD_UP)in.btn|=B_UP; if(k&PAD_DOWN)in.btn|=B_DOWN; if(k&PAD_LEFT)in.btn|=B_LEFT; if(k&PAD_RIGHT)in.btn|=B_RIGHT;
            if(k&PAD_CROSS)in.btn|=B_CROSS; if(k&PAD_CIRCLE)in.btn|=B_CIRCLE; if(k&PAD_SQUARE)in.btn|=B_SQUARE; if(k&PAD_TRIANGLE)in.btn|=B_TRIANGLE;
            if(k&PAD_L1)in.btn|=B_L1; if(k&PAD_R1)in.btn|=B_R1; if(k&PAD_L2)in.btn|=B_L2; if(k&PAD_R2)in.btn|=B_R2; if(k&PAD_START)in.btn|=B_START; if(k&PAD_SELECT)in.btn|=B_SELECT;
            in.lx=b.ljoy_h-128; in.ly=b.ljoy_v-128; in.rx=b.rjoy_h-128; in.ry=b.rjoy_v-128; }
        cur_frame++; game_tick(&in); gsKit_clear(gs,GS_SETREG_RGBAQ(0,0,0,0,0)); game_draw(); gsKit_queue_exec(gs); gsKit_sync_flip(gs);
    }
}
