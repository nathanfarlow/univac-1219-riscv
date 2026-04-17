CC = riscv64-unknown-elf-gcc
OBJCOPY = riscv64-unknown-elf-objcopy
OBJDUMP = riscv64-unknown-elf-objdump
SIZE = riscv64-unknown-elf-size
TOOLCHAIN_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

ARCH = -march=rv32i_zmmul -mabi=ilp32 -mcmodel=medlow
FLAGS = -nostartfiles -ffunction-sections -fdata-sections -no-pie -static --specs=picolibc.specs
# Base compiler flags always applied
BASE_CFLAGS = -DUNIVAC -flto -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer -I$(TOOLCHAIN_DIR)
# Individual programs can set EXTRA_CFLAGS to add optimization and other flags (e.g., -Os, -O2, -g)
EXTRA_CFLAGS ?= -Os
CFLAGS = $(BASE_CFLAGS) $(EXTRA_CFLAGS)
# Use __i_vfprintf (integer-only) by default, or __d_vfprintf for float support
PRINTF_VARIANT ?= __i_vfprintf
LDFLAGS = -T $(TOOLCHAIN_DIR)/linker.ld -Wl,--gc-sections,--strip-debug,--defsym=vfprintf=$(PRINTF_VARIANT) -flto

PROGRAM ?= $(notdir $(CURDIR))
BUILD = build
TOOLCHAIN_SRCS = start.c syscalls.c
PROGRAM_SRCS ?= main.c
SRCS = $(PROGRAM_SRCS) $(TOOLCHAIN_SRCS)
OBJS = $(SRCS:%.c=$(BUILD)/%.o)
ELF = $(BUILD)/$(PROGRAM).elf
BIN = $(BUILD)/$(PROGRAM).bin
PROGRAM76 = $(BUILD)/$(PROGRAM).76
PROGRAM76BIN = $(BUILD)/$(PROGRAM).76.bin
UNIVAC_DIR = $(TOOLCHAIN_DIR)/../asm/riscv-emu
UNIVAC_EMU_DIR = $(TOOLCHAIN_DIR)/../univac-1219-emu
UNIVAC_ASSEMBLER = $(UNIVAC_EMU_DIR)/target/release/assembler
UNIVAC_EMULATOR = $(UNIVAC_EMU_DIR)/target/release/emulator
EMU_FLAGS ?=
TOOLS_DIR = $(TOOLCHAIN_DIR)/../tools
DUNE_BUILD = $(TOOLS_DIR)/_build/default/bin/main.exe

all: $(BIN) $(PROGRAM76) $(PROGRAM76BIN)

# Ensure dune project is built
$(DUNE_BUILD):
	@cd $(TOOLS_DIR) && dune build

$(BUILD):
	@mkdir -p $@

$(BUILD)/%.o: %.c | $(BUILD)
	@$(CC) $(ARCH) $(FLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: %.s | $(BUILD)
	@$(CC) $(ARCH) -c -o $@ $<

$(BUILD)/%.o: $(TOOLCHAIN_DIR)/%.c | $(BUILD)
	@$(CC) $(ARCH) $(FLAGS) $(CFLAGS) -c -o $@ $<

$(ELF): $(OBJS) | $(BUILD)
	@$(CC) $(ARCH) $(FLAGS) $(LDFLAGS) -o $@ $(OBJS)

$(BIN): $(ELF)
	@$(OBJCOPY) -O binary $< $@

$(UNIVAC_ASSEMBLER) $(UNIVAC_EMULATOR):
	@cd $(UNIVAC_EMU_DIR) && cargo build --quiet --release

$(PROGRAM76): $(DUNE_BUILD) $(BIN) $(UNIVAC_ASSEMBLER) | $(BUILD)
	@$(SIZE) -A $(ELF) | grep '^\.text' | awk '{print $$2}' > $(BUILD)/.text_size
	@cd $(TOOLS_DIR) && dune exec -- tools univac-embed \
		$(CURDIR)/$(BIN) -n $$(expr $$(cat $(CURDIR)/$(BUILD)/.text_size) / 4) \
		-o $(CURDIR)/$(BUILD)/program.univac.pp
	@$(UNIVAC_ASSEMBLER) $(BUILD)/program.univac.pp -o $@
	@perl -pi -e 's/$$/\r/' $@

$(PROGRAM76BIN): $(PROGRAM76) $(DUNE_BUILD) | $(BUILD)
	@cd $(TOOLS_DIR) && dune exec -- tools tape $(CURDIR)/$(PROGRAM76) -o $(CURDIR)/$@

ocaml-run: $(DUNE_BUILD) $(BIN)
	@cd $(TOOLS_DIR) && ./_build/default/bin/main.exe run $(CURDIR)/$(BIN)

run: $(PROGRAM76) $(UNIVAC_EMULATOR)
	@cd $(UNIVAC_EMU_DIR) && ./target/release/emulator --stop 0 $(EMU_FLAGS) --tape-file $(CURDIR)/$(PROGRAM76)

run-realtime: $(PROGRAM76) $(UNIVAC_EMULATOR)
	@cd $(UNIVAC_EMU_DIR) && ./target/release/emulator --stop 0 $(EMU_FLAGS) --tape-file $(CURDIR)/$(PROGRAM76) --realtime

disasm: $(ELF)
	@$(OBJDUMP) -d $<

info: $(ELF) $(BIN) $(PROGRAM76)
	@$(SIZE) $(ELF)
	@echo "Binary: $$(wc -c < $(BIN)) bytes"
	@echo "UNIVAC tape: $$(echo "$$(wc -l < $(PROGRAM76)) / 3" | bc) words"

clean:
	@rm -rf $(BUILD)

.PHONY: all run run-realtime univac-run disasm info clean
