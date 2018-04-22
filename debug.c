#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "breakpoint.h"

/**
 * How to run
 * ./debug ./hello fact
 * or
 * ./debug ./hello fib
 */


// https://stackoverflow.com/questions/18577956/how-to-use-ptrace-to-get-a-consistent-view-of-multiple-threads

void parent(pid_t pid, char* func);
void get_func_ptr(char* func);
int call_threads();
void* wrapper(void* param);

int fact(int n);
int fib(int n);

void* func_ptr = NULL;

int main(int argc, char** argv) {
  if (argc != 2){
    puts("Usage: ./debug <function>");
    exit(1);
  }

  get_func_ptr(argv[1]);

  pid_t pid = fork();
  if (pid == 0) {
//    kill(getpid(), SIGSTOP);

    ptrace(PTRACE_TRACEME, pid);
    puts("execing");
    execlp("./hello", "./hello", NULL);
    puts("Exec failed");
//    call_threads();
//    printf("fact(5) = %d\n", fact(5));
//    printf("fib(5) = %d\n", fib(5));
    exit(1);
  }

//  wait(NULL);
//  ptrace(PTRACE_O_TRACEEXEC, pid);
//  ptrace(PTRACE_CONT, pid);
  parent(pid, argv[1]);
//  printf("%d\n", __LINE__);
//  sleep(3);
//  while(1){}
  return 0;
}

void parent(pid_t pid, char* func) {
  struct breakpoint *bp, *last_break = NULL;
  void *fact_ip = func_ptr, *last_ip;

  int status = 0;

  // while((status >> 8) != (SIGTRAP | (PTRACE_EVENT_EXIT << 8)))
  // {
  //   puts("In while loop of waitpid");
  //   waitpid(pid, &status, 0);
  // }
  // puts("exited");

  wait(NULL);
//  breakpoint_attach(pid);
  bp = breakpoint_break(pid, fact_ip);

  ptrace(PTRACE_CONT, pid);

  char* line = NULL;
  size_t size = 0;
  int opt = PTRACE_CONT;
  while (breakpoint_run(pid, last_break, opt)) {
    last_ip = breakpoint_getip(pid);
    if (last_ip == fact_ip) {
      int arg = ptrace(PTRACE_PEEKUSER, pid, sizeof(long) * RDI);
      printf("%p:\tHit breakpoint at %s(%d)\n", last_ip, func, arg);
      last_break = bp;
    } else {
      printf("%p:\tUnknown trap\n", last_ip);
      last_break = NULL;
    }

    if (getline(&line, &size, stdin) != -1) {
      line[size] = '\0';
      if (!strcmp(line, "s\n")){
        opt = PTRACE_SINGLESTEP;
      }
      else{
        opt = PTRACE_CONT;
      }
    }
  }
  free(line);
}

/**
 * Get the function pointer using objdump
 * @param func Name of the function.
 */
void get_func_ptr(char* func) {
  int fd[2];
  pipe(fd);
  pid_t child = fork();
  if (child == 0) {
    // child here
    close(fd[0]);
    dup2(fd[1], 1);
    char inst[1024];
    sprintf(inst, "objdump -d hello | grep \"<%s>:\" | cut -d ' ' -f 1 | sed 's/^0*//g'", func);
    system(inst);
    exit(-1);
  }
  close(fd[1]);
  char reading_buf[1], addr[1024];
  int size = 0;
  memset(addr, '\0', sizeof(addr));
  while(read(fd[0], reading_buf, 1) > 0) {
      strncpy(addr + size, reading_buf, 1);
      size ++;
  }
  close(fd[0]);
  int status;
  waitpid(child, &status, 0);
  func_ptr = (void*)strtol(addr, NULL, 16);
  printf("Function %s is at address %p\n", func, func_ptr);
}



// Functions for debugging
//============================
//============================
//============================

int fact(int n) {
  if (n <= 1)
    return 1;
  return n * fact(n-1);
}

int fib(int n) {
  if (n <= 2)
    return 1;
  return fib(n - 1) + fib(n - 2);
}

void* wrapper(void* param)
{puts("133");
  int result = fact(*(int*)param);
  fprintf(stderr, "Result: %d\n", result);
  return NULL;
}

void* test(void* param)
{
  return NULL;
}

/* Maybe use proc to help??? */
int call_threads()
{
  pthread_t p1, p2;
  int n1 = 10;
  int n2 = 11;

  fprintf(stderr, "%d\n", pthread_create(&p1, NULL, wrapper, (void*)(&n1)));
//  fprintf(stderr, "%d\n", pthread_create(&p2, NULL, wrapper, (void*)(&n2)));

//  sleep(2);

  pthread_join(p1, NULL);
//  pthread_join(p2, NULL);

//  wrapper(&n1);
  puts("Done with call_threads()");
  return 0;
}