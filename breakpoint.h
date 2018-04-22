#ifndef _BREAKPOINT_H
#define _BREAKPOINT_H

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <stdlib.h>
#include <stdio.h>

typedef void* target_addr_t;

#if defined(__i386)
#define REGISTER_IP EIP
#define TRAP_LEN    1
#define TRAP_INST   0xCC
#define TRAP_MASK   0xFFFFFF00

#elif defined(__x86_64)
#define REGISTER_IP RIP     /* instruction pointer offset, provided by sys/reg.h */
#define TRAP_LEN    1
#define TRAP_INST   0xCC    /* int3 (send SIGTRAP) */
#define TRAP_MASK   0xFFFFFFFFFFFFFF00

#else
#error Unsupported architecture
#endif

struct breakpoint {
  target_addr_t addr;       /* Address of the code we replaced */
  long orig_code;           /* The original code */
};

/**
 * Parent process attach ptrace to the child process pid.
 *
 * @param  pid              The PID of tracee process to be traced.
 */
void breakpoint_attach(pid_t pid);

/**
 * Read the child's instruction pointer.
 * The instruction pointer is stored inside USER area.
 *
 * @param   pid             The PID of tracee process.
 *
 * @return                  A pointer to the next instruction child will execute.
 */
target_addr_t breakpoint_getip(pid_t pid);

/**
 * Enable a breakpoint bp by replacing the orig code with the trap instruction.
 * Save the orig code in bp struct.
 *
 * @param   pid             The PID of tracee process.
 * @param   bp              The breakpoint enable.
 */
void enable(pid_t pid, struct breakpoint *bp);

/**
 * Create a breakpoint and enable it.
 *
 * @param   pid             The PID of tracee process.
 * @param   addr            The code address of the breakpoint.
 *
 * @return                  A enabled breakpoint pointer.
 */
void disable(pid_t pid, struct breakpoint *bp);

/**
 * Disable the breakpoint by writing back the saved original code.
 *
 * @param   pid             The PID of tracee process.
 * @param   bp              The breakpoint to disable.
 */
struct breakpoint *breakpoint_break(pid_t pid, target_addr_t addr);

/**
 * Continue the process.
 *
 * @param   pid             The PID of tracee process.
 * @param   bp              The breakpoint to run.
 * @param   single_or_cont  Whether to single step or continue
 *
 * @return                  0 if the target exits
 *                          1 otherwise
 */
int breakpoint_run(pid_t pid, struct breakpoint *bp, int single_or_cont);

/**
 * Run the tracee with cmd. Wait for a signal or exit.
 *
 * @param   pid             The PID of tracee process.
 * @param   cmd             The type of ptrace instruction.
 *
 * @return                  0 if the target exits
 *                          1 otherwise
 */
int run(pid_t pid, int cmd);

#endif