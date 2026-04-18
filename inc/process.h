#ifndef PROCESS_H_GUARD
#define PROCESS_H_GUARD

/* WARNING: DO NOT MODIFY THIS FILE
 * Header files may be replaced by the teachers during evaluation.
 * If required definitions are changed, the project may fail to compile.
 */

#include "memory.h"
#include "main.h"

/* Identifies which child routine launch_process() should run. */
enum process_type
{
    SENSOR_PROCESS,
    CONTROLLER_PROCESS,
    SERVER_PROCESS
};

/* Function that starts a new process using fork().
 * process_type selects whether the child executes execute_sensor(),
 * execute_controller(), or execute_server().
 * launch_process() returns the PID of the created child process.
 */
int launch_process(enum process_type process_type, int process_id, struct info_container *info, struct buffers *buffs);

/* Function that waits for a process with PID process_id using waitpid().
 * Returns the process exit status when the process ends normally.
 */
int wait_process(int process_id);

#endif
