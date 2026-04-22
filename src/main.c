#include "main.h"
#include "process.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void main_args(int argc, char *argv[], struct info_container *info)
{
    char *endptr;
    long value;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s n_sensors n_servers buffers_size\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    errno = 0;
    endptr = NULL;
    value = strtol(argv[1], &endptr, 10);
    if (errno != 0 || endptr == argv[1] || *endptr != '\0' || value <= 0 || value > INT_MAX) {
        fprintf(stderr, "Invalid n_sensors\n");
        exit(EXIT_FAILURE);
    }
    info->n_sensors = (int)value;

    errno = 0;
    endptr = NULL;
    value = strtol(argv[2], &endptr, 10);
    if (errno != 0 || endptr == argv[2] || *endptr != '\0' || value <= 0 || value > INT_MAX) {
        fprintf(stderr, "Invalid n_servers\n");
        exit(EXIT_FAILURE);
    }
    info->n_servers = (int)value;

    errno = 0;
    endptr = NULL;
    value = strtol(argv[3], &endptr, 10);
    if (errno != 0 || endptr == argv[3] || *endptr != '\0' || value <= 0 || value > INT_MAX) {
        fprintf(stderr, "Invalid buffers_size\n");
        exit(EXIT_FAILURE);
    }
    info->buffers_size = (int)value;

    if (info->buffers_size < 2) {
        fprintf(stderr, "buffers_size must be at least 2\n");
        exit(EXIT_FAILURE);
    }

    if (info->buffers_size < info->n_sensors) {
        fprintf(stderr, "buffers_size must be >= n_sensors\n");
        exit(EXIT_FAILURE);
    }

    info->sensors_pids = NULL;
    info->controllers_pids = NULL;
    info->servers_pids = NULL;
    info->num_generated_measurements = NULL;
    info->num_invalid_measurements = NULL;
    info->num_estimates = NULL;
    info->total_measurements = NULL;
    info->terminate = NULL;
}

void create_dynamic_memory_structs(struct info_container *info, struct buffers *buffs)
{
    info->sensors_pids = allocate_dynamic_memory((int)(sizeof(int) * info->n_sensors));
    info->controllers_pids = allocate_dynamic_memory((int)(sizeof(int) * info->n_sensors));
    info->servers_pids = allocate_dynamic_memory((int)(sizeof(int) * info->n_servers));

    buffs->buff_main_sensors = allocate_dynamic_memory((int)sizeof(struct circ_buffer));
    buffs->buff_sensors_controllers = allocate_dynamic_memory((int)sizeof(struct ra_buffer));
    buffs->buff_controllers_servers = allocate_dynamic_memory((int)sizeof(struct ra_buffer));

    if (info->sensors_pids == NULL || info->controllers_pids == NULL ||
        info->servers_pids == NULL || buffs->buff_main_sensors == NULL ||
        buffs->buff_sensors_controllers == NULL || buffs->buff_controllers_servers == NULL) {
        fprintf(stderr, "Failed to allocate dynamic memory\n");
        exit(EXIT_FAILURE);
    }
}

void create_shared_memory_structs(struct info_container *info, struct buffers *buffs)
{
    buffs->buff_main_sensors->ptrs = create_shared_memory(ID_SHM_MAIN_SENSORS_PTR,
                                                          (int)sizeof(struct pointers));
    buffs->buff_main_sensors->buffer = create_shared_memory(ID_SHM_MAIN_SENSORS_BUFFER,
                                                            (int)(sizeof(MeasurementInfo) * info->buffers_size));

    buffs->buff_sensors_controllers->ptrs = create_shared_memory(ID_SHM_SENSORS_CONTROLLERS_PTR,
                                                                 (int)(sizeof(int) * info->buffers_size));
    buffs->buff_sensors_controllers->buffer = create_shared_memory(ID_SHM_SENSORS_CONTROLLERS_BUFFER,
                                                                   (int)(sizeof(MeasurementInfo) * info->buffers_size));

    buffs->buff_controllers_servers->ptrs = create_shared_memory(ID_SHM_CONTROLLERS_SERVERS_LATEST,
                                                                 (int)(sizeof(int) * info->buffers_size));
    buffs->buff_controllers_servers->buffer = create_shared_memory(ID_SHM_CONTROLLERS_SERVERS_VERSION,
                                                                   (int)(sizeof(MeasurementInfo) * info->buffers_size));

    info->num_generated_measurements = create_shared_memory(ID_SHM_SENSORS_GENERATED,
                                                            (int)(sizeof(int) * info->n_sensors));
    info->num_invalid_measurements = create_shared_memory(ID_SHM_SENSORS_INVALID,
                                                          (int)(sizeof(int) * info->n_sensors));
    info->num_estimates = create_shared_memory(ID_SHM_SERVERS_PROCESSED,
                                               (int)(sizeof(int) * info->n_servers));
    info->total_measurements = create_shared_memory(ID_SHM_TOTAL_MEASUREMENTS,
                                                    (int)sizeof(int));
    info->terminate = create_shared_memory(ID_SHM_TERMINATE,
                                           (int)sizeof(int));

    *(info->terminate) = 0;
    *(info->total_measurements) = 0;
}

void destroy_dynamic_memory_structs(struct info_container *info, struct buffers *buffs)
{
    deallocate_dynamic_memory(info->sensors_pids);
    deallocate_dynamic_memory(info->controllers_pids);
    deallocate_dynamic_memory(info->servers_pids);

    deallocate_dynamic_memory(buffs->buff_main_sensors);
    deallocate_dynamic_memory(buffs->buff_sensors_controllers);
    deallocate_dynamic_memory(buffs->buff_controllers_servers);
}

void destroy_shared_memory_structs(struct info_container *info, struct buffers *buffs)
{
    destroy_shared_memory(ID_SHM_MAIN_SENSORS_PTR,
                          buffs->buff_main_sensors->ptrs,
                          (int)sizeof(struct pointers));
    destroy_shared_memory(ID_SHM_MAIN_SENSORS_BUFFER,
                          buffs->buff_main_sensors->buffer,
                          (int)(sizeof(MeasurementInfo) * info->buffers_size));

    destroy_shared_memory(ID_SHM_SENSORS_CONTROLLERS_PTR,
                          buffs->buff_sensors_controllers->ptrs,
                          (int)(sizeof(int) * info->buffers_size));
    destroy_shared_memory(ID_SHM_SENSORS_CONTROLLERS_BUFFER,
                          buffs->buff_sensors_controllers->buffer,
                          (int)(sizeof(MeasurementInfo) * info->buffers_size));

    destroy_shared_memory(ID_SHM_CONTROLLERS_SERVERS_LATEST,
                          buffs->buff_controllers_servers->ptrs,
                          (int)(sizeof(int) * info->buffers_size));
    destroy_shared_memory(ID_SHM_CONTROLLERS_SERVERS_VERSION,
                          buffs->buff_controllers_servers->buffer,
                          (int)(sizeof(MeasurementInfo) * info->buffers_size));

    destroy_shared_memory(ID_SHM_SENSORS_GENERATED,
                          info->num_generated_measurements,
                          (int)(sizeof(int) * info->n_sensors));
    destroy_shared_memory(ID_SHM_SENSORS_INVALID,
                          info->num_invalid_measurements,
                          (int)(sizeof(int) * info->n_sensors));
    destroy_shared_memory(ID_SHM_SERVERS_PROCESSED,
                          info->num_estimates,
                          (int)(sizeof(int) * info->n_servers));
    destroy_shared_memory(ID_SHM_TOTAL_MEASUREMENTS,
                          info->total_measurements,
                          (int)sizeof(int));
    destroy_shared_memory(ID_SHM_TERMINATE,
                          info->terminate,
                          (int)sizeof(int));
}

void create_processes(struct info_container *info, struct buffers *buffs)
{
    int i;

    for (i = 0; i < info->n_sensors; i++) {
        info->sensors_pids[i] = launch_process(SENSOR_PROCESS, i, info, buffs);
        if (info->sensors_pids[i] < 0) {
            fprintf(stderr, "Failed to create sensor %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < info->n_sensors; i++) {
        info->controllers_pids[i] = launch_process(CONTROLLER_PROCESS, i, info, buffs);
        if (info->controllers_pids[i] < 0) {
            fprintf(stderr, "Failed to create controller %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < info->n_servers; i++) {
        info->servers_pids[i] = launch_process(SERVER_PROCESS, i, info, buffs);
        if (info->servers_pids[i] < 0) {
            fprintf(stderr, "Failed to create server %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
}

void user_interaction(struct info_container *info, struct buffers *buffs)
{
    char line[128];
    char *command;
    char *arg;
    int next_measurement_id = 1;

    help();

    while (*(info->terminate) == 0) {
        printf(">> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            end_execution(info, buffs);
            return;
        }

        line[strcspn(line, "\n")] = '\0';
        command = strtok(line, " \t");

        if (command == NULL) {
            continue;
        }

        if (strcmp(command, "measure") == 0) {
            MeasurementInfo req;

            req.state = REQUEST;
            req.m_id = next_measurement_id;
            req.sensor_id = -1;
            req.controller_id = -1;
            req.value = 0.0;
            req.counter_sensors = info->n_sensors;
            req.counter_servers = 0;

            while (*(info->terminate) == 0) {
                if (write_main_sensors_buffer(buffs->buff_main_sensors,
                                              info->buffers_size,
                                              &req) == 1) {
                    printf("Measurement cycle %d requested\n", next_measurement_id);
                    next_measurement_id++;
                    break;
                }
                usleep(1000);
            }
        }
        else if (strcmp(command, "read") == 0) {
            char *endptr;
            long value;

            arg = strtok(NULL, " \t");
            if (arg == NULL) {
                printf("Usage: read <m_id>\n");
                continue;
            }

            errno = 0;
            endptr = NULL;
            value = strtol(arg, &endptr, 10);
            if (errno != 0 || endptr == arg || *endptr != '\0' || value <= 0 || value > INT_MAX) {
                printf("Invalid measurement id\n");
                continue;
            }

            read_estimate(info, (int)value);
        }
        else if (strcmp(command, "stat") == 0) {
            print_stat(info);
        }
        else if (strcmp(command, "help") == 0) {
            help();
        }
        else if (strcmp(command, "end") == 0) {
            end_execution(info, buffs);
            return;
        }
        else {
            printf("Unknown command\n");
        }
    }
}

void write_final_statistics(struct info_container *info)
{
    int i;

    printf("Total measurements: %d\n", *(info->total_measurements));

    for (i = 0; i < info->n_sensors; i++) {
        printf("Sensor %d: generated=%d invalid=%d\n",
               i,
               info->num_generated_measurements[i],
               info->num_invalid_measurements[i]);
    }

    for (i = 0; i < info->n_servers; i++) {
        printf("Server %d: estimates=%d\n", i, info->num_estimates[i]);
    }
}

void end_execution(struct info_container *info, struct buffers *buffs)
{
    (void)buffs;

    *(info->terminate) = 1;
    wait_processes(info);
    write_final_statistics(info);
}

void wait_processes(struct info_container *info)
{
    int i;

    for (i = 0; i < info->n_sensors; i++) {
        wait_process(info->sensors_pids[i]);
    }

    for (i = 0; i < info->n_sensors; i++) {
        wait_process(info->controllers_pids[i]);
    }

    for (i = 0; i < info->n_servers; i++) {
        wait_process(info->servers_pids[i]);
    }
}

void read_estimate(struct info_container *info, int m_id)
{
    (void)info;

    if (m_id <= 0) {
        printf("Invalid measurement id\n");
        return;
    }

    printf("Estimate reading is not available in the current implementation\n");
}

void print_stat(struct info_container *info)
{
    int i;

    printf("n_sensors: %d\n", info->n_sensors);
    printf("n_servers: %d\n", info->n_servers);
    printf("buffers_size: %d\n", info->buffers_size);
    printf("total_measurements: %d\n", *(info->total_measurements));
    printf("terminate: %d\n", *(info->terminate));

    for (i = 0; i < info->n_sensors; i++) {
        printf("Sensor %d: pid=%d generated=%d invalid=%d\n",
               i,
               info->sensors_pids[i],
               info->num_generated_measurements[i],
               info->num_invalid_measurements[i]);
    }

    for (i = 0; i < info->n_sensors; i++) {
        printf("Controller %d: pid=%d\n", i, info->controllers_pids[i]);
    }

    for (i = 0; i < info->n_servers; i++) {
        printf("Server %d: pid=%d estimates=%d\n",
               i,
               info->servers_pids[i],
               info->num_estimates[i]);
    }
}

void help(void)
{
    printf("Commands:\n");
    printf("measure\n");
    printf("read <m_id>\n");
    printf("stat\n");
    printf("help\n");
    printf("end\n");
}

int main(int argc, char *argv[])
{
    struct info_container info;
    struct buffers buffs;

    main_args(argc, argv, &info);
    create_dynamic_memory_structs(&info, &buffs);
    create_shared_memory_structs(&info, &buffs);
    create_processes(&info, &buffs);
    user_interaction(&info, &buffs);
    destroy_shared_memory_structs(&info, &buffs);
    destroy_dynamic_memory_structs(&info, &buffs);

    return 0;
}
