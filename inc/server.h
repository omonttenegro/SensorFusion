/*
 * Grupo 20
 * André Montenegro, Nº63755
 * Francisco Costa, Nº63691
 * Nicholas Antunes, Nº63783
 */
#ifndef SERVER_H_GUARD
#define SERVER_H_GUARD

/* WARNING: DO NOT MODIFY THIS FILE
 * Header files may be replaced by the teachers during evaluation.
 * If required definitions are changed, the project may fail to compile.
 */

#include "memory.h"
#include "main.h"

/* Function executed by one Server process.
 * server_id identifies which server is running.
 * info provides shared flags, configuration, and statistics.
 * buffs provides access to shared communication buffers.
 * The server caches controller measurements for the current cycle and
 * only computes the estimate after all controllers contributed.
 */
int execute_server(int server_id, struct info_container *info, struct buffers *buffs);

/* Function that reads one measurement for this server for the current expected cycle.
 * m receives the measurement for expected_m_id, or m_id = -1 if no unread
 * measurement for that cycle is currently available.
 * received_flags tells which controller measurements for expected_m_id were already read.
 * info provides runtime configuration and termination flag.
 * buffs provides access to shared buffers.
 */
void read_expected_cycle_measurement(MeasurementInfo *m, int expected_m_id, const int *received_flags, struct info_container *info, struct buffers *buffs);

/* Function that updates server aggregation state with one measurement.
 * m is the received measurement.
 * server_id identifies the server that is processing m.
 * info provides controller count and server statistics.
 * estimate receives the computed estimate when it becomes ready.
 * valid_count receives how many valid measurements were used,
 * so the server can average only valid measurements and avoid division by zero.
 * Returns 1 when a complete estimate is ready, otherwise returns 0.
 */
int server_process_measurement(MeasurementInfo *m, int server_id, struct info_container *info, double *estimate, int *valid_count);

/* Function that prints one completed estimate on the terminal.
 * m_id identifies the measurement cycle being reported.
 * estimate is the computed estimate.
 * valid_count is how many valid measurements were used.
 */
void server_print_estimate(int server_id, int m_id, double estimate, int valid_count);

#endif
