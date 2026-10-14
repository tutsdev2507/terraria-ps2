# PORT_STATUS
Nucleo C99 unico (src/game.c) + camada pal.h; backends: web/Android(Termux), headless, PS2.
Executado (host Linux): conversor, pal_test (600 ticks, PPM verificados visualmente), servidor web (GET / e /f respondem). Mundo real do Tutorial.world renderizado com tiles/jogador/itens originais; andar, pular, colisao, step-up, minerar/colocar, hotbar OK em teste roteirizado.
Compilado/validado no PS2: NAO (sem PS2SDK). pal_ps2.c nao verificado; VRAM: 50 folhas CT32 nao cabem em 4MB -> falta LRU/CT16.
Testar no Android: Termux -> `pkg install clang make` ; `sh run_android.sh` ; abrir http://localhost:8080 (toque = cursor/usar, botoes virtuais). Tambem funciona em PC.
Regenerar dados: `python3 tools/extract_assets.py TerrariaAssets/Images.zip img && python3 tools/convert_all.py TerrariaAssets img data`
Falta: secoes pos-tile do .world (baus/NPCs), iluminacao, dia/noite, liquidos dinamicos, combate/IA, fonte/UI/menus, audio (wav->VAG), saves, hair, merges de tiles, geracao procedural, otimizacao PS2.
Proximo: decodificar secoes apos offset 197946 (baus, placas, NPCs); LRU de texturas CT16 no PS2; iluminacao.
