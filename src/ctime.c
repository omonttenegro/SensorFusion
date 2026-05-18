#include "ctime.h"
#include <time.h>
#include <stddef.h>

void init_timestamps(struct timestamps *t)
{
    if (t == NULL) {
        return;
    }

    t->main_time = 0;
    t->sensor_time = 0;
    t->controller_time = 0;
    t->server_time = 0;
}


long get_current_time_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void set_main_time(struct timestamps *t)
{
    t->main_time = get_current_time_ms();
}

void set_sensor_time(struct timestamps *t)
{
    t->sensor_time = get_current_time_ms();
}

void set_controller_time(struct timestamps *t)
{
    t->controller_time = get_current_time_ms();
}

void set_server_time(struct timestamps *t)
{
    t->server_time = get_current_time_ms();
}