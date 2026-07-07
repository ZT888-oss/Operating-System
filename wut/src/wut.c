#include "wut.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/signal.h>
#include <ucontext.h>
#include <valgrind/valgrind.h>

#define MAX_THREADS 1024
#define STACK_SIZE SIGSTKSZ

#define STATUS_READY 1
#define STATUS_RUNNING 2
#define STATUS_UNUSED -1

#define STATUS_CANCELLED 128
#define STATUS_BLOCKED 3

static void die(const char *msg) {
  perror(msg);
  exit(errno);
}

static char *new_stack(void) {
  char *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (stack == MAP_FAILED)
    die("mmap stack failed");

  VALGRIND_STACK_REGISTER(stack, stack + STACK_SIZE);
  return stack;
}

static void delete_stack(char *stack) {
  if (stack) {
    if (munmap(stack, STACK_SIZE) == -1)
      die("munmap stack failed");
  }
}

typedef struct tcb {
  int id;
  ucontext_t context;
  char *stack;

  int status;
  int waiting_for;
  int joined;
} tcb_t;

static tcb_t tcb_array[MAX_THREADS];

static int current_thread = 0;

static int ready_queue[MAX_THREADS];
static int ready_front = 0;
static int ready_rear = -1;


static void enqueue(int tid) { ready_queue[++ready_rear] = tid; }

static int dequeue() {
  if (ready_front > ready_rear)
    return -1;

  int tid = ready_queue[ready_front++];

  if (ready_front > ready_rear) {
    ready_front = 0;
    ready_rear = -1;
  }

  return tid;
}

static void thread_wrapper(void (*func)(void)) {
  func();
  wut_exit(0);
}

void wut_init(void) {

  for (int i = 0; i < MAX_THREADS; i++) {
    tcb_array[i].status = STATUS_UNUSED;
    tcb_array[i].waiting_for = -1;
    tcb_array[i].joined = 0;
    tcb_array[i].stack = NULL;
  }

  current_thread = 0;

  tcb_array[0].id = 0;
  tcb_array[0].status = STATUS_RUNNING;

  getcontext(&tcb_array[0].context);

  ready_front = 0;
  ready_rear = -1;
}

int wut_id(void) { return current_thread; }

int wut_create(void (*func)(void)) {

  int id = -1;

  for (int i = 0; i < MAX_THREADS; i++) {
    if (tcb_array[i].status == STATUS_UNUSED) {
      id = i;
      break;
    }
  }

  if (id == -1)
    return -1;

  tcb_array[id].id = id;
  tcb_array[id].stack = new_stack();
  tcb_array[id].status = STATUS_READY;
  tcb_array[id].waiting_for = -1;
  tcb_array[id].joined = 0;

  getcontext(&tcb_array[id].context);

  tcb_array[id].context.uc_stack.ss_sp = tcb_array[id].stack;
  tcb_array[id].context.uc_stack.ss_size = STACK_SIZE;
  tcb_array[id].context.uc_link = NULL;

  makecontext(&tcb_array[id].context, (void (*)(void))thread_wrapper, 1, func);

  enqueue(id);

  return id;
}

int wut_yield(void) {

  int prev = current_thread;

  enqueue(prev);

  int next = dequeue();

  while (next != -1 && tcb_array[next].status != STATUS_READY) {
    next = dequeue();
  }

  if (next == -1)
    return -1;

  current_thread = next;

  tcb_array[prev].status = STATUS_READY;
  tcb_array[next].status = STATUS_RUNNING;

  swapcontext(&tcb_array[prev].context, &tcb_array[next].context);

  return 0;
}

int wut_cancel(int tid) {

  if (tid < 0 || tid >= MAX_THREADS)
    return -1;

  if (tid == current_thread)
    return -1;

  if (tcb_array[tid].status == STATUS_UNUSED)
    return -1;

  if (tcb_array[tid].status == STATUS_CANCELLED)
    return -1;

  for (int i = ready_front; i <= ready_rear; i++) {
    if (ready_queue[i] == tid) {
      for (int j = i; j < ready_rear; j++)
        ready_queue[j] = ready_queue[j + 1];
      ready_rear--;
      break;
    }
  }

  tcb_array[tid].status = STATUS_CANCELLED;

  for (int i = 0; i < MAX_THREADS; i++) {
    if (tcb_array[i].waiting_for == tid) {

      tcb_array[i].waiting_for = -1;

      if (tcb_array[i].status != STATUS_CANCELLED) {
        tcb_array[i].status = STATUS_READY;
        enqueue(i);
      }
    }
  }
  return 0;
}

int wut_join(int tid) {

  if (tid < 0 || tid >= MAX_THREADS)
    return -1;

  if (tid == current_thread)
    return -1;

  if (tcb_array[tid].status == STATUS_UNUSED)
    return -1;

  if (tcb_array[tid].joined)
    return -1;

  //tcb_array[tid].joined = 1;

  if (tcb_array[tid].status != STATUS_READY &&
      tcb_array[tid].status != STATUS_RUNNING) {

    int status = tcb_array[tid].status;

    delete_stack(tcb_array[tid].stack);

    tcb_array[tid].stack = NULL;
    tcb_array[tid].status = STATUS_UNUSED;

    return status;
  }

  tcb_array[current_thread].waiting_for = tid;
  tcb_array[current_thread].status = STATUS_BLOCKED;

  int next = dequeue();

  while (next != -1 && tcb_array[next].status != STATUS_READY) {
    next = dequeue();
  }

  if (next == -1)
    exit(1);

  int prev = current_thread;

  current_thread = next;

  tcb_array[next].status = STATUS_RUNNING;

  swapcontext(&tcb_array[prev].context, &tcb_array[next].context);

  int status = tcb_array[tid].status;

  delete_stack(tcb_array[tid].stack);

  tcb_array[tid].stack = NULL;
  tcb_array[tid].status = STATUS_UNUSED;
  tcb_array[tid].waiting_for = -1;
  tcb_array[tid].joined = 1;

  return status;
}

void wut_exit(int status) {

  int tid = current_thread;

  status &= 0xFF;

  tcb_array[tid].status = status;

  for (int i = 0; i < MAX_THREADS; i++) {

    if (tcb_array[i].waiting_for == tid &&
        tcb_array[i].status != STATUS_CANCELLED) {

      tcb_array[i].waiting_for = -1;
      tcb_array[i].status = STATUS_READY;
      enqueue(i);
    }
  }

  int next = dequeue();

  if (next == -1)
    exit(status);

  current_thread = next;

  tcb_array[next].status = STATUS_RUNNING;

  setcontext(&tcb_array[next].context);
}