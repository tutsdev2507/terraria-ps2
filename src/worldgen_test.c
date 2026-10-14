/* Teste headless da geracao de mundo (make -f Makefile.host test).
   Valida invariantes, determinismo e gera um mapa PPM para inspecao visual.
   Uso: ./worldgen_test [mapa.ppm] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "worldgen.h"

static int failures;
#define CHECK(cond,...) do{ if(!(cond)){ printf("  FALHA: " __VA_ARGS__); printf("\n"); failures++; } }while(0)

typedef struct { long cnt[256]; long air_below, under, water, lava;
                 int smin, smax; long trees, plants, chests; } Stats;

static int allowed_type(int t){
    switch(t){ case 255: case 0: case 1: case 2: case 3: case 5: case 6: case 7:
        case 8: case 9: case 21: case 53: case 58: case 59: case 60: return 1; }
    return 0;
}
static int col_top(const Tile *t,int w,int h,int x){ /* primeira celula solida da coluna */
    for(int y=0;y<h;y++) if(t[x*h+y].type!=255) return y;
    return -1;
}
static int rgb_type(uint8_t type,int is_air_below){
    switch(type){
        case 0:   return 0x8C6446;   /* terra */
        case 1:   return 0x6E6E6E;   /* pedra */
        case 2:   return 0x46A046;   /* grama */
        case 3:   return 0x78DC78;   /* planta */
        case 5:   return 0x28822E;   /* arvore */
        case 6:   return 0xAAAAcc;   /* ferro */
        case 7:   return 0xE68C3C;   /* cobre */
        case 8:   return 0xFFDC3C;   /* ouro */
        case 9:   return 0xE6E6FF;   /* prata */
        case 21:  return 0xF0B43C;   /* bau */
        case 53:  return 0xDCC878;   /* areia */
        case 58:  return 0x962828;   /* inferno */
        case 59:  return 0x6E5046;   /* lama */
        case 60:  return 0x3C8C28;   /* grama de selva */
        default:  return is_air_below?0x202020:0x78B4FF;
    }
}
static void write_map(const Tile *t,int w,int h,const char *path){
    int f=2; int mw=w/f, mh=h/f;
    int *top=malloc((size_t)w*sizeof(int));
    if(!top) return;
    for(int x=0;x<w;x++) top[x]=col_top(t,w,h,x);
    FILE *fp=fopen(path,"wb");
    if(!fp){ free(top); return; }
    fprintf(fp,"P6\n%d %d\n255\n",mw,mh);
    for(int y=0;y<mh;y++) for(int x=0;x<mw;x++){
        int px=0x202020;
        for(int j=0;j<f&&!((x*f+j)>=w);j++) for(int i=0;i<f;i++){
            int sx=x*f+i, sy=y*f+j; if(sx>=w||sy>=h) continue;
            const Tile *c=&t[sx*h+sy];
            int air_below = top[sx]>=0 && sy>top[sx];
            if(c->type!=255){ px=rgb_type(c->type,air_below); goto done; }
            if(c->liq){ px=c->lf&1?0xF05A14:0x286EE6; goto done; }
            px=air_below?0x202020:0x78B4FF;
        }
        done: ;
        fputc((px>>16)&255,fp); fputc((px>>8)&255,fp); fputc(px&255,fp);
    }
    fclose(fp); free(top);
    printf("  mapa: %s (%dx%d, factor 2)\n",path,mw,mh);
}

static void run_case(int w,int h,unsigned seed){
    printf("mundo %dx%d seed=%u:\n",w,h,seed);
    size_t n=(size_t)w*(size_t)h;
    Tile *a=malloc(n*sizeof(Tile)), *b=malloc(n*sizeof(Tile));
    if(!a||!b){ printf("  FALHA: alocacao de %zu tiles\n",n); free(a); free(b); failures++; return; }
    int spawn=-1;
    clock_t t0=clock();
    CHECK(wg_generate(a,w,h,seed,&spawn)==1,"wg_generate retornou 0");
    double secs=(double)(clock()-t0)/CLOCKS_PER_SEC;
    t0=clock();
    CHECK(wg_generate(b,w,h,seed,&spawn)==1,"wg_generate (2a vez) retornou 0");
    double secs2=(double)(clock()-t0)/CLOCKS_PER_SEC;
    CHECK(memcmp(a,b,n*sizeof(Tile))==0,"geracao nao e deterministica para a mesma semente");
    int sp2=-1;
    CHECK(wg_generate(b,w,h,seed+1,&sp2)==1,"wg_generate seed+1 retornou 0");
    CHECK(memcmp(a,b,n*sizeof(Tile))!=0,"semente diferente produziu mundo identico");

    Stats s; memset(&s,0,sizeof s);
    int *top=malloc((size_t)w*sizeof(int));
    for(int x=0;x<w;x++){
        top[x]=-1;
        for(int y=0;y<h;y++){
            const Tile *c=&a[(size_t)x*h+y];
            s.cnt[c->type]++;
            if(c->type==255&&c->liq==255&&(c->lf&1)) s.lava++;
            else if(c->type==255&&c->liq==255) s.water++;
            if(c->type!=255&&top[x]<0) top[x]=y;
        }
        if(top[x]<0) CHECK(0,"coluna %d totalmente vazia",x);
    }
    for(int x=0;x<w;x++) for(int y=top[x];y<h;y++)
        if(a[(size_t)x*h+y].type==255) s.air_below++;
    s.under=0; for(int x=0;x<w;x++) s.under+=h-top[x];
    for(int x=0;x<w;x++){ if(top[x]<0) continue;
        if(s.smin==0&&s.smax==0){ s.smin=s.smax=top[x]; }
        if(top[x]<s.smin) s.smin=top[x];
        if(top[x]>s.smax) s.smax=top[x]; }
    s.trees=s.cnt[5]; s.plants=s.cnt[3]; s.chests=s.cnt[21];

    /* invariantes */
    for(int x=0;x<w;x++) for(int y=0;y<h;y++){
        const Tile *c=&a[(size_t)x*h+y];
        if(!allowed_type(c->type)) { CHECK(0,"tipo invalido %d em (%d,%d)",c->type,x,y); x=w; break; }
        if(c->wall!=0&&c->wall!=2&&c->wall!=27){ CHECK(0,"parede invalida %d em (%d,%d)",c->wall,x,y); x=w; break; }
    }
    CHECK(spawn>=0&&spawn<w,"spawn fora dos limites (%d)",spawn);
    if(spawn>=0&&spawn<w){
        int sy=top[spawn];
        CHECK(sy>=0&&sy<h-3,"spawn sem chao solido");
        if(sy>=3){
            for(int y=sy-3;y<sy;y++){
                const Tile *c=&a[(size_t)spawn*h+y];
                CHECK(c->type==255&&c->liq==0,"spawn bloqueado/na agua em y=%d (type=%d liq=%d)",y,c->type,c->liq);
            }
        }
    }
    int rel=s.under? (int)(s.air_below*100/s.under) : 0;
    CHECK(s.smax-s.smin>=h/20,"terreno chapado (min=%d max=%d h=%d)",s.smin,s.smax,h);
    CHECK(rel>=3&&rel<=60,"%% de ar subterraneo fora do esperado (%d%%)",rel);
    CHECK(s.cnt[0]>0,"sem terra");
    CHECK(s.cnt[1]>0,"sem pedra");
    CHECK(s.cnt[2]>0,"sem grama");
    CHECK(s.cnt[53]>0,"sem areia (oceano)");
    CHECK(s.cnt[59]>0,"sem lama (selva)");
    CHECK(s.cnt[60]>0,"sem grama de selva");
    CHECK(s.cnt[6]>0&&s.cnt[7]>0&&s.cnt[8]>0&&s.cnt[9]>0,"minerios faltando (Fe=%ld Cu=%ld Au=%ld Ag=%ld)",
          s.cnt[6],s.cnt[7],s.cnt[8],s.cnt[9]);
    CHECK(s.water>0,"sem agua em cavernas");
    CHECK(s.lava>0,"sem lava");
    CHECK(s.trees>0,"sem arvores");
    CHECK(s.plants>0,"sem plantas");
    CHECK(s.chests>0,"sem baus");

    /* oceanos: nas duas bordas deve haver praia (areia) e agua, sem lama/jselva
       invadindo (regressao: selva ja vazou para dentro do oceano) */
    {
        int ow=2*(w/16);
        for(int side=0;side<2;side++){
            long sand=0,mud=0,jg=0,wat=0;
            int x0=side?w-ow:0, x1=side?w:ow;
            for(int x=x0;x<x1&&x<w;x++) for(int y=0;y<h;y++){
                const Tile *c=&a[(size_t)x*h+y];
                if(c->type==53) sand++;
                else if(c->type==59) mud++;
                else if(c->type==60) jg++;
                if(c->type==255&&c->liq==255&&!(c->lf&1)) wat++;
            }
            CHECK(sand>0,"borda %d sem praia (areia)",side);
            CHECK(wat>0,"borda %d sem agua de oceano",side);
            CHECK(mud==0&&jg==0,"borda %d: selva invadiu o oceano (lama=%ld jselva=%ld)",side,mud,jg);
        }
    }

    printf("  tempo 1a/2a geracao: %.3fs/%.3fs  spawn=%d\n",secs,secs2,spawn);
    printf("  superficie y: min=%d max=%d | ar subterraneo: %ld/%ld (%d%%) | agua=%ld lava=%ld\n",
           s.smin,s.smax,s.air_below,s.under,rel,s.water,s.lava);
    printf("  tipos: terra=%ld pedra=%ld grama=%ld areia=%ld lama=%ld jselva=%ld arvore=%ld planta=%ld bau=%ld\n",
           s.cnt[0],s.cnt[1],s.cnt[2],s.cnt[53],s.cnt[59],s.cnt[60],s.trees,s.plants,s.chests);
    printf("  minerios: ferro=%ld cobre=%ld ouro=%ld prata=%ld | inferno=%ld\n",
           s.cnt[6],s.cnt[7],s.cnt[8],s.cnt[9],s.cnt[58]);
    free(top); free(a); free(b);
}

int main(int argc,char **argv){
    const char *map = argc>1?argv[1]:"wg_map.ppm";
    run_case(160,96,1);            /* mundos pequenos: saneamento rapido */
    run_case(320,200,42);
    run_case(320,200,999);
    run_case(1750,1000,12345);     /* tamanho real do port (14 MB de tiles) */
    {
        size_t n=1750u*1000u; Tile *t=malloc(n*sizeof(Tile)); int sp=-1;
        if(t&&wg_generate(t,1750,1000,12345,&sp)) write_map(t,1750,1000,map);
        free(t);
    }
    if(failures){ printf("\n%d FALHA(S) na geracao de mundo\n",failures); return 1; }
    printf("\nOK: todos os invariantes passaram\n");
    return 0;
}
