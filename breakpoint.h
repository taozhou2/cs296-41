#ifndef _BREAKPOINT_H
#define _BREAKPOINT_H
#include <sys/types.h>

typedef void *target_addr_t;
struct breakpoint;

void breakpoint_attach(pid_t pid);
target_addr_t breakpoint_getip(pid_t pid);
struct breakpoint *breakpoint_break(pid_t pid, target_addr_t addr);
int breakpoint_run(pid_t pid, struct breakpoint *bp, int single_or_cont);

#endif
