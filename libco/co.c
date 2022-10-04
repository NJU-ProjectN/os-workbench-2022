#include "co.h"
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#ifdef LOCAL_MACHINE
#define debug(...) printf(__VA_ARGS__)
#else
#define debug()
#endif

#define STACK_SIZE (64 * 1024)

enum co_status {
  CO_NEW = 1, // new created, have not been executed before
  CO_RUNNING, // already running.
  CO_WAITING, // waiting for awaking
  CO_DEAD,    // done, while not release src.
};

struct co {
  const char *name;
  void (*func)(void *);
  void *arg;

  enum co_status status;
  struct co *waiter;
  jmp_buf context;
  uint8_t stack[STACK_SIZE];
};

// Define coroutine list(pool).
typedef struct COPOOL {
  struct co *coroutine;
  struct COPOOL *pre;
  struct COPOOL *next;
} CoPool;

static CoPool *co_node = NULL;
static CoPool* co_head = NULL;
static struct co *current = NULL;

static void co_insert(struct co* coroutine) {
  CoPool* node = (CoPool*)malloc(sizeof(CoPool));
  assert(node);
  node->coroutine = coroutine;

  if (co_node == NULL) {
    co_node = node;
    co_node->pre = co_node->next = node;
    co_head = node;
  } else {
    node->next = co_node;
    node->pre = co_node->pre;
    co_node->pre->next = node;
    co_node->pre = node;
  }
}

// remove co_node
static CoPool* co_remove(CoPool* remove_node) {
  CoPool* node = NULL;
  if (remove_node == NULL) {
    return NULL;
  } else if (remove_node->next == remove_node) {
    node = remove_node;
    remove_node = NULL;
  } else {
    node = remove_node;
    remove_node->next->pre = remove_node->pre;
    remove_node->pre->next = remove_node->next;
    remove_node = remove_node->next;
  }
  return node;
}

static inline void stack_switch_call(void* sp, void *entry, void* arg) {
  asm volatile(
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; callq *%1"
    ::"b"((uintptr_t)sp - 16), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#else
    "movl %0, %%rsp; movl %2, 4(%0); call *%1"
    ::"b"((uintptr_t)sp - 8), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#endif
  );
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *co_ptr = (struct co *)malloc(sizeof(struct co));
  assert(co_ptr);
  co_ptr->name = name;
  co_ptr->func = func;
  co_ptr->arg = arg;

  co_ptr->status = CO_NEW;
  co_ptr->waiter = NULL;
  memset(co_ptr->stack, 0, STACK_SIZE);
  co_insert(co_ptr);
  return co_ptr;
}

void co_wait(struct co *co) {
  assert(co);
  if (co->status != CO_DEAD) {
    co->status = CO_WAITING;
    co->waiter = current;
    co_yield();
  }

  /** CoPool* node = co_head; */
  co_node = co_head;
  while (co_node->coroutine != co) {
    co_node = co_node->next;
  }
  assert(co_node->coroutine == co);
  // child->parent deallocate order.
  free(co);
  free(co_remove(co_node));
}

void co_yield() {
  if (current == NULL) {
    assert(co_node);
    current = co_node->coroutine;
  }
  int val = setjmp(current->context);
  if (val == 0) {
    current->status = CO_RUNNING;
    // select a coroutine to yield
    co_node = co_node->next;
    while(co_node) {
      if(co_node->coroutine->status == CO_NEW) {
        current = co_node->coroutine;
        break;
      }
      if (co_node->coroutine->status == CO_RUNNING) {
        current = co_node->coroutine;
        break;
      }
      co_node = co_node->next;
    }

    assert(current);
    if (current->status == CO_RUNNING) {
      longjmp(current->context, 1);
    } else {
      ((struct co volatile *)current)->status = CO_RUNNING;
      void *sp = current->stack + STACK_SIZE;
      void* entry = current->func;
      void * arg = current->arg;
      stack_switch_call(sp, entry, arg);
      current->status = CO_DEAD;
      if (current->waiter) {
        current->waiter->status = CO_RUNNING;
      }
      co_yield();
    }
  } else {
    ;
  }
}
