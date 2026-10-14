# BUILD

## Host / Termux (testado; PC e Android)
    make -f Makefile.host && make -f Makefile.host test
    sh run_android.sh          # abre servidor em http://localhost:8080
Regenerar dados:
    python3 tools/extract_assets.py TerrariaAssets/Images.zip img
    python3 tools/convert_all.py TerrariaAssets img data

## PS2 (gsKit + ps2sdk)
Precisa do toolchain. Nesta maquina (Termux/Android) ele NAO esta instalado e
nao existe pacote pronto; use uma das opcoes abaixo.

### A) GitHub Actions (nao precisa de toolchain local)
1. Crie um repositorio Git e envie a pasta `terraria_ps2/` como raiz.
2. O workflow `.github/workflows/ps2.yml` compila na imagem oficial
   `ps2dev/ps2dev` (ja traz ps2sdk + gsKit).
3. Na aba **Actions**, baixe o artefato `terraria-elf` -> `terraria.elf`.

### B) Docker local
    sh tools/build_ps2.sh        # docker run ps2dev/ps2dev:latest make clean all

### C) Toolchain instalado (PC/WSL)
    export PS2DEV=/usr/local/ps2dev
    export PS2SDK=$PS2DEV/ps2sdk
    export GSKIT=$PS2DEV/gsKit
    make                          # -> terraria.elf

### Rodando no PS2
O jogo carrega tudo por `DATA_ROOT`. O workflow gera 3 variantes:

| ELF                  | DATA_ROOT   | Quando usar                       |
|----------------------|-------------|-----------------------------------|
| `terraria-host.elf`  | `host:data` | ps2link/ps2client ou PCSX2        |
| `terraria-mass.elf`  | `mass:data` | pendrive USB (uLaunchELF/FMCB)    |
| `terraria-mc0.elf`   | `mc0:data`  | memory card                       |

**Pendrive USB (mais simples, sem PC):**
1. Copie `terraria-mass.elf` para a raiz do pendrive.
2. Copie a pasta `data/` (todos os `*.tex`, `tileframe.bin`, `wallcell.bin`)
   para `mass:/data/`.
3. No PS2 com FMCB, abra o **uLaunchELF**, selecione o `terraria-mass.elf` e
   execute. (O uLaunchELF monta o USB; se o jogo nao achar os arquivos, carregue
   `USBHDFSD`/`USBD` por um launch com IRX.)

**ps2link:** rode o `ps2client` numa pasta que contenha `data/` e envie
`terraria-host.elf`.

Ajuste a resolucao/regiao se necessario: o framebuffer e NTSC 640x448.

Observacoes de VRAM: framebuffer CT16 640x448 duplo (~1.1MB) + 16 chunks
256x256 CT16 (~2MB). O Z-buffer foi **desligado** (`gs->ZBuffering=0`) para
liberar ~0.6MB — o jogo e 2D.
