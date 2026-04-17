# RISC-V on UNIVAC-1219 [![CI](https://github.com/nathanfarlow/univac-1219-riscv/actions/workflows/ci.yml/badge.svg)](https://github.com/nathanfarlow/univac-1219-riscv/actions/workflows/ci.yml)

Run modern 32-bit RISC-V programs on the UNIVAC 1219, a 1960s 18-bit computer. [Blog post](https://farlow.dev/2026/04/17/running-a-minecraft-server-and-more-on-a-1960s-univac-computer.html)

## Quick Start (Linux/Windows/macOS)

Install Docker. Then:

```bash
git clone --recursive https://github.com/nathanfarlow/univac-1219-riscv
cd univac-1219-riscv

# Might take a while!
docker build -t riscv-univac .

# Run a C program
docker run --rm -it -v "$(pwd):/workspace" -w /workspace riscv-univac bash -l -c "make -C c/hello run"

# Run the tests (might take a while!)
docker run --rm -it -v "$(pwd):/workspace" -w /workspace riscv-univac bash -l -c "./run-tests.sh"
```

## Repository Guide

- [`asm/riscv-emu/`](asm/riscv-emu/) - RISC-V emulator written in UNIVAC-1219 assembly
- [`asm/overstrike/`](asm/overstrike/) - Teletype overstrike art generator
- [`c/`](c/) - Example C programs (Minecraft server, NES emulator, games, BASIC, etc)
- [`tools/`](tools/) - OCaml RISC-V parser & emulator, fuzzer, test case reducer, and code generation tools
- [`toolchain/`](toolchain/) - RISC-V toolchain configuration and build scripts
- [`editors/`](editors/) - Emacs major mode and VS Code extension
- [`univac-1219-emu/`](univac-1219-emu/) - UNIVAC-1219 emulator

## License

[MIT licensed](LICENSE), except where individual files or directories contain their own license notice.
