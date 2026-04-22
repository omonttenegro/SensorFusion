/*
 * Grupo 20
 * André Montenegro, Nº63755
 * Francisco Costa, Nº63691
 * Nicholas Antunes, Nº63783
 */
#ifndef CONTROLLER_H_GUARD
#define CONTROLLER_H_GUARD

#include "memory.h"
#include "main.h"

/*
 * Function executed by one controller process.
 * controller_id identifies which controller is running.
 * info provides shared flags and statistics.
 * buffs provides access to shared communication buffers.
 */
int execute_controller(int controller_id, struct info_container *info, struct buffers *buffs);

/*
 * Function that reads one measurement for this controller from
 * the sensor->controller shared buffer.
 * m receives the read measurement (or an empty marker when no data exists).
 * controller_id identifies which paired sensor/controller message to read.
 * info provides runtime configuration and terminate flag.
 * buffs provides access to shared buffers.
 */
void controller_receive_measurement(MeasurementInfo *m, int controller_id, struct info_container *info, struct buffers *buffs);

/*
 * Function that validates a measurement value and updates statistics.
 * m is the measurement to validate, and its state field is updated.
 * controller_id identifies which controller is validating the measurement.
 * info stores the shared invalid-measurement counters updated by the controller.
 */
void controller_process_measurement(MeasurementInfo *m, int controller_id, struct info_container *info);

/*
 * Function that sends one processed measurement to the shared controller->server buffer.
 * m is the measurement to publish.
 * info provides the shared buffer size and the termination flag.
 * buffs provides access to shared buffers.
 */
void controller_send_measurement(MeasurementInfo *m, struct info_container *info, struct buffers *buffs);

#endif
