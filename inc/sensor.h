#ifndef SENSOR_H_GUARD
#define SENSOR_H_GUARD

/* WARNING: DO NOT MODIFY THIS FILE
 * Header files may be replaced by the teachers during evaluation.
 * If required definitions are changed, the project may fail to compile.
 */

#include "memory.h"
#include "main.h"

/* Function executed by one Sensor process.
 * sensor_id identifies which sensor is running.
 * info provides shared flags and statistics.
 * buffs provides access to shared communication buffers.
 *
 * Notice: when this function starts, seed_sensor_rng(sensor_id) must be
 * called to initialize that sensor's random number generator state.
 * Otherwise, different sensor processes may generate the same random
 * sequence after fork().
 */
int execute_sensor(int sensor_id, struct info_container *info, struct buffers *buffs);

/* Function that reads the next request from the main->sensors circular buffer.
 * req receives the MeasurementInfo request data, or m_id = -1 when no request exists.
 * info provides runtime configuration and terminate flag.
 * buffs provides access to shared buffers.
 */
void sensor_receive_request(MeasurementInfo *req, int expected_m_id, struct info_container *info, struct buffers *buffs);

/* Function that creates one measurement from a received request.
 * m receives the generated measurement data.
 * sensor_id identifies the source sensor.
 * info provides shared counters updated by the sensor.
 */
void sensor_process_request(MeasurementInfo *m, int sensor_id, struct info_container *info);

/* Function that sends one generated measurement to the sensor->controller buffer.
 * m is the measurement to send.
 * info provides buffer size and terminate flag.
 * buffs provides access to shared buffers.
 */
void sensor_send_measurement(MeasurementInfo *m, struct info_container *info, struct buffers *buffs);

#endif
