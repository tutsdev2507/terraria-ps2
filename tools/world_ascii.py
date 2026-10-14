#!/usr/bin/env python3
"""Le um dump binario de mundo (formato WG01 gerado por wgdump.c) e imprime
uma visao ASCII legivel do terreno, para inspecao sem depender de imagens.

Uso:
    world_ascii.py arquivo.bin [--cols N] [--rows N]
    world_ascii.py arquivo.bin --zoom X Y [--cols N] [--rows N]

Formato WG01: 'WG01' + u16 w + u16 h + u16 spawnx + tiles (8 bytes cada,
coluna-maior idx = x*h + y): type,u8 wall,u8 liq,u8 lf,u16 fx,u16 fy.
"""
import sys, struct
from collections import Counter

T = {0:'d',1:'#',2:'"',3:'*',5:'T',6:'i',7:'c',8:'g',9:'s',21:'C',
     53:':',58:'=',59:'m',60:'j',255:' '}
PRIO = ['T','C','*','j','m','~','^',':','=','"','d','#','.','c','i','s','g',' ']

def load(path):
    d=open(path,'rb').read()
    assert d[:4]==b'WG01', d[:4]
    w,h,sp=struct.unpack('<HHH', d[4:10])
    assert len(d)>=10+w*h*8, (len(d), 10+w*h*8)
    return d,w,h,sp

def make(d,w,h):
    tops=[0]*w
    for x in range(w):
        for y in range(h):
            o=10+(x*h+y)*8
            if d[o]!=255: tops[x]=y; break
        else: tops[x]=h
    def ch(x,y):
        o=10+(x*h+y)*8
        t=d[o]
        if t!=255: return T.get(t,'?')
        if d[o+2]: return '^' if d[o+3]&1 else '~'
        return ' ' if y<tops[x] else '.'
    return ch,tops

def render(ch,x0,x1,y0,y1,cols,rows,title,tops):
    print("== %s == x=%d..%d y=%d..%d"%(title,x0,x1,y0,y1))
    bw=max(1,(x1-x0)//cols); bh=max(1,(y1-y0)//rows)
    print('    '+''.join(str((x0+i*bw)//100%10) for i in range((x1-x0)//bw)))
    for r in range((y1-y0)//bh):
        line=''
        for c in range((x1-x0)//bw):
            cnt=Counter()
            for yy in range(y0+r*bh,min(y0+(r+1)*bh,y1)):
                for xx in range(x0+c*bw,min(x0+(c+1)*bw,x1)):
                    cnt[ch(xx,yy)]+=1
            best=max(cnt.items(), key=lambda kv:(kv[1],-PRIO.index(kv[0])))
            line+=best[0]
        print('%4d '%(y0+r*bh)+line)

def main():
    a=sys.argv
    path=a[1]; cols=rows=None; zoom=None
    i=2
    while i<len(a):
        if a[i]=='--cols': cols=int(a[i+1]); i+=2
        elif a[i]=='--rows': rows=int(a[i+1]); i+=2
        elif a[i]=='--zoom': zoom=(int(a[i+1]),int(a[i+2])); i+=3
        else: i+=1
    d,w,h,sp=load(path); ch,tops=make(d,w,h)
    print("mundo %dx%d spawn=%d  (tiles: '%s')"%(w,h,sp,
        ''.join(sorted(set(T.values())))))
    if zoom:
        cx,cy=zoom
        cols=cols or 120; rows=rows or 70
        render(ch,max(0,cx-cols//2),min(w,cx+cols//2),max(0,cy-rows//2),
               min(h,cy+rows//2),cols,rows,'ZOOM x=%d y=%d'%(cx,cy),tops)
    else:
        render(ch,0,w,0,h,cols or 175,rows or 58,'MUNDO COMPLETO',tops)

if __name__=='__main__':
    main()
