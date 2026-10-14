/* Backend headless: roda N ticks com entrada roteirizada e grava PPMs. Uso: pal_test data_dir out_prefix */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pal.h"
const char *data_root="data"; extern uint8_t sw_fb[];
static void dump(const char *pfx,int n){ char p[256]; snprintf(p,sizeof p,"%s_%03d.ppm",pfx,n); FILE *f=fopen(p,"wb"); fprintf(f,"P6\n%d %d\n255\n",SCR_W,SCR_H);
    for(int i=0;i<SCR_W*SCR_H;i++) fwrite(sw_fb+i*4,1,3,f); fclose(f); }
int main(int argc,char**argv){ if(argc>1)data_root=argv[1]; const char*pfx=argc>2?argv[2]:"shot"; game_init(); Input in; memset(&in,0,sizeof in);
    for(int t=0;t<600;t++){ in.btn=0;
        if(t>30&&t<150) in.btn|=B_RIGHT; if(t==60||t==100) in.btn|=B_CROSS; if(t>=60&&t<75) in.btn|=B_CROSS;
        if(t>200&&t<400){ in.btn|=B_SQUARE; in.btn|=B_LEFT*(t>300&&t<340); }
        game_tick(&in); if(t==20||t==120||t==300||t==599){ game_draw(); dump(pfx,t); } }
    return 0; }
