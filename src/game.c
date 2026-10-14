/* Terraria Mobile (antigo) - nucleo portatil. Constantes de fisica vindas do Terraria PC 1.2 (aprox.; ver docs/uncertainties.md). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "pal.h"

#define TS 16
#include "tile.h"                 /* Tile: 8 bytes (TW01); type 255 = vazio; lf bit0 = lava */
#include "worldgen.h"
#include "light.h"
static struct { int w,h; Tile *t; int spawnx; } W;
#define TL(x,y) (&W.t[(x)*W.h+(y)])
static inline int inb(int x,int y){ return x>=0&&y>=0&&x<W.w&&y<W.h; }

static uint8_t g_lut[256][16][3][2]; static uint8_t g_wallcell[256][2];
static uint8_t SOLID[256], SOLID_LG[256], IMP[256];
static Tex *tiletex[256]; static Tex *walltex[256]; static Tex *playertex, *slottex, *itemtex[64];
static int tiletex_tried[256], walltex_tried[256], itemtex_tried[64];
static Tex *mobtex[16]; static int mobtex_tried[16];
/* folha de sprite por tipo de mob: npc_slime_real.tex = 44x68 = 2 quadros 44x34 */
static const int mob_fw[16]={44}; static const int mob_fh[16]={34};
static LightMap LM;

static Tex *get_tile_tex(int t){ if(!tiletex_tried[t]){tiletex_tried[t]=1;char p[64];snprintf(p,sizeof p,"tiles_%d.tex",t);tiletex[t]=pal_tex_load(p);} return tiletex[t]; }
static Tex *get_wall_tex(int t){ if(!walltex_tried[t]){walltex_tried[t]=1;char p[64];snprintf(p,sizeof p,"wall_%d.tex",t);walltex[t]=pal_tex_load(p);} return walltex[t]; }
static Tex *get_item_tex(int id){ if(id<0||id>=64) return NULL; if(!itemtex_tried[id]){itemtex_tried[id]=1;char p[64];snprintf(p,sizeof p,"item_%d.tex",id);itemtex[id]=pal_tex_load(p);} return itemtex[id]; }

/* ---------- itens (ids = sprites Item_N do APK; tabela minima, ver docs) ---------- */
enum { K_NONE, K_PICK, K_AXE, K_HAMMER, K_BLOCK, K_SWORD };
typedef struct { int id; const char *name; int kind; int power; int tile; } ItemDef;
static const ItemDef ITEMS[] = {
    {1,"Iron Pickaxe",K_PICK,40,0},{10,"Iron Axe",K_AXE,40,0},{7,"Iron Hammer",K_HAMMER,40,0},{4,"Iron Broadsword",K_SWORD,18,0},{12,"Wood Sword",K_SWORD,8,0},
    {2,"Dirt Block",K_BLOCK,0,0},{3,"Stone Block",K_BLOCK,0,1},{9,"Wood",K_BLOCK,0,30},{8,"Torch",K_BLOCK,0,4},
};
static const ItemDef *item_def(int id){ for(unsigned i=0;i<sizeof ITEMS/sizeof *ITEMS;i++) if(ITEMS[i].id==id) return &ITEMS[i]; return NULL; }
static int tile_drop(int type){ switch(type){case 0:case 2:return 2;case 1:return 3;case 30:case 5:return 9;case 4:return 8;default:return 0;} }

typedef struct { int id, n; } Slot;
static Slot inv[10]; static int sel;
typedef struct { float x,y,vx,vy; int hp,life,type,onground,frame,anim; } Mob;
#define MAX_MOB 32
static Mob mobs[MAX_MOB]; static int nmob;

/* ---------- jogador ---------- */
static struct { float x,y,vx,vy; int jump, dir, onground; float anim; int frame; float swing; } P;
#define PW 20
#define PH 42
static float camx, camy; static int curx=200, cury=100; static int use_t; static struct { int x,y,dmg,t; } hit;
static uint32_t prev_btn; static int frame_no;
static float g_time=0.25f;             /* 0=meia-noite .25=manha .5=meio-dia .75=noite */
static float g_daylight=1.0f;          /* 0..1 fator do sol aplicado ao ceu    */
static int   g_dayframe=0;

static int solid_at(int x,int y){ if(x<0||x>=W.w) return 1; if(y<0) return 0; if(y>=W.h) return 1; Tile *t=TL(x,y); return t->type!=255 && SOLID[t->type]; }
static int rect_hits(float px,float py,int w,int h){
    int x0=(int)floorf(px/TS),x1=(int)floorf((px+w-0.01f)/TS),y0=(int)floorf(py/TS),y1=(int)floorf((py+h-0.01f)/TS);
    for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) if(solid_at(x,y)) return 1; return 0;
}
static int box_hits(float px,float py){
    int x0=(int)floorf(px/TS),x1=(int)floorf((px+PW-0.01f)/TS),y0=(int)floorf(py/TS),y1=(int)floorf((py+PH-0.01f)/TS);
    for(int x=x0;x<=x1;x++) for(int y=y0;y<=y1;y++) if(solid_at(x,y)) return 1; return 0;
}
static int platform_below(float px,float py){ /* plataformas (tipo 19): so colidem por cima */
    int y=(int)floorf((py+PH)/TS); if(y<0||y>=W.h) return 0; if(fmodf(py+PH,TS)>6.f) return 0;
    for(int x=(int)floorf(px/TS);x<=(int)floorf((px+PW-0.01f)/TS);x++) if(inb(x,y)&&TL(x,y)->type==19) return 1; return 0;
}

/* ---------- entidades: itens dropados (caem e sao pegos) + particulas ---------- */
#define MAX_DROPS 256
static int inv_add(int id,int n);
#define MAX_PART  512
typedef struct { float x,y,vx,vy; int onground,id,n,life; } Drop;
typedef struct { float x,y,vx,vy; int life; uint32_t c; } Part;
static Drop drops[MAX_DROPS]; static int ndrop;
static Part parts[MAX_PART];  static int npart;

static uint32_t tile_color(int type){
    switch(type){
        case 0: case 2: return RGBA(150,105,55,255);    /* terra/grama */
        case 1:         return RGBA(120,120,130,255);   /* pedra */
        case 5:         return RGBA(120,80,40,255);     /* madeira */
        case 53:        return RGBA(230,210,140,255);   /* areia */
        case 59:        return RGBA(120,70,40,255);     /* lama */
        case 60:        return RGBA(80,140,60,255);     /* grama selva */
        case 6:         return RGBA(185,145,115,255);   /* ferro */
        case 7:         return RGBA(200,130,90,255);    /* cobre */
        case 8:         return RGBA(230,210,90,255);    /* ouro */
        case 9:         return RGBA(205,205,215,255);   /* prata */
        case 58:        return RGBA(220,80,20,255);     /* inferno */
        case 21:        return RGBA(160,110,60,255);    /* bau */
        default:        return RGBA(140,140,140,255);
    }
}
static void spawn_drop(float x,float y,int id,int n){
    if(!id||n<=0||ndrop>=MAX_DROPS) return;
    Drop *d=&drops[ndrop++];
    d->x=x; d->y=y; d->vx=((rand()%100)/100.0f-0.5f)*3.0f; d->vy=-1.5f-(rand()%100)/100.0f*1.5f;
    d->onground=0; d->id=id; d->n=n; d->life=60*300;
}
static void spawn_particles(float x,float y,int type,int count){
    uint32_t c=tile_color(type);
    for(int i=0;i<count && npart<MAX_PART;i++){
        Part *p=&parts[npart++];
        p->x=x+((rand()%100)/100.0f)*10-5; p->y=y+((rand()%100)/100.0f)*10-5;
        p->vx=((rand()%100)/100.0f-0.5f)*2.5f; p->vy=-((rand()%100)/100.0f)*2.5f;
        p->life=18+rand()%20; p->c=c;
    }
}
static void spawn_gore(float x,float y,uint32_t c,int count){
    for(int i=0;i<count && npart<MAX_PART;i++){
        Part *p=&parts[npart++];
        p->x=x+((rand()%100)/100.0f)*12-6; p->y=y+((rand()%100)/100.0f)*12-6;
        p->vx=((rand()%100)/100.0f-0.5f)*5.0f; p->vy=-((rand()%100)/100.0f)*4.0f-0.5f;
        p->life=30+rand()%30; p->c=c;
    }
}
static void update_entities(void){
    for(int i=0;i<ndrop;i++){
        Drop *d=&drops[i];
        d->vy+=0.25f; if(d->vy>8.f)d->vy=8.f;
        float nx=d->x+d->vx;
        if(rect_hits(nx-4,d->y-4,8,8)) d->vx=-d->vx*0.4f; else d->x=nx;
        float ny=d->y+d->vy; d->onground=0;
        if(rect_hits(d->x-4,ny-4,8,8)){
            while(!rect_hits(d->x-4,d->y-4+1,8,8) && d->vy>0) d->y+=1;
            d->vy=0; d->vx*=0.7f; d->onground=1;
        } else d->y=ny;
        if(d->x<4)d->x=4; if(d->x>W.w*TS-4)d->x=W.w*TS-4;
        if(d->y>W.h*TS-4)d->y=W.h*TS-4;
        if(fabsf(d->x-(P.x+PW/2))<26 && fabsf(d->y-(P.y+PH/2))<26 && inv_add(d->id,d->n)){ drops[i]=drops[--ndrop]; i--; continue; }
        if(--d->life<=0){ drops[i]=drops[--ndrop]; i--; }
    }
    for(int i=0;i<npart;i++){
        Part *p=&parts[i];
        p->vy+=0.2f; p->x+=p->vx; p->y+=p->vy;
        if(rect_hits(p->x-1,p->y-1,2,2)){ p->vy=0; p->vx*=0.6f; }
        if(--p->life<=0){ parts[i]=parts[--npart]; i--; }
    }
}

static void update_mobs(void){
    float px=P.x+PW/2, py=P.y+PH/2;
    for(int i=0;i<nmob;i++){
        Mob *m=&mobs[i];
        if(m->hp<=0){
            spawn_gore(m->x,m->y,RGBA(120,210,110,255),16);
            spawn_drop(m->x,m->y,23,1+rand()%2); /* gel */
            mobs[i]=mobs[--nmob]; i--; continue;
        }
        m->vy+=0.25f; if(m->vy>8.f)m->vy=8.f;
        if(frame_no%90==0 && m->onground){
            float mx=m->x; m->vx=(px>mx?0.7f:-0.7f); m->vy=-2.2f; m->onground=0;
        }
        float nx=m->x+m->vx;
        if(rect_hits(nx-8,m->y-8,16,16)) m->vx*=-0.4f; else m->x=nx;
        float ny=m->y+m->vy;
        if(rect_hits(m->x-8,ny-8,16,16)){ m->vy=0; m->onground=1; m->vx*=0.85f; }
        else { m->y=ny; m->onground=0; }
        if(fabsf(m->x-px)<20 && fabsf(m->y-py)<20 && frame_no%40==0){
            /* knockback simples */
            P.vx += (px<m->x?1.5f:-1.5f); P.vy += -1.0f;
        }
        m->life++; m->anim++;
    }
}

static void load_tables(void){
    static const int imp[]={21,3,5,28,93,15,90,79,82,135,105,137,10,4,36,73,17,138,141,83};
    for(unsigned i=0;i<sizeof imp/sizeof *imp;i++) IMP[imp[i]]=1;
    static const int sol[]={0,1,2,6,7,8,9,30,38,39,40,45,53,56,58,59,60,63,64,65,66,67,68,80,130,137,140};
    for(unsigned i=0;i<sizeof sol/sizeof *sol;i++) SOLID[sol[i]]=1;
    memcpy(SOLID_LG,SOLID,sizeof SOLID);
    void *b; long n;
    if(pal_file_read("tileframe.bin",&b,&n)&&n==sizeof g_lut){memcpy(g_lut,b,n);free(b);}
    if(pal_file_read("wallcell.bin",&b,&n)&&n==sizeof g_wallcell){memcpy(g_wallcell,b,n);free(b);}
}
static int load_world(const char *rel){
    void *b; long n; if(!pal_file_read(rel,&b,&n)) return 0;
    uint8_t *p=b; if(n<16||memcmp(p,"TW01",4)){free(b);return 0;}
    W.w=p[4]|p[5]<<8; W.h=p[6]|p[7]<<8; W.spawnx=p[12]|p[13]<<8;
    size_t cnt=(size_t)W.w*W.h; if((long)(16+cnt*8)!=n&&(long)(16+cnt*8)>n){free(b);return 0;}
    W.t=malloc(cnt*sizeof(Tile)); const uint8_t *s=p+16;
    for(size_t i=0;i<cnt;i++,s+=8){ W.t[i].type=s[0];W.t[i].wall=s[1];W.t[i].liq=s[2];W.t[i].lf=s[3];W.t[i].fx=s[4]|s[5]<<8;W.t[i].fy=s[6]|s[7]<<8; }
    free(b); return 1;
}
/* Sempre gera um mundo procedural novo ao iniciar. Semente: data/seed.txt se
   existir (mundo reproduzivel); caso contrario, baseada no relogio (mundo novo
   a cada inicio), como o jogador escolheria "novo mundo" no original. */
static int gen_world(unsigned seed){
    W.w=1750; W.h=1000;
    W.t=malloc((size_t)W.w*W.h*sizeof(Tile));
    if(!W.t) return 0;
    if(!wg_generate(W.t,W.w,W.h,seed,&W.spawnx)){ free(W.t); W.t=NULL; return 0; }
    return 1;
}
static int read_seed(unsigned *out){
    void *b; long n; if(!pal_file_read("seed.txt",&b,&n)) return 0;
    char s[32]; int k=n<31?(int)n:31; if(k<1){ free(b); return 0; }
    memcpy(s,b,k); s[k]=0; free(b);
    *out=(unsigned)strtoul(s,NULL,0); return 1;
}
void game_init(void){
    load_tables();
    unsigned seed;
    if(read_seed(&seed)){ pal_log("worldgen: gerando mundo com semente de seed.txt"); }
    else { seed=(unsigned)time(NULL); pal_log("worldgen: gerando mundo novo (semente do relogio)"); }
    if(!gen_world(seed)){
        pal_log("ERRO: falha na geracao de mundo"); W.w=64;W.h=64;W.t=calloc(64*64,sizeof(Tile)); for(int i=0;i<64*64;i++)W.t[i].type=255; for(int x=0;x<64;x++)for(int y=40;y<64;y++)TL(x,y)->type=0; W.spawnx=32;
    }
    (void)load_world;
    playertex=pal_tex_load("player.tex"); slottex=pal_tex_load("inventory_back.tex");
    mobtex[0]=pal_tex_load("npc_slime_real.tex"); mobtex_tried[0]=1;
    int sx=W.spawnx,sy=0; while(sy<W.h-2 && !solid_at(sx,sy+2) ) sy++;
    P.x=sx*TS; P.y=(sy+2)*TS-PH-1; P.dir=1;
    /* se spawn dentro de bloco, sobe */ while(box_hits(P.x,P.y)&&P.y>0) P.y-=TS;
    inv[0].id=1;inv[0].n=1;inv[1].id=10;inv[1].n=1;inv[2].id=7;inv[2].n=1;inv[3].id=4;inv[3].n=1;
    inv[4].id=2;inv[4].n=99;inv[5].id=3;inv[5].n=99;inv[6].id=9;inv[6].n=99;inv[7].id=8;inv[7].n=99;
    camx=P.x-SCR_W/2; camy=P.y-SCR_H/2;
    light_free(&LM);
    if(W.w>0&&W.h>0){ light_init(&LM,W.w,W.h); light_recompute(&LM,W.t,SOLID_LG); }
    /* spawn slime */
    for(int i=0;i<3 && nmob<MAX_MOB;i++){
        float mx=P.x + (i-1)*200; float my=P.y-60;
        if(mx<0)mx=0; if(mx>W.w*TS-PW)mx=W.w*TS-PW;
        mobs[nmob++]=(Mob){.x=mx,.y=my,.vx=0,.vy=0,.hp=20,.life=0,.type=0,.onground=0,.frame=0,.anim=0};
    }
}

static int inv_add(int id,int n){
    for(int i=0;i<10;i++) if(inv[i].id==id&&inv[i].n<999){inv[i].n+=n;return 1;}
    for(int i=0;i<10;i++) if(!inv[i].id){inv[i].id=id;inv[i].n=n;return 1;} return 0;
}
static void set_tile_empty(int x,int y){ Tile *t=TL(x,y); t->type=255; t->fx=t->fy=0; light_relight(&LM,W.t,SOLID_LG,x,y,x,y); }

/* ---------- jogador: fisica (Terraria: gravidade .4, queda max 10, pulo 5.01 x 15 ticks, corrida max 3 acel .08 desacel .2) ---------- */
static void player_update(const Input *in){
    int left=(in->btn&B_LEFT)||in->lx<-40, right=(in->btn&B_RIGHT)||in->lx>40;
    int jumpkey=(in->btn&B_CROSS)||(in->btn&B_UP);
    if(left&&!right){ if(P.vx>-3.f) P.vx-= (P.vx>0)?0.2f+0.08f:0.08f; if(P.vx<-3.f)P.vx=-3.f; P.dir=-1; }
    else if(right&&!left){ if(P.vx<3.f) P.vx+= (P.vx<0)?0.2f+0.08f:0.08f; if(P.vx>3.f)P.vx=3.f; P.dir=1; }
    else { float s=P.onground?0.2f:0.04f; if(P.vx>s)P.vx-=s; else if(P.vx<-s)P.vx+=s; else P.vx=0; }
    if(jumpkey){ if(P.onground&&P.jump==0&&!(prev_btn&B_CROSS&&!P.onground)){ P.jump=15; P.vy=-5.01f; } else if(P.jump>0){ P.vy=-5.01f; P.jump--; } }
    else P.jump=0;
    if(P.jump==0){ P.vy+=0.4f; if(P.vy>10.f)P.vy=10.f; }
    /* X */
    float nx=P.x+P.vx; 
    if(box_hits(nx,P.y)){
        int stepped=0; if(P.onground||P.vy>=0){ for(int d=1;d<=TS;d++){ if(!box_hits(nx,P.y-d)&&!box_hits(P.x,P.y-d)){ P.y-=d; stepped=1; break; } } }
        if(stepped) P.x=nx;
        else { float step=P.vx>0?1.f:-1.f; while(!box_hits(P.x+step,P.y)) P.x+=step; P.vx=0; }
    } else P.x=nx;
    if(P.x<0){P.x=0;P.vx=0;} if(P.x>W.w*TS-PW){P.x=W.w*TS-PW;P.vx=0;}
    /* Y */
    float ny=P.y+P.vy; P.onground=0;
    if(box_hits(P.x,ny)){
        float step=P.vy>0?1.f:-1.f; while(!box_hits(P.x,P.y+step)) P.y+=step;
        if(P.vy>0) P.onground=1; else P.jump=0; P.vy=0;
    } else if(P.vy>0&&platform_below(P.x,ny)&&!platform_below(P.x,P.y+0.f)){ P.y=floorf((ny+PH)/TS)*TS-PH; P.vy=0; P.onground=1; }
    else P.y=ny;
    if(P.y<0)P.y=0;
    if(!P.onground && box_hits(P.x,P.y+1)) P.onground=1;
    /* animacao (frames 56px: 0 parado, 5 pulo, 7..19 corrida) */
    if(!P.onground) P.frame=5;
    else if(fabsf(P.vx)>0.1f){ P.anim+=fabsf(P.vx)*1.3f; while(P.anim>8.f){P.anim-=8.f;P.frame++;} if(P.frame<7||P.frame>19)P.frame=7; }
    else { P.frame=0; P.anim=0; }
}

/* ---------- uso de itens ---------- */
static int hit_resist(int type){ switch(type){case 0:case 2:case 53:case 59:case 60:return 100;case 56:return 400;case 1:return 100;default:return 100;} }

/* ataque corpo-a-corpo: acerta mobs dentro do alcance, na frente do jogador */
static void melee_attack(float reach,int dmg){
    float pcx=P.x+PW/2, pcy=P.y+PH/2;
    for(int i=0;i<nmob;i++){
        Mob *m=&mobs[i];
        float dx=m->x-pcx, dy=m->y-pcy;
        if(fabsf(dx)<=reach && fabsf(dy)<=reach && (dx*P.dir)>=-6.0f){
            m->hp-=dmg; spawn_gore(m->x,m->y,RGBA(120,210,110,255),5);
            m->vx+=(dx>0?1.8f:-1.8f); m->vy=-1.6f;
        }
    }
}

/* baú (tipo 21) ocupa 2x2: acha o canto sup.-esq., limpa tudo e dropa o baú (uma vez) */
static void break_chest(int tx,int ty){
    int ox=tx, oy=ty, guard=0;
    while(ox>0 && TL(ox-1,ty)->type==21 && guard++<16) ox--;
    guard=0;
    while(oy>0 && TL(ox,oy-1)->type==21 && guard++<16) oy--;
    for(int x=ox;x<ox+2 && x<W.w;x++) for(int y=oy;y<oy+2 && y<W.h;y++)
        if(TL(x,y)->type==21){ spawn_particles(x*TS+8,y*TS+8,21,4); set_tile_empty(x,y); }
    spawn_drop(ox*TS+8,oy*TS+8,21,1);
}
/* árvore (tipo 5): quebra o tronco inteiro e dropa madeira (como no original) */
static void break_tree(int tx,int ty){
    int y=ty;
    while(y>0 && TL(tx,y-1)->type==5) y--;
    int n=0;
    while(y<W.h && TL(tx,y)->type==5){ spawn_particles(tx*TS+8,y*TS+8,5,3); set_tile_empty(tx,y); y++; n++; }
    if(n) spawn_drop(tx*TS+8,ty*TS+8,9,n);
}

static void use_item(const Input *in){
    int ox=(int)(camx)+curx, oy=(int)(camy)+cury; int tx=ox/TS, ty=oy/TS; if(ox<0||oy<0||!inb(tx,ty)) return;
    float pcx=P.x+PW/2, pcy=P.y+PH/2; if(fabsf(tx*TS+8-pcx)>6.5f*TS||fabsf(ty*TS+8-pcy)>6.5f*TS) return;
    Slot *s=&inv[sel]; const ItemDef *d=s->id?item_def(s->id):NULL; if(!d) return; Tile *t=TL(tx,ty);
    if(use_t>0) return;
    if(d->kind==K_PICK||d->kind==K_AXE){
        P.swing=1.0f; P.dir=(tx*TS+8<pcx)?-1:1;
        int ok=0; if(t->type!=255){ if(d->kind==K_PICK&&!(t->type==5)&&(SOLID[t->type]||t->type==19)) ok=1; if(d->kind==K_AXE&&t->type==5) ok=1; if(d->kind==K_PICK&&!SOLID[t->type]&&t->type!=5&&t->type!=3&&t->type!=52&&t->type!=51) ok=1; }
        if(!ok){ return; }
        if(hit.x!=tx||hit.y!=ty||hit.t<=0){ hit.x=tx;hit.y=ty;hit.dmg=0; } hit.t=60; hit.dmg+=d->power; use_t=13;
        { float px=tx*TS+8,py=ty*TS+8; spawn_particles(px,py,t->type,6); }
        if(hit.dmg>=hit_resist(t->type)){
            if(t->type==21) break_chest(tx,ty);
            else if(t->type==5) break_tree(tx,ty);
            else { int dr=tile_drop(t->type); if(dr) spawn_drop(tx*TS+8,ty*TS+8,dr,1); set_tile_empty(tx,ty); }
            hit.dmg=0; hit.t=0;
        }
    } else if(d->kind==K_SWORD){
        P.swing=1.0f; P.dir=(tx*TS+8<pcx)?-1:1; use_t=16;
        melee_attack(2.6f*TS, d->power);
    } else if(d->kind==K_BLOCK&&s->n>0){
        if(t->type!=255) return;
        int adj=solid_at(tx-1,ty)||solid_at(tx+1,ty)||solid_at(tx,ty-1)||solid_at(tx,ty+1)||t->wall;
        if(!adj) return;
        float bx0=tx*TS,by0=ty*TS; if(bx0<P.x+PW&&bx0+TS>P.x&&by0<P.y+PH&&by0+TS>P.y&&SOLID[d->tile]) return;
        t->type=d->tile; t->fx=t->fy=0; t->liq=0; if(--s->n<=0){s->id=0;} use_t=10; light_relight(&LM,W.t,SOLID_LG,tx,ty,tx,ty);
    }
    (void)in;
}

void game_tick(const Input *in){
    frame_no++; if(use_t>0)use_t--; if(hit.t>0)hit.t--;
    if(P.swing>0){ P.swing-=1.0f/12.0f; if(P.swing<0)P.swing=0; }
    /* ciclo dia/noite (240 s). g_time: .25=manha .5=meio-dia .75=noite */
    g_time+=1.0f/(60.0f*240.0f); if(g_time>=1.0f)g_time-=1.0f;
    { float s=sinf(g_time*6.2831853f); g_daylight=0.62f+0.38f*(s*0.5f+0.5f); g_dayframe=(int)(g_time*24.0f); }
    player_update(in);
    update_entities();
    update_mobs();
    uint32_t pr=in->btn&~prev_btn;
    if(pr&B_R1) sel=(sel+1)%10; if(pr&B_L1) sel=(sel+9)%10;
    /* cursor: stick direito ou ponteiro. Sem entrada NAO reseta: mantem a
       ultima posicao (como no original). */
    if(in->ptr_down){ curx=in->ptr_x; cury=in->ptr_y; }
    else if(abs(in->rx)>20||abs(in->ry)>20){ curx+=in->rx/32; cury+=in->ry/32; if(curx<0)curx=0; if(curx>SCR_W-1)curx=SCR_W-1; if(cury<0)cury=0; if(cury>SCR_H-1)cury=SCR_H-1; }
    if(in->ptr_down||(in->btn&(B_SQUARE|B_R2))) use_item(in);
    prev_btn=in->btn;
    /* camera */
    float tx=P.x+PW/2-SCR_W/2, ty=P.y+PH/2-SCR_H/2; camx+=(tx-camx)*0.2f; camy+=(ty-camy)*0.2f;
    if(camx<0)camx=0; if(camy<0)camy=0; if(camx>W.w*TS-SCR_W)camx=W.w*TS-SCR_W; if(camy>W.h*TS-SCR_H)camy=W.h*TS-SCR_H;
}

/* ---------- render ---------- */
/* luz suavizada (media dos vizinhos) + fator dia/noite, evita blocos pretos duros */
static uint8_t rlight(int x,int y){
    int a=light_at(&LM,x,y);
    int r=light_at(&LM,x+1,y)+light_at(&LM,x-1,y)+light_at(&LM,x,y+1)+light_at(&LM,x,y-1);
    int v=(a*4+r)/8; if(v<0)v=0; if(v>255)v=255; return (uint8_t)v;
}
static uint8_t shade_l(uint8_t l){ int v=(int)(l*g_daylight); if(v>255)v=255; if(v<0)v=0; return (uint8_t)v; }
static int nbr_open(int x,int y){ /* bit: 1=cima 2=baixo 4=esq 8=dir abertos */
    int m=0; if(!solid_at(x,y-1)) m|=1; if(!solid_at(x,y+1)) m|=2; if(!solid_at(x-1,y)) m|=4; if(!solid_at(x+1,y)) m|=8; return m; }
static void draw_tile(Tile *t,int x,int y,int sx,int sy){
    Tex *tx=get_tile_tex(t->type); 
    if(!tx){ pal_rect(sx,sy,TS,TS,RGBA(255,0,255,255)); return; }
    if(IMP[t->type]){ int w=16,h=16; if(t->type==5){w=20;h=20; pal_blit(tx,t->fx,t->fy,w,h,sx-2,sy-2,w,h,0xffffffff,0); return;} pal_blit(tx,t->fx,t->fy,w,h,sx,sy,w,h,0xffffffff,0); return; }
    { uint8_t l=shade_l(rlight(x,y)); if(l<LG_AMBIENT)l=LG_AMBIENT; int tnt=RGBA(l,l,l,255);
      if(tx->w==288&&SOLID[t->type]){ int m=nbr_open(x,y), v=(x*7+y*13)%3; if(m==15) v=(x+y)%3; int cx=g_lut[t->type][m][v][0],cy=g_lut[t->type][m][v][1]; pal_blit(tx,cx*18,cy*18,16,16,sx,sy,16,16,tnt,0); }
      else pal_blit(tx,0,0,16,16,sx,sy,16,16,tnt,0); }
}
static void draw_num(int n,int x,int y){ static const uint16_t F[10]={0x7B6F,0x2C97,0x73E7,0x73CF,0x5BC9,0x79CF,0x79EF,0x7249,0x7BEF,0x7BCF}; char b[8]; int l=snprintf(b,8,"%d",n);
    for(int i=0;i<l;i++){ uint16_t g=F[b[i]-'0']; for(int r=0;r<5;r++)for(int c=0;c<3;c++) if(g&(1<<(14-(r*3+c)))) pal_rect(x+i*4+c,y+r,1,1,0xffffffff); } }
void game_draw(void){
    int cx=(int)camx, cy=(int)camy;
    for(int y=0;y<SCR_H;y+=8){ int g=y*100/SCR_H; pal_rect(0,y,SCR_W,8,RGBA(70+g/3,140+g/2,235-g/4,255)); }
    int tx0=cx/TS, ty0=cy/TS, tx1=(cx+SCR_W)/TS+1, ty1=(cy+SCR_H)/TS+1;
    for(int x=tx0;x<=tx1;x++)for(int y=ty0;y<=ty1;y++){ if(!inb(x,y))continue; Tile *t=TL(x,y); int sx=x*TS-cx,sy=y*TS-cy;
        if(t->wall&&(t->type==255||!SOLID[t->type])){ uint8_t l=shade_l(rlight(x,y)); if(l<LG_AMBIENT)l=LG_AMBIENT; int wt=RGBA((150*l)/255,(150*l)/255,(150*l)/255,255); Tex *w=get_wall_tex(t->wall); if(w) pal_blit(w,g_wallcell[t->wall][0]*36+2+(x&1)*16,g_wallcell[t->wall][1]*36+2+(y&1)*16,16,16,sx,sy,16,16,wt,0); else { int wr=RGBA((60*l)/255,(50*l)/255,(40*l)/255,255); pal_rect(sx,sy,TS,TS,wr); } } }
    for(int x=tx0;x<=tx1;x++)for(int y=ty0;y<=ty1;y++){ if(!inb(x,y))continue; Tile *t=TL(x,y); if(t->type!=255) draw_tile(t,x,y,x*TS-cx,y*TS-cy); }
    /* jogador */
    if(playertex){ pal_blit(playertex,0,P.frame*56,40,56,(int)P.x-10-cx,(int)P.y-10-cy,40,56,0xffffffff,P.dir<0); }
    else pal_rect((int)P.x-cx,(int)P.y-cy,PW,PH,0xffffffff);
    for(int x=tx0;x<=tx1;x++)for(int y=ty0;y<=ty1;y++){ if(!inb(x,y))continue; Tile *t=TL(x,y); if(t->liq){ int h=t->liq*TS/255; if(!h)h=1; int up=inb(x,y-1)&&TL(x,y-1)->liq; if(up)h=TS; uint8_t l=light_at(&LM,x,y); if(l<LG_AMBIENT) l=LG_AMBIENT;
        uint32_t c=(t->lf&1)?RGBA((255*l)/255,(90*l)/255,(10*l)/255,200):RGBA((40*l)/255,(90*l)/255,(230*l)/255,130);
        pal_rect(x*TS-cx,y*TS-cy+TS-h,TS,h,c); } }
    /* itens dropados */
    for(int i=0;i<ndrop;i++){ Drop *d=&drops[i]; int sx=(int)d->x-cx-8, sy=(int)d->y-cy-8; uint8_t l=light_at(&LM,(int)d->x/TS,(int)d->y/TS); if(l<LG_AMBIENT+40)l=LG_AMBIENT+40;
        Tex *it=get_item_tex(d->id); if(it) pal_blit(it,0,0,it->w,it->h,sx,sy,16,16,RGBA(l,l,l,255),0); else         pal_rect(sx+3,sy+3,10,10,RGBA(255,255,0,255)); }
    /* particulas */
    for(int i=0;i<npart;i++){ Part *p=&parts[i]; int a=p->life<12?p->life*20:255; uint32_t c=(p->c&0xffffff00)|(uint32_t)((p->c&255)*a/255); pal_rect((int)p->x-cx-1,(int)p->y-cy-1,2,2,c); }
    /* mobs */
    for(int i=0;i<nmob;i++){
        Mob *m=&mobs[i];
        uint8_t l=light_at(&LM,(int)m->x/TS,(int)m->y/TS); if(l<LG_AMBIENT+40)l=LG_AMBIENT+40;
        Tex *mt=mobtex[m->type&15];
        if(mt){
            int fw=mob_fw[m->type&15], fh=mob_fh[m->type&15];
            if(fw>mt->w) fw=mt->w; if(fh>mt->h) fh=mt->h;
            int nf=mt->h/fh; if(nf<1) nf=1;
            int ff=(m->anim/8)%nf;
            int sx=(int)m->x-cx-fw/2, sy=(int)m->y-cy+8-fh;   /* piso = base da caixa (y+8) */
            pal_blit(mt,0,ff*fh,fw,fh,sx,sy,fw,fh,RGBA(l,l,l,255),m->vx<0);
        } else { int sx=(int)m->x-cx-8, sy=(int)m->y-cy-4; pal_rect(sx,sy,16,12,RGBA(120,230,100,255)); }
    }
    /* cursor e dano */
    int tx=(cx+curx)/TS, ty=(cy+cury)/TS; pal_rect(tx*TS-cx,ty*TS-cy,TS,1,RGBA(255,255,255,200)); pal_rect(tx*TS-cx,ty*TS-cy+TS-1,TS,1,RGBA(255,255,255,200));
    pal_rect(tx*TS-cx,ty*TS-cy,1,TS,RGBA(255,255,255,200)); pal_rect(tx*TS-cx+TS-1,ty*TS-cy,1,TS,RGBA(255,255,255,200));
    if(hit.t>0){ int bw=hit.dmg*TS/100; pal_rect(hit.x*TS-cx,hit.y*TS-cy+TS-2,bw,2,RGBA(255,60,60,255)); }
    /* hotbar */
    for(int i=0;i<10;i++){ int x=6+i*24,y=4; if(slottex) pal_blit(slottex,0,0,78,78,x,y,22,22,i==sel?0xffffffff:RGBA(255,255,255,170),0); else pal_rect(x,y,22,22,RGBA(30,30,80,150));
        if(inv[i].id){ Tex *it=get_item_tex(inv[i].id); if(it) pal_blit(it,0,0,it->w,it->h,x+3,y+3,16,16,0xffffffff,0); if(inv[i].n>1) draw_num(inv[i].n,x+3,y+16); } }
}
