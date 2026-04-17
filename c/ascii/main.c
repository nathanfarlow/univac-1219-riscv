#include <stdio.h>
#include <syscalls.h>
#include <stdint.h>

static inline void raw_putc(uint8_t b) {
  SYSCALL2(SYS_PUTC, b, 0);
}

int main(void) {
  for (int i = 0; i < 128; i++) {
    printf("%3d %03o ", i, i);
    /* if (i <= 13 || i == 127) */
    /*   putchar('.'); */
    /* else */
      raw_putc(i);
    if (i % 4 == 3)
      putchar('\n');
    else
      printf("  ");
  }
}
