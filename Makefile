# PS2 (gsKit/ps2sdk). Ver BUILD.md.
# Ambiente pronto: export PS2DEV=/usr/local/ps2dev PS2SDK=$PS2DEV/ps2sdk ; make
# O diretorio de dados vai como DATA_ROOT (padrao host:data = ps2link/pcsx2).
EE_BIN = terraria.elf
EE_OBJS = src/game.o src/worldgen.o src/light.o src/texcache.o src/pal_ps2.o
EE_INCS += -I$(PS2DEV)/gsKit/include -Isrc
EE_LDFLAGS += -L$(PS2DEV)/gsKit/lib
EE_LIBS = -lgskit -ldmakit -lpad -lm -lc -lkernel
# Otimizacoes para rodar liso no EE (R5900). Sem -ffast-math: preserva a
# determinismo da geracao/fisica (mesma sequencia de floats do host).
EE_CFLAGS += -O2 -funroll-loops
# Origem dos dados: host:data (ps2link/pcsx2) | mass:data (USB) | mc0:data (MC)
DATA_ROOT ?= host:data
EE_CFLAGS += -DDATA_ROOT='"$(DATA_ROOT)"'
all: $(EE_BIN)
clean:
	rm -f $(EE_BIN) $(EE_OBJS)
include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
