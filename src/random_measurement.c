#include <stdlib.h>
#include <time.h>
#include "random_measurement.h"

void seed_sensor_rng(int sensor_id)
{
    srand((unsigned int)time(NULL) + (unsigned int)sensor_id);
}

double normal_random(double mean, double stddev)
{
    double u1 = (double)rand() / RAND_MAX;
    double u2 = (double)rand() / RAND_MAX;

    double z = sqrt(-2.0 * log(u1)) * cos(2 * M_PI * u2);
    return mean + z * stddev;
}

/* Generates one measurement centered at V with Gaussian noise (standard deviation STD). */
double get_measurement(void)
{
    double noise = normal_random(0.0, STD);
    return V + noise;
}

/* Returns true only when measurement is within the valid range. */
bool is_valid_measurement(double measurement)
{
    return measurement >= V_LOWER_BOUND && measurement <= V_UPPER_BOUND;
}
