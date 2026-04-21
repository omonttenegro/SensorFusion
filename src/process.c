#include "process.h"
#include "sensor.h"
#include "controller.h"
#include "server.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>

int launch_process(enum process_type process_type, int process_id, struct info_container *info, struct buffers *buffs)
{
    int pid = fork();
    if (pid < 0){
        fprintf(stderr, "Failed to fork process\n");
        return -1;
    }
    
    if (pid == 0){
        int status = 0;
        switch (process_type)
        {
            case SENSOR_PROCESS:
                status = execute_sensor(process_id, info, buffs);
                break;
            case CONTROLLER_PROCESS:
                status = execute_controller(process_id, info, buffs);
                break;
            case SERVER_PROCESS:
                status = execute_server(process_id, info, buffs);
                break;
            default:
                fprintf(stderr, "Invalid process type\n");
                return -1;
        }
        return status;
    }
    
    else{
        return pid;
    }
}

int wait_process(int process_id){
    int status;
    if (waitpid(process_id, &status, 0) == -1){
        fprintf(stderr, "Failed to wait for process\n");
        return -1;
    }
    
    if (WIFEXITED(status)){
        return WEXITSTATUS(status);
    }
    
    return -1; // Process did not end normally
}