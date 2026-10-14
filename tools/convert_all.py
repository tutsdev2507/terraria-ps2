#!/usr/bin/env python3
"""Conversor reproduzivel: Tutorial.world -> data/tutorial.tw ; PNGs -> data/*.tex ; LUT de autotile -> data/tileframe.bin
Uso: convert_all.py <TerrariaAssets dir> <img dir (Images.zip extraido)> <out data dir>"""
import sys, os, struct, numpy as np
from PIL import Image
ASSETS, IMG, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
os.makedirs(OUT, exist_ok=True)
# ---------------- mundo ----------------
d = open(os.path.join(ASSETS, 'Tutorial.world'), 'rb').read()
W, H = 1750, 1000
# frameImportant aprendido por busca (docs/world_format.md)
IMP = {0:0,56:0,1:0,21:1,3:1,2:0,7:0,53:0,40:0,5:1,28:1,93:1,59:0,15:1,52:0,90:1,58:0,6:0,79:1,9:0,82:1,66:0,135:1,105:1,51:0,63:0,67:0,8:0,137:1,45:0,10:1,4:1,36:1,65:0,30:0,124:0,73:1,19:0,17:1,138:1,130:0,38:0,39:0,140:0,141:1,83:1,80:0,60:0}
T = np.full((W,H),255,np.uint8); WL = np.zeros((W,H),np.uint8); LQ = np.zeros((W,H),np.uint8)
LF = np.zeros((W,H),np.uint8); FX = np.zeros((W,H),np.uint16); FY = np.zeros((W,H),np.uint16)
o = 92
for x in range(W):
    y = 0
    while y < H:
        c = (d[o] | d[o+1] << 8) + 1; o += 2
        if x == W-1 and y+c >= H: break
        a = d[o]; o += 1; t = 255; fx = fy = 0
        if a:
            t = d[o]; o += 1
            if IMP[t]: fx, fy = struct.unpack_from('<HH', d, o); o += 4
        w = d[o]; o += 1; l = d[o]; o += 1; lt = 0
        if l: lt = d[o]; o += 1
        o += 1
        sl = slice(c-1, c) if y == 0 else slice(y, y+c)   # regra: 1o run da coluna = (c-1) vazios + tile
        T[x,sl]=t; WL[x,sl]=w; LQ[x,sl]=l; LF[x,sl]=lt; FX[x,sl]=fx; FY[x,sl]=fy
        y += c
# area util: colunas/linhas com algo alem do preenchimento de lava de borda
content = ((T != 255) & (T != 56)) | ((LQ > 0) & (LF == 0))
xs = np.where(content.sum(1) > 20)[0]; ys = np.where(content[xs.min():xs.max()+1].any(0))[0]
x0, x1, y0, y1 = max(0,xs.min()-4), min(W,xs.max()+5), 0, min(H, ys.max()+12)
cw, ch = x1-x0, y1-y0
print('bbox', x0, x1, y0, y1, cw, ch)
rec = np.zeros((cw,ch), dtype=[('t','u1'),('w','u1'),('l','u1'),('f','u1'),('fx','<u2'),('fy','<u2')])
rec['t']=T[x0:x1,y0:y1]; rec['w']=WL[x0:x1,y0:y1]; rec['l']=LQ[x0:x1,y0:y1]; rec['f']=LF[x0:x1,y0:y1]
rec['fx']=FX[x0:x1,y0:y1]; rec['fy']=FY[x0:x1,y0:y1]
spawn = struct.unpack_from('<h', d, 24+14)[0]   # hipotese: spawnTileX (873)
with open(os.path.join(OUT,'tutorial.tw'),'wb') as f:
    f.write(b'TW01'+struct.pack('<HHHHHH', cw, ch, x0, y0, 873-x0, 0))
    # layout coluna-maior: indice = x*ch + y
    f.write(rec.tobytes())
# ---------------- texturas ----------------
USED = set(np.unique(rec['t']).tolist()); USEDW = set(np.unique(rec['w']).tolist())
def savetex(img, name):
    img = img.convert('RGBA'); w, h = img.size
    with open(os.path.join(OUT, name), 'wb') as f:
        f.write(b'TEX0'+struct.pack('<HH', w, h)); f.write(img.tobytes())
tilelut = {}
def cell_masks(im):
    a = np.array(im.convert('RGBA')); h, w = a.shape[:2]
    res = {}
    for cy in range(h//18):
        for cx in range(w//18):
            c = a[cy*18:cy*18+16, cx*18:cx*18+16]
            if c.shape[0] < 16 or c.shape[1] < 16 or c[...,3].max() == 0: continue
            fr = lambda s: float((s[...,3] < 200).mean())
            m = (fr(c[0:2,2:14])>0.2)<<0 | (fr(c[14:16,2:14])>0.2)<<1 | (fr(c[2:14,0:2])>0.2)<<2 | (fr(c[2:14,14:16])>0.2)<<3
            res.setdefault(m, []).append((cx, cy))
    return res
lut = np.zeros((256,16,3,2), np.uint8)   # [tipo][mascara aberta t|b<<1|l<<2|r<<3][variante][cx,cy]
ntypes = 0
for t in range(256):
    p = os.path.join(IMG, 'Tiles', f'Tiles_{t}.png')
    if not os.path.exists(p): continue
    if t not in USED and t not in (0,1,2,3,30,31,4): continue
    im = Image.open(p); savetex(im, f'tiles_{t}.tex'); ntypes += 1
    if im.size[0] == 288:                      # folhas de autotile 16 colunas
        res = cell_masks(im)
        for m in range(16):
            cand = [c for c in res.get(m, []) if c[1] <= 4 and c[0] <= 5] or res.get(m, [])
            if m == 15: cand = [(9,3),(10,3),(11,3)]
            if not cand:  # vizinho mais proximo por Hamming
                best = sorted(res.items(), key=lambda kv: bin(kv[0]^m).count('1'))
                cand = [c for c in best[0][1] if c[1] <= 4] or best[0][1] if best else [(1,1)]
            for v in range(3): lut[t,m,v] = cand[v % len(cand)]
open(os.path.join(OUT,'tileframe.bin'),'wb').write(lut.tobytes())
for t in range(1, 70):
    p = os.path.join(IMG, 'Walls', f'Wall_{t}.png')
    if t in USEDW and os.path.exists(p): savetex(Image.open(p), f'wall_{t}.tex')
wallcell = np.zeros((256,2),np.uint8)
for t in range(1,70):
    p=os.path.join(IMG,'Walls',f'Wall_{t}.png')
    if not os.path.exists(p): continue
    a=np.array(Image.open(p).convert('RGBA'));done=False
    for cy in range(a.shape[0]//36):
        for cx in range(a.shape[1]//36):
            c=a[cy*36+2:cy*36+34, cx*36+2:cx*36+34]
            if (c[...,3]==255).all(): wallcell[t]=(cx,cy);done=True;break
        if done:break
open(os.path.join(OUT,'wallcell.bin'),'wb').write(wallcell.tobytes())
# jogador composto com as cores padrao do Terraria
PB = os.path.join(IMG, 'Sprites', 'PlayerBase')
def tint(name, col):
    im = Image.open(os.path.join(PB, name)).convert('RGBA')
    if im.size[1] < 1118: return Image.new('RGBA',(40,1118),(0,0,0,0))
    im = im.crop((0,0,40,1118)); a = np.array(im).astype(np.float32)
    a[...,0] *= col[0]/255.; a[...,1] *= col[1]/255.; a[...,2] *= col[2]/255.
    return Image.fromarray(a.astype(np.uint8), 'RGBA')
skin=(255,125,90); hair=(215,90,55); eye=(105,90,75); shirt=(175,165,140); under=(160,180,215); pants=(255,230,175); shoes=(160,105,60)
base = Image.new('RGBA', (40,1118), (0,0,0,0))
for name, col in [('Player_Head.png',skin),('Player_Undershirt.png',under),('Player_Pants.png',pants),('Player_Shirt.png',shirt),
                  ('Player_Shoes.png',shoes),('Player_Hands.png',skin),('Player_Eye_Whites.png',(255,255,255)),('Player_Eyes.png',eye),('Player_Hair_1.png',hair)]:
    base = Image.alpha_composite(base, tint(name, col))
savetex(base, 'player.tex')
for n in range(1, 40):
    p = os.path.join(IMG, 'Sprites', 'Items', f'Item_{n}.png')
    if os.path.exists(p): savetex(Image.open(p), f'item_{n}.tex')
for n in ('Inventory_Back.png','Inventory_Back3.png'):
    savetex(Image.open(os.path.join(ASSETS, n)), n.replace('.png','.tex').lower())
print('ok', ntypes, 'sheets')
