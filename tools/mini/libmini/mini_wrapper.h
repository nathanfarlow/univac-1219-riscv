#pragma once
#include <stdint.h>

struct EmulatorResult {
  uint32_t regs[32];
};

struct EmulatorResult *execute_binary_file(const char *file_path,
                                           int max_instructions);
void free_emulator_result(struct EmulatorResult *result);
