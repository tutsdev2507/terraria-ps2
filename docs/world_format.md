# Tutorial.world (versao 49) - CONFIRMADO por parse completo + render
- Cabecalho: i32 ver=49, i32 0, string(i32 len), u32 worldID, campos (1750x1000 tiles) ... ate offset 92 (restante nao decodificado).
- Tiles a partir do offset 92, coluna-maior (x=0..1749), cada coluna = segmentos ate somar 1000:
  u16 cnt (run = cnt+1) | u8 ativo | [u8 tipo | (4B frameX,frameY u16 se frameImportant)] | u8 parede | u8 liquido | [u8 tipoLiquido se liquido>0] | u8 flags
- REGRA do 1o segmento da coluna: vale (cnt) tiles = (cnt) vazios + 1 tile (o do payload) no final (ex.: arvore/planta sobre a grama).
- Ultimo segmento da ultima coluna nao tem payload; apos offset 197946 vem marcador u32 5678 e secoes nao decodificadas (baus/placas/NPCs ~1.3MB: pendente).
- frameImportant aprendido por busca (tools/convert_all.py IMP). Mundo util: x 532..1079 (547x450); fora disso e preenchimento (lava, obsidiana).
- Incerto: significado de flags(=16 em 650 tiles), liquido tipo 1=lava, coluna de listras altas em x~735-780 (possivel 1o run com formato diferente).
