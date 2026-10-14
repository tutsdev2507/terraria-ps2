#include <stdio.h>
#include <stdlib.h>
#include "texcache.h"
#define CHECK(c) do{ if(!(c)){ printf("FALHA linha %d: %s\n",__LINE__,#c); return 1; } }while(0)
int main(void){
    TcSlot st[3]; TexCache c; tc_init(&c,st,3); int m;
    int a=tc_acquire(&c,10,&m); CHECK(m==1);
    int b=tc_acquire(&c,11,&m); CHECK(m==1&&b!=a);
    int d=tc_acquire(&c,12,&m); CHECK(m==1&&d!=a&&d!=b);
    CHECK(tc_acquire(&c,10,&m)==a&&m==0);        /* hit; 11 vira LRU */
    int e=tc_acquire(&c,13,&m); CHECK(m==1&&e==b); /* evicta 11 */
    CHECK(tc_acquire(&c,11,&m)>=0&&m==1);          /* 11 foi evictado */
    CHECK(c.evictions==2&&c.hits==1&&c.misses==5);
    /* simulacao: 50 folhas, 10 slots, cena usa ~6 por frame -> apos aquecer, 0 misses */
    TcSlot s2[10]; TexCache c2; tc_init(&c2,s2,10); int work[6]={0,1,2,5,7,30};
    for(int f=0;f<300;f++) for(int i=0;i<6;i++) tc_acquire(&c2,work[i],&m);
    CHECK(c2.misses==6);
    /* conversao CT16 */
    uint8_t px[8]={255,0,0,255, 0,255,255,0}; uint16_t o[2]; tc_rgba_to_ct16(px,o,2);
    CHECK(o[0]==0x801F); CHECK(o[1]==(uint16_t)((31<<10)|(31<<5)));
    printf("texcache_test OK (hits=%d misses=%d evictions=%d)\n",c.hits,c.misses,c.evictions); return 0; }
