#include "memory.h"
#include "main.h"
#include "random_measurement.h"
#include "sensor.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> 


/* Function that reads the next request from the main->sensors circular buffer.
 * req receives the MeasurementInfo request data, or m_id = -1 when no request exists.
 * info provides runtime configuration and terminate flag.
 * buffs provides access to shared buffers.
 */
void sensor_receive_request(MeasurementInfo *req, int expected_m_id, struct info_container *info, struct buffers *buffs){
    read_main_sensors_buffer(buffs->buff_main_sensors, info->buffers_size, expected_m_id, req);
}

/* Function that creates one measurement from a received request.
 * m receives the generated measurement data.
 * sensor_id identifies the source sensor.
 * info provides shared counters updated by the sensor.
 */
void sensor_process_request(MeasurementInfo *m, int sensor_id, struct info_container *info){

    m->state = MEASURED;
    m->sensor_id = sensor_id;
    m->controller_id = -1;
    m->value = get_measurement();
    m->counter_servers = info->n_servers;

    if (info->num_generated_measurements != NULL) {
        info->num_generated_measurements[sensor_id]++;
    }

    if (info->total_measurements != NULL) {
        (*(info->total_measurements))++;
    }
}

/* Function that sends one generated measurement to the sensor->controller buffer.
 * m is the measurement to send.
 * info provides buffer size and terminate flag.
 * buffs provides access to shared buffers.
 */
void sensor_send_measurement(MeasurementInfo *m, struct info_container *info, struct buffers *buffs)
{
    while (*(info->terminate) == 0) {
        int success = write_sensor_controller_buffer(buffs->buff_sensors_controllers,info->buffers_size, m);

        if (success) {
            return;
        }
        
        usleep(1000);
    }
}

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
int execute_sensor(int sensor_id, struct info_container *info, struct buffers *buffs){

    int expected_m_id = 1;

    seed_sensor_rng(sensor_id);

    while (*(info->terminate) == 0) {
        MeasurementInfo m;

        sensor_receive_request(&m, expected_m_id, info, buffs);

        if (m.m_id == -1) {
            usleep(1000);
            continue;
        }

        sensor_process_request(&m, sensor_id, info);
        sensor_send_measurement(&m, info, buffs);
        expected_m_id++;
    }

    return 0;
}