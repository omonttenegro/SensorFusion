/*
 * Grupo 20
 * André Montenegro, Nº63755
 * Francisco Costa, Nº63691
 * Nicholas Antunes, Nº63783
 */
#ifndef CSIGNAL_H_GUARD
#define CSIGNAL_H_GUARD

#include <signal.h>
#include "main.h"
#include "memory.h"

/*
 * Sets up signal handlers for SIGINT and SIGALRM signals.
 * info provides shared flags and statistics for signal handlers to access.
 * buffs provides shared buffers for signal handlers to access.
 */
void setup_signal_handlers(struct info_container *info, struct buffers *buffs);

/*
 * Signal handler for SIGINT (interrupt signal from Ctrl+C).
 * Initiates graceful shutdown of the system.
 * signal contains the signal number received.
 */
void sigint_handler(int signal);

/*
 * Signal handler for SIGALRM (alarm signal).
 * Prints periodic measurement statistics during execution.
 * signal contains the signal number received.
 */
void sigalrm_handler(int signal);

/*
 * Sets up a periodic alarm to trigger every period seconds.
 * period specifies the alarm interval in seconds.
 */
void setup_alarm(unsigned int period);

/*
 * Resets an existing alarm to a new period.
 * period specifies the new alarm interval in seconds.
 */
void reset_alarm(unsigned int period);

/*
 * Configures the process to ignore SIGCHLD, SIGPIPE, and other signals
 * to avoid interruptions during normal operation.
 */
void ignore_signals(void);

/*
 * Prints the current status of measurement processing.
 * info provides shared statistics about generated and invalid measurements.
 * buffers provides access to shared buffers for status reporting.
 */
void print_measurement_status(struct info_container *info, struct buffers *buffers);

#endif