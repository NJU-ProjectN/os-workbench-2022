#include <stddef.h>
#include <stdint.h>
#include "../kernel/framework/syscall.h"
#include "../kernel/framework/user.h"

static inline long syscall(int num, long x1, long x2, long x3, long x4) {
  register long a0 asm ("rax") = num;
  register long a1 asm ("rdi") = x1;
  register long a2 asm ("rsi") = x2;
  register long a3 asm ("rdx") = x3;
  register long a4 asm ("rcx") = x4;
  asm volatile("int $0x80"
    : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
    : "memory");
  return a0;
}

static inline int kputc(char ch) {
  return syscall(SYS_kputc, ch, 0, 0, 0);
}

static inline int fork() {
  return syscall(SYS_fork, 0, 0, 0, 0);
}

static inline int wait(int *status) {
  return syscall(SYS_wait, (uint64_t)status, 0, 0, 0);
}

static inline int exit(int status) {
  return syscall(SYS_exit, status, 0, 0, 0);
}

static inline int kill(int pid) {
  return syscall(SYS_kill, pid, 0, 0, 0);
}

static inline void *mmap(void *addr, int length, int prot, int flags) {
  return (void *)syscall(SYS_mmap, (uint64_t)addr, length, prot, flags);
}

static inline int getpid() {
  return syscall(SYS_getpid, 0, 0, 0, 0);
}

static inline int sleep(int seconds) {
  return syscall(SYS_sleep, seconds, 0, 0, 0);
}

static inline int64_t uptime() {
  return syscall(SYS_uptime, 0, 0, 0, 0);
}
