#include<stdio.h>

int main() {
  unsigned int x = 1;
  char *c = (char*) &x;
  printf("%d\n", (int)*c);

  if((int)*c == 1) {
    printf("Little endian\n");
  } else {
    printf("Big endian\n");
  }
  return 0;
}
