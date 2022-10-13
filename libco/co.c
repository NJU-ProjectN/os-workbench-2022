
#include "co.h"
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STACK_SIZE 20480
#if __x86_64__
#define ADDR_SIZE sizeof(void *)
#else
#define ADDR_SIZE -sizeof(void *)
#endif

enum co_status {
  CO_NEW = 0,
  CO_RUNNING,
  CO_WAITING,
  CO_DEAD,
};

struct co;

struct co_handle {
  struct co *this_;
  struct co_handle *prev_;
  struct co_handle *next_;
};

struct co {
  struct co_handle list_handle_;

  const char *name_;
  void (*func_)(void *arg);
  void *arg_;
  void (*exit_func)(void *);

  enum co_status status_;
  struct co *waiter_;
  jmp_buf context_;
  uint8_t stack_[STACK_SIZE];
};

static struct co *g_running_co;
static struct co_handle sched_list_guard = {
    .prev_ = &sched_list_guard,
    .next_ = &sched_list_guard,
    .this_ = NULL,
};
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

struct co *RemoveFromList(struct co *cur) {
  cur->list_handle_.prev_->next_ = cur->list_handle_.next_;
  cur->list_handle_.next_->prev_ = cur->list_handle_.prev_;
  return cur;
}

void InsertToList(struct co_handle *guard, struct co *x) {
  x->list_handle_.next_ = guard->next_;
  x->list_handle_.prev_ = guard;
  guard->next_ = &x->list_handle_;
  x->list_handle_.next_->prev_ = &x->list_handle_;
}

struct co *GetCoByHandle(struct co_handle *handle) {
  // struct co temp_co;
  // off_t offset = (uintptr_t)&temp_co.list_handle_ - (uintptr_t)&temp_co;
  return handle->this_;
}

void schedule() {
  struct co *co_to_run = GetCoByHandle(sched_list_guard.next_);

  if (g_sched_list_size == 0) {
    return;
  }
  // find a coroutine to run
  assert(g_sched_list_size != 0);
  int chosed_num = rand() % g_sched_list_size;
  while (--chosed_num >= 0) {
    co_to_run = GetCoByHandle(co_to_run->list_handle_.next_);
  }

  // no coroutine to run, return to main workflow
  assert(co_to_run != NULL);

  // save current and run next
  int i = setjmp(g_running_co->context_);
  if (i == 0) {
    g_running_co = co_to_run;
    if (co_to_run->status_ == CO_NEW) {
      co_to_run->status_ = CO_RUNNING;
      memcpy(co_to_run->stack_ + STACK_SIZE - ADDR_SIZE,
             (&co_to_run->exit_func), ADDR_SIZE);
      stack_switch_call(co_to_run->stack_ + STACK_SIZE - ADDR_SIZE,
                        co_to_run->func_, (uintptr_t)co_to_run->arg_);
    } else {
      co_to_run->status_ = CO_RUNNING;
      longjmp(co_to_run->context_, 1);
    }
  }
}

void co_exit() {
  struct co *co_self = g_running_co;
  struct co *waiter = co_self->waiter_;
  if (waiter != NULL) {
    // wake waiter
    InsertToList(&sched_list_guard, waiter);
    g_sched_list_size++;
  }
  co_self->status_ = CO_DEAD;
  RemoveFromList(co_self);
  g_sched_list_size--;

  // run next
  schedule();
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *new_co = (struct co *)malloc(sizeof(struct co));
  new_co->name_ = name;
  new_co->func_ = func;
  new_co->arg_ = arg;
  new_co->exit_func = co_exit;
  new_co->status_ = CO_NEW;
  new_co->waiter_ = NULL;
  memset(new_co->stack_, 0, sizeof(uint8_t) * STACK_SIZE);

  // context and status should be set before running
  new_co->list_handle_.this_ = new_co;
  InsertToList(&sched_list_guard, new_co);
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

  if (co_to_wait->status_ != CO_DEAD) {
    co_to_wait->waiter_ = g_running_co;
    // move g_running_co to waiting_list
    RemoveFromList(g_running_co);
    g_sched_list_size--;

    co_yield();
  }

  // recycle co
  free(co_to_wait);
}

void co_yield() {
  g_running_co->status_ = CO_WAITING;
  schedule();
}
