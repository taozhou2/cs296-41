#ifndef _DEBUG_H
#define _DEBUG_H

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

/**
 * Executes commands that the parent uses to trace/debug the child.
 *
 * @param   pid             PID of the child.
 * @param   func            Function in the child to set the breakpoint
 */
void parent(pid_t pid, char* func);

/**
 * Handles user input.
 *
 * @param   input           User input that contains instruction.
 *
 * @return                  2 if it is ok to step through the program,
 *                          1 if it is ok to continue the program, 
 *                          0 otherwise
 */
int handle_input(char* input, pid_t pid);

/**
 * Find and print the variable.
 *
 * @param   variable        Name of the variable, extracted from user input.
 */
void debugger_print(char* variable, pid_t pid);

/**
 * Print available commands.
 */
void debugger_help();

/**
 * Use dwarf to get the offset of the variable.
 *
 * @param   variable        Name of the variable.
 *
 * @return                  Offset (positive) if variable is valid,
 *                          -1 otherwise.
 */
int get_variable_offset(char* variable);

/**
 * Get the function pointer using objdump
 *
 * @param   func            Name of the function.
 */
void get_func_ptr(char* func);

/**
 * 
 *
 * @param   ptr             
 */
void* decodedline(char* ptr);

#endif