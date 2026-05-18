#ifndef CTIME_H_GUARD
#define CTIME_H_GUARD

struct timestamps {
    long main_time;
    long sensor_time;
    long controller_time;
    long server_time;
};

long get_current_time_ms(void);

void set_main_time(struct timestamps *t);
void set_sensor_time(struct timestamps *t);
void set_controller_time(struct timestamps *t);
void set_server_time(struct timestamps *t);

#endif