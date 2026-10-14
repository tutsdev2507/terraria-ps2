#!/bin/sh
# Compila o ELF do PS2 usando a imagem Docker oficial (ps2sdk + gsKit prontos).
# Requer docker. Roda a partir da raiz do projeto.
#   sh tools/build_ps2.sh
# Saida: ./terraria.elf
set -e
docker run --rm -v "$PWD":/src -w /src ps2dev/ps2dev:latest make clean all
echo "OK: terraria.elf"
