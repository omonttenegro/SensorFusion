/*
 * Grupo 20
 * André Montenegro, Nº63755
 * Francisco Costa, Nº63691
 * Nicholas Antunes, Nº63783
 */

#define MAX_MEASUREMENTS 10000;
#include "main.h"
#include "process.h"
#include "ctime.h"
#include "csettings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

MeasurementInfo *measurement_history;
static struct info_container *global_info;
static struct buffers *global_buffers;

void main_args(int argc, char *argv[], struct info_container *info) {
    read_settings(argc, argv, info);
}


void create_dynamic_memory_structs(struct info_container *info, struct buffers *buffs){
    //são locais pois só o main precisa de saber os pids para depois fazer wait(pid)
    info->sensors_pids = allocate_dynamic_memory(sizeof(int) * info->n_sensors);
    info->controllers_pids = allocate_dynamic_memory(sizeof(int) * info->n_sensors);
    info->servers_pids = allocate_dynamic_memory(sizeof(int) * info->n_servers);

    buffs->buff_main_sensors = allocate_dynamic_memory(sizeof(struct circ_buffer));
    buffs->buff_sensors_controllers = allocate_dynamic_memory(sizeof(struct ra_buffer));
    buffs->buff_controllers_servers= allocate_dynamic_memory(sizeof(struct ra_buffer));
}

void create_shared_memory_structs(struct info_container *info, struct buffers *buffs){
    buffs->buff_main_sensors->ptrs = create_shared_memory(ID_SHM_MAIN_SENSORS_PTR, sizeof(struct pointers));
    buffs->buff_main_sensors->buffer = create_shared_memory(ID_SHM_MAIN_SENSORS_BUFFER, sizeof(MeasurementInfo) * info->buffers_size);

    buffs->buff_sensors_controllers->ptrs = create_shared_memory(ID_SHM_SENSORS_CONTROLLERS_PTR, sizeof(int) * info->buffers_size);
    buffs->buff_sensors_controllers->buffer = create_shared_memory(ID_SHM_SENSORS_CONTROLLERS_BUFFER, sizeof(MeasurementInfo) * info->buffers_size);

    buffs->buff_controllers_servers->ptrs = create_shared_memory(ID_SHM_CONTROLLERS_SERVERS_LATEST, sizeof(int) * info->buffers_size);
    buffs->buff_controllers_servers->buffer = create_shared_memory(ID_SHM_CONTROLLERS_SERVERS_VERSION, sizeof(MeasurementInfo) * info->buffers_size);

    info->num_generated_measurements = create_shared_memory(ID_SHM_SENSORS_GENERATED, sizeof(int) * info->n_sensors);
    info->num_invalid_measurements = create_shared_memory(ID_SHM_SENSORS_INVALID, sizeof(int) * info->n_sensors);
    info->num_estimates = create_shared_memory(ID_SHM_SERVERS_PROCESSED, sizeof(int) * info->n_servers);
    info->total_measurements = create_shared_memory(ID_SHM_TOTAL_MEASUREMENTS, sizeof(int));
    info->terminate = create_shared_memory(ID_SHM_TERMINATE, sizeof(int));
    measurement_history = create_shared_memory("SHM_MEASUREMENT_HISTORY", sizeof(MeasurementInfo)* info->buffers_size);

}

void destroy_dynamic_memory_structs(struct info_container *info, struct buffers *buffs){
    deallocate_dynamic_memory(info->sensors_pids);
    deallocate_dynamic_memory(info->controllers_pids);
    deallocate_dynamic_memory(info->servers_pids);

    deallocate_dynamic_memory(buffs->buff_main_sensors);
    deallocate_dynamic_memory(buffs->buff_sensors_controllers);
    deallocate_dynamic_memory(buffs->buff_controllers_servers);
}

void destroy_shared_memory_structs(struct info_container *info, struct buffers *buffs){
    destroy_shared_memory(ID_SHM_MAIN_SENSORS_PTR, buffs->buff_main_sensors->ptrs, sizeof(struct pointers));
    destroy_shared_memory(ID_SHM_MAIN_SENSORS_BUFFER, buffs->buff_main_sensors->buffer, sizeof(MeasurementInfo) * info->buffers_size);

    destroy_shared_memory(ID_SHM_SENSORS_CONTROLLERS_PTR, buffs->buff_sensors_controllers->ptrs, sizeof(int) * info->buffers_size);
    destroy_shared_memory(ID_SHM_SENSORS_CONTROLLERS_BUFFER, buffs->buff_sensors_controllers->buffer, sizeof(MeasurementInfo) * info->buffers_size);

    destroy_shared_memory(ID_SHM_CONTROLLERS_SERVERS_LATEST, buffs->buff_controllers_servers->ptrs, sizeof(int) * info->buffers_size);
    destroy_shared_memory(ID_SHM_CONTROLLERS_SERVERS_VERSION, buffs->buff_controllers_servers->buffer, sizeof(MeasurementInfo) * info->buffers_size);

    destroy_shared_memory(ID_SHM_SENSORS_GENERATED, info->num_generated_measurements, sizeof(int) * info->n_sensors);
    destroy_shared_memory(ID_SHM_SENSORS_INVALID, info->num_invalid_measurements, sizeof(int) * info->n_sensors);
    destroy_shared_memory(ID_SHM_SERVERS_PROCESSED, info->num_estimates, sizeof(int) * info->n_servers);
    destroy_shared_memory(ID_SHM_TOTAL_MEASUREMENTS, info->total_measurements, sizeof(int));
    destroy_shared_memory(ID_SHM_TERMINATE, info->terminate, sizeof(int));
    destroy_shared_memory("SHM_MEASUREMENT_HISTORY", measurement_history, sizeof(MeasurementInfo) * info->buffers_size);

}

void create_processes(struct info_container *info, struct buffers *buffs){
    int i;

    for (i = 0; i < info->n_sensors; i++)
        info->sensors_pids[i] = launch_process(SENSOR_PROCESS, i, info, buffs);

    for (i = 0; i < info->n_sensors; i++)
        info->controllers_pids[i] = launch_process(CONTROLLER_PROCESS, i, info, buffs);

    for (i = 0; i < info->n_servers; i++)
        info->servers_pids[i] = launch_process(SERVER_PROCESS, i, info, buffs);
}

void user_interaction(struct info_container *info, struct buffers *buffs){
    char linha[100];
    int id = 1;

    help();

    while (*(info->terminate) == 0) {
        printf(">> ");
        fflush(stdout);

        if (fgets(linha, sizeof(linha), stdin) == NULL) {
            printf("\n");
            end_execution(info, buffs);
            return;
        }

        linha[strcspn(linha, "\n")] = '\0';

        if (strcmp(linha, "measure") == 0) {
            MeasurementInfo m;
            init_timestamps(&m.change_time);
            m.state = REQUEST;
            m.m_id = id;
            m.sensor_id = -1;
            m.controller_id = -1;
            m.value = 0;
            m.counter_sensors = info->n_sensors;
            m.counter_servers = 0;
            set_main_time(&m.change_time);
            measurement_history[id-1] = m;
            sem_wait(info->sems->main_sensors->free_space);
            sem_wait(info->sems->main_sensors->mutex);
            
            write_main_sensors_buffer(buffs->buff_main_sensors, info->buffers_size, &m);

            sem_post(info->sems->main_sensors->mutex);

            for (int i = 0; i < info->n_sensors; i++) {
                sem_post(info->sems->main_sensors->unread);
            }
        }
        else if (strncmp(linha, "read", 4) == 0) {
            int m_id;
            if (sscanf(linha, "read %d", &m_id) == 1)
                read_estimate(info, m_id);
            else
                printf("Usage: read <m_id>\n");
        }
        else if (strcmp(linha, "stat") == 0) {
            print_stat(info);
        }
        else if (strcmp(linha, "help") == 0) {
            help();
        }
        else if (strcmp(linha, "end") == 0) {
            end_execution(info, buffs);
            return;
        }
        else if (strlen(linha) > 0) {
            printf("Unknown command\n");
        }
    }
}

void write_final_statistics(struct info_container *info){
    int i;

    printf("Total measurements: %d\n", *(info->total_measurements));

    for (i = 0; i < info->n_sensors; i++)
        printf("Sensor %d: generated=%d invalid=%d\n", i, info->num_generated_measurements[i], info->num_invalid_measurements[i]);

    for (i = 0; i < info->n_servers; i++)
        printf("Server %d: estimates=%d\n", i, info->num_estimates[i]);
}

void end_execution(struct info_container *info, struct buffers *buffs){
    (void)buffs;
    sem_wait(info->sems->terminate_mutex);
    *(info->terminate) = 1;
    sem_post(info->sems->terminate_mutex);
    wakeup_processes(info);
    wait_processes(info);
    write_final_statistics(info);
}

void wait_processes(struct info_container *info){
    int i;

    for (i = 0; i < info->n_sensors; i++)
        wait_process(info->sensors_pids[i]);

    for (i = 0; i < info->n_sensors; i++)
        wait_process(info->controllers_pids[i]);

    for (i = 0; i < info->n_servers; i++)
        wait_process(info->servers_pids[i]);
}

void read_estimate(struct info_container *info, int m_id){
    (void)info;
    (void)m_id;
    printf("Estimate reading is not available in the current implementation\n");
}

void print_stat(struct info_container *info){
    int i;

    printf("n_sensors: %d\n", info->n_sensors);
    printf("n_servers: %d\n", info->n_servers);
    printf("buffers_size: %d\n", info->buffers_size);
    printf("total_measurements: %d\n", *(info->total_measurements));
    printf("terminate: %d\n", *(info->terminate));

    for (i = 0; i < info->n_sensors; i++)
        printf("Sensor %d: pid=%d generated=%d invalid=%d\n", i, info->sensors_pids[i], info->num_generated_measurements[i], info->num_invalid_measurements[i]);

    for (i = 0; i < info->n_sensors; i++)
        printf("Controller %d: pid=%d\n", i, info->controllers_pids[i]);

    for (i = 0; i < info->n_servers; i++)
        printf("Server %d: pid=%d estimates=%d\n", i, info->servers_pids[i], info->num_estimates[i]);
}

void help(void){
    printf("Commands:\n");
    printf("measure\n");
    printf("read <m_id>\n");
    printf("stat\n");
    printf("help\n");
    printf("end\n");
}

int main(int argc, char *argv[]){
    struct info_container info;
    struct buffers buffs;

    main_args(argc, argv, &info);
    create_dynamic_memory_structs(&info, &buffs);
    create_shared_memory_structs(&info, &buffs);
    info.sems = create_all_semaphores(info.buffers_size);
    create_processes(&info, &buffs);
    user_interaction(&info, &buffs);
    destroy_all_semaphores(info.sems);
    destroy_shared_memory_structs(&info, &buffs);
    destroy_dynamic_memory_structs(&info, &buffs);

    return 0;
}
