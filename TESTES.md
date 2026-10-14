# TESTES
Executados nesta sessao (host Linux x86_64, gcc):
- make -f Makefile.host: compila sem erros/avisos. Resultado: OK.
- pal_test data: 600 ticks, 4 capturas PPM; render apos mudanca de pal.h identico (cmp) ao anterior. OK.
- texcache_test: LRU (hit/miss/eviction), working set de 6 folhas em 10 slots = 6 misses em 300 quadros, conversao CT16. OK.
- chunk_test: 1308 casos de recorte em chunks 256 (inclui texturas 144x2916 e 1836 de largura, flipx) vs amostragem direta. 0 falhas.
- worldgen_test: mundos 160x96 (seed 1), 320x200 (42, 999) e 1750x1000 (12345). Verifica determinismo (mesma seed == mundo igual; seed+1 != igual), spawn valido e desobstruido, tipos/paredes validos, variacao de terreno (smax-smin >= h/20), 3-60% de ar subterraneo, presenca de terra/pedra/grama/areia/lama/jselva/4 minerios/agua/lava/arvores/plantas/baus e **oceanos limpos** (areia+agua nas 2 bordas, sem lama/jselva invadindo). Mundo real 1750x1000: 0,032s/geracao; superficie y 72..355; ar subterraneo 13%; agua 17165, lava 1359; arvores 608, plantas 210, bau 32; Fe 11698 Cu 13717 Au 8047 Ag 9757. OK.
- light_test: teste sintetico (sky, superficie clara, interior escuro, lava emissora, determinismo, relight vs recompute global) e teste com mundo gerado (320x200 seed 42). relight vs global maxdiff 0 em caso sintetico. `light_recompute` 1750x1000 ~ 0,081s, relight ~ 0,0002s. OK.
- pal_test <data>: sem world.tw/seed.txt em data/ -> "worldgen: arquivos ausentes, gerando mundo procedural"; 600 ticks, 4 PPMs. Render agora com iluminacao por tile aplicada a paredes/tiles/liquidos; conferido por ASCII. OK.
- wgdump + tools/world_ascii.py: dump binario do mundo real e inspecao textual; usados para achar/corrigir a invasao da selva no oceano.
Pendentes: Termux ARM64 (compilar/rodar), PS2SDK (compilar pal_ps2.c), execucao em PS2/PCSX2, medicao de desempenho.

## Nota de ambiente (Termux/Android)
`/storage/emulated/0` e montado **noexec**: binarios nao executam de dentro do repo.
Compilar/rodar em diretorio executavel, ex. `/data/data/com.termux/files/tmp/opencode/`
(e `chmod +x` apos copiar para la). `make -f Makefile.host` compila no repo, mas os
alvos de teste devem ser executados a partir de um diretorio executavel.
