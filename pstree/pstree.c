#include <stdio.h>
#include <assert.h>

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  // process parameter;
  // get pid list;
  // construct tree, find the root of each pid, and construt the tree;
  // print the pid tree;

  return 0;
}
