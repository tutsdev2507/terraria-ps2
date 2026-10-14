#!/usr/bin/env python3
"""Converte os .tex (TEX0,w,h,RGBA8) em um pacote unico CT16 ja fatiado em
chunks 128x128, para o backend PS2 ler 1 bloco contiguo por chunk (rapido).

Formato (little-endian):
  magic  "P2TEX1\\0\\0" (8)
  u32    count
  count x { char name[64]; u16 w,h,cw,ch,ncx,ncy; u32 dataoff; u32 chunksize; }  (84 bytes)
  dados  chunks CT16 (A1B5G5R5), row-major, cada chunk cw*ch*2 bytes (padding transparente)

Uso: python3 tools/make_ps2pak.py <data_dir> <saida.pak>
"""
import os, sys, struct, glob

CH = 128

def build(data_dir, out_path):
    files = sorted(glob.glob(os.path.join(data_dir, "*.tex")))
    entries = []
    blobs = []
    dataoff = 12
    for fp in files:
        name = os.path.basename(fp)
        raw = open(fp, "rb").read()
        if len(raw) < 8 or raw[:4] != b"TEX0":
            continue
        w, h = struct.unpack_from("<HH", raw, 4)
        px = raw[8:]
        cw = ch = CH
        ncx = (w + CH - 1) // CH
        ncy = (h + CH - 1) // CH
        chunksize = cw * ch * 2
        out = bytearray()
        for cy in range(ncy):
            for cx in range(ncx):
                buf = bytearray(cw * ch * 2)
                for r in range(ch):
                    y = cy * CH + r
                    if y >= h:
                        break
                    o = (y * w + cx * CH) * 4
                    bo = r * cw * 2
                    last = min(cw, w - cx * CH)
                    for i in range(last):
                        rr = px[o]; gg = px[o + 1]; bb = px[o + 2]; aa = px[o + 3]
                        o += 4
                        if aa >= 128:
                            v = 0x8000 | ((bb >> 3) << 10) | ((gg >> 3) << 5) | (rr >> 3)
                        else:
                            v = 0
                        bo2 = bo + i * 2
                        buf[bo2] = v & 0xFF
                        buf[bo2 + 1] = (v >> 8) & 0xFF
                out += buf
        entries.append((name, w, h, cw, ch, ncx, ncy, dataoff, chunksize))
        blobs.append(out)
        dataoff += len(out)
    with open(out_path, "wb") as f:
        f.write(b"P2TEX1\x00\x00")
        f.write(struct.pack("<I", len(entries)))
        for (name, w, h, cw, ch, ncx, ncy, off, cs) in entries:
            nb = name.encode()[:63]
            f.write(nb + b"\x00" * (64 - len(nb)))
            f.write(struct.pack("<HHHHHHII", w, h, cw, ch, ncx, ncy, off, cs))
        for b in blobs:
            f.write(b)
    total = sum(len(b) for b in blobs)
    print(f"ok: {len(entries)} texturas, {total} bytes de chunks -> {out_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(__doc__); sys.exit(1)
    build(sys.argv[1], sys.argv[2])
