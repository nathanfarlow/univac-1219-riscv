#include <stdio.h>
#include <math.h>

int main(void) {
  double x;
  printf("ENTER A NUMBER: ");
  scanf("%lf", &x);
  printf("SIN(%.15f) = %.15f\n", x, sin(x));
}
