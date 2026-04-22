#include <stdlib.h>
#include "controller.h"
#include "memory.h"
#include "random_measurement.h"
#include <unistd.h>

int execute_controller(int controller_id, struct info_container *info, struct buffers *buffs){
    MeasurementInfo m;

    while (!(*info->terminate)) {
        controller_receive_measurement(&m, controller_id, info, buffs);

        if (m.m_id == -1) {
            usleep(1000);
            continue;
        }

        controller_process_measurement(&m, controller_id, info);
        controller_send_measurement(&m, info, buffs);
    }

    return 0;
}

void controller_receive_measurement(MeasurementInfo *m, int controller_id, struct info_container *info, struct buffers *buffs){
    read_sensor_controller_buffer(buffs->buff_sensors_controllers, controller_id, info->buffers_size, m);
}
void controller_process_measurement(MeasurementInfo *m, int controller_id, struct info_container *info){
   m->controller_id = controller_id;

    if (!is_valid_measurement(m->value)) {
        m->state = INVALID;

        info->num_invalid_measurements[m->sensor_id]++;
    }
    else {
        m->state = VALID;
    }
}
void controller_send_measurement(MeasurementInfo *m, struct info_container *info, struct buffers *buffs){
    while (!(*info->terminate)) {

        int success = write_controller_servers_buffer(buffs->buff_controllers_servers,info->buffers_size,m);

        if (success) {
            break;
        }
    }
}
