#include "light.h"
#include <stdlib.h>
#include <string.h>

#define LG_ITERS  3    /* iteracoes fwd+bwd de relaxamento */
#define LG_MARGIN 20   /* folga (tiles) ao recalcular localmente apos edicao */

int light_init(LightMap *lm,int w,int h){
    lm->w=w; lm->h=h; lm->lum=malloc((size_t)w*(size_t)h);
    if(!lm->lum){ lm->w=lm->h=0; return 0; }
    memset(lm->lum,0,(size_t)w*(size_t)h);
    return 1;
}
void light_free(LightMap *lm){ free(lm->lum); lm->lum=NULL; lm->w=lm->h=0; }

static int loss_of(const Tile *t,const uint8_t *solid){
    if(t->type==255) return t->liq?LG_LOSS_LIQ:LG_LOSS_AIR;
    if(solid[t->type]) return LG_LOSS_SOLID;
    return LG_LOSS_LEAF;
}
static uint8_t emit_of(const Tile *t){
    if(t->type==4) return LG_TORCH;                               /* tocha */
    if(t->type==255 && t->liq && (t->lf&1)) return LG_LAVA;       /* lava */
    return 0;
}

/* ar/folhagem acima do primeiro bloco solido recebem luz do ceu (e o proprio bloco). */
static void seed_sky(LightMap *lm,const Tile *T,const uint8_t *solid,int xa,int xb){
    int h=lm->h; uint8_t *L=lm->lum;
    for(int x=xa;x<=xb;x++) for(int y=0;y<h;y++){
        size_t i=(size_t)x*h+y;
        L[i]=LG_SKY;
        if(solid[T[i].type]) break;
    }
}
static void seed_emit(LightMap *lm,const Tile *T,int xa,int xb,int ya,int yb){
    int h=lm->h; uint8_t *L=lm->lum;
    for(int x=xa;x<=xb;x++) for(int y=ya;y<=yb;y++){
        size_t i=(size_t)x*h+y; uint8_t e=emit_of(&T[i]);
        if(e>L[i]) L[i]=e;
    }
}
/* relaxamento por varreduras dentro da caixa; le vizinhos fora da caixa como fontes fixas. */
static void relax(LightMap *lm,const Tile *T,const uint8_t *solid,
                  int xa,int xb,int ya,int yb,int iters){
    int w=lm->w,h=lm->h; uint8_t *L=lm->lum;
    for(int it=0;it<iters;it++){
        for(int x=xa;x<=xb;x++) for(int y=ya;y<=yb;y++){
            size_t i=(size_t)x*h+y; int lv=L[i];
            if(x>0){ int c=L[i-h]-loss_of(&T[i-h],solid); if(c>lv)lv=c; }
            if(y>0){ int c=L[i-1]-loss_of(&T[i-1],solid); if(c>lv)lv=c; }
            if(lv>(int)L[i]) L[i]=(uint8_t)lv;
        }
        for(int x=xb;x>=xa;x--) for(int y=yb;y>=ya;y--){
            size_t i=(size_t)x*h+y; int lv=L[i];
            if(x<w-1){ int c=L[i+h]-loss_of(&T[i+h],solid); if(c>lv)lv=c; }
            if(y<h-1){ int c=L[i+1]-loss_of(&T[i+1],solid); if(c>lv)lv=c; }
            if(lv>(int)L[i]) L[i]=(uint8_t)lv;
        }
    }
}

void light_recompute(LightMap *lm,const Tile *T,const uint8_t *solid){
    int w=lm->w,h=lm->h;
    memset(lm->lum,0,(size_t)w*h);
    seed_sky(lm,T,solid,0,w-1);
    seed_emit(lm,T,0,w-1,0,h-1);
    relax(lm,T,solid,0,w-1,0,h-1,LG_ITERS);
}

void light_relight(LightMap *lm,const Tile *T,const uint8_t *solid,
                   int x0,int y0,int x1,int y1){
    int w=lm->w,h=lm->h;
    int xa=x0-LG_MARGIN,xb=x1+LG_MARGIN,ya=y0-LG_MARGIN,yb=y1+LG_MARGIN;
    if(xa<0)xa=0; if(ya<0)ya=0; if(xb>w-1)xb=w-1; if(yb>h-1)yb=h-1;
    if(xa>xb||ya>yb) return;
    for(int x=xa;x<=xb;x++) memset(&lm->lum[(size_t)x*h+ya],0,(size_t)(yb-ya+1));
    seed_sky(lm,T,solid,xa,xb);
    seed_emit(lm,T,xa,xb,ya,yb);
    relax(lm,T,solid,xa,xb,ya,yb,LG_ITERS);
}

uint8_t light_at(const LightMap *lm,int x,int y){
    if(!lm->lum||x<0||y<0||x>=lm->w||y>=lm->h) return 0;
    return lm->lum[(size_t)x*lm->h+y];
}
