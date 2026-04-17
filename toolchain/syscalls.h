#ifndef SYSCALLS_H
#define SYSCALLS_H

// Syscall numbers
#define SYS_GETC 0
#define SYS_PUTC 1
#define SYS_EXIT 2
#define SYS_MEMCLEAR 3
#define SYS_TIME 4

// Default I/O channel for stdin/stdout/stderr (set to change output destination)
extern int __io_channel;

// Syscall macro for single-argument syscalls
#define SYSCALL(n, x)                                                          \
  ({                                                                           \
    register long a0 asm("a0") = (long)(x);                                    \
    register long a7 asm("a7") = (n);                                          \
    asm volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");                     \
    a0;                                                                        \
  })

// Syscall macro for 2-argument syscalls
#define SYSCALL2(n, x, y)                                                      \
  ({                                                                           \
    register long a0 asm("a0") = (long)(x);                                    \
    register long a1 asm("a1") = (long)(y);                                    \
    register long a7 asm("a7") = (n);                                          \
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a7) : "memory");            \
    a0;                                                                        \
  })

#endif // SYSCALLS_H
