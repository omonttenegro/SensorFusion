#ifndef SYNCHRONIZATION_H_GUARD
#define SYNCHRONIZATION_H_GUARD

#include <semaphore.h>

struct info_container;

#define SEM_MAIN_SENSORS_UNREAD "/sem_main_sensors_unread"
#define SEM_MAIN_SENSORS_FREE "/sem_main_sensors_free"
#define SEM_MAIN_SENSORS_MUTEX "/sem_main_sensors_mutex"

#define SEM_SENSORS_CONTROLLERS_UNREAD "/sem_sensors_controllers_unread"
#define SEM_SENSORS_CONTROLLERS_FREE "/sem_sensors_controllers_free"
#define SEM_SENSORS_CONTROLLERS_MUTEX "/sem_sensors_controllers_mutex"

#define SEM_CONTROLLERS_SERVERS_UNREAD "/sem_controllers_servers_unread"
#define SEM_CONTROLLERS_SERVERS_FREE "/sem_controllers_servers_free"
#define SEM_CONTROLLERS_SERVERS_MUTEX "/sem_controllers_servers_mutex"

#define SEM_TERMINATE_MUTEX "/sem_terminate_mutex"
#define SEM_LOG_MUTEX "/sem_log_mutex"

/* Semaphores for one producer/consumer buffer.
 * unread counts how many items can be read. 
 * free_space counts how many positions can be written.
 * mutex protects the shared buffer while it is being accessed.
 */
struct buffer_semaphores
{
    sem_t *unread;
    sem_t *free_space;
    sem_t *mutex;
};

/* All semaphores used by SOestimate.
 * Each shared buffer path has its own producer/consumer semaphores.
 * terminate_mutex protects the shared termination flag.
 * log_mutex protects writes to the log file.
 */
struct semaphores
{
    struct buffer_semaphores *main_sensors;
    struct buffer_semaphores *sensors_controllers;
    struct buffer_semaphores *controllers_servers;
    sem_t *terminate_mutex;
    sem_t *log_mutex;
};

/* Create one named semaphore with the given initial value. */
sem_t *create_semaphore(char *name, int value);

/* Close and remove one named semaphore. */
void destroy_semaphore(char *name, sem_t *sem);

/* Create the unread, free_space, and mutex semaphores for one shared buffer. */
struct buffer_semaphores *create_buffer_semaphores(int value, char *unread_name, char *free_name, char *mutex_name);

/* Destroy the unread, free_space, and mutex semaphores for one shared buffer. */
void destroy_buffer_semaphores(struct buffer_semaphores *sems, char *unread_name, char *free_name, char *mutex_name);

/* Create all semaphores used by SOestimate.
 * value is the configured shared buffer size.
 */
struct semaphores *create_all_semaphores(unsigned value);

/* Destroy all semaphores used by SOestimate. */
void destroy_all_semaphores(struct semaphores *sems);

/* Print a simple synchronization status message. */
void print_all_semaphores(struct semaphores *sems);

/* Wake child processes that may be blocked in sem_wait(). */
void wakeup_processes(struct info_container *info);

#endif
