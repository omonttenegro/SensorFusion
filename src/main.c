#include "main.h"
#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void main_args(int argc, char *argv[], struct info_container *info)
{
    if (argc != 4) {
        printf("Usage: %s n_sensors n_servers buffers_size\n", argv[0]);
        exit(1);
    }

    info->n_sensors = atoi(argv[1]);
    info->n_servers = atoi(argv[2]);
    info->buffers_size = atoi(argv[3]);

    if (info->n_sensors <= 0 || info->n_servers <= 0 || info->buffers_size <= 0) {
        printf("Invalid arguments\n");
        exit(1);
    }

    if (info->buffers_size < info->n_sensors) {
        printf("buffers_size must be >= n_sensors\n");
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
    info->sensors_pids = allocate_dynamic_memory(sizeof(int) * info->n_sensors);
    info->controllers_pids = allocate_dynamic_memory(sizeof(int) * info->n_sensors);
    info->servers_pids = allocate_dynamic_memory(sizeof(int) * info->n_servers);

    buffs->buff_main_sensors = allocate_dynamic_memory(sizeof(struct circ_buffer));
    buffs->buff_sensors_controllers = allocate_dynamic_memory(sizeof(struct ra_buffer));
    buffs->buff_controllers_servers = allocate_dynamic_memory(sizeof(struct ra_buffer));
}

void create_shared_memory_structs(struct info_container *info, struct buffers *buffs)
{
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
}

void create_processes(struct info_container *info, struct buffers *buffs)
{
    int i;

    for (i = 0; i < info->n_sensors; i++)
        info->sensors_pids[i] = launch_process(SENSOR_PROCESS, i, info, buffs);

    for (i = 0; i < info->n_sensors; i++)
        info->controllers_pids[i] = launch_process(CONTROLLER_PROCESS, i, info, buffs);

    for (i = 0; i < info->n_servers; i++)
        info->servers_pids[i] = launch_process(SERVER_PROCESS, i, info, buffs);
}

void user_interaction(struct info_container *info, struct buffers *buffs)
{
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
            m.state = REQUEST;
            m.m_id = id;
            m.sensor_id = -1;
            m.controller_id = -1;
            m.value = 0;
            m.counter_sensors = info->n_sensors;
            m.counter_servers = 0;

            while (*(info->terminate) == 0) {
                if (write_main_sensors_buffer(buffs->buff_main_sensors, info->buffers_size, &m)) {
                    printf("Measurement cycle %d requested\n", id);
                    id++;
                    break;
                }
                usleep(1000);
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

void write_final_statistics(struct info_container *info)
{
    int i;

    printf("Total measurements: %d\n", *(info->total_measurements));

    for (i = 0; i < info->n_sensors; i++)
        printf("Sensor %d: generated=%d invalid=%d\n", i, info->num_generated_measurements[i], info->num_invalid_measurements[i]);

    for (i = 0; i < info->n_servers; i++)
        printf("Server %d: estimates=%d\n", i, info->num_estimates[i]);
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

    for (i = 0; i < info->n_sensors; i++)
        wait_process(info->sensors_pids[i]);

    for (i = 0; i < info->n_sensors; i++)
        wait_process(info->controllers_pids[i]);

    for (i = 0; i < info->n_servers; i++)
        wait_process(info->servers_pids[i]);
}

void read_estimate(struct info_container *info, int m_id)
{
    (void)info;
    (void)m_id;
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

    for (i = 0; i < info->n_sensors; i++)
        printf("Sensor %d: pid=%d generated=%d invalid=%d\n", i, info->sensors_pids[i], info->num_generated_measurements[i], info->num_invalid_measurements[i]);

    for (i = 0; i < info->n_sensors; i++)
        printf("Controller %d: pid=%d\n", i, info->controllers_pids[i]);

    for (i = 0; i < info->n_servers; i++)
        printf("Server %d: pid=%d estimates=%d\n", i, info->servers_pids[i], info->num_estimates[i]);
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
