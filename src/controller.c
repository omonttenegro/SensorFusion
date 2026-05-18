/*
 * Grupo 20
 * André Montenegro, Nº63755
 * Francisco Costa, Nº63691
 * Nicholas Antunes, Nº63783
 */
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
            continue;
        }

        controller_process_measurement(&m, controller_id, info);
        controller_send_measurement(&m, info, buffs);
    }

    return 0;
}

void controller_receive_measurement(MeasurementInfo *m, int controller_id, struct info_container *info, struct buffers *buffs){
    m->m_id = -1;

    sem_wait(info->sems->sensors_controllers->unread);

    if (*(info->terminate) != 0) {
        sem_post(info->sems->sensors_controllers->unread);
        return;
    }

    sem_wait(info->sems->sensors_controllers->mutex);

    read_sensor_controller_buffer(
        buffs->buff_sensors_controllers,
        controller_id,
        info->buffers_size,
        m
    );

    sem_post(info->sems->sensors_controllers->mutex);

    if (m->m_id != -1) {
        sem_post(info->sems->sensors_controllers->free_space);
    } else {
        sem_post(info->sems->sensors_controllers->unread);
    }
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
    
    set_controller_time(&m->change_time);
}

void controller_send_measurement(MeasurementInfo *m, struct info_container *info, struct buffers *buffs){
    m->counter_servers = info->n_servers;

    sem_wait(info->sems->controllers_servers->free_space);

    if (*(info->terminate) != 0) {
        sem_post(info->sems->controllers_servers->free_space);
        return;
    }

    sem_wait(info->sems->controllers_servers->mutex);

    int success = write_controller_servers_buffer(buffs->buff_controllers_servers, info->buffers_size, m);

    sem_post(info->sems->controllers_servers->mutex);

    if (success) {
        for (int i = 0; i < info->n_servers; i++) {
            sem_post(info->sems->controllers_servers->unread);
        }
    } else {
        sem_post(info->sems->controllers_servers->free_space);
    }
}
