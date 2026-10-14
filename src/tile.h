/* Celula do mundo (8 bytes, mesmo layout do arquivo TW01). type 255 = vazio; lf bit0 = lava. */
#ifndef TILE_H
#define TILE_H
#include <stdint.h>
typedef struct { uint8_t type, wall, liq, lf; uint16_t fx, fy; } Tile;
#endif
