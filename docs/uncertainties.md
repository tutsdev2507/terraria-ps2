# Incertezas / aproximacoes (nao confirmadas no binario)
- Fisica: constantes do Terraria PC 1.2 (grav .4, pulo 5.01x15, corrida 3). Mobile pode diferir.
- Autotile: tabela derivada por analise de bordas das folhas Tiles_N (sem merge entre tipos, sem cantos internos).
- Paredes: celula opaca 32x32 amostrada em quadrantes; sem selecao por vizinhos.
- Minerar: resistencia unica 100 e poder fixo; itens/drops/ids: tabela minima (ids = Item_N do APK).
- Cabelo do jogador omitido (Player_Hair_N tem 784px de altura, layout nao entendido). Arvores: quadro 20x20 aproximado.
- Sem iluminacao, dia/noite, NPCs, inimigos, saves ainda. Fonte original (softfont .dat) nao decodificada (digitos 3x5 provisorios).

## Geracao de mundo (src/worldgen.c)
- Constantes numericas sao aproximacoes de Terraria 1.2 (largura/altura de camadas,
  amplitudes de ruido, faixas de minerio, contagens), NAO extraidas do binario.
  A ordem dos passes sim tem evidencia (docs/worldgen_evidencia.md).
- IDs de minerios 6 (ferro), 7 (cobre), 8 (ouro), 9 (prata) inferidos por cor de
  sprite e por `game.c`; ordem ferro/cobre pode estar trocada.
- Paredes de caverna limitadas a 2 (terra) e 27 (pedra) - unicas com textura de
  fundo (`data/wall_*.tex`). Demais paredes do jogo nao possuem textura no port.
- Bioma selva: um lado, recuado da faixa dos oceanos (regressao coberta pelo
  `worldgen_test`: lama/jselva nao podem aparecer na faixa de oceano).
- Ilhas flutuantes: forma eliptica aproximada + cavidade interna com parede de
  terra; numero/tamanho/altura sao heuristicas.
- Liquidos: inundacao estatica por flood-fill confinado; nao ha simulacao
  dinamica (agua corrente, drenagem, mistura agua+lava).
- Iluminacao (src/light.c): modelo simplificado (dia fixo, sem ciclo dia/noite,
  sem tochas). Ceu (LG_SKY=255) nos tiles expostos ao ceu; lava emite
  LG_LAVA=190; propagacao por relaxamento em varreduras (3 iteracoes fwd+bwd)
  com atenuacao por tipo: ar=16, liquido=28, solido=42, folhagem=20; piso
  de brilho LG_AMBIENT=24 no render para evitar preto absoluto. 
  `light_recompute` global (chamado ao carregar/gerar mundo) e `light_relight`
  regional apos edicoes (com margem 20) — aproximacoes (pode haver pequenos
  desvios frente a Terraria 1.2 em cavernas profundas/afuniladas).
- Ausentes: corrupcao/crimson, dungeon, templo, gelo/neve, arvores vivas,
  piramide, colmeia, shroom patch, casas/mine houses, potes, SmoothWorld,
  AddHellHouses, Lakinater. Ver docs/worldgen_evidencia.md.
- Determinismo: a mesma semente produz o mesmo mundo (coberto por teste).
