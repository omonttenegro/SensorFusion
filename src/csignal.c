#define _POSIX_C_SOURCE 200809L
#include "csignal.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
extern MeasurementInfo *measurement_history;

static struct info_container *global_info;
static struct buffers *global_buffers;
extern MeasurementInfo *measurement_history;
void setup_signal_handlers(struct info_container *info, struct buffers *buffers){
    global_info = info;
    global_buffers = buffers;

    struct sigaction acao;
    sigemptyset(&acao.sa_mask);
    acao.sa_flags = 0;
    
        acao.sa_handler = sigint_handler;

    if(sigaction(SIGINT, &acao, NULL) == -1){
        perror("sigaction SIGINT");
        exit(1);
    }
    acao.sa_handler = sigalrm_handler;

    if(sigaction(SIGALRM, &acao, NULL) == -1){
        perror("sigaction SIGALRM");
        exit(1);
    }
}

void sigint_handler(int signal){
    if(global_info != NULL && global_buffers != NULL) {
        end_execution(global_info, global_buffers);
    }
    sem_wait(global_info->sems->terminate_mutex);
    *(global_info->terminate) = 1;
    sem_post(global_info->sems->terminate_mutex);
    wakeup_processes(global_info);
    end_execution(global_info, global_buffers);
}

void sigalrm_handler(int signal){
    if(global_info != NULL && global_buffers != NULL) {
        return;
    }
    print_measurement_status(global_info, global_buffers);
    reset_alarm(global_info->period);
}
void reset_alarm(unsigned int period){
    alarm(period);
}

void ignore_signals(void){
    struct sigaction acao;
    sigemptyset(&acao.sa_mask);
    acao.sa_flags = 0;
    acao.sa_handler = SIG_IGN;
    if(sigaction(SIGINT, &acao, NULL) == -1){
        perror("sigaction SIGINT");
        exit(1);
    }
}

void print_measurement_status(struct info_container *info, struct buffers *buffers){
    if (info == NULL|| buffers == NULL) {
        return;
    }
    long current_time = get_current_time_ms();
     for(int i =0; i< *(info->total_measurements);i++){
        MeasurementInfo *m = &measurement_history[i];
        long elapsed_time;
        if(m->change_time.server_time != 0){
            elapsed_time= m->change_time.server_time - m->change_time.main_time;
        }
        else{
            elapsed_time = current_time - m->change_time.main_time;
        }
        printf("%d %.3f \n", m->m_id, elapsed_time / 1000.0);
    }
}