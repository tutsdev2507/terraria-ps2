/* Cache LRU de slots de VRAM + conversao RGBA8 -> CT16 (ABGR1555 do GS). Sem dependencia de PS2SDK: testavel no host. */
#ifndef TEXCACHE_H
#define TEXCACHE_H
#include <stdint.h>
#include <stddef.h>
typedef struct { int key; uint32_t last_use; } TcSlot;
typedef struct { TcSlot *s; int n; uint32_t clock; int hits, misses, evictions; } TexCache;
void tc_init(TexCache *c, TcSlot *storage, int nslots);
/* Retorna slot de 'key'. *miss=1 se precisa (re)enviar para a VRAM (slot recem-atribuido). Nunca falha. */
int  tc_acquire(TexCache *c, int key, int *miss);
/* RGBA8 (r,g,b,a bytes) -> CT16: bit15=a>=128, 14..10=B5, 9..5=G5, 4..0=R5 */
void tc_rgba_to_ct16(const uint8_t *rgba, uint16_t *out, size_t npix);
#endif
