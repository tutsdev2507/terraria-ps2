/* Geracao procedural de mundo (deterministica por semente). Ver src/worldgen.c. */
#ifndef WORLDGEN_H
#define WORLDGEN_H
#include <stdint.h>
#include "tile.h"
/* Preenche tiles[0 .. w*h-1] (layout coluna-maior: indice = x*h+y) e devolve o
 * x de spawn em *spawnx. Retorna 1 em sucesso, 0 se dimensoes invalidas. */
int wg_generate(Tile *tiles, int w, int h, uint32_t seed, int *spawnx);
#endif
