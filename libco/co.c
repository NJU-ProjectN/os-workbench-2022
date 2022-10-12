
#include "co.h"
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STACK_SIZE 20480

enum co_status {
  CO_NEW = 0,
  CO_RUNNING,
  CO_WAITING,
  CO_DEAD,
};

struct co {
  const char *name_;
  void (*func_)(void *arg);
  void *arg_;
  void (*exit_func)(void *);

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
uint32_t main_waited = 0;

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

struct co *RemoveFromList(struct co *guard, const char *name) {
  struct co *prev = NULL, *curr = guard;
  while (curr->name_ != name) {
    prev = curr;
    curr = curr->next_;
  }

  assert(curr != NULL);
  prev->next_ = curr->next_;
  return curr;
}

void InsertToList(struct co *guard, struct co *x) {
  if (guard == NULL) {
    guard = x;
    guard->next_ = NULL;
    return;
  }
  x->next_ = guard->next_;
  guard->next_ = x;
}

void co_exit() {
  struct co *co_self = g_running_co;
  struct co *waiter = co_self->waiter_;
  assert(waiter == RemoveFromList(waiting_list_guard, waiter->name_));
  // wake waiter
  InsertToList(sched_list_guard, waiter);
}

void schedule() {
  // find a coroutine to run
  int chosed_num = rand() % g_sched_list_size;
  struct co *co_to_run = sched_list_guard;
  while (--chosed_num >= 0) {
    co_to_run = co_to_run->next_;
  }

  // no coroutine to run, return to main workflow
  if (co_to_run == NULL) {
    return;
  }

  // save current and run next
  int i = setjmp(g_running_co->context_);
  if (i == 0) {
    g_running_co->status_ = CO_WAITING;
    g_running_co = co_to_run;
    if (co_to_run->status_ == CO_NEW) {
      co_to_run->status_ = CO_RUNNING;
      memcpy(co_to_run->stack_ + 0x4fe8, (co_to_run->exit_func), 4);
      stack_switch_call(co_to_run->stack_ + 0x4fe8, co_to_run->func_,
                        (uintptr_t)co_to_run->arg_);
    } else {
      co_to_run->status_ = CO_RUNNING;
      longjmp(co_to_run->context_, 1);
    }
  }
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *new_co = (struct co *)malloc(sizeof(struct co));
  new_co->name_ = name;
  new_co->func_ = func;
  new_co->arg_ = arg;
  new_co->exit_func = co_exit;
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
  return new_co;
}

void co_wait(struct co *co_to_wait) {
  if (g_running_co == NULL) {
    // this is main workflow
    struct co *main_co = co_start("main_coroutine", NULL, NULL);
    main_co->status_ = CO_RUNNING;
    g_running_co = main_co;
  }
  co_to_wait->waiter_ = g_running_co;

  // move g_running_co to waiting_list
  RemoveFromList(sched_list_guard, g_running_co->name_);
  InsertToList(waiting_list_guard, g_running_co);

  schedule();

  // recycle co
  free(co_to_wait);
}

void co_yield() { schedule(); }
