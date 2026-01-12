#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <math.h>
#include "utils.h"

time_t time_end;
int shm_sim_settings_id;
int shm_store_data_id;
int shm_semaphores_id;
int shm_queues_id;
int shm_ss_checkouts_id;
SimSettings *shm_sim_settings;
StoreData *shm_store_data;
Semaphores *shm_semaphores;
Queues *shm_queues;
SelfServiceCheckouts *shm_ss_checkouts;

char logger_message[320];

int clients_in_a_line;
float wait_time;

void do_work();
void shm_init();
void shm_close();
void count_open_ss_checkouts();
void menage_checkouts();

int main(int argc, char *argv[]) {

    shm_init();
    do_work();
    shm_close();

    exit(0);
}

void do_work() {
    time_end = time(NULL) + (time_t)ceil(shm_sim_settings->sim_length / (double) shm_sim_settings->sim_speed);

    save_a_log(LOG_MANAGER, "Otwieram sklep!\n", shm_queues->msq_logger);

    operation_wait(shm_semaphores->sem_store_data);
    shm_store_data->open = true;
    operation_signal(shm_semaphores->sem_store_data);

    while (time(NULL) < time_end) {
        clients_in_a_line = queue_length(shm_queues->msq_ss_checkouts);

        operation_wait(shm_semaphores->sem_checkouts);

        count_open_ss_checkouts();

        wait_time = (10 + 3.0 * 6.5) * clients_in_a_line / shm_ss_checkouts->checkouts_opened / 60.;

        menage_checkouts();

        sprintf(logger_message,"Czas oczekiwania %.2f \tOtwarte kasy: %d\n", wait_time, shm_ss_checkouts->checkouts_opened);
        save_a_log(LOG_MANAGER, logger_message, shm_queues->msq_logger);

        operation_signal(shm_semaphores->sem_checkouts);

        usleep(30.0 * 1000000 / shm_sim_settings->sim_speed);

    }

    save_a_log(LOG_MANAGER, "Zamykam sklep.\n", shm_queues->msq_logger);

    operation_wait(shm_semaphores->sem_store_data);
    shm_store_data->open = false;
    operation_signal(shm_semaphores->sem_store_data);

    while (true) {
        if (shm_store_data->all_clients <= 0) {
            save_a_log(LOG_MANAGER, "Sklep zamkniety.\n", shm_queues->msq_logger);
            kill(getppid(), SIGINT);
            exit(0);
        }
        sleep(1);
    }
};

void count_open_ss_checkouts() {
    if (shm_ss_checkouts->checkout[5].open) {
        shm_ss_checkouts->checkouts_opened = 6;
    } else if (shm_ss_checkouts->checkout[4].open) {
        shm_ss_checkouts->checkouts_opened = 5;
    } else if (shm_ss_checkouts->checkout[3].open) {
        shm_ss_checkouts->checkouts_opened = 4;
    } else {
        shm_ss_checkouts->checkouts_opened = 3;
    }
};

void menage_checkouts() {
    if (wait_time > 2.0) {
        shm_ss_checkouts->checkout[3].open = true;
    } else {
        shm_ss_checkouts->checkout[3].open = false;
    }

    if (wait_time > 4.0) {
        shm_ss_checkouts->checkout[4].open = true;
    } else {
        shm_ss_checkouts->checkout[4].open = false;
    }

    if (wait_time > 6.0) {
        shm_ss_checkouts->checkout[5].open = true;
    } else {
        shm_ss_checkouts->checkout[5].open = false;
    }
};

void shm_init() {
    shm_queues = (Queues*) shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*) shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_store_data = (StoreData*) shm_att(&shm_store_data_id, STORE_DATA);
    shm_sim_settings = (SimSettings*) shm_att(&shm_sim_settings_id, SIM_SETTINGS);
    shm_ss_checkouts = (SelfServiceCheckouts*) shm_att(&shm_ss_checkouts_id, SS_CHECKOUTS);
};

void shm_close() {
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_store_data);
    shm_det(shm_sim_settings);
    shm_det(shm_ss_checkouts);
};
