#include "co.h"
#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

#define STACK_SIZE (64 * 1024)
#define COROUTINE_NAME_LENGTH (32)

#define LOCAL_MACHINE

#ifdef LOCAL_MACHINE
#define debug(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

enum co_status {
  CO_NEW = 1, // new created, have not been executed before
  CO_RUNNING, // already running.
  CO_WAITING, // waiting for awaking
  CO_DEAD,    // done, while not release src.
};

struct co {
  char name[COROUTINE_NAME_LENGTH];
  void (*func)(void *);
  void *arg;

  enum co_status status;
  struct co *waiter;
  jmp_buf context;
  uint8_t stack[STACK_SIZE];
};

// Define coroutine doule-linked-cylic-list(pool).
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

void restore_return() {
  asm volatile("xor %%rax, %%rax; cmpq %%rax, %%rax"::);
}

static inline void stack_switch_call(void* sp, void *entry, void* arg) {
  asm volatile(
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; callq *%1"
    ::"b"((uintptr_t)sp - 1024), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#else
    "movl %0, %%rsp; movl %2, 4(%0); call *%1"
    ::"b"((uintptr_t)sp - 8), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#endif
  );
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co *co_ptr = (struct co *)malloc(sizeof(struct co));
  assert(co_ptr);
  /** memset(co_ptr->name, 0, COROUTINE_NAME_LENGTH); */
  /** strcpy(co_ptr->name, name); */
  size_t name_len = strlen(name);
  debug("name %s has length %ld\n", name, strlen(name));
  for (size_t i = 0; i < name_len; ++i) {
    co_ptr->name[i] = name[i];
  }
  for (size_t i = name_len; i < COROUTINE_NAME_LENGTH; ++i) {
    co_ptr->name[i] = '\0';
  }
  debug("name: %s\n", co_ptr->name);
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
    if (co->status == CO_RUNNING) {
      co->status = CO_WAITING;
    }
    if (strcmp(current->name, "main") == 0) {
      current->status = CO_WAITING;
      current->waiter = co;
    }
    if (co->status == CO_WAITING) {
      co->waiter = current;
    }
    co_yield();
  }

  /** CoPool* node = co_head; */
  /** co_node = co_head; */
  while (co_node->coroutine != co) {
    co_node = co_node->next;
  }
  assert(co_node->coroutine == co);
  // child->parent deallocate order.
  free(co);
  free(co_remove(co_node));
}

bool is_all_coroutines_done() {
  CoPool* node = co_head;
  assert(node);
  bool done = true;
  if ((strcmp(node->coroutine->name, "main") != 0) &&
      node->coroutine->status != CO_DEAD) {
    return false;
  }
  node = node->next;
  while (node && node != co_head) {
    if ((strcmp(node->coroutine->name, "main") != 0) &&
        node->coroutine->status != CO_DEAD) {
      done = false;
      break;
    }
    node = node->next;
  }
  return done;
}

void co_yield() {
  int val = setjmp(current->context);
  if (val == 0) {
    /** current->status = CO_RUNNING; */
    // select a coroutine to yield
    co_node = co_node->next;
    while(co_node) {
      /** debug("coroutine name: %s\n", co_node->coroutine->name); */
      if (strcmp(co_node->coroutine->name, "main") == 0) {
        if (is_all_coroutines_done()) {
          current = co_node->coroutine;
          current->status = CO_RUNNING;
          break;
        }
        co_node = co_node->next;
        continue;
      }
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
      stack_switch_call(current->stack + STACK_SIZE, current->func,
                        current->arg);
      // necessary for later condition test?
      // without this, current->waiter is NULL, but the PC will still enter the
      // body of 'if'.
      restore_return();
      ((struct co volatile *)current)->status = CO_DEAD;
      /** current->status = CO_DEAD; */
      if (current->waiter) {
        current->waiter->status = CO_RUNNING;
      }
      co_yield();
    }
  } else {
    ;
  }
}

static __attribute__((constructor)) void co_constructor(void) {
  current = co_start("main", NULL, NULL);
  current->status = CO_RUNNING;
}

static __attribute__((destructor)) void co_destructor(void) {
  /** co_node = co_head; */
/**   while(co_node) { */
    /** current = co_node->coroutine; */
    /** free(current); */
    /** free(co_remove(co_node)); */
  /** } */
}
