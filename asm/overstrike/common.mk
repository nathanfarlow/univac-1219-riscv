# Set before including:
#   IMAGE           - source image file (e.g. me.png)
#   OVERSTRIKE_ARGS - overstrike.py flags (--rows, --cols, --gamma, etc.)

OVERSTRIKE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BUILD = build
SRCS = $(BUILD)/art.univac

.DEFAULT_GOAL := all

$(BUILD)/art.txt: $(IMAGE) $(OVERSTRIKE_DIR)/overstrike.py $(OVERSTRIKE_DIR)/TTY35TD-Book.ttf | $(BUILD)
	python3 $(OVERSTRIKE_DIR)/overstrike.py $(IMAGE) $(OVERSTRIKE_ARGS) \
		--font $(OVERSTRIKE_DIR)/TTY35TD-Book.ttf \
		--output $@ --preview $(BUILD)/preview.png

$(BUILD)/art.univac: $(BUILD)/art.txt $(DUNE_BUILD) | $(BUILD)
	cd $(TOOLS_DIR) && dune exec -- tools ascii-art -raw $(abspath $<) > $(abspath $@)

include $(OVERSTRIKE_DIR)../common.mk
