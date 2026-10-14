/* Backend web: servidor HTTP minimo. Android: Termux (pkg install clang make; make -f Makefile.host; ./terraria_web data) e abra http://localhost:8080 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "pal.h"
const char *data_root="data"; extern uint8_t sw_fb[];
static const char *HTML =
"<!doctype html><meta name=viewport content='width=device-width,initial-scale=1,user-scalable=no'><title>Terraria PS2/Android</title>"
"<style>html,body{margin:0;background:#000;height:100%;overflow:hidden;touch-action:none;user-select:none}canvas{width:100vw;height:calc(100vw*224/320);max-height:100vh;max-width:calc(100vh*320/224);image-rendering:pixelated;display:block;margin:auto;position:absolute;inset:0}"
".b{position:fixed;width:56px;height:56px;border-radius:50%;background:#fff3;color:#fff;font:700 18px sans-serif;display:flex;align-items:center;justify-content:center;border:2px solid #fff6}</style>"
"<canvas id=c width=320 height=224></canvas><div id=ui></div><script>"
"const c=document.getElementById('c'),x=c.getContext('2d'),im=x.createImageData(320,224);let B=0,pd=0,px=0,py=0;"
"const bits={U:1,D:2,L:4,R:8,X:16,O:32,S:64,T:128,L1:256,R1:512};"
"const lay=[['L',10,'80px','◀'],['R',90,'80px','▶'],['U',50,'140px','▲'],['D',50,'20px','▼']];"
"function btn(n,l,r,b,t){const e=document.createElement('div');e.className='b';e.textContent=t;e.style.cssText+=(l!=null?'left:'+l+'px;':'right:'+r+'px;')+'bottom:'+b+';';"
"const on=ev=>{B|=bits[n];ev.preventDefault();e.style.background='#fff8'},off=ev=>{B&=~bits[n];ev.preventDefault();e.style.background=''};"
"e.addEventListener('pointerdown',on);e.addEventListener('pointerup',off);e.addEventListener('pointercancel',off);e.addEventListener('pointerleave',off);document.getElementById('ui').appendChild(e)}"
"btn('L',10,null,'60px','◀');btn('R',140,null,'60px','▶');btn('X',null,10,'60px','✕');btn('S',null,80,'120px','□');btn('L1',10,null,'200px','L1');btn('R1',null,10,'200px','R1');"
"const km={ArrowLeft:4,a:4,ArrowRight:8,d:8,ArrowUp:1,w:1,' ':16,j:64,q:256,e:512};"
"onkeydown=e=>{if(km[e.key]){B|=km[e.key];e.preventDefault()}};onkeyup=e=>{if(km[e.key])B&=~km[e.key]};"
"function pos(e){const r=c.getBoundingClientRect();px=Math.floor((e.clientX-r.left)*320/r.width);py=Math.floor((e.clientY-r.top)*224/r.height)}"
"c.addEventListener('pointerdown',e=>{pos(e);pd=1});c.addEventListener('pointermove',e=>{if(pd)pos(e)});onpointerup=()=>pd=0;"
"async function f(){try{const r=await fetch('/f?b='+B+'&pd='+pd+'&px='+px+'&py='+py);const a=new Uint8Array(await r.arrayBuffer());"
"for(let i=0,j=0;i<320*224;i++){im.data[i*4]=a[j++];im.data[i*4+1]=a[j++];im.data[i*4+2]=a[j++];im.data[i*4+3]=255}x.putImageData(im,0,0)}catch(e){await new Promise(r=>setTimeout(r,200))}requestAnimationFrame(f)}f();"
"</script>";
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static int qint(const char *q,const char *k,int d){ char key[16]; snprintf(key,sizeof key,"%s=",k); const char *p=strstr(q,key); while(p&&p!=q&&p[-1]!='&'&&p[-1]!='?')p=strstr(p+1,key); return p?atoi(p+strlen(key)):d; }
int main(int argc,char **argv){
    int port=8080; if(argc>1)data_root=argv[1]; if(argc>2)port=atoi(argv[2]);
    game_init();
    int s=socket(AF_INET,SOCK_STREAM,0),one=1; setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&one,sizeof one);
    struct sockaddr_in a={0}; a.sin_family=AF_INET; a.sin_port=htons(port); a.sin_addr.s_addr=htonl(INADDR_ANY);
    if(bind(s,(struct sockaddr*)&a,sizeof a)||listen(s,8)){ perror("bind"); return 1; }
    printf("Abra http://localhost:%d no navegador (dados: %s)\n",port,data_root); fflush(stdout);
    double last=now(); static uint8_t rgb[SCR_W*SCR_H*3];
    for(;;){
        int c=accept(s,NULL,NULL); if(c<0) continue; char req[1024]; int n=read(c,req,sizeof req-1); if(n<=0){close(c);continue;} req[n]=0;
        if(!strncmp(req,"GET /f?",7)){
            Input in; memset(&in,0,sizeof in); const char *q=req+7; in.btn=qint(q,"b",0); in.ptr_down=qint(q,"pd",0); in.ptr_x=qint(q,"px",0); in.ptr_y=qint(q,"py",0);
            double t=now(); int ticks=(int)((t-last)*60.0); if(ticks>5){ticks=5;last=t;} else last+=ticks/60.0; if(ticks<1)ticks=1;
            for(int i=0;i<ticks;i++) game_tick(&in);
            game_draw(); for(int i=0;i<SCR_W*SCR_H;i++){ rgb[i*3]=sw_fb[i*4]; rgb[i*3+1]=sw_fb[i*4+1]; rgb[i*3+2]=sw_fb[i*4+2]; }
            char h[160]; int hl=snprintf(h,sizeof h,"HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\nCache-Control: no-store\r\n\r\n",SCR_W*SCR_H*3);
            write(c,h,hl); int off=0,tot=SCR_W*SCR_H*3; while(off<tot){ int w=write(c,rgb+off,tot-off); if(w<=0)break; off+=w; }
        } else { char h[160]; int hl=snprintf(h,sizeof h,"HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n\r\n",(int)strlen(HTML)); write(c,h,hl); write(c,HTML,strlen(HTML)); }
        close(c);
    }
}
