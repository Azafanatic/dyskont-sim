#include "utils.h"
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/** @brief Constant K */
#define K 10.0

/** @brief End time */
useconds_t time_end;
/** @brief Time elapsed */
useconds_t time_elapsed;
/** @brief Time jump */
useconds_t time_jump;

/** @brief Shared memory IDs */
int shm_sim_settings_id;
int shm_store_data_id;
int shm_semaphores_id;
int shm_queues_id;
int shm_ss_checkouts_id;
int shm_checkouts_id;

/** @brief Shared memory pointers */
SimSettings* shm_sim_settings;
StoreData* shm_store_data;
Semaphores* shm_semaphores;
Queues* shm_queues;
SelfServiceCheckouts* shm_ss_checkouts;
Checkouts* shm_checkouts;

/** @brief Active flag */
int active;

/** @brief Logger message buffer */
char logger_message[320];

bool status[MAX_SS_CHECKOUTS] = { false };
bool open_checkout[MAX_CHECKOUTS + MAX_SS_CHECKOUTS] = { false };

void do_work();
void shm_init();
void shm_close();
void menage_checkouts();
void close_all_checkouts();

int main(int argc, char* argv[])
{

    init_i18n();
    shm_init();
    do_work();
    shm_close();

    exit(0);
}

/** @brief Performs manager work */
void do_work()
{
    time_elapsed = 0;
    time_end =
    (useconds_t)ceil(
        (shm_sim_settings->sim_length * 1000000.0) /
        shm_sim_settings->sim_speed
    );

    time_jump =
    (useconds_t)(10.0 * 1000000 / shm_sim_settings->sim_speed);

    save_a_log(LOG_MANAGER, _("Time to open the store!\n"), shm_queues->msq_logger);

    operation_wait(shm_semaphores->sem_store_data);
    shm_store_data->open = true;
    operation_signal(shm_semaphores->sem_store_data);

    usleep(300000);

    while (time_elapsed < time_end) {

        menage_checkouts();

        usleep(time_jump);
        time_elapsed += time_jump;
    }

    if (shm_sim_settings->evacuation) {
        save_a_log(LOG_MANAGER, _("Time to evacuate!\n"), shm_queues->msq_logger);
        operation_wait(shm_semaphores->sem_store_data);
        shm_store_data->open = false;
        operation_signal(shm_semaphores->sem_store_data);
        if (kill(getppid(), SIGALRM) == -1) {
            perror(_("Kill error\n"));
        }
        close_all_checkouts();
        while (true) {
            if (shm_store_data->all_clients <= 0) {
                save_a_log(LOG_MANAGER, _("Everyone left the store. The store is now closed.\n"), shm_queues->msq_logger);
                exit(0);
            }
            usleep(10.0 * 1000000 / shm_sim_settings->sim_speed);
        }
    } else {
        save_a_log(LOG_MANAGER, _("Closing the store.\n"), shm_queues->msq_logger);

        operation_wait(shm_semaphores->sem_store_data);
        shm_store_data->open = false;
        operation_signal(shm_semaphores->sem_store_data);

        operation_wait(shm_semaphores->sem_checkouts);
        for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
            shm_ss_checkouts->checkout[i].open = (i < 3) ? 1 : 0;
        }
        if (kill(shm_checkouts->checkout[0].pid, SIGUSR1) == -1) {
            perror(_("Kill error\n"));
        }
        if (kill(shm_checkouts->checkout[1].pid, SIGUSR1) == -1) {
            perror(_("Kill error\n"));
        }
        shm_checkouts->checkouts_opened = 2;
        shm_ss_checkouts->checkouts_opened = 3;
        operation_signal(shm_semaphores->sem_checkouts);

        while (true) {
            if (shm_store_data->all_clients <= 0) {
                save_a_log(LOG_MANAGER, _("Everyone left the store. The store is now closed.\n"), shm_queues->msq_logger);
                close_all_checkouts();

                usleep(11000000 / shm_sim_settings->sim_speed);

                if (kill(getppid(), SIGINT) == -1) {
                    perror(_("Kill error\n"));
                }
                exit(0);
            }
            usleep(10.0 * 1000000 / shm_sim_settings->sim_speed);
        }
    }
};

/** @brief Manages checkouts */
void menage_checkouts()
{
    bool prev_open_checkout[MAX_CHECKOUTS + MAX_SS_CHECKOUTS];

    for (int i = 0; i < MAX_CHECKOUTS + MAX_SS_CHECKOUTS; i++) {
        prev_open_checkout[i] = open_checkout[i];
    }

    active = floor(shm_store_data->all_clients / K);

    if (shm_store_data->all_clients < K * (active - 3))
        active--;

    if (active < 3)
        active = 3;
    if (active > MAX_CHECKOUTS + MAX_SS_CHECKOUTS)
        active = MAX_CHECKOUTS + MAX_SS_CHECKOUTS;

    if (queue_length(shm_queues->msq_checkout_one) > 3) {
        open_checkout[MAX_SS_CHECKOUTS] = true;
    } else if (queue_length(shm_queues->msq_checkout_one) == 0) {
        open_checkout[MAX_SS_CHECKOUTS] = false;
    }

    int i = 0;
    while (active > 0) {
        if (open_checkout[i] == false) {
            open_checkout[i] = true;
        };
        active--;
        i++;
    }

    operation_wait(shm_semaphores->sem_checkouts);

    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        if (prev_open_checkout[i] == open_checkout[i])
            continue;
        shm_ss_checkouts->checkout[i].open = (open_checkout[i]) ? 1 : 0;
    }

    for (int i = MAX_SS_CHECKOUTS; i < MAX_SS_CHECKOUTS + MAX_CHECKOUTS; i++) {
        if (prev_open_checkout[i] == open_checkout[i])
            continue;
        if (open_checkout[i]) {
            if (kill(shm_checkouts->checkout[i - MAX_SS_CHECKOUTS].pid, SIGUSR1) == -1) {
                perror(_("Kill error\n"));
            }
        } else {
            if (time(NULL) >= shm_checkouts->checkout[i - MAX_SS_CHECKOUTS].last_client + 30. / shm_sim_settings->sim_speed)
                if (kill(shm_checkouts->checkout[i - MAX_SS_CHECKOUTS].pid, SIGUSR2) == -1) {
                    perror(_("Kill error\n"));
                }
        }
    }

    int count = 0;
    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        if (open_checkout[i])
            count++;
    }
    shm_ss_checkouts->checkouts_opened = count;

    count = 0;
    for (int i = MAX_SS_CHECKOUTS; i < MAX_SS_CHECKOUTS + MAX_CHECKOUTS; i++) {
        if (open_checkout[i])
            count++;
    }
    shm_checkouts->checkouts_opened = count;

    operation_signal(shm_semaphores->sem_checkouts);
};

/** @brief Initializes shared memory implementation */
void shm_init()
{
    shm_queues = (Queues*)shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*)shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_store_data = (StoreData*)shm_att(&shm_store_data_id, STORE_DATA);
    shm_sim_settings = (SimSettings*)shm_att(&shm_sim_settings_id, SIM_SETTINGS);
    shm_ss_checkouts = (SelfServiceCheckouts*)shm_att(&shm_ss_checkouts_id, SS_CHECKOUTS);
    shm_checkouts = (Checkouts*)shm_att(&shm_checkouts_id, CHECKOUTS);
};

/** @brief Closes shared memory implementation */
void shm_close()
{
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_store_data);
    shm_det(shm_sim_settings);
    shm_det(shm_ss_checkouts);
    shm_det(shm_checkouts);
};

/** @brief Closes all checkouts */
void close_all_checkouts()
{
    save_a_log(LOG_MANAGER, _("Closing all checkouts...\n"), shm_queues->msq_logger);
    operation_wait(shm_semaphores->sem_checkouts);
    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        shm_ss_checkouts->checkout[i].open = 0;
    }
    if (kill(shm_checkouts->checkout[0].pid, SIGUSR2) == -1) {
        perror(_("Kill error\n"));
    }
    if (kill(shm_checkouts->checkout[1].pid, SIGUSR2) == -1) {
        perror(_("Kill error\n"));
    }
    shm_checkouts->checkouts_opened = 0;
    shm_ss_checkouts->checkouts_opened = 0;
    operation_signal(shm_semaphores->sem_checkouts);
};
