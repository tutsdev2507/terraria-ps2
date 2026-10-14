/* Gerador de mundo procedural do port Terraria PS2 (deterministico por semente).
 *
 * ORDEM DE PASSES derivada da desmontagem de WorldGen::generateWorld() em
 * libTerraria.so (rizin, modo Thumb; 2715 chamadas mapeadas - ver
 * docs/worldgen_evidencia.md): resetGen -> terreno (Mountinater/TileRunner x N)
 * -> JungleRunner/MudWallRunner -> SpreadGrass/CleanupWorld -> TileRunners
 * (minerios) -> CreateOcean -> CreateFloatingIslands -> QuickWater/WaterCheck
 * -> AddBuriedChest -> AddTrees/AddPlants -> SquareTileFrame.
 *
 * VALORES NUMERICOS = aproximacoes de Terraria 1.2 (docs/uncertainties.md).
 * ARVORES/PLANTAS/BAUS = celulas copiadas literalmente do tutorial (src/templates.inc),
 * garantindo fidelidade de sprite (fx,fy) no render com tileframe.bin.
 *
 * IDs usados (textureis existem em data/): 0 terra, 1 pedra, 2 grama, 3 planta,
 * 5 arvore, 6 ferro, 7 cobre, 8 ouro, 9 prata, 21 bau, 53 areia, 58 bloco
 * escuro (inferno), 59 lama, 60 grama de selva, 255 vazio; paredes 2 (terra) e
 * 27 (pedra) - unicas com textura de fundo para cavernas no port (data/wall_*.tex).
 */
/* Port 1:1 do WorldGen do Terraria 1.2.4.1 (Client/Terraria/WorldGen.cs).
 * Referencia local: /data/data/com.termux/files/tmp/opencode/re_terraria/ref124/
 * O RNG e o System.Random do .NET (src/dotnet_rand.c), reproduzindo a mesma
 * sequencia de sorteios (genRand) do jogo. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "worldgen.h"
#include "dotnet_rand.h"
#include "templates.inc"

enum { T_DIRT=0, T_STONE=1, T_GRASS=2, T_PLANT=3, T_TREE=5, T_IRON=6, T_COPPER=7,
       T_GOLD=8, T_SILVER=9, T_CHEST=21, T_SAND=53, T_HELL=58, T_MUD=59,
       T_JGRASS=60, T_EMPTY=255 };
enum { W_DIRT=2, W_STONE=27 };
enum { RUN_CARVE=-1, RUN_DIRT=0, RUN_STONE=1 };

static Tile *G;
static int GW, GH;
static unsigned g_seed;
static DotNetRandom R;               /* genRand */
static int *g_surf, *g_dirtb;        /* topo solido / base da terra (por coluna) */
static uint8_t *g_jung;
static int g_min_cave;
static int *g_stack; static int g_scap;
static double g_worldSurface, g_rockLayer, g_worldSurfaceLow, g_surfHigh, g_rockHigh;
static int g_waterLine, g_lavaLine;

#define IN(x,y) ((x)>=0&&(x)<GW&&(y)>=0&&(y)<GH)
#define TLG(x,y) (&G[(x)*GH+(y)])
static int divi(int a,int b){ int v=a/b; return v<1?1:v; }

/* genRand: Next(lo,hi) e Next(hi) do System.Random */
static int gr(int lo,int hi){ return dnr_next_range(&R,lo,hi); }
static int gr0(int hi){ return dnr_next_max(&R,hi); }
/* auxiliar inclusivo [lo,hi] sobre o mesmo genRand */
static int rndr(int lo,int hi){ if(hi<=lo) return lo; return dnr_next_range(&R,lo,hi+1); }

/* ---------- TileRunner exato (WorldGen.cs:37343) ----------
 * type<0 escava (o -2 tambem preenche liquido); type>=0 coloca/sobrescreve. */
static void tile_runner(int i,int j,double strength,int steps,int type,int addTile,
                        float speedX,float speedY,int noYChange,int overRide){
    double num=strength; float num2=(float)steps;
    float vx=(float)i, vy=(float)j;
    float sx=(float)gr(-10,11)*0.1f, sy=(float)gr(-10,11)*0.1f;
    static const float th[12]={50,100,150,200,250,300,400,500,600,700,800,900};
    if(speedX!=0.0f||speedY!=0.0f){ sx=speedX; sy=speedY; }
    while(num>0.0 && num2>0.0f){
        if(vy<0.0f && num2>0.0f && type==59) num2=0.0f;
        num=strength*(double)(num2/(float)steps);
        num2-=1.0f;
        int x0=(int)((double)vx-num*0.5), x1=(int)((double)vx+num*0.5);
        int y0=(int)((double)vy-num*0.5), y1=(int)((double)vy+num*0.5);
        if(x0<1)x0=1; if(x1>GW-1)x1=GW-1; if(y0<1)y0=1; if(y1>GH-1)y1=GH-1;
        for(int k=x0;k<x1;k++) for(int l=y0;l<y1;l++){
            if((double)(fabsf((float)k-vx)+fabsf((float)l-vy)) < strength*0.5*(1.0+(double)gr(-10,11)*0.015)){
                Tile *t=TLG(k,l);
                if(type<0){
                    if(type==-2 && t->type!=T_EMPTY && (l<g_waterLine||l>g_lavaLine)){
                        t->liq=255; if(l>g_lavaLine) t->lf|=1;
                    }
                    t->type=T_EMPTY;
                }else{
                    if((overRide||t->type==T_EMPTY) && (type!=40||t->type!=T_SAND)){
                        if(t->type!=T_SAND || (double)l>=g_worldSurface) t->type=(uint8_t)type;
                        else if(type==59) t->type=(uint8_t)type;
                    }
                    if(addTile){ t->type=(uint8_t)type; t->liq=0; t->lf=0; }
                    if(noYChange && (double)l<g_worldSurface && type!=59) t->wall=W_DIRT;
                    if(type==59 && l>g_waterLine && t->liq>0){ t->lf=0; t->liq=0; }
                }
            }
        }
        vx+=sx; vy+=sy;
        for(int ti=0; ti<12 && num>th[ti]; ti++){
            vx+=sx; num2-=1.0f;
            sy+=(float)gr(-10,11)*0.05f; sx+=(float)gr(-10,11)*0.05f;
        }
        sx+=(float)gr(-10,11)*0.05f; if(sx>1.0f)sx=1.0f; if(sx<-1.0f)sx=-1.0f;
        if(!noYChange){ sy+=(float)gr(-10,11)*0.05f; if(sy>1.0f)sy=1.0f; if(sy<-1.0f)sy=-1.0f; }
        else if(type!=59 && num<3.0){ if(sy>1.0f)sy=1.0f; if(sy<-1.0f)sy=-1.0f; }
        if(type==59 && !noYChange){
            if(sy>0.5f)sy=0.5f; if(sy<-0.5f)sy=-0.5f;
            if((double)vy<g_rockLayer+100.0)sy=1.0f;
            if(vy>(float)(GH-300))sy=-1.0f;
        }
    }
}

/* ---------- inundacao por inundacao (agua/lava nas cavernas) ---------- */
static void stack_push(int *sp,int x,int y){
    if(*sp>=g_scap){
        int nc=g_scap?g_scap*2:4096;
        int *ns=realloc(g_stack,(size_t)nc*sizeof(int));
        if(!ns) return;
        g_stack=ns; g_scap=nc;
    }
    if(*sp<g_scap){ g_stack[(*sp)++]=y*GW+x; }
}
static int flood_air(int sx,int sy,int liq,int lf,int ymin,int ymax){
    if(!IN(sx,sy)) return 0;
    if(sy<=g_surf[sx]||sy<ymin||sy>ymax) return 0;   /* so subterraneo, na faixa */
    Tile *s=TLG(sx,sy);
    if(s->type!=T_EMPTY||s->liq) return 0;
    if(!g_stack){ g_stack=malloc(4096*sizeof(int)); if(!g_stack) return 0; g_scap=4096; }
    int sp=0, n=0;
    s->liq=254; stack_push(&sp,sx,sy);
    while(sp>0){
        int idx=g_stack[--sp]; int x=idx%GW, y=idx/GW;
        Tile *t=TLG(x,y);
        t->liq=(uint8_t)liq; t->lf=(uint8_t)lf; n++;
        static const int dx4[4]={0,0,-1,1}, dy4[4]={-1,1,0,0};
        for(int k=0;k<4;k++){
            int nx=x+dx4[k], ny=y+dy4[k];
            if(!IN(nx,ny)) continue;
            Tile *nt=TLG(nx,ny);
            if(ny<=g_surf[nx]||ny<ymin||ny>ymax) continue;   /* nao vaza p/ ceu */
            if(nt->type==T_EMPTY&&nt->liq==0){ nt->liq=254; stack_push(&sp,nx,ny); }
        }
    }
    return n;
}

/* ---------- Mountinater exato (WorldGen.cs:38553): montanha de terra ---------- */
static void mountinater(int i,int j){
    double num=(double)gr(80,120);
    float num2=(float)gr(40,55);
    float vx=(float)i, vy=(float)j+num2/2.0f;
    float sx=(float)gr(-10,11)*0.1f, sy=(float)gr(-20,-10)*0.1f;
    while(num>0.0 && num2>0.0f){
        num-=(double)gr0(4);
        num2-=1.0f;
        int x0=(int)((double)vx-num*0.5), x1=(int)((double)vx+num*0.5);
        int y0=(int)((double)vy-num*0.5), y1=(int)((double)vy+num*0.5);
        if(x0<0)x0=0; if(x1>GW)x1=GW; if(y0<0)y0=0; if(y1>GH)y1=GH;
        double num7=num*(double)gr(80,120)*0.01;
        for(int k=x0;k<x1;k++) for(int l=y0;l<y1;l++){
            float a=fabsf((float)k-vx), b=fabsf((float)l-vy);
            if(sqrt((double)(a*a+b*b)) < num7*0.4 && TLG(k,l)->type==T_EMPTY){
                TLG(k,l)->type=T_DIRT;
            }
        }
        vx+=sx; vy+=sy;
        sx+=(float)gr(-10,11)*0.05f; sy+=(float)gr(-10,11)*0.05f;
        if(sx>0.5f)sx=0.5f; if(sx<-0.5f)sx=-0.5f;
        if(sy>-0.5f)sy=-0.5f; if(sy<-1.5f)sy=-1.5f;
    }
}

int wg_generate(Tile *tiles,int w,int h,uint32_t seed,int *spawnx){
    if(!tiles||w<64||h<64) return 0;
    G=tiles; GW=w; GH=h; g_seed=seed?seed:1; dnr_seed(&R,(int)g_seed);
    g_stack=NULL; g_scap=0;
    if(spawnx) *spawnx=w/2;

    memset(tiles,0,(size_t)w*(size_t)h*sizeof(Tile));
    for(size_t i=0;i<(size_t)w*(size_t)h;i++) tiles[i].type=T_EMPTY;

    g_surf=malloc((size_t)w*sizeof(int));
    g_dirtb=malloc((size_t)w*sizeof(int));
    g_jung=malloc((size_t)w);
    if(!g_surf||!g_dirtb||!g_jung){ free(g_surf); free(g_dirtb); free(g_jung); return 0; }
    g_min_cave=divi(GH,25);

    /* ---- 1. superficie: random-walk exato do generateWorld (1.2.4.1) ---- */
    double n4=(double)GH*0.3*(double)gr(90,110)*0.005;
    double n5=(n4+(double)GH*0.2)*(double)gr(90,110)*0.01;
    double n6=n4, n7=n5, n8=n5, n4min=n4;
    int n2=0, n3=0;
    for(int i=0;i<GW;i++){
        if(n4>n6) n6=n4;
        if(n4<n4min) n4min=n4;
        if(n5<n7) n7=n5;
        if(n5>n8) n8=n5;
        if(n3<=0){
            n2=gr(0,5); n3=gr(5,40);
            if(n2==0) n3*=(int)((double)gr(5,30)*0.2);
        }
        n3--;
        if((double)i>GW*0.43 && (double)i<GW*0.57 && n2>=3) n2=gr0(3);
        if((double)i>GW*0.47 && (double)i<GW*0.53) n2=0;
        if(n2==0){ while(gr0(7)==0) n4+=(double)gr(-1,2); }
        else if(n2==1){ while(gr0(4)==0) n4-=1.0; while(gr0(10)==0) n4+=1.0; }
        else if(n2==2){ while(gr0(4)==0) n4+=1.0; while(gr0(10)==0) n4-=1.0; }
        else if(n2==3){ while(gr0(2)==0) n4-=1.0; while(gr0(6)==0) n4+=1.0; }
        else { while(gr0(2)==0) n4+=1.0; while(gr0(5)==0) n4-=1.0; }
        if(n4<(double)GH*0.17){ n4=(double)GH*0.17; n3=0; }
        else if(n4>(double)GH*0.3){ n4=(double)GH*0.3; n3=0; }
        if((i<275||i>GW-275)&&n4>(double)GH*0.25){ n4=(double)GH*0.25; n3=1; }
        while(gr0(3)==0) n5+=(double)gr(-2,3);
        if(n5<n4+(double)GH*0.05) n5+=1.0;
        if(n5>n4+(double)GH*0.35) n5-=1.0;
        g_surf[i]=(int)n4;
        g_dirtb[i]=(int)n5;
    }
    g_worldSurface=n6+25.0;
    g_worldSurfaceLow=n4min; g_surfHigh=n6; g_rockHigh=n8;
    { double rk=n8; rk=g_worldSurface+(double)((int)((rk-g_worldSurface)/6.0)*6); g_rockLayer=rk; }
    g_waterLine=((int)(g_rockLayer+(double)GH))/2;
    g_waterLine+=gr(-100,20);
    g_lavaLine=g_waterLine+gr(50,80);
    int base=0; for(int x=0;x<GW;x++) base+=g_surf[x]; base/=GW;
    int y0=0;

    /* ---- 2. oceanos nas duas bordas (CreateOcean): chao desce ate o mar ---- */
    int g_sea=base+GH/100;
    int bw=divi(GW,16); if(bw>GW/4) bw=GW/4;
    int odepth=GH*5/100; if(odepth<8) odepth=8;
    for(int side=0;side<2;side++) for(int i=0;i<bw;i++){
        int x=side?GW-1-i:i;
        int t=(i+1)*1000/bw; if(t>1000) t=1000;
        int sm=(t*t*(3000-2*t))/1000000;              /* smoothstep 0..1 (x1e3) */
        int edge=g_sea+odepth;
        int gr=edge+(g_surf[x]-edge)*sm/1000;
        if(gr<3) gr=3; if(gr>GH-6) gr=GH-6;
        g_surf[x]=gr;
    }

    /* ---- 3. camadas: terra ate g_dirtb (do random-walk) e pedra abaixo ---- */
    for(int x=0;x<GW;x++){
        int db=g_dirtb[x];
        if(db<g_surf[x]) db=g_surf[x];
        if(db>GH-6) db=GH-6;
        g_dirtb[x]=db;
        for(int y=g_surf[x];y<=db;y++) TLG(x,y)->type=T_DIRT;
        for(int y=db+1;y<GH;y++) TLG(x,y)->type=T_STONE;
    }
    int helly0=GH-GH/30;
    for(int y=helly0;y<GH;y++) for(int x=0;x<GW;x++) TLG(x,y)->type=T_HELL;

    /* ---- 3b. montanhas (Mountinater) na superficie, longe do centro ---- */
    {
        int mCaveX[256], numMC=0;
        int total=(int)(GW*0.0008);
        for(int n=0;n<total;n++){
            int x=gr(divi(GW,4),GW*3/4), guard=0;
            while(1){
                while(x>GW/2-100 && x<GW/2+100) x=gr(divi(GW,4),GW*3/4);
                int clash=0;
                for(int m=0;m<numMC;m++) if(x>mCaveX[m]-50 && x<mCaveX[m]+50){clash=1;break;}
                if(!clash) break;
                if(++guard>=200) break;
            }
            if(guard>=200) continue;
            for(int yy=0;yy<(int)g_surfHigh;yy++) if(TLG(x,yy)->type!=T_EMPTY){
                mountinater(x,yy);
                if(numMC<256) mCaveX[numMC++]=x;
                break;
            }
        }
    }

    /* ---- 4. selva (JungleRunner/MudWallRunner): um lado vira lama ---- */
    int jside=rndr(0,1);
    int jw=GW*3/10;
    int ow=bw*2;                                   /* faixa dos oceanos (com praia) */
    int inset=ow+divi(GW,50);                      /* selva recuada do oceano */
    int jx0=jside?GW-jw-inset:inset;
    int jfw=divi(GW,60);                           /* franja aleatoria da borda da selva */
    memset(g_jung,0,(size_t)GW);
    for(int x=jx0-jfw;x<jx0+jw+jfw;x++){
        if(x<ow||x>=GW-ow) continue;               /* nunca invade o oceano */
        g_jung[x]=(x>=jx0&&x<jx0+jw)?1:(rndr(0,9)<3);
    }
    for(int x=0;x<GW;x++){
        if(!g_jung[x]) continue;
        int db=g_surf[x]+GH*22/100; if(db>GH-6) db=GH-6;
        for(int y=g_surf[x];y<=db;y++){
            Tile *t=TLG(x,y);
            if(t->type==T_DIRT||t->type==T_STONE) t->type=T_MUD;
        }
    }

    /* ---- 5. areia + agua dos oceanos ---- */
    int sandd=GH/14;
    for(int side=0;side<2;side++) for(int i=0;i<bw*2&&i<GW;i++){
        int x=side?GW-1-i:i;
        int d0=g_surf[x]+sandd; if(d0>g_dirtb[x]) d0=g_dirtb[x];
        for(int y=g_surf[x];y<=d0;y++){
            Tile *t=TLG(x,y);
            if(t->type==T_DIRT) t->type=T_SAND;
        }
        if(g_surf[x]>g_sea)
            for(int y=g_surf[x]-1;y>=g_sea;y--){
                Tile *t=TLG(x,y);
                if(t->type==T_EMPTY&&t->liq==0){ t->liq=255; }
                else break;
            }
    }

    /* ---- 6. cavernas: passes exatos do generateWorld (TileRunner -1/-2) ---- */
    for(int x=0;x<GW;x++) y0+=g_surf[x];
    y0/=GW;
    double area=(double)GW*(double)GH;
    /* 6a. cavernas pequenas/grandes (gen[7]) */
    for(int k=0;k<(int)(area*0.0015);k++){
        int t5=(gr0(5)==0)?-2:-1;
        tile_runner(gr0(GW), gr((int)g_surfHigh,GH), (double)gr(2,5), gr(2,20), t5,0,0.0f,0.0f,0,1);
        tile_runner(gr0(GW), gr((int)g_surfHigh,GH), (double)gr(8,15), gr(7,30), t5,0,0.0f,0.0f,0,1);
    }
    /* 6b. tuneis medios (gen[8]) */
    for(int k=0;k<(int)(area*3E-05);k++){
        int t6=(gr0(6)==0)?-2:-1;
        tile_runner(gr0(GW), gr((int)g_worldSurfaceLow,(int)g_rockHigh+1), (double)gr(5,15), gr(30,200), t6,0,0.0f,0.0f,0,1);
    }
    /* 6c. tuneis grandes (gen[9]) */
    for(int k=0;k<(int)(area*0.00013);k++){
        int t7=(gr0(10)==0)?-2:-1;
        tile_runner(gr0(GW), gr((int)g_rockHigh,GH), (double)gr(6,20), gr(50,300), t7,0,0.0f,0.0f,0,1);
    }
    /* 6d. tuneis verticais a partir da superficie */
    for(int k=0;k<(int)(GW*0.002);k++){
        int x=gr0(GW); while(x>GW*0.45 && x<GW*0.55) x=gr0(GW);
        for(int yy=0;yy<(int)g_surfHigh;yy++) if(TLG(x,yy)->type!=T_EMPTY){
            tile_runner(x,yy,(double)gr(3,6), gr(5,50), -1,0,(float)gr(-10,11)*0.1f,1.0f,0,1); break; }
    }
    for(int k=0;k<(int)(GW*0.0007);k++){
        int x=gr0(GW); while(x>GW*0.43 && x<GW*0.58) x=gr0(GW);
        for(int yy=0;yy<(int)g_surfHigh;yy++) if(TLG(x,yy)->type!=T_EMPTY){
            tile_runner(x,yy,(double)gr(10,15), gr(50,130), -1,0,(float)gr(-10,11)*0.1f,2.0f,0,1); break; }
    }
    for(int k=0;k<(int)(GW*0.0003);k++){
        int x=gr0(GW); while(x>GW*0.4 && x<GW*0.6) x=gr0(GW);
        for(int yy=0;yy<(int)g_surfHigh;yy++) if(TLG(x,yy)->type!=T_EMPTY){
            tile_runner(x,yy,(double)gr(12,25), gr(150,500), -1,0,(float)gr(-10,11)*0.1f,4.0f,0,1);
            tile_runner(x,yy,(double)gr(8,17), gr(60,200), -1,0,(float)gr(-10,11)*0.1f,2.0f,0,1);
            tile_runner(x,yy,(double)gr(5,13), gr(40,170), -1,0,(float)gr(-10,11)*0.1f,2.0f,0,1);
            break; }
    }
    for(int k=0;k<(int)(GW*0.0004);k++){
        int x=gr0(GW); while(x>GW*0.4 && x<GW*0.6) x=gr0(GW);
        for(int yy=0;yy<(int)g_surfHigh;yy++) if(TLG(x,yy)->type!=T_EMPTY){
            tile_runner(x,yy,(double)gr(7,12), gr(150,250), -1,0,0.0f,1.0f,1,1); break; }
    }

    /* ---- 7. minerios (OreRunner): cobre/ferro/prata/ouro em faixas ---- */
    static const int ores[4]={T_COPPER,T_IRON,T_SILVER,T_GOLD};
    for(int k=0;k<4;k++){
        int top=y0+(GH-y0)*(k==3?40:k==2?20:0)/100;
        int bot=y0+(GH-y0)*(k==0?45:k==1?70:k==2?90:100)/100;
        if(bot>GH-6) bot=GH-6;
        if(bot<=top+4) continue;
        int count=(GW*GH)/(6000+k*1400); if(count<20) count=20;
        for(int i=0;i<count;i++)
            tile_runner(rndr(2,GW-3),rndr(top,bot),rndr(2,5),rndr(1,3),ores[k],0,0,0,0,1);
    }

    /* ---- 8. ilhas flutuantes (CreateFloatingIslands) ---- */
    int ni=3+rndr(0,3);
    for(int k=0;k<ni;k++){
        int cx=rndr(divi(GW,8),GW-divi(GW,8));
        int r=rndr(divi(GW,55),divi(GW,30)); if(r<6) r=6; if(r>26) r=26;
        int cy=g_surf[cx]-rndr(divi(GH,18),divi(GH,9))-r;
        if(cy-r<4) continue;
        int rx=(r*3)/2;
        for(int dy=-r;dy<=r;dy++) for(int dx=-rx;dx<=rx;dx++){
            long a=(long)dx*dx*100/((long)rx*rx), b=(long)dy*dy*100/((long)r*r);
            if(a+b>100) continue;
            int x=cx+dx, y=cy+dy; if(!IN(x,y)) continue;
            Tile *t=TLG(x,y);
            t->type=(y<cy)?T_GRASS:T_DIRT; t->fx=t->fy=0; t->liq=t->lf=0;
        }
        /* cavidade interna com parede de terra */
        int irx=rx/2, iry=r/2; if(irx<2||iry<2) continue;
        for(int dy=-iry;dy<=iry;dy++) for(int dx=-irx;dx<=irx;dx++){
            if(dx*dx*iry*iry+dy*dy*irx*irx>irx*irx*iry*iry) continue;
            int x=cx+dx, y=cy+dy; if(!IN(x,y)) continue;
            Tile *t=TLG(x,y);
            t->type=T_EMPTY; t->fx=t->fy=0; t->liq=t->lf=0; t->wall=W_DIRT;
        }
    }

    /* ---- 9. agua e lava nas cavernas (QuickWater/WaterCheck) ---- */
    int waterline=y0+GH*12/100;
    int lavaline=GH-GH*8/100;
    if(lavaline>waterline+5){
        int lakes=2+rndr(0,divi(GW*GH,400000));
        for(int k=0;k<lakes;k++)
            flood_air(rndr(0,GW-1),rndr(waterline,lavaline-1),255,0,waterline,lavaline);
        int lp=3+rndr(0,4), got=0;
        for(int k=0;k<lp;k++)
            if(flood_air(rndr(0,GW-1),rndr(lavaline,GH-4),255,1,lavaline,GH-3)) got=1;
        if(!got){   /* garante lava no submundo mesmo sem caverna aberta la */
            for(int k=0;k<4&&!got;k++){
                int x=rndr(divi(GW,4),GW-divi(GW,4));
                int y=rndr(lavaline+2,GH-6); if(y<=lavaline+1) continue;
                tile_runner(x,y,rndr(3,6),rndr(1,2),RUN_CARVE,0,0,0,0,1);
                if(flood_air(x,y,255,1,lavaline,GH-3)) got=1;
            }
        }
    }

    /* ---- 10. paredes de fundo nas cavernas (CleanupWorld) ---- */
    for(int x=0;x<GW;x++) for(int y=g_surf[x]+1;y<GH;y++){
        Tile *t=TLG(x,y);
        if(t->type==T_EMPTY&&t->wall==0) t->wall=(y<=g_dirtb[x])?W_DIRT:W_STONE;
    }

    /* ---- 11. grama (SpreadGrass): terra/lama exposta ao ar ---- */
    for(int x=0;x<GW;x++) for(int y=g_surf[x];y<GH;y++){
        Tile *t=TLG(x,y);
        if(y<=0) break;
        Tile *up=TLG(x,y-1);
        if(up->type!=T_EMPTY) continue;
        if(t->type==T_DIRT) t->type=T_GRASS;
        else if(t->type==T_MUD) t->type=T_JGRASS;
    }

    /* ---- 12. arvores, plantas e baus (AddTrees/AddPlants/AddBuriedChest) ---- */
    int last=-99;
    for(int x=6;x<GW-6;x++){
        if(x-last<4+rndr(0,3)) continue;
        if(rndr(0,99)>10) continue;
        int y=g_surf[x]; if(y<4||y>=GH-4) continue;
        Tile *g=TLG(x,y);
        if(g->type!=T_GRASS&&g->type!=T_JGRASS) continue;
        if(abs(g_surf[x-2]-y)>1||abs(g_surf[x+2]-y)>1) continue;
        int ti=rndr(0,(int)wg_tree_count-1);
        const WTplCell *tr=wg_trees[ti];
        int ok=1;
        for(unsigned c=0;c<wg_tree_sizes[ti]&&ok;c++){
            int tx=x+tr[c].dx, ty=y-1+tr[c].dy;
            if(!IN(tx,ty)||TLG(tx,ty)->type!=T_EMPTY||TLG(tx,ty)->liq) ok=0;
        }
        if(!ok) continue;
        for(unsigned c=0;c<wg_tree_sizes[ti];c++){
            Tile *t=TLG(x+tr[c].dx,y-1+tr[c].dy);
            t->type=T_TREE; t->fx=tr[c].fx; t->fy=tr[c].fy; t->wall=0;
        }
        last=x;
    }
    for(int x=4;x<GW-4;x++){
        int y=g_surf[x]; if(y<3) continue;
        Tile *g=TLG(x,y);
        if(g->type!=T_GRASS&&g->type!=T_JGRASS) continue;
        Tile *up=TLG(x,y-1);
        if(up->type!=T_EMPTY||up->liq) continue;
        if(rndr(0,99)>16) continue;
        int fi=rndr(0,(int)wg_plant_count-1);
        up->type=T_PLANT; up->fx=wg_plant_frames[fi][0]; up->fy=wg_plant_frames[fi][1];
    }
    int placed=0;
    for(int tries=0;tries<6000&&placed<8;tries++){
        int x=rndr(6,GW-8), y=rndr(y0+divi(GH,6),GH-10);
        Tile *probe=TLG(x,y);
        if(probe->type!=T_EMPTY||probe->liq) continue;   /* acha ar de caverna... */
        for(int v=0;v<2&&placed<8;v++){                  /* ...e encaixa o bau 2x2 */
            int ox=x, oy=v?y-1:y;
            if(ox>=GW-2||oy<1||oy>=GH-4) continue;
            int ok=1;
            for(int j=0;j<2&&ok;j++) for(int i=0;i<2&&ok;i++){
                Tile *t=TLG(ox+i,oy+j);
                if(t->type!=T_EMPTY||t->liq) ok=0;
            }
            for(int i=0;i<2&&ok;i++){
                Tile *t=TLG(ox+i,oy+2);
                if(t->type==T_EMPTY||t->liq) ok=0;
            }
            if(!ok) continue;
            for(unsigned c=0;c<4;c++){
                Tile *t=TLG(ox+wg_chest[c].dx,oy+wg_chest[c].dy);
                t->type=T_CHEST; t->fx=wg_chest[c].fx; t->fy=wg_chest[c].fy;
            }
            placed++;
        }
    }

    /* ---- 13. spawn: terreno plano perto do centro, longe d'agua ---- */
    int bestx=GW/2, best=0x7fffffff;
    for(int x=GW/2-divi(GW,10);x<=GW/2+divi(GW,10);x++){
        if(x<6||x>GW-7) continue;
        int s=0;
        for(int i=-5;i<=5;i++) s+=abs(g_surf[x+i]-g_surf[x]);
        /* chao limpo: nada de agua/objetos nas colunas proximas (arvores, plantas) */
        for(int i=-3;i<=3;i++){
            int yy0=g_surf[x+i]-16; if(yy0<1) yy0=1;
            for(int yy=yy0;yy<g_surf[x+i];yy++){
                Tile *c=TLG(x+i,yy);
                if(c->liq||c->type!=T_EMPTY) s+=4000;
            }
        }
        if(g_surf[x]>GH*85/100) s+=1000;
        if(s<best){ best=s; bestx=x; }
    }
    if(spawnx) *spawnx=bestx;

    free(g_surf); free(g_dirtb); free(g_jung); free(g_stack);
    g_surf=NULL; g_dirtb=NULL; g_jung=NULL; g_stack=NULL; g_scap=0;
    return 1;
}
