# Set ART_FILE before including. E.g.: ART_FILE = foo.txt
ART_FILE ?= art.txt
ASCII_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BUILD = build
SRCS = $(BUILD)/art.univac

$(BUILD)/art.univac: $(ART_FILE) $(DUNE_BUILD) | $(BUILD)
	cd $(TOOLS_DIR) && dune exec -- tools ascii-art $(abspath $<) > $(abspath $@)

include $(ASCII_DIR)../common.mk
