# Assets (TerrariaAssets.zip, 918 entradas)
- Images.zip (7573 arquivos, caminhos com '\\'): Sprites 5233, Tiles 1475, NPC 470, Walls 171, Backgrounds 90, Ground 89, Clouds 24, Back 21.
- Tiles_N.png: 288x270 RGBA. Wall_N.png: 468x180.
- 161 .wav (Dig_*, Item_*, Door_*...), 272 .xml de UI (9-patch e menus), .dat de fontes (softfont*.dat, Tempesta_10px.dat), 10 *Localization.txt, Tutorial.world, TerrariaUI.pack, atlas.meta/dmeta, meta.data (indice nome->dimensoes de sprites), shaders .glsl/.hlsl (nao usados no PS2).
- Pendente: formato de TerrariaUI.pack, atlas.meta, softfont .dat; musica nao identificada (so SFX .wav listados ate agora).
- Conversao: tools/extract_assets.py (escrito). Proximo: png->PSMT8/CT16 (.tex) e wav->VAG/ADPCM.
