#include "main.h"
#include "process.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int parse_positive_int_or_fail(const char *text, const char *name)
{
    char *endptr = NULL;
    long value;

    errno = 0;
    value = strtol(text, &endptr, 10);

    if (errno != 0 || endptr == text || *endptr != '\0' || value <= 0) {
        fprintf(stderr, "Invalid %s: %s\n", name, text);
        exit(1);
    }

    if (value > 2147483647L) {
        fprintf(stderr, "%s is too large: %s\n", name, text);
        exit(1);
    }

    return (int)value;
}

static int parse_positive_int_safe(const char *text, int *value)
{
    char *endptr = NULL;
    long parsed;

    errno = 0;
    parsed = strtol(text, &endptr, 10);

    if (errno != 0 || endptr == text || *endptr != '\0' || parsed <= 0 || parsed > 2147483647L) {
        return 0;
    }

    *value = (int)parsed;
    return 1;
}

static void issue_measure_request(struct info_container *info, struct buffers *buffs, int m_id)
{
    MeasurementInfo req;

    req.state = REQUEST;
    req.m_id = m_id;
    req.sensor_id = -1;
    req.controller_id = -1;
    req.value = 0.0;
    req.counter_sensors = info->n_sensors;
    req.counter_servers = 0;

    while (*(info->terminate) == 0) {
        if (write_main_sensors_buffer(buffs->buff_main_sensors, info->buffers_size, &req) == 1) {
            printf("Measurement cycle %d requested.\n", m_id);
            return;
        }
        usleep(1000);
    }
}

void main_args(int argc, char *argv[], struct info_container *info)
{
    if (info->n_sensors <= 0 || info->n_servers <= 0 || info->buffers_size <= 0) {
        fprintf(stderr, "Invalid arguments\n");
        exit(EXIT_FAILURE);
    }

    if (info->buffers_size < info->n_sensors) {
        fprintf(stderr, "buffers_size must be >= n_sensors\n");
        exit(EXIT_FAILURE);
    }
    
    if (argc != 4) {
        fprintf(stderr, "Usage: %s n_sensors n_servers buff_size\n", argv[0]);
        exit(1);
    }

    info->n_sensors = parse_positive_int_or_fail(argv[1], "n_sensors");
    info->n_servers = parse_positive_int_or_fail(argv[2], "n_servers");
    info->buffers_size = parse_positive_int_or_fail(argv[3], "buff_size");

    if (info->buffers_size < 2) {
        fprintf(stderr, "buff_size must be at least 2.\n");
        exit(1);
    }

    if (info->buffers_size < info->n_sensors) {
        fprintf(stderr, "buff_size must be at least n_sensors with the current RA-buffer implementation.\n");
        exit(1);
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
        fprintf(stderr, "Failed to allocate dynamic memory.\n");
        exit(1);
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
    info->total_measurements = create_shared_memory(ID_SHM_TOTAL_MEASUREMENTS, (int)sizeof(int));
    info->terminate = create_shared_memory(ID_SHM_TERMINATE, (int)sizeof(int));

    *(info->terminate) = 0;
    *(info->total_measurements) = 0;
}

void destroy_dynamic_memory_structs(struct info_container *info, struct buffers *buffs)
{
    if (info != NULL) {
        deallocate_dynamic_memory(info->sensors_pids);
        deallocate_dynamic_memory(info->controllers_pids);
        deallocate_dynamic_memory(info->servers_pids);
        deallocate_dynamic_memory(info);
    }

    if (buffs != NULL) {
        deallocate_dynamic_memory(buffs->buff_main_sensors);
        deallocate_dynamic_memory(buffs->buff_sensors_controllers);
        deallocate_dynamic_memory(buffs->buff_controllers_servers);
        deallocate_dynamic_memory(buffs);
    }
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
            fprintf(stderr, "Failed to create sensor %d.\n", i);
            exit(1);
        }
    }

    for (i = 0; i < info->n_sensors; i++) {
        info->controllers_pids[i] = launch_process(CONTROLLER_PROCESS, i, info, buffs);
        if (info->controllers_pids[i] < 0) {
            fprintf(stderr, "Failed to create controller %d.\n", i);
            exit(1);
        }
    }

    for (i = 0; i < info->n_servers; i++) {
        info->servers_pids[i] = launch_process(SERVER_PROCESS, i, info, buffs);
        if (info->servers_pids[i] < 0) {
            fprintf(stderr, "Failed to create server %d.\n", i);
            exit(1);
        }
    }
}

void read_estimate(struct info_container *info, int m_id)
{
    (void)info;

    if (m_id <= 0) {
        printf("Invalid measurement cycle.\n");
        return;
    }

    printf("The current code does not persist estimates, so cycle %d cannot be retrieved yet.\n", m_id);
}

void print_stat(struct info_container *info)
{
    int i;

    printf("\n=== SOestimate status ===\n");
    printf("n_sensors: %d\n", info->n_sensors);
    printf("n_servers: %d\n", info->n_servers);
    printf("buffers_size: %d\n", info->buffers_size);
    printf("total_measurements: %d\n", *(info->total_measurements));
    printf("terminate: %d\n", *(info->terminate));

    printf("\nSensors:\n");
    for (i = 0; i < info->n_sensors; i++) {
        printf("  sensor[%d]: pid=%d generated=%d invalid=%d\n",
               i,
               info->sensors_pids[i],
               info->num_generated_measurements[i],
               info->num_invalid_measurements[i]);
    }

    printf("\nControllers:\n");
    for (i = 0; i < info->n_sensors; i++) {
        printf("  controller[%d]: pid=%d\n", i, info->controllers_pids[i]);
    }

    printf("\nServers:\n");
    for (i = 0; i < info->n_servers; i++) {
        printf("  server[%d]: pid=%d estimates=%d\n",
               i,
               info->servers_pids[i],
               info->num_estimates[i]);
    }

    printf("========================\n\n");
}

void help(void)
{
    printf("Available commands:\n");
    printf("  measure          - starts a new measurement cycle\n");
    printf("  read <m_id>      - tries to read the estimate of cycle <m_id>\n");
    printf("  stat             - shows current system state\n");
    printf("  help             - shows this help message\n");
    printf("  end              - terminates the system\n");
}

void write_final_statistics(struct info_container *info)
{
    int i;

    printf("\n=== Final statistics ===\n");
    printf("Total generated measurements: %d\n", *(info->total_measurements));

    for (i = 0; i < info->n_sensors; i++) {
        printf("Sensor %d -> generated: %d | invalid: %d\n",
               i,
               info->num_generated_measurements[i],
               info->num_invalid_measurements[i]);
    }

    for (i = 0; i < info->n_servers; i++) {
        printf("Server %d -> estimates: %d\n", i, info->num_estimates[i]);
    }

    printf("========================\n");
}

void wait_processes(struct info_container *info)
{
    int i;

    for (i = 0; i < info->n_sensors; i++) {
        if (info->sensors_pids[i] > 0) {
            wait_process(info->sensors_pids[i]);
        }
    }

    for (i = 0; i < info->n_sensors; i++) {
        if (info->controllers_pids[i] > 0) {
            wait_process(info->controllers_pids[i]);
        }
    }

    for (i = 0; i < info->n_servers; i++) {
        if (info->servers_pids[i] > 0) {
            wait_process(info->servers_pids[i]);
        }
    }
}

void end_execution(struct info_container *info, struct buffers *buffs)
{
    (void)buffs;

    if (*(info->terminate) == 0) {
        *(info->terminate) = 1;
    }

    wait_processes(info);
    write_final_statistics(info);
}

void user_interaction(struct info_container *info, struct buffers *buffs)
{
    char line[128];
    int next_measurement_id = 1;

    help();

    while (*(info->terminate) == 0) {
        char *command;
        char *arg;

        printf(">> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nEnd of input.\n");
            end_execution(info, buffs);
            return;
        }

        line[strcspn(line, "\n")] = '\0';
        command = strtok(line, " \t");

        if (command == NULL) {
            continue;
        }

        if (strcmp(command, "measure") == 0) {
            issue_measure_request(info, buffs, next_measurement_id);
            next_measurement_id++;
        }
        else if (strcmp(command, "read") == 0) {
            int m_id;
            arg = strtok(NULL, " \t");
            if (arg == NULL) {
                printf("Usage: read <m_id>\n");
            }
            else if (!parse_positive_int_safe(arg, &m_id)) {
                printf("Invalid m_id.\n");
            }
            else {
                read_estimate(info, m_id);
            }
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
            printf("Unknown command. Type 'help' to see the available commands.\n");
        }
    }
}

int main(int argc, char *argv[])
{
    struct info_container *info = allocate_dynamic_memory((int)sizeof(struct info_container));
    struct buffers *buffs = allocate_dynamic_memory((int)sizeof(struct buffers));

    if (info == NULL || buffs == NULL) {
        fprintf(stderr, "Failed to allocate main structures.\n");
        return 1;
    }

    main_args(argc, argv, info);
    create_dynamic_memory_structs(info, buffs);
    create_shared_memory_structs(info, buffs);
    create_processes(info, buffs);
    user_interaction(info, buffs);
    destroy_shared_memory_structs(info, buffs);
    destroy_dynamic_memory_structs(info, buffs);

    return 0;
}