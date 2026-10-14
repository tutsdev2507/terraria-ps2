# Geracao de mundo: evidencia de engenharia reversa

Fonte: `libTerraria.so` (mobile 1.2, `armeabi`), ~5.66 MB, extraida de
`~/terraria_pre_historico/extracted/lib/armeabi/libTerraria.so`.

## Metodo
- Ferramentas: `nm` + rizin (desmontagem **Thumb**; o dump ARM generico
  `dumps gulosos/desassemblagem_terraria.txt` esta no modo errado e e inutilizavel).
- Simbolo alvo: `WorldGen::generateWorld()` @ `0x37b468`, tamanho `0xd670`
  (proximo simbolo @ `0x388ad8`). ELF com vaddr == paddr (base 0).
- Artefatos intermediarios (em temp, nao versionados): `gen_thumb.asm`
  (26014 linhas), `gen_calls.txt` (2715 chamadas BL mapeadas), `syms.py`.
- A ordem de passes abaixo foi obtida pela sequencia de chamadas BL dentro de
  `generateWorld` (nomes resolvidos por simbolo).

## Ordem de passes observada
```
resetGen -> randomBackgrounds -> setCaveBacks
Terreno:        TileRunner x5 -> PlaceTile -> Mountinater -> TileRunner x20
Biomas:         JungleRunner -> MudWallRunner x2 -> TileRunner x4 -> SpreadGrass
Limpeza:        CleanupWorld -> ShroomPatch -> ... -> Liquid.QuickWater
Estruturas:     AddHellHouses -> Lakinater -> MakeDungeon -> GenerateCrimsonBiome
Cavernas:       CaveOpenater -> Cavinator
Bordas:         CreateOcean -> CreateFloatingIslands -> GenerateCorruptionBiome
Minerios/rocha: DirtyRockRunner -> GrowLivingTree -> makeTemple -> IceHeart
Estruturas 2:   Pyramid -> MakeHive -> SmoothWorld
Populacao:      AddLifeCrystal / AddBuriedChest / ... -> AddTrees -> AddPlants
Final:          SquareTileFrame
```

## O que o port implementa hoje (src/worldgen.c)
Seguindo a ordem acima de forma simplificada:
1. `resetGen`
2. superficie (3 octavas de ruido) + `Mountinater`
3. `CreateOcean` (2 bordas, praia de areia + agua ate o nivel do mar)
4. camadas terra/pedra + regiao do inferno (bloco 58) no fundo
5. `JungleRunner`/`MudWallRunner` (selva de lama de um lado, **recuada do oceano**)
6. cavernas `TileRunner` (tuneis/bolhas/cavernas, `RUN_CARVE`)
7. minerios `OreRunner` em faixas de profundidade (cobre/ferro/prata/ouro)
8. `CreateFloatingIslands` (ilhas flutuantes com cavidade interna)
9. `QuickWater`/`WaterCheck` (inundacao estatica de agua/lava nas cavernas)
10. `CleanupWorld` (paredes de fundo 2/27 nas cavernas)
11. `SpreadGrass` (grama / grama de selva nas superficies expostas)
12. `AddTrees` / `AddPlants` / `AddBuriedChest` (celulas de `src/templates.inc`,
    copiadas literalmente do `data/tutorial.tw` para fidelidade de sprite)
13. spawn: escolhe coluna plana perto do centro, longe de agua/objetos

## Nao implementado (pendente, ver docs/uncertainties.md)
Corrupcao (Corruption/Crimson), Dungeon, Templo (`makeTemple`), gelo/neve
(`IceHeart`), arvores vivas (`GrowLivingTree`), Piramide, Colmeia (`MakeHive`),
`ShroomPatch`, casas/mine houses, potes, `SmoothWorld`, `AddHellHouses`,
`Lakinater`, `CaveOpenater`/`Cavinator` (usa uma aproximacao unica de cavernas),
simulacao dinamica de liquidos (apenas render do estado estatico).

## Inspecao sem imagens
`make -f Makefile.host wgdump` gera um dump binario (formato `WG01`) que pode ser
lido como texto com `python3 tools/world_ascii.py saida.bin`.
