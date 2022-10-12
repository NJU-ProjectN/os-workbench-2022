
#include "co.h"
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STACK_SIZE 4096

enum co_status {
  CO_NEW = 0,
  CO_RUNNING,
  CO_WAITING,
  CO_DEAD,
};

struct co {
  char *name_;
  void (*func_)(void *arg);
  void *arg_;

  enum co_status status_;
  struct co *waiter_;
  struct co *next_;
  jmp_buf context_;
  uint8_t stack_[STACK_SIZE];
};

static struct co *g_running_co;
static struct co *waiting_list_guard;
static struct co *sched_list_guard;
static uint32_t g_sched_list_size = 0;

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile(
#if __x86_64__
      "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      :
      : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
      : "memory"
#else
      "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      :
      : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg)
      : "memory"
#endif
  );
}

void schedule() {
  int chosed_num = rand() % g_sched_list_size;
  struct co *co_to_run = sched_list_guard;
  while (--chosed_num >= 0) {
    co_to_run = co_to_run->next_;
  }
  int i = setjmp(g_running_co->context_);
  if (i == 0) {
    stack_switch_call();
  }
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *new_co = (struct co *)malloc(sizeof(struct co));
  new_co->name_ = name;
  new_co->func_ = func;
  new_co->arg_ = arg;
  new_co->status_ = CO_NEW;
  memset(new_co->stack_, 0, sizeof(uint8_t) * STACK_SIZE);
  // context and status should be set before running
  if (sched_list_guard == NULL) {
    sched_list_guard = new_co;
  } else {
    new_co->next_ = sched_list_guard->next_;
    sched_list_guard->next_ = new_co;
  }
  g_sched_list_size++;
}

void co_wait(struct co *co) {}

void co_yield() {}
