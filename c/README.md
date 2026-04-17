# RISC-V Programs

Programs compiled with the RV32IM toolchain and running on RISC-V emulated on UNIVAC-1219.

## Structure

Each program lives in its own subdirectory with a minimal Makefile that includes the shared build rules:

```makefile
EXTRA_CFLAGS = -Os

include ../../toolchain/common.mk
```

Optional variables:
- `PROGRAM_SRCS` - Program source files (default: `main.c`; `start.c` and `syscalls.c` are auto-included)
- `EXTRA_CFLAGS` - Compiler flags (default: `-Os`)
- `PRINTF_VARIANT` - Use `__d_vfprintf` for float support (default: `__i_vfprintf` integer-only)

## Building

```bash
cd c/smolnes
make            # Build binary and UNIVAC .76 file
make ocaml-run  # Run in OCaml RISC-V emulator
make run        # Run on UNIVAC emulator
make run-realtime # Run on UNIVAC emulator in realtime mode
make info       # Show binary size info
make disasm     # Show disassembly
make clean      # Clean build artifacts
```

## Creating New Programs

1. Create a new directory: `mkdir c/myprogram`
2. Add your C source file: `main.c`
3. Create a minimal Makefile (see fizzbuzz as an example)
4. Run `make`

All build artifacts will be in `build/`.
