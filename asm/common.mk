# Common Makefile for UNIVAC-1219 test programs
# Include from subdirectories: include ../common.mk

PROGRAM ?= $(notdir $(CURDIR))
SRCS ?= $(sort $(wildcard *.univac))

# Paths to tools
ASM_DIR = $(dir $(lastword $(MAKEFILE_LIST)))
UNIVAC_EMU_DIR = $(ASM_DIR)../univac-1219-emu
TOOLS_DIR = $(ASM_DIR)../tools
ASSEMBLER = $(UNIVAC_EMU_DIR)/target/release/assembler
EMULATOR = $(UNIVAC_EMU_DIR)/target/release/emulator
DUNE_BUILD = $(TOOLS_DIR)/_build/default/bin/main.exe

# Build directory
BUILD = build

# Output files
PP = $(BUILD)/$(PROGRAM).pp
TAPE = $(BUILD)/$(PROGRAM).76
BIN = $(BUILD)/$(PROGRAM).76.bin

.PHONY: all clean run run-verbose run-realtime assemble-verbose

all: $(TAPE) $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

# Build the assembler/emulator if needed
$(ASSEMBLER) $(EMULATOR):
	cargo build --release --manifest-path $(UNIVAC_EMU_DIR)/Cargo.toml

# Build dune tools if needed
$(DUNE_BUILD):
	cd $(TOOLS_DIR) && dune build

# Concatenate sources and preprocess (run OCaml template blocks)
$(BUILD)/combined.univac: $(SRCS) | $(BUILD)
	cat $(SRCS) > $@

$(PP): $(BUILD)/combined.univac $(DUNE_BUILD) | $(BUILD)
	cd $(TOOLS_DIR) && dune exec -- tools template $(abspath $<) > $(abspath $@)

# Assemble the program to tape format
$(TAPE): $(PP) $(ASSEMBLER) | $(BUILD)
	$(ASSEMBLER) $(PP) -o $@

# Convert tape to binary (exclude checksum)
$(BIN): $(TAPE) $(DUNE_BUILD) | $(BUILD)
	cd $(TOOLS_DIR) && dune exec -- tools tape $(abspath $(TAPE)) -o $(abspath $@)

# Run the program on the emulator
run: $(TAPE) $(EMULATOR)
	cd $(UNIVAC_EMU_DIR) && ./target/release/emulator --stop 0 --tape-file $(abspath $(TAPE))

# Run with verbose output (instruction trace)
run-verbose: $(TAPE) $(EMULATOR)
	cd $(UNIVAC_EMU_DIR) && ./target/release/emulator --stop 0 --tape-file $(abspath $(TAPE)) --verbose

# Run in realtime mode (matches original hardware speed)
run-realtime: $(TAPE) $(EMULATOR)
	cd $(UNIVAC_EMU_DIR) && ./target/release/emulator --stop 0 --tape-file $(abspath $(TAPE)) --realtime

# Assemble with verbose output (shows label table)
assemble-verbose: $(PP) $(ASSEMBLER) | $(BUILD)
	$(ASSEMBLER) $(PP) -o $(TAPE) --verbose

clean:
	rm -rf $(BUILD)
