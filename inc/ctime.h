/*
 * Grupo 20
 * André Montenegro, Nº63755
 * Francisco Costa, Nº63691
 * Nicholas Antunes, Nº63783
 */
#ifndef CTIME_H_GUARD
#define CTIME_H_GUARD

/*
 * Structure that stores timestamps recorded at different stages
 * of measurement processing by various processes in the system.
 *
 * main_time: timestamp when the main process creates a measurement request.
 * sensor_time: timestamp when a sensor process measures and processes the value.
 * controller_time: timestamp when a controller process validates the measurement.
 * server_time: timestamp when a server process generates an estimate.
 */
struct timestamps {
    long main_time;
    long sensor_time;
    long controller_time;
    long server_time;
};

/*
 * Returns the current system time in milliseconds.
 * Uses CLOCK_REALTIME to get the actual wall-clock time.
 * Returns the current time as milliseconds since epoch.
 */
long get_current_time_ms(void);

/*
 * Records the current system time in the main_time field of the
 * timestamps structure.
 * t points to the timestamps structure to be updated.
 */
void set_main_time(struct timestamps *t);

/*
 * Records the current system time in the sensor_time field of the
 * timestamps structure.
 * t points to the timestamps structure to be updated.
 */
void set_sensor_time(struct timestamps *t);

/*
 * Records the current system time in the controller_time field of the
 * timestamps structure.
 * t points to the timestamps structure to be updated.
 */
void set_controller_time(struct timestamps *t);

/*
 * Records the current system time in the server_time field of the
 * timestamps structure.
 * t points to the timestamps structure to be updated.
 */
void set_server_time(struct timestamps *t);

/*
 * Initializes all timestamp fields in the timestamps structure to 0.
 * t points to the timestamps structure to be initialized.
 */
void init_timestamps(struct timestamps *t);

#endif