#include "breakpoint.h"

void breakpoint_attach(pid_t pid) {
  int status;
  /**
   * PTRACE_ATTACH: send the tracee pid a SIGSTOP.
   * Need to wait till it stops.
   * TODO: add error checking for waitpid and ptrace
   */
  ptrace(PTRACE_ATTACH, pid);
  waitpid(pid, &status, 0);
  ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACEEXIT);    // TODO: ??
}

target_addr_t breakpoint_getip(pid_t pid) {
  /**
   * PTRACE_PEEKUSER: Read a word at offset addr in the tracee's USER area.
   * TODO: probably should change to sizeof(int) for 32-bit machines.
   */
  long v = ptrace(PTRACE_PEEKUSER, pid, sizeof(long) * REGISTER_IP);
  return (target_addr_t) (v - TRAP_LEN);  // TODO: not sure why ?
  // After we hit a breakpoint, the saved IP points to the instruction after
  // the trap instruction. When we resume execution, we'll go back and execute
  // the original instruction that we overwrote with the trap.
  // So we subtract TRAP_LEN to give the true address of the next instruction.
}

void enable(pid_t pid, struct breakpoint *bp) {
  /**
   * PTRACE_PEEKTEXT: read a word at offset addr in the tracee's memory.
   * Same as PEEKDATA.
   */
  long orig = ptrace(PTRACE_PEEKTEXT, pid, bp->addr);
  /**
   * PTRACE_POKETEXT: copy the word to addr in the tracee's memory.
   * Same as POKEDATA
   */
  ptrace(PTRACE_POKETEXT, pid, bp->addr, (orig & TRAP_MASK) | TRAP_INST);
  bp->orig_code = orig;
}

struct breakpoint *breakpoint_break(pid_t pid, target_addr_t addr) {
  printf("Set breakpoint at address %p\n", addr);
  struct breakpoint *bp = malloc(sizeof(*bp));
  bp->addr = addr;
  enable(pid, bp);
  return bp;
}

void disable(pid_t pid, struct breakpoint *bp) {
  ptrace(PTRACE_POKETEXT, pid, bp->addr, bp->orig_code);
}

int breakpoint_run(pid_t pid, struct breakpoint *bp, int single_or_cont) {
  if (bp) {
    ptrace(PTRACE_POKEUSER, pid, sizeof(long) * REGISTER_IP, bp->addr);
    disable(pid, bp);
    /**
     * PTRACE_SINGLESTEP: continue for one instruction.
     * CPU will raise int3 after one instruction.
     */
    if (!run(pid, PTRACE_SINGLESTEP))
      return 0;
    enable(pid, bp);    // re-enable it for future breaks
  }

  return run(pid, single_or_cont);   // continue the process
}

int run(pid_t pid, int cmd) {
  int status, last_sig = 0, event;
  while (1) {
    ptrace(cmd, pid, 0, last_sig);
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
      return 0;

    if (WIFSTOPPED(status)) {
      last_sig = WSTOPSIG(status);
      if (last_sig == SIGTRAP) {
        event = (status >> 16) & 0xffff;
        return (event == PTRACE_EVENT_EXIT) ? 0 : 1;
      }
    }
  }
}
