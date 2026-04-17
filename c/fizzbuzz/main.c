#include <stdio.h>

int main(void) {
  for (int i = 1; i <= 20; i++) {
    printf("%d", i);
    if (i % 15 == 0) {
      printf(" FIZZBUZZ");
    } else if (i % 3 == 0) {
      printf(" FIZZ");
    } else if (i % 5 == 0) {
      printf(" BUZZ");
    }
    printf("\n");
  }

  return 0;
}
