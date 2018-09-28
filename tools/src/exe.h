#ifndef __EXE_H__
#define __EXE_H__

/**
 * @file
 * @brief Routines for running executables.
 */

/**
 * @brief Executing Programs
 * @defgroup Exe
 *
 * @{
 *
 * Execute programs.
 *
 */

/**
 * Execute command and return a string with the output.
 *
 * @param command The command string to execute.
 *
 * @return The string containing the output from given command.
 */
char *
exe_response(const char *command);

/**
 * Execute command and wait for its completion retuning its status.
 *
 * @param command The path of the command to execute.
 * @param args Arguments to be passed to the command.
 *
 * @return The status code of the completed command execution.
 */
int
exe_wait(const char *command, const char *args);

/**
 * Run a command via the shell and return its exit status.
 *
 * @param command The command(s) to execute.
 *
 * @return The status code of the command upon completion.
 */
int
exe_shell(const char *command);

/**
 * @}
 */

#endif
