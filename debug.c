#include "breakpoint.h"
#include "debug.h"
#include "dictionary.h"


dictionary* lines = NULL;
void* func_ptr = NULL;
void* func_ptr_orig = NULL;
char* exe = NULL;

void parent(pid_t pid, char* func) {
  struct breakpoint *bp, *last_break = NULL;
  void *breakpoint_ip = func_ptr, *last_ip;

  wait(NULL);
  // printf("Set breakpoint at address %p\n", breakpoint_ip);
  bp = breakpoint_break(pid, breakpoint_ip);
  ptrace(PTRACE_CONT, pid);

  char* line = NULL;
  size_t size = 0;
  int opt = PTRACE_CONT;

  while (breakpoint_run(pid, last_break, opt)) {
    last_ip = breakpoint_getip(pid);
    if (last_ip == breakpoint_ip) {
      printf("%p:\tHit breakpoint\n", last_ip);
      last_break = bp;
    } else {
      printf("%p:\tUnknown trap\n", last_ip);
      last_break = NULL;
    }

    int cont = 0;

    while (!cont) {
      int len = 0;
      printf("%s", ">> ");
      fflush(stdout);
      if ((len = getline(&line, &size, stdin)) != -1) {
        line[len - 1] = '\0';      // get rid of the '\n' at the end
        cont = handle_input(line, pid);
        if (cont == 3) {
          void* curr_ptr = debugger_next_line(last_ip, pid, bp);
          cont = 0;
          if (!curr_ptr)  cont = 1;
          last_ip = curr_ptr;
          last_break = NULL;
        }
      }
    }

    if(cont == 1){
      opt = PTRACE_CONT;
    }
    else if(cont == 2){
      opt = PTRACE_SINGLESTEP;
    }
  }
  free(line);
}

int handle_input(char* input, pid_t pid) {
  if (strcmp(input, "continue") == 0 || strcmp(input, "c") == 0) {
    return 1;
  }
  else if (strcmp(input, "step") == 0 || strcmp(input, "s") == 0) {
    return 2;
  }

  if (strcmp(input, "help") == 0) {
    debugger_help();
  } else if (strncmp(input, "print ", 6) == 0) {
    debugger_print(input + 6, pid);
  } else if (strncmp(input, "p ", 2) == 0) {
    debugger_print(input + 2, pid);
  } else if (strcmp(input, "next") == 0 || strcmp(input, "n") == 0) {
    return 3;
  }

  return 0;
}

void* debugger_next_line(void *curr_ptr, pid_t pid, struct breakpoint *bp) {
  void* next_line = get_next_line_addr(curr_ptr);
  // fprintf(stderr, "%p:\tnext line is at %p\n", curr_ptr, next_line);

  if (!next_line) return NULL;

  struct breakpoint *run_bp = bp;
  while (breakpoint_run(pid, run_bp, PTRACE_SINGLESTEP)) {
    void* last_ip = breakpoint_getip(pid);
    // fprintf(stderr, "%p\n", last_ip);
    if (last_ip >= next_line) {
      printf("%p:\tStopped at %p\n", last_ip, next_line);
      break;
    } else if (last_ip == curr_ptr) {
      run_bp = bp;
    } else {
      run_bp = NULL;
    }
  }

  return next_line;
}

void debugger_print(char* variable, pid_t pid) {
  // 1. objdump --dwarf=frames-interp debug
  // frame base = rbp + 16

  // 2. objdump --dwarf=info debug
  int offset = get_variable_offset(variable);
  if (offset == -1) {
    printf("variable %s not in scope\n", variable);
    return;
  }
  void* rbp = (void*)ptrace(PTRACE_PEEKUSER, pid, sizeof(long) * (RBP));
  void* frame_base = rbp + 16;
  int val = ptrace(PTRACE_PEEKDATA, pid, frame_base - offset);
  printf("%s = %d\n", variable, val);
}

void debugger_help() {
  puts("Debugger Command Help:");
  puts("help\t\t\t\tList commands");
  puts("continue[c]\t\t\tContinue executing until next breakpoint");
  puts("step[s]\t\t\t\tAdvance one assembly instruction");
  puts("next[n]\t\t\t\tAdvance one line of code");
  puts("print[p] <variable name>\tPrint value stored in variable.");
}

int get_variable_offset(char* variable) {
  int fd[2];
  pipe(fd);

  pid_t child = fork();
  if (child == 0) {
    // child here
    close(fd[0]);
    dup2(fd[1], 1);
    char inst[1024];

    sprintf(inst, "objdump --dwarf=info %s | sed -n '/%p/,/<1>/p' | grep \": %s\" -A 10 | grep -Po 'fbreg: -\\K[^)]*' -m 1", exe, func_ptr_orig, variable);
    system(inst);
    exit(-1);
  }
  close(fd[1]);
  char reading_buf[1], result[1024];
  int size = 0;
  memset(result, '\0', sizeof(result));
  while(read(fd[0], reading_buf, 1) > 0) {
      strncpy(result + size, reading_buf, 1);
      size ++;
  }
  close(fd[0]);
  int status;
  waitpid(child, &status, 0);
  if (strlen(result) == 0) {
    return -1;
  }
  return strtol(result, NULL, 10);
}

void get_func_ptr(char* func) {
  int fd[2];
  pipe(fd);
  pid_t child = fork();
  if (child == 0) {
    close(fd[0]);
    dup2(fd[1], 1);
    char inst[1024];

    sprintf(inst, "objdump -d %s | grep \"<%s>:\" | cut -d ' ' -f 1 | sed 's/^0*//g'", exe, func);
    // TODO: use dwarf=decodedline to get the address after setup

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

  addr[strlen(addr) - 1] = '\0';    // ending '\n'
  // func_ptr = decodedline(addr);
  func_ptr_orig = (void*)strtol(addr, NULL, 16);
  func_ptr = get_next_line_addr(func_ptr_orig);
  printf("Function %s is at address %p\n", func, func_ptr);
}

void* get_next_line_addr(void* curr_line) {
  vector *vals = dictionary_values(lines);
  size_t i;
  for (i = 1; i < vector_size(vals); i ++) {
    char* curr_str = (char*)vector_get(vals, i - 1);
    void* curr_ptr = (void*)strtol(curr_str + 2, NULL, 16);
    if (curr_ptr == curr_line) {
      char* next_str = (char*)vector_get(vals, i);
      return (void*)strtol(next_str + 2, NULL, 16);
    }
    // fprintf(stderr, "%s, %p\n", curr_str, curr_ptr);
  }
  return NULL;
}

// void* decodedline(char* ptr) {
//   int fd[2];
//   pipe(fd);
//   pid_t child = fork();
//   if (child == 0) {
//     // child here
//     close(fd[0]);
//     dup2(fd[1], 1);
//     char inst[1024];
//
//     sprintf(inst, "objdump --dwarf=decodedline %s | grep \"%s\" -A 1 | awk '{print $3}' | tail -n1", exe, ptr);
//     // TODO: use dwarf=decodedline to get the address after setup
//
//     system(inst);
//     exit(-1);
//   }
//   close(fd[1]);
//   char reading_buf[1], addr[1024];
//   int size = 0;
//   memset(addr, '\0', sizeof(addr));
//   while(read(fd[0], reading_buf, 1) > 0) {
//       strncpy(addr + size, reading_buf, 1);
//       size ++;
//   }
//   close(fd[0]);
//   int status;
//   waitpid(child, &status, 0);
//
//   return (void*)strtol(addr + 2, NULL, 16);
// }

void parse_line_number() {
  fprintf(stderr, "%s\n", "Parsing line number...");
  char* raw = get_raw_lines();

  char* newlinechar = "\n";
  char* line;

  line = strtok(raw, newlinechar);

  while(line) {
    size_t i = strlen(line) / 2;
    while (isspace(line[i])) {
      i ++;
    }
    int len = 0;
    while (!isspace(line[i + len])) {
      len ++;
    }
    char* key = malloc(len + 1);
    strncpy(key, line + i, len);
    key[len] = '\0';

    i = i + len;
    while (isspace(line[i])) {
      i ++;
    }
    char* val = malloc(strlen(line + i) + 1);
    strcpy(val, line + i);

    // fprintf(stderr, "%s:%s\n", key, val);

    dictionary_set(lines, key, val);
    // fprintf(stderr, "%s\n", (char*) dictionary_get(lines, key));
    free(key);
    free(val);

    line = strtok(NULL, newlinechar);
  }
  free(raw);
}

char* get_raw_lines() {
  int fd[2];
  pipe(fd);
  pid_t child = fork();
  if (child == 0) {
    // child here
    close(fd[0]);
    dup2(fd[1], 1);
    char inst[1024];

    sprintf(inst, "objdump --dwarf=decodedline %s | grep \"0x\"", exe);

    system(inst);
    exit(-1);
  }
  close(fd[1]);
  char reading_buf[1], *output;
  output = malloc(4096);      // TODO realloc?
  int size = 0;
  memset(output, '\0', 4096);
  while(read(fd[0], reading_buf, 1) > 0) {
      strncpy(output + size, reading_buf, 1);
      size ++;
  }
  close(fd[0]);
  int status;
  waitpid(child, &status, 0);
  return output;
}

int main(int argc, char** argv) {

  if (argc != 3){
    puts("Usage: ./debug <executable> <function>");
    exit(1);
  }

  lines = string_to_string_dictionary_create();
  exe = argv[1];
  parse_line_number();
  get_func_ptr(argv[2]);

  pid_t pid = fork();
  if (pid == 0) {
    ptrace(PTRACE_TRACEME, pid);
    execvp(argv[1], argv+1);
    perror("exec failed");
    exit(1);
  }

  parent(pid, argv[2]);
  dictionary_destroy(lines);
  return 0;
}
