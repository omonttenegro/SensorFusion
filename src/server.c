/*
 * Grupo 20
 * André Montenegro, Nº63755
 * Francisco Costa, Nº63691
 * Nicholas Antunes, Nº63783
 */
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int execute_server(int server_id, struct info_container *info, struct buffers *buffs) {
    int expected_m_id = 1;
    double estimate = 0.0;
    int valid_count = 0;

    int *received_flags = (int *)calloc((size_t)info->n_sensors, sizeof(int));
    if (received_flags == NULL) {
        perror("calloc received_flags");
        return -1;
    }

    while (*(info->terminate) == 0) {
        MeasurementInfo m;
        read_expected_cycle_measurement(&m, expected_m_id, received_flags, info, buffs);

        if (m.m_id == -1) {
            usleep(1000);
            continue;
        }

        if (m.controller_id >= 0 && m.controller_id < info->n_sensors) {
            received_flags[m.controller_id] = 1;
        }

        if (server_process_measurement(&m, server_id, info, &estimate, &valid_count) == 1) {
            server_print_estimate(server_id, expected_m_id, estimate, valid_count);

            if (info->num_estimates != NULL) {
                info->num_estimates[server_id]++;
            }

            expected_m_id++;
            estimate = 0.0;
            valid_count = 0;
            for (int i = 0; i < info->n_sensors; i++) {
                received_flags[i] = 0;
            }
        }
    }

    free(received_flags);
    return 0;
}

void read_expected_cycle_measurement(MeasurementInfo *m, int expected_m_id, const int *received_flags, struct info_container *info, struct buffers *buffs) {
    m->m_id = -1;

    for (int i = 0; i < info->n_sensors; i++) {
        if (received_flags[i] != 0) {
            continue;
        }

        read_controller_servers_buffer(
            buffs->buff_controllers_servers,
            info->buffers_size,
            expected_m_id,
            i,
            m
        );

        if (m->m_id != -1) {
            return;
        }
    }
}

int server_process_measurement(MeasurementInfo *m, int server_id, struct info_container *info, double *estimate, int *valid_count) {
    static int processed_in_cycle = 0;
    static int tracked_cycle = -1;

    (void)server_id;

    if (tracked_cycle != m->m_id) {
        tracked_cycle = m->m_id;
        processed_in_cycle = 0;
    }

    if (m->state == VALID) {
        *estimate += m->value;
        (*valid_count)++;
    }

    processed_in_cycle++;

    if (processed_in_cycle >= info->n_sensors) {
        if (*valid_count > 0) {
            *estimate = *estimate / (double)(*valid_count);
        } else {
            *estimate = 0.0;
        }

        processed_in_cycle = 0;
        tracked_cycle = -1;
        return 1;
    }

    return 0;
}

void server_print_estimate(int server_id, int m_id, double estimate, int valid_count) {
    if (valid_count == 0) {
        printf("SERVER %d CYCLE %d NULL\n", server_id, m_id);
    } else {
        printf("SERVER %d CYCLE %d %.6f\n", server_id, m_id, estimate);
    }
    fflush(stdout);
}
