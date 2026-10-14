/* Rasterizador por software (backends web/test). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pal.h"
uint8_t sw_fb[SCR_W*SCR_H*4];
extern const char *data_root;
Tex *pal_tex_load(const char *rel){
    char p[512]; snprintf(p,sizeof p,"%s/%s",data_root,rel);
    FILE *f=fopen(p,"rb"); if(!f) return NULL;
    uint8_t h[8]; if(fread(h,1,8,f)!=8||memcmp(h,"TEX0",4)){fclose(f);return NULL;}
    Tex *t=calloc(1,sizeof *t); t->w=h[4]|h[5]<<8; t->h=h[6]|h[7]<<8;
    t->px=malloc((size_t)t->w*t->h*4); if(fread(t->px,1,(size_t)t->w*t->h*4,f)!=(size_t)t->w*t->h*4){fclose(f);free(t->px);free(t);return NULL;}
    fclose(f); return t;
}
int pal_file_read(const char *rel,void **buf,long *len){
    char p[512]; snprintf(p,sizeof p,"%s/%s",data_root,rel);
    FILE *f=fopen(p,"rb"); if(!f) return 0; fseek(f,0,SEEK_END); *len=ftell(f); fseek(f,0,SEEK_SET);
    *buf=malloc(*len); if(fread(*buf,1,*len,f)!=(size_t)*len){fclose(f);free(*buf);return 0;} fclose(f); return 1;
}
int pal_file_write(const char *rel,const void *buf,long len){
    char p[512]; snprintf(p,sizeof p,"%s/%s",data_root,rel);
    FILE *f=fopen(p,"wb"); if(!f) return 0; int ok=fwrite(buf,1,len,f)==(size_t)len; fclose(f); return ok;
}
void pal_log(const char *s){ fprintf(stderr,"%s\n",s); }
void pal_rect(int x,int y,int w,int h,uint32_t c){
    int a=c&255,r=c>>24,g=(c>>16)&255,b=(c>>8)&255;
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;} if(x+w>SCR_W)w=SCR_W-x; if(y+h>SCR_H)h=SCR_H-y;
    for(int j=0;j<h;j++){ uint8_t *d=sw_fb+((y+j)*SCR_W+x)*4;
        for(int i=0;i<w;i++,d+=4){
            if(a==255){d[0]=r;d[1]=g;d[2]=b;d[3]=255;}
            else{d[0]+=((r-d[0])*a)/255;d[1]+=((g-d[1])*a)/255;d[2]+=((b-d[2])*a)/255;}
        } }
}
void pal_blit(Tex *t,int sx,int sy,int sw,int sh,int dx,int dy,int dw,int dh,uint32_t tint,int flipx){
    if(!t||sw<=0||sh<=0||dw<=0||dh<=0) return;
    int tr=tint>>24,tg=(tint>>16)&255,tb=(tint>>8)&255,ta=tint&255;
    for(int j=0;j<dh;j++){ int y=dy+j; if(y<0||y>=SCR_H) continue; int v=sy+j*sh/dh; if(v<0||v>=t->h) continue;
        for(int i=0;i<dw;i++){ int x=dx+i; if(x<0||x>=SCR_W) continue;
            int u=flipx? sx+(dw-1-i)*sw/dw : sx+i*sw/dw; if(u<0||u>=t->w) continue;
            const uint8_t *s=t->px+((size_t)v*t->w+u)*4; int a=s[3]*ta/255; if(!a) continue;
            uint8_t *d=sw_fb+(y*SCR_W+x)*4; int r=s[0]*tr/255,g=s[1]*tg/255,b=s[2]*tb/255;
            if(a>=250){d[0]=r;d[1]=g;d[2]=b;} else {d[0]+=((r-d[0])*a)/255;d[1]+=((g-d[1])*a)/255;d[2]+=((b-d[2])*a)/255;}
            d[3]=255; } }
}
