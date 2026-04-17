#include "mini_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MINIRV32_RAM_IMAGE_OFFSET 0x80000000

#define MINI_RV32_RAM_SIZE (64 * 1024 * 1024)
#define MINIRV32_IMPLEMENTATION
#define MINIRV32_POSTEXEC(pc, ir, retval)                                      \
  do {                                                                         \
    (void)(pc);                                                                \
    (void)(ir);                                                                \
    (void)(retval);                                                            \
  } while (0)
#define MINIRV32_HANDLE_MEM_STORE_CONTROL(addy, val)                           \
  do {                                                                         \
    (void)(addy);                                                              \
    (void)(val);                                                               \
  } while (0)
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL(addy, val)                            \
  do {                                                                         \
    (void)(addy);                                                              \
    (void)(val);                                                               \
  } while (0)
#define MINIRV32_OTHERCSR_WRITE(csrno, value)                                  \
  do {                                                                         \
    (void)(csrno);                                                             \
    (void)(value);                                                             \
  } while (0)
#define MINIRV32_OTHERCSR_READ(csrno, value)                                   \
  do {                                                                         \
    (void)(csrno);                                                             \
    (void)(value);                                                             \
  } while (0)

#include "mini-rv32ima.h"

struct EmulatorResult *execute_binary_file(const char *file_path,
                                           int max_instructions) {
  struct EmulatorResult *result = malloc(sizeof(struct EmulatorResult));
  if (!result)
    return NULL;
  memset(result, 0, sizeof(struct EmulatorResult));

  FILE *file = fopen(file_path, "rb");
  if (!file)
    return result;

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size <= 0 || file_size > MINI_RV32_RAM_SIZE) {
    fclose(file);
    return result;
  }

  uint8_t *image = malloc(MINI_RV32_RAM_SIZE);
  if (!image) {
    fclose(file);
    return result;
  }
  memset(image, 0, MINI_RV32_RAM_SIZE);

  if (fread(image, 1, file_size, file) != (size_t)file_size) {
    fclose(file);
    free(image);
    return result;
  }
  fclose(file);

  struct MiniRV32IMAState cpu_state = {.pc = MINIRV32_RAM_IMAGE_OFFSET};
  for (int i = 0; i < max_instructions; i++) {
    if (MiniRV32IMAStep(&cpu_state, image, MINIRV32_RAM_IMAGE_OFFSET, 0, 1) < 0)
      break;
  }

  memcpy(result->regs, cpu_state.regs, sizeof(result->regs));

  free(image);
  return result;
}

void free_emulator_result(struct EmulatorResult *result) { free(result); }
