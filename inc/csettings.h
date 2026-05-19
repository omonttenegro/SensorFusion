#ifndef CSETTINGS_H_GUARD
#define CSETTINGS_H_GUARD

#include "main.h"

/*
 * Reads and validates the initial configuration of the program: args.txt, settings.txt.
 */
void read_settings(int argc, char *argv[], struct info_container *info);

#endif