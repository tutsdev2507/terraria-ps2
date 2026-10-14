/* Teste headless da iluminacao (make -f Makefile.host test).
   Usa um mundo sintetico e um mundo gerado por worldgen. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "light.h"
#include "worldgen.h"

static int failures;
#define CHECK(cond,...) do{ if(!(cond)){ printf("  FALHA: " __VA_ARGS__); printf("\n"); failures++; } }while(0)

static void solid_init(uint8_t *solid){
    static const int s[]={0,1,2,6,7,8,9,30,38,39,40,45,53,56,58,59,60,63,64,65,66,67,68,80,130,137,140};
    memset(solid,0,256);
    for(unsigned i=0;i<sizeof s/sizeof *s;i++) solid[s[i]]=1;
}
static void put(Tile *t,int w,int h,int x,int y,int type,int liq,int lf){
    Tile *c=&t[(size_t)x*h+y]; c->type=(uint8_t)type; c->liq=(uint8_t)liq; c->lf=(uint8_t)lf;
}

/* ---- mundo sintetico: superficie solida, caverna e lava ---- */
static void test_synthetic(void){
    int w=48,h=48; uint8_t solid[256]; solid_init(solid);
    Tile *t=calloc((size_t)w*h,sizeof(Tile));
    for(int i=0;i<w*h;i++) t[i].type=255;
    for(int x=0;x<w;x++) for(int y=24;y<h;y++) t[(size_t)x*h+y].type=1;   /* pedra */
    for(int x=20;x<=30;x++) for(int y=36;y<=40;y++) t[(size_t)x*h+y].type=255; /* caverna */
    put(t,w,h,24,46,255,255,1);                                           /* lava no fundo */

    LightMap lm; CHECK(light_init(&lm,w,h),"light_init falhou");
    light_recompute(&lm,t,solid);

    CHECK(lm.lum!=NULL,"mapa de luz nulo");
    CHECK(light_at(&lm,-1,0)==0&&light_at(&lm,0,-1)==0&&light_at(&lm,w,0)==0&&light_at(&lm,0,h)==0,
          "light_at fora dos limites deve retornar 0");
    CHECK(light_at(&lm,0,0)==LG_SKY,"ceu (0,0) deveria ser %d, veio %d",LG_SKY,light_at(&lm,0,0));
    CHECK(light_at(&lm,5,23)==LG_SKY,"ar de superficie deveria ser ceu");
    CHECK(light_at(&lm,5,24)>=LG_SKY-LG_LOSS_AIR,"bloco de superficie deveria ser claro (veio %d)",light_at(&lm,5,24));
    CHECK(light_at(&lm,2,46)<40,"interior profundo longe de lava deveria ser escuro (veio %d)",light_at(&lm,2,46));
    CHECK(light_at(&lm,24,46)>=LG_LAVA,"lava deveria emitir >= %d (veio %d)",LG_LAVA,light_at(&lm,24,46));
    CHECK(light_at(&lm,24,44)>light_at(&lm,2,46),"luz da lava deveria iluminar a caverna proxima");

    /* determinismo */
    uint8_t *copy=malloc((size_t)w*h); memcpy(copy,lm.lum,(size_t)w*h);
    light_recompute(&lm,t,solid);
    CHECK(memcmp(copy,lm.lum,(size_t)w*h)==0,"light_recompute nao e deterministico");

    /* relight: abre um poco ligando a superficie a caverna e compara com recompute global */
    int before=light_at(&lm,24,38);
    for(int y=24;y<=35;y++) put(t,w,h,24,y,255,0,0);   /* shaft vertical */
    light_relight(&lm,t,solid,24,24,24,35);
    int after=light_at(&lm,24,38);
    CHECK(after>before,"abrir poco deveria clarear a caverna (%d -> %d)",before,after);

    uint8_t *rel=malloc((size_t)w*h); memcpy(rel,lm.lum,(size_t)w*h);
    light_recompute(&lm,t,solid);
    int maxd=0; for(int x=4;x<=44;x++) for(int y=0;y<h;y++){ int d=abs((int)rel[(size_t)x*h+y]-(int)lm.lum[(size_t)x*h+y]); if(d>maxd)maxd=d; }
    CHECK(maxd<=3,"light_relight diverge do recompute global (maxdiff=%d)",maxd);

    free(rel); free(copy); light_free(&lm); free(t);
    printf("  sintetico: ok (relight-vs-global maxdiff=%d)\n",maxd);
}

/* ---- mundo gerado: ceu claro, fundo escuro, lava iluminada ---- */
static void test_generated(void){
    int w=320,h=200; uint8_t solid[256]; solid_init(solid);
    Tile *t=malloc((size_t)w*h*sizeof(Tile)); int sp=-1;
    CHECK(t&&wg_generate(t,w,h,42,&sp),"wg_generate falhou");
    if(!t) return;
    LightMap lm; CHECK(light_init(&lm,w,h),"light_init falhou");
    light_recompute(&lm,t,solid);
    int top=-1; for(int y=0;y<h;y++) if(t[(size_t)sp*h+y].type!=255){ top=y; break; }
    CHECK(light_at(&lm,sp,0)==LG_SKY,"ceu acima do spawn deveria ser claro");
    if(top>0) CHECK(light_at(&lm,sp,top-1)==LG_SKY,"ar logo acima do chao deveria ser claro");
    CHECK(light_at(&lm,sp,h-3)<80,"fundo do mundo deveria ser escuro (veio %d)",light_at(&lm,sp,h-3));
    int lit=0,found=0; for(size_t i=0;i<(size_t)w*h;i++) if(t[i].type==255&&t[i].liq&&(t[i].lf&1)){ found++; if(light_at(&lm,(int)(i/h),(int)(i%h))>=LG_LAVA) lit++; }
    CHECK(found>0,"mundo gerado sem lava");
    CHECK(found==0||lit==found,"toda lava deveria ser emissora (%d/%d)",lit,found);
    light_free(&lm); free(t);
    printf("  gerado 320x200 seed 42: ok (lava=%d)\n",found);
}

int main(void){
    test_synthetic();
    test_generated();
    if(failures){ printf("\n%d FALHA(S) na iluminacao\n",failures); return 1; }
    printf("\nOK: iluminacao\n");
    return 0;
}
