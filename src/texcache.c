#include "texcache.h"
void tc_init(TexCache *c, TcSlot *st, int n){ c->s=st; c->n=n; c->clock=0; c->hits=c->misses=c->evictions=0; for(int i=0;i<n;i++){ st[i].key=-1; st[i].last_use=0; } }
int tc_acquire(TexCache *c, int key, int *miss){
    c->clock++; int lru=0;
    for(int i=0;i<c->n;i++){
        if(c->s[i].key==key){ c->s[i].last_use=c->clock; c->hits++; *miss=0; return i; }
        if(c->s[i].last_use<c->s[lru].last_use) lru=i;
    }
    if(c->s[lru].key>=0) c->evictions++;
    c->s[lru].key=key; c->s[lru].last_use=c->clock; c->misses++; *miss=1; return lru;
}
void tc_rgba_to_ct16(const uint8_t *p, uint16_t *o, size_t n){
    for(size_t i=0;i<n;i++,p+=4) o[i]=(uint16_t)((p[3]>=128?0x8000:0)|((p[2]>>3)<<10)|((p[1]>>3)<<5)|(p[0]>>3));
}
