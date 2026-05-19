#include "csettings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void initialize_info(struct info_container *info)
{
    info->n_sensors = 0;
    info->n_servers = 0;
    info->buffers_size = 0;

    info->sensors_pids = NULL;
    info->controllers_pids = NULL;
    info->servers_pids = NULL;

    info->num_generated_measurements = NULL;
    info->num_invalid_measurements = NULL;
    info->num_estimates = NULL;

    info->total_measurements = NULL;
    info->terminate = NULL;
    info->last_logged_id = NULL;

    info->sems = NULL;

    info->log_filename[0] = '\0';
    info->statistics_filename[0] = '\0';

    info->period = 0;
}

static void read_args_file(const char *filename, struct info_container *info)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Failed to open args file %s\n", filename);
        exit(1);
    }

    if (fscanf(file, "%d", &info->n_sensors) != 1) {
        fprintf(stderr, "Failed to read n_sensors from %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fscanf(file, "%d", &info->n_servers) != 1) {
        fprintf(stderr, "Failed to read n_servers from %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fscanf(file, "%d", &info->buffers_size) != 1) {
        fprintf(stderr, "Failed to read buffers_size from %s\n", filename);
        fclose(file);
        exit(1);
    }

    fclose(file);
}

static void read_settings_file(const char *filename, struct info_container *info)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        fprintf(stderr, "Failed to open settings file %s\n", filename);
        exit(1);
    }

    if (fscanf(file, "%99s", info->log_filename) != 1) {
        fprintf(stderr, "Failed to read log filename from %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fscanf(file, "%99s", info->statistics_filename) != 1) {
        fprintf(stderr, "Failed to read statistics filename from %s\n", filename);
        fclose(file);
        exit(1);
    }

    if (fscanf(file, "%u", &info->period) != 1) {
        fprintf(stderr, "Failed to read period from %s\n", filename);
        fclose(file);
        exit(1);
    }

    fclose(file);
}

static void validate_settings(struct info_container *info)
{
    if (info->n_sensors <= 0) {
        fprintf(stderr, "Invalid number of sensors\n");
        exit(1);
    }

    if (info->n_servers <= 0) {
        fprintf(stderr, "Invalid number of servers\n");
        exit(1);
    }

    if (info->buffers_size <= 0) {
        fprintf(stderr, "Invalid buffer size\n");
        exit(1);
    }

    if (info->buffers_size < info->n_sensors) {
        fprintf(stderr, "buffers_size must be greater than or equal to n_sensors\n");
        exit(1);
    }

    if (info->log_filename[0] == '\0') {
        fprintf(stderr, "Invalid log filename\n");
        exit(1);
    }

    if (info->statistics_filename[0] == '\0') {
        fprintf(stderr, "Invalid statistics filename\n");
        exit(1);
    }

    if (info->period == 0) {
        fprintf(stderr, "Invalid period\n");
        exit(1);
    }
}

void read_settings(int argc, char *argv[], struct info_container *info)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s args.txt settings.txt\n", argv[0]);
        exit(1);
    }

    initialize_info(info);

    read_args_file(argv[1], info);
    read_settings_file(argv[2], info);

    validate_settings(info);
}
