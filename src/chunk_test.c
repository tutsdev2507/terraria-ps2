/* Valida a logica de recorte por chunks de pal_ps2.c:pal_blit (mesma aritmetica) contra amostragem direta. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CH 256
static int W=144,H=2916; static int *src;
static int fb_a[64*64], fb_b[64*64];
static int chunk_px(int cx,int cy,int u,int v){ return src[(cy*CH+v)*W+cx*CH+u]; }
static int run(int sx,int sy,int sw,int sh,int flip){
    memset(fb_a,-1,sizeof fb_a); memset(fb_b,-1,sizeof fb_b);
    for(int j=0;j<sh;j++)for(int i=0;i<sw;i++) fb_a[j*64+i]=src[(sy+j)*W+sx+(flip?sw-1-i:i)];
    for(int cy=sy/CH;cy<=(sy+sh-1)/CH;cy++) for(int cx=sx/CH;cx<=(sx+sw-1)/CH;cx++){
        int x0=sx>cx*CH?sx:cx*CH,x1=sx+sw<(cx+1)*CH?sx+sw:(cx+1)*CH,y0=sy>cy*CH?sy:cy*CH,y1=sy+sh<(cy+1)*CH?sy+sh:(cy+1)*CH;
        float fx0=(float)(x0-sx)/sw,fx1=(float)(x1-sx)/sw; if(flip){float a=fx0;fx0=1.f-fx1;fx1=1.f-a;}
        int dx0=(int)(sw*fx0+.5f),dx1=(int)(sw*fx1+.5f);
        for(int j=y0;j<y1;j++)for(int i=dx0;i<dx1;i++){ int u=flip?(x1-1-(i-dx0)):(x0+(i-dx0)); fb_b[(j-sy)*64+i]=chunk_px(cx,cy,u-cx*CH,j-cy*CH); }
    }
    return memcmp(fb_a,fb_b,sizeof fb_a)!=0;
}
int main(void){ src=malloc(sizeof(int)*W*H); for(int i=0;i<W*H;i++)src[i]=i;
    int bad=0,n=0; for(int f=0;f<2;f++) for(int sy=0;sy<H-60;sy+=37) for(int sx=0;sx<W-40;sx+=13){ bad+=run(sx,sy,sx%2?16:40,sy%3?16:56,f); n++; }
    int sx0=250; W=1836; free(src); src=malloc(sizeof(int)*W*H); for(int i=0;i<W*H;i++)src[i]=i; H=54;
    for(int f=0;f<2;f++) for(int sx=sx0-20;sx<sx0+10;sx++){ bad+=run(sx,0,16,16,f); n++; }
    printf("chunk_test: %d casos, %d falhas\n",n,bad); return bad!=0; }
