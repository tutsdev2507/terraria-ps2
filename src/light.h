/* Iluminacao por tile (independente de plataforma; usa so Tile[]).
 *
 * Modelo simplificado de Terraria: luz do ceu (dia) + lava como emissor,
 * propagada por varreduras (relaxamento) com atenuacao por tipo de tile.
 * Valores aproximados de Terraria 1.2 -> docs/uncertainties.md.
 *
 * Indexacao identica ao mundo em game.c: idx = x*h + y (coluna-maior).
 */
#ifndef LIGHT_H
#define LIGHT_H
#include <stdint.h>
#include "tile.h"

typedef struct { int w, h; uint8_t *lum; } LightMap;

/* Constantes do modelo (aproximacoes). */
#define LG_SKY        255   /* luz do ceu (dia) */
#define LG_LAVA       190   /* brilho emitido pela lava */
#define LG_TORCH      235   /* brilho de uma tocha */
#define LG_AMBIENT    24    /* piso de brilho ao renderizar (jogo sem tochas) */
#define LG_LOSS_AIR   16    /* atenuacao por tile de ar */
#define LG_LOSS_LIQ   28    /* atenuacao por tile com liquido */
#define LG_LOSS_SOLID 42    /* atenuacao por tile solido */
#define LG_LOSS_LEAF  20    /* atenuacao por tile nao-solido (planta/arvore) */

int     light_init(LightMap *lm, int w, int h);
void    light_free(LightMap *lm);
/* Recalcula o mapa inteiro (ceu + emissores + propagacao). */
void    light_recompute(LightMap *lm, const Tile *t, const uint8_t *solid);
/* Recalcula apenas a regiao [x0..x1]x[y0..y1] redistribuindo a luz a partir
   das bordas (para edicoes de tile). */
void    light_relight(LightMap *lm, const Tile *t, const uint8_t *solid,
                      int x0, int y0, int x1, int y1);
uint8_t light_at(const LightMap *lm, int x, int y);
#endif
