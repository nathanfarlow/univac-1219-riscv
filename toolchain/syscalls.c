#include "syscalls.h"
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

// Default I/O channel for stdin/stdout/stderr (UNIVAC channel 0)
int __io_channel = 0;

static int putc_impl(char c, FILE *f) {
  (void)f;
  if (c == '\n') {
    SYSCALL2(SYS_PUTC, '\r', __io_channel);
    SYSCALL2(SYS_PUTC, '\n', __io_channel);
    return 0;
  }

  if (c > 94) {
    SYSCALL2(SYS_PUTC, '?', __io_channel);
    return 0;
  }

  SYSCALL2(SYS_PUTC, c, __io_channel);
  return 0;
}

static int getc_impl(FILE *f) {
  (void)f;
  int result;

  do {
    result = SYSCALL(SYS_GETC, __io_channel);
  } while (result == '\r');

  return result;
}

static FILE __stdin =
    FDEV_SETUP_STREAM(NULL, getc_impl, NULL, _FDEV_SETUP_READ);
static FILE __stdout =
    FDEV_SETUP_STREAM(putc_impl, NULL, NULL, _FDEV_SETUP_WRITE);

__attribute__((used, externally_visible))
FILE *const stdin = &__stdin,
            *const stdout = &__stdout, *const stderr = &__stdout;

extern char __heap_start;

void _exit(int s) {
  SYSCALL(SYS_EXIT, s);
  __builtin_unreachable();
}
void *_sbrk(int n) {
  static char *h = &__heap_start;
  void *p = h;
  h += n;
  return p;
}
int _read(int fd, char *b, int n) {
  (void)fd; (void)b; (void)n;
  return -1;
}
int _write(int fd, char *b, int n) {
  (void)fd; (void)b; (void)n;
  return -1;
}
int _close(int fd) { return -1; }
int _lseek(int fd, int o, int w) { return -1; }
int _fstat(int fd, struct stat *s) {
  s->st_mode = S_IFCHR;
  return 0;
}
int _isatty(int fd) { return fd <= 2; }
int _kill(int p, int s) { return -1; }
int _getpid(void) { return 1; }
time_t time(time_t *t) {
  time_t result = SYSCALL(SYS_TIME, 0);
  if (t)
    *t = result;
  return result;
}
