
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>

int main() {
  int n = 0;
  jmp_buf buf;
  n = setjmp(buf);
  printf("here is n: %d.\n", n);
  usleep(100);
  longjmp(buf, n + 1);
  return 0;
}