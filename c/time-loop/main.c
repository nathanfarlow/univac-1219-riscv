#include <stdio.h>
#include <time.h>

int main(void)
{
	for (;;) {
      for (int i = 0; i < 10; i++) {
        printf("%ld ", (volatile long)time(0));
      }
      printf("\n");
	}
}
