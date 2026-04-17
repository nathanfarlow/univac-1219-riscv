#include <stdio.h>
#include <string.h>

int main(void) {
  char buf[256];
  for (;;) {
    printf("> ");
    if (!fgets(buf, sizeof(buf), stdin))
      return 0;
    buf[strcspn(buf, "\r\n")] = 0;
    if (!buf[0])
      continue;
    if (buf[0] == 'Q' && buf[1] == 0)
      return 0;
    int len = strlen(buf);
    printf("\n ");
    for (int i = 0; i < len + 2; i++)
      putchar('=');
    printf("\n< %s >\n ", buf);
    for (int i = 0; i < len + 2; i++)
      putchar('=');
    printf("\n        \\   \\==/\n"
           "         \\  (OO)\\=======\n"
           "            (==)\\       )\\/\\\n"
           "                !!----W !\n"
           "                !!     !!\n\n");
  }
}
