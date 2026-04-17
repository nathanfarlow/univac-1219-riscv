#include <stdio.h>
#include <syscalls.h>

#define BUFSIZE 1024

int main(void) {
  __io_channel = 7; // Channel for serial IO

  char line[BUFSIZE];
  while (1) {
    printf("HELLO\n");

    int i = 0, c;
    while (i < BUFSIZE - 1 && (c = getchar()) != '\r' && c != '\n')
      line[i++] = c;
    line[i] = 0;

    printf("YOU SAID: %s\n", line);
  }
}
