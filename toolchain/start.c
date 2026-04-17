#include "syscalls.h"

extern int main(void);
extern void exit(int);
extern char __bss_start[], __bss_end[];

__attribute__((used)) void _start_c(void) {
  SYSCALL2(SYS_MEMCLEAR, __bss_start, __bss_end);
  exit(main());
}

__attribute__((section(".text.start"))) __attribute__((naked))
__attribute__((noreturn)) void
_start(void) {
  asm volatile("lui sp, 0x12\n"
               "j _start_c\n");
  __builtin_unreachable();
}
