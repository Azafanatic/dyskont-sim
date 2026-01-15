#include "utils.h"
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define K 10.0

time_t time_end;
int shm_sim_settings_id;
int shm_store_data_id;
int shm_semaphores_id;
int shm_queues_id;
int shm_ss_checkouts_id;
SimSettings* shm_sim_settings;
StoreData* shm_store_data;
Semaphores* shm_semaphores;
Queues* shm_queues;
SelfServiceCheckouts* shm_ss_checkouts;
int active;

char logger_message[320];

bool status[MAX_SS_CHECKOUTS] = { false };

void do_work();
void shm_init();
void shm_close();
void count_open_ss_checkouts();
void menage_checkouts();

int main(int argc, char* argv[])
{

    init_i18n();
    shm_init();
    do_work();
    shm_close();

    exit(0);
}

void do_work()
{
    time_end = time(NULL) + (time_t)ceil(shm_sim_settings->sim_length / (double)shm_sim_settings->sim_speed);

    save_a_log(LOG_MANAGER, _("Time to open the store!\n"), shm_queues->msq_logger);

    operation_wait(shm_semaphores->sem_store_data);
    shm_store_data->open = true;
    operation_signal(shm_semaphores->sem_store_data);

    usleep(1000000);

    while (time(NULL) < time_end) {

        menage_checkouts();

        operation_wait(shm_semaphores->sem_checkouts);
        count_open_ss_checkouts();
        operation_signal(shm_semaphores->sem_checkouts);

        usleep(10.0 * 1000000 / shm_sim_settings->sim_speed);
    }

    save_a_log(LOG_MANAGER, _("Closing the store.\n"), shm_queues->msq_logger);

    operation_wait(shm_semaphores->sem_store_data);
    shm_store_data->open = false;
    operation_signal(shm_semaphores->sem_store_data);

    while (true) {
        menage_checkouts();
        if (shm_store_data->all_clients <= 0) {
            save_a_log(LOG_MANAGER, _("Everyone left the store. The store is now closed.\n"), shm_queues->msq_logger);
            kill(getppid(), SIGINT);
            exit(0);
        }
        usleep(10.0 * 1000000 / shm_sim_settings->sim_speed);
    }
};

void count_open_ss_checkouts()
{
    int result = 0;
    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        if (status[i] == true)
            result++;
    }
    shm_ss_checkouts->checkouts_opened = result;
};

void menage_checkouts()
{
    active = floor(shm_store_data->all_clients / K);
    if (active < 3)
        active = 3;
    if (active > 6)
        active = 6;

    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        status[i] = (i < active) ? true : false;
    };

    operation_wait(shm_semaphores->sem_checkouts);
    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        shm_ss_checkouts->checkout[i].open = (i < active) ? 1 : 0;
    };
    operation_signal(shm_semaphores->sem_checkouts);
};

void shm_init()
{
    shm_queues = (Queues*)shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*)shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_store_data = (StoreData*)shm_att(&shm_store_data_id, STORE_DATA);
    shm_sim_settings = (SimSettings*)shm_att(&shm_sim_settings_id, SIM_SETTINGS);
    shm_ss_checkouts = (SelfServiceCheckouts*)shm_att(&shm_ss_checkouts_id, SS_CHECKOUTS);
};

void shm_close()
{
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_store_data);
    shm_det(shm_sim_settings);
    shm_det(shm_ss_checkouts);
};
