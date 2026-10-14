#!/usr/bin/env python3
"""Extrai Images.zip (caminhos com '\\') para build/images/ preservando estrutura e gera inventario."""
import zipfile, os, sys, io
from PIL import Image
src = sys.argv[1] if len(sys.argv) > 1 else 'TerrariaAssets/Images.zip'
out = sys.argv[2] if len(sys.argv) > 2 else 'build/images'
os.makedirs(out, exist_ok=True)
inv = open(os.path.join(os.path.dirname(out) or '.', 'inventory_images.tsv'), 'w')
z = zipfile.ZipFile(src)
for n in z.namelist():
    p = n.replace('\\', '/').lstrip('/')
    if not p or p.endswith('/'): continue
    dst = os.path.join(out, p); os.makedirs(os.path.dirname(dst), exist_ok=True)
    data = z.read(n); open(dst, 'wb').write(data)
    try:
        im = Image.open(io.BytesIO(data)); inv.write(f"{p}\t{im.size[0]}x{im.size[1]}\t{im.mode}\n")
    except Exception as e:
        inv.write(f"{p}\t?\t{e}\n")
