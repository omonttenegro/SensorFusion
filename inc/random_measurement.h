/*
 * Grupo 20
 * André Montenegro, Nº63755
 * Francisco Costa, Nº63691
 * Nicholas Antunes, Nº63783
 */
#ifndef RANDOM_MEASUREMENT_H_GUARD
#define RANDOM_MEASUREMENT_H_GUARD

/* WARNING: DO NOT MODIFY THIS FILE
 * Header files may be replaced by the teachers during evaluation.
 * If required definitions are changed, the project may fail to compile.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

// Valid range limits for one measurement value
#define V_LOWER_BOUND 80.0
#define V_UPPER_BOUND 120.0

// Fallback definition of PI when it is not provided by the system headers
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Parameters of the generated normal distribution (mean and standard deviation)
#define V 100.0
#define STD 20.0

/* Returns one random value sampled from a normal distribution
 * with the given mean and standard deviation.
 */
double normal_random(double mean, double stddev);

/* Seeds the random number generator for one sensor. */
void seed_sensor_rng(int sensor_id);

/* Generates one measurement using the configured distribution parameters
 * (mean V and standard deviation STD).
 */
double get_measurement(void);

/* Validates one measurement value.
 * Returns true when the value is between V_LOWER_BOUND and V_UPPER_BOUND.
 */
bool is_valid_measurement(double measurement);

#endif
