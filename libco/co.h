#include <stdio.h>

#ifdef LOCAL_MACHINE
#define debug(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define debug()
#endif

struct co* co_start(const char *name, void (*func)(void *), void *arg);
void co_yield();
void co_wait(struct co *co);
