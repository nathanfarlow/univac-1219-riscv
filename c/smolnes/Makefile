# smolnes - NES emulator for RISC-V

# Optimized flags for UNIVAC emulator performance: 2.59% faster than -O3 baseline
EXTRA_CFLAGS = -O3 -funroll-loops --param max-inline-insns-auto=200
PROGRAM_SRCS = main.c sdl_stubs.c rom_data.c

include ../../toolchain/common.mk
