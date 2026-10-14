# ARQUITETURA
- Nucleo compartilhado: src/game.c (mundo, tiles, fisica, itens, render logico). So usa pal.h.
- PAL (src/pal.h): pal_tex_load, pal_rect, pal_blit, pal_file_*, pal_log; entrada via struct Input (pad ou toque).
- Backends: pal_web.c + sw_gfx.c (Android/Termux via navegador, rasterizador por software), pal_test.c + sw_gfx.c (headless, PPM), pal_ps2.c (gsKit/libpad).
- PS2 VRAM: framebuffer CT16 640x448 x2 + 16 slots de chunks 256x256 CT16 (texcache.c: LRU). Texturas = so metadados na RAM; pixels lidos do arquivo ao fazer upload.
- Dados: tools/*.py convertem Images.zip/Tutorial.world -> data/*.tex (TEX0,w,h,RGBA8) e tutorial.tw. Formato do mundo: docs/world_format.md. Incertezas: docs/uncertainties.md.
