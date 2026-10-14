/* Dump binario de um mundo gerado, para inspecao via tools/world_ascii.py.
 *
 * Uso: wgdump <largura> <altura> <semente> <saida.bin>
 * Formato (WG01): 'WG01' + u16 w + u16 h + u16 spawnx + tiles (8 bytes cada,
 * coluna-maior idx = x*h + y): type,u8 wall,u8 liq,u8 lf,u16 fx,u16 fy.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "worldgen.h"

int main(int argc,char **argv){
    if(argc<5){ fprintf(stderr,"uso: %s <w> <h> <seed> <saida.bin>\n",argv[0]); return 2; }
    int w=atoi(argv[1]), h=atoi(argv[2]);
    unsigned seed=(unsigned)strtoul(argv[3],0,0);
    Tile *t=malloc((size_t)w*h*sizeof(Tile));
    if(!t) return 1;
    int sp=-1;
    if(!wg_generate(t,w,h,seed,&sp)){ free(t); return 1; }
    FILE *f=fopen(argv[4],"wb");
    if(!f){ free(t); return 1; }
    unsigned char hdr[10]; memcpy(hdr,"WG01",4);
    hdr[4]=w&255; hdr[5]=(w>>8)&255; hdr[6]=h&255; hdr[7]=(h>>8)&255;
    hdr[8]=sp&255; hdr[9]=(sp>>8)&255;
    fwrite(hdr,1,10,f);
    fwrite(t,1,(size_t)w*h*sizeof(Tile),f);
    fclose(f); free(t);
    return 0;
}
