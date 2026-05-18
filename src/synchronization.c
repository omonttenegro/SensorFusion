#include "synchronization.h"
#include "main.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

sem_t *create_semaphore(char *name, int value)
{
    sem_t *sem;

    sem = sem_open(name, O_CREAT | O_EXCL, 0666, value);

    if (sem == SEM_FAILED) {
        fprintf(stderr, "Failed to create semaphore %s: %s\n", name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return sem;
}

void destroy_semaphore(char *name, sem_t *sem){
    if (sem != NULL && sem != SEM_FAILED) {
        if (sem_close(sem) == -1) {
            fprintf(stderr, "Failed to close semaphore %s: %s\n", name, strerror(errno));
        }
    }

    if (sem_unlink(name) == -1 && errno != ENOENT) {
        fprintf(stderr, "Failed to unlink semaphore %s: %s\n", name, strerror(errno));
    }
}

struct buffer_semaphores *create_buffer_semaphores(int value, char *unread_name, char *free_name, char *mutex_name){
    struct buffer_semaphores *sems;

    sems = (struct buffer_semaphores *)malloc(sizeof(struct buffer_semaphores));
    if (sems == NULL) {
        fprintf(stderr, "Failed to allocate buffer semaphores\n");
        return NULL;
    }

    sems->unread = create_semaphore(unread_name, 0);
    sems->free_space = create_semaphore(free_name, value);
    sems->mutex = create_semaphore(mutex_name, 1);

    return sems;
}

void destroy_buffer_semaphores(struct buffer_semaphores *sems, char *unread_name, char *free_name, char *mutex_name){
    if (sems == NULL) {
        return;
    }

    destroy_semaphore(unread_name, sems->unread);
    destroy_semaphore(free_name, sems->free_space);
    destroy_semaphore(mutex_name, sems->mutex);

    free(sems);
}

struct semaphores *create_all_semaphores(unsigned value){
    struct semaphores *sems = malloc(sizeof(struct semaphores));

    if (sems == NULL) {
        fprintf(stderr, "Failed to allocate semaphores\n");
        return NULL;
    }

    sems->main_sensors = create_buffer_semaphores((int)value, SEM_MAIN_SENSORS_UNREAD, SEM_MAIN_SENSORS_FREE, SEM_MAIN_SENSORS_MUTEX);

    sems->sensors_controllers = create_buffer_semaphores((int)value, SEM_SENSORS_CONTROLLERS_UNREAD, SEM_SENSORS_CONTROLLERS_FREE,SEM_SENSORS_CONTROLLERS_MUTEX);

    sems->controllers_servers = create_buffer_semaphores((int)value, SEM_CONTROLLERS_SERVERS_UNREAD, SEM_CONTROLLERS_SERVERS_FREE, SEM_CONTROLLERS_SERVERS_MUTEX);

    sems->terminate_mutex = create_semaphore(SEM_TERMINATE_MUTEX, 1);
    sems->log_mutex = create_semaphore(SEM_LOG_MUTEX, 1);

    return sems;
}

void destroy_all_semaphores(struct semaphores *sems){
    if (sems == NULL) {
        return;
    }

    destroy_buffer_semaphores(sems->main_sensors, SEM_MAIN_SENSORS_UNREAD, SEM_MAIN_SENSORS_FREE, SEM_MAIN_SENSORS_MUTEX);

    destroy_buffer_semaphores(sems->sensors_controllers, SEM_SENSORS_CONTROLLERS_UNREAD, SEM_SENSORS_CONTROLLERS_FREE, SEM_SENSORS_CONTROLLERS_MUTEX);

    destroy_buffer_semaphores(sems->controllers_servers, SEM_CONTROLLERS_SERVERS_UNREAD, SEM_CONTROLLERS_SERVERS_FREE, SEM_CONTROLLERS_SERVERS_MUTEX);

    destroy_semaphore(SEM_TERMINATE_MUTEX, sems->terminate_mutex);
    destroy_semaphore(SEM_LOG_MUTEX, sems->log_mutex);

    free(sems);
}

void print_all_semaphores(struct semaphores *sems){
    int value;

    //Main_sensor
    printf("Main -> Sensors:\n");

    if (sems->main_sensors->unread != NULL && sem_getvalue(sems->main_sensors->unread, &value) != -1)
        printf("  unread: %d\n", value);
    else
        printf("  unread: unavailable\n");

    if (sems->main_sensors->free_space != NULL && sem_getvalue(sems->main_sensors->free_space, &value) != -1)
        printf("  free: %d\n", value);
    else
        printf("  free: unavailable\n");

    if (sems->main_sensors->mutex != NULL && sem_getvalue(sems->main_sensors->mutex, &value) != -1)
        printf("  mutex: %d\n", value);
    else
        printf("  mutex: unavailable\n");

    //Sensor_controller
    printf("Sensors -> Controllers:\n");

    if (sems->sensors_controllers->unread != NULL && sem_getvalue(sems->sensors_controllers->unread, &value) != -1)
        printf("  unread: %d\n", value);
    else
        printf("  unread: unavailable\n");

    if (sems->sensors_controllers->free_space != NULL && sem_getvalue(sems->sensors_controllers->free_space, &value) != -1)
        printf("  free: %d\n", value);
    else
        printf("  free: unavailable\n");

    if (sems->sensors_controllers->mutex != NULL && sem_getvalue(sems->sensors_controllers->mutex, &value) != -1)
        printf("  mutex: %d\n", value);
    else
        printf("  mutex: unavailable\n");

    //Controller_server
    printf("Controllers -> Servers:\n");

    if (sems->controllers_servers->unread != NULL && sem_getvalue(sems->controllers_servers->unread, &value) != -1)
        printf("  unread: %d\n", value);
    else
        printf("  unread: unavailable\n");

    if (sems->controllers_servers->free_space != NULL && sem_getvalue(sems->controllers_servers->free_space, &value) != -1)
        printf("  free: %d\n", value);
    else
        printf("  free: unavailable\n");

    if (sems->controllers_servers->mutex != NULL && sem_getvalue(sems->controllers_servers->mutex, &value) != -1)
        printf("  mutex: %d\n", value);
    else
        printf("  mutex: unavailable\n");


    if (sems->terminate_mutex != NULL && sem_getvalue(sems->terminate_mutex, &value) != -1)
        printf("Terminate mutex: %d\n", value);
    else
        printf("Terminate mutex: unavailable\n");

    if (sems->log_mutex != NULL && sem_getvalue(sems->log_mutex, &value) != -1)
        printf("Log mutex: %d\n", value);
    else
        printf("Log mutex: unavailable\n");
}

static void wakeup_buffer(struct buffer_semaphores *sems, int n){
    if (sems == NULL) {
        return;
    }

    for (int i = 0; i < n; i++) {
        sem_post(sems->unread);
        sem_post(sems->free_space);
    }
}

void wakeup_processes(struct info_container *info){
    int n;

    if (info == NULL || info->sems == NULL) {
        return;
    }

    // Número suficiente para libertar processos que estejam bloqueados em sem_wait().
    n = info->n_sensors + info->n_servers + 10;

    wakeup_buffer(info->sems->main_sensors, n);
    wakeup_buffer(info->sems->sensors_controllers, n);
    wakeup_buffer(info->sems->controllers_servers, n);
}