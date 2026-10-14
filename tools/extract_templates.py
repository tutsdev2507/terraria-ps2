#!/usr/bin/env python3
"""Extrai templates de objetos do mundo de referencia (data/tutorial.tw) para src/templates.inc.

Motivo: arvores/plantas/baus usam frames (fx,fy) uteis apenas para tipos frameImportant;
copiando celulas EXATAS do tutorial (mesmo APK) garantimos fidelidade de sprite no PS2.
Uso: extract_templates.py <data/tutorial.tw> <src/templates.inc>
"""
import struct, sys

src, dst = sys.argv[1], sys.argv[2]
d = open(src, 'rb').read()
assert d[:4] == b'TW01', 'nao e TW01'
W, H, X0, Y0, SPX, _ = struct.unpack('<HHHHHH', d[4:16])
N = W * H
T = bytearray(N); FX = [0]*N; FY = [0]*N
for i in range(N):
    o = 16 + i*8
    T[i] = d[o]; FX[i] = d[o+4] | d[o+5] << 8; FY[i] = d[o+6] | d[o+7] << 8

def at(x, y): return T[x*H + y]

def components(t, diag=True):
    seen = bytearray(N); comps = []
    nb = [(-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)] if diag else [(0,1),(0,-1),(1,0),(-1,0)]
    for x in range(W):
        for y in range(H):
            if at(x, y) != t or seen[x*H+y]: continue
            q = [(x, y)]; seen[x*H+y] = 1; cells = []
            while q:
                cx, cy = q.pop(); cells.append((cx, cy))
                for dx, dy in nb:
                    nx, ny = cx+dx, cy+dy
                    if 0 <= nx < W and 0 <= ny < H and at(nx, ny) == t and not seen[nx*H+ny]:
                        seen[nx*H+ny] = 1; q.append((nx, ny))
            comps.append(cells)
    return comps

def bbox(cells):
    xs = [c[0] for c in cells]; ys = [c[1] for c in cells]
    return min(xs), min(ys), max(xs), max(ys)

# --- arvores (tipo 5) ---
trees = []
for cells in components(5):
    x0, y0, x1, y1 = bbox(cells)
    bw, bh = x1-x0+1, y1-y0+1
    if bw > 4 or bh > 18 or len(cells) < 8: continue
    # celula de base deve ter terra/grama embaixo (grafia do tutorial)
    base_ok = all((at(cx, cy+1) in (0, 2)) for cx, cy in cells if cy == y1)
    if not base_ok: continue
    trees.append((len(cells), bw, bh, x0, y1, sorted((cx-x0, cy-y1, FX[cx*H+cy], FY[cx*H+cy]) for cx, cy in cells)))
trees.sort(key=lambda t: t[0])
# escolhe 3 tamanhos distintos (pequena/media/grande)
picks = []
for idx in (0, len(trees)//2, len(trees)-1):
    if idx not in [p[0] for p in picks]:
        picks.append((idx, trees[idx]))
picks = [p[1] for p in picks]
# deduplica por conteudo
uniq = []
for t in picks:
    if not any(t[5:] == u[5:] for u in uniq): uniq.append(t)
picks = uniq
print('arvores: %d candidatas, escolhidas %d: %s' % (len(trees), len(picks),
      [(p[0], p[1], p[2]) for p in picks]))

# --- plantas 1x1 (tipo 3) ---
frames = {}
for cells in components(3, diag=False):
    if len(cells) != 1: continue
    cx, cy = cells[0]
    if at(cx, cy+1) not in (0, 2, 59, 60): continue   # precisa de chao embaixo
    f = (FX[cx*H+cy], FY[cx*H+cy])
    frames[f] = frames.get(f, 0) + 1
plants = sorted(frames.items(), key=lambda kv: -kv[1])
print('plantas 1x1: frames distintos %d -> %s' % (len(plants), plants))

# --- bau 2x2 (tipo 21) ---
chests = []
for cells in components(21):
    x0, y0, x1, y1 = bbox(cells)
    if len(cells) != 4 or (x1-x0) != 1 or (y1-y0) != 1: continue
    # chao solido embaixo
    if not (at(x0, y1+1) != 255 and at(x0+1, y1+1) != 255): continue
    chests.append(sorted((cx-x0, cy-y0, FX[cx*H+cy], FY[cx*H+cy]) for cx, cy in cells))
if not chests:
    print('AVISO: nenhum bau 2x2 valido')
else:
    print('baus: %d candidatos; usarei o 1o: %s' % (len(chests), chests[0]))

# --- emite C ---
def emit_cells(name, cells):
    body = ', '.join('{%d,%d,%d,%d}' % c for c in cells)
    return 'static const WTplCell %s[] = {%s};\n' % (name, body)

out = []
out.append('/* Gerado por tools/extract_templates.py de data/tutorial.tw (mundo de referencia do APK Terraria mobile).\n'
           '   CELULAS COPIADAS LITERALMENTE: dx,dy relativos a origem (base da arvore = dy 0, topo do bau = dy 0),\n'
           '   fx,fy = offsets de sprite (frames important). NAO editar mao. */\n')
out.append('typedef struct { signed char dx, dy; unsigned short fx, fy; } WTplCell;\n')
for i, t in enumerate(picks):
    out.append('/* arvore %d: %d celulas, bbox %dx%d */\n' % (i, t[0], t[1], t[2]))
    out.append(emit_cells('wg_tree%d' % i, t[5]))
out.append('static const WTplCell *const wg_trees[] = {%s};\n' % ', '.join('wg_tree%d' % i for i in range(len(picks))))
out.append('static const unsigned wg_tree_count = %d;\n' % len(picks))
out.append('static const unsigned wg_tree_sizes[] = {%s};\n' % ', '.join(str(len(t[5])) for t in picks))
out.append('static const unsigned short wg_plant_frames[][2] = {%s};\n' %
           ', '.join('{%d,%d}' % f for f, _ in plants))
out.append('static const unsigned wg_plant_count = %d;\n' % len(plants))
if chests:
    out.append('/* bau 2x2 (topo-esquerda = origem) */\n')
    out.append(emit_cells('wg_chest', chests[0]))
open(dst, 'w').write(''.join(out))
print('escreveu', dst)
