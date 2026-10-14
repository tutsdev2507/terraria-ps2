#!/usr/bin/env python3
"""Sonda o cabecalho de um .world do Terraria Mobile (versao 49). Uso: inspect_world.py arquivo.world"""
import struct, sys
d = open(sys.argv[1], 'rb').read()
ver, flags, nlen = struct.unpack_from('<iii', d, 0)
name = d[12:12+nlen].decode('latin1'); o = 12 + nlen
wid, = struct.unpack_from('<I', d, o); o += 4
print(f"versao={ver} flags={flags} nome={name!r} worldID=0x{wid:08x} offset_pos_id={o}")
print("int16 seguintes:", struct.unpack_from('<20h', d, o))
print("int32 seguintes:", struct.unpack_from('<6i', d, o))
