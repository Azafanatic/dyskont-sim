#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include "utils.h"

#define MAIN_PROCESSES 6

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

pid_t pids[MAIN_PROCESSES];
char logger_message[240];

void sigint_handler(int sig);

void shm_init();
void sem_create();
void msq_create();

void shm_close();
void sem_destroy();
void msq_destroy();

int main(int argc, char *argv[]) {

    shm_init();
    sem_create();
    msq_create();

    operation_wait(shm_semaphores->sem_store_data);
    shm_store_data->products_sold = 0;
    shm_store_data->all_clients = 0;
    operation_signal(shm_semaphores->sem_store_data);

    operation_wait(shm_semaphores->sem_sim_settings);
    shm_sim_settings->stop_sim = 0;
    shm_sim_settings->sim_length = (argc >= 2) ? atoi(argv[1]) : 3600;
    shm_sim_settings->sim_speed = (argc >= 3) ? atoi(argv[2]) : 60;
    operation_signal(shm_semaphores->sem_sim_settings);

    save_a_log(LOG_SIM_INFO, "ROZPOCZYNAM SYMULACJE\n", shm_queues->msq_logger);
    sprintf(logger_message, "Dane symulacji:\nCzas trwania:%d (sek)\t Predkosc: %d\t\n", shm_sim_settings->sim_length, shm_sim_settings->sim_speed);
    save_a_log(LOG_SIM_INFO, logger_message, shm_queues->msq_logger);

    {
        char *process_names[MAIN_PROCESSES] = {"logger", "manager", "client", "self_service_checkout", "checkout", "staff"};
        char exec_path[32];

        for (int i = 0; i < MAIN_PROCESSES; i++) {
            pids[i] = fork();

            sprintf(exec_path, "./%s", process_names[i]);

            if (pids[i] < 0) {
                perror("Błąd forka\n");
                exit(1);
            } else if (pids[i] == 0) {
                execlp(exec_path, process_names[i], (char *)NULL);
                perror("Błąd exec\n");
                exit(1);
            }
        }
    }

    signal(SIGINT, sigint_handler);

    while (!shm_sim_settings->stop_sim) {
        sleep(1);
    }

    save_a_log(LOG_SIM_INFO, "KONCZE SYMULACJE\n", shm_queues->msq_logger);

    sleep(1);
    for (int i = 0; i < MAIN_PROCESSES; i++) {
        kill(pids[i], SIGINT);
    }
    for (int i = 0; i < MAIN_PROCESSES; i++) {
        waitpid(pids[i], NULL, 0);
    }

    msq_destroy();
    sem_destroy();
    shm_close();

    return 0;
}

void sigint_handler(int sig) {
    if (sig == SIGINT) {
        operation_wait(shm_semaphores->sem_sim_settings);
        shm_sim_settings->stop_sim = 1;
        operation_signal(shm_semaphores->sem_sim_settings);
    }
};

void shm_init() {
    shm_queues = (Queues*) shm_create(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*) shm_create(&shm_semaphores_id, SEMAPHORES);
    shm_store_data = (StoreData*) shm_create(&shm_store_data_id, STORE_DATA);
    shm_sim_settings = (SimSettings*) shm_create(&shm_sim_settings_id, SIM_SETTINGS);
    shm_ss_checkouts = (SelfServiceCheckouts*) shm_create(&shm_ss_checkouts_id, SS_CHECKOUTS);
};

void shm_close() {
    shm_destroy(shm_queues_id, shm_queues);
    shm_destroy(shm_semaphores_id, shm_semaphores);
    shm_destroy(shm_store_data_id, shm_store_data);
    shm_destroy(shm_sim_settings_id, shm_sim_settings);
    shm_destroy(shm_ss_checkouts_id, shm_ss_checkouts);
};

void sem_create() {
    shm_semaphores->sem_queues = create_a_semaphore(SEM_ID_QUEUES);
    shm_semaphores->sem_store_data = create_a_semaphore(SEM_ID_STORE_DATA);
    shm_semaphores->sem_checkouts = create_a_semaphore(SEM_ID_CHECKOUTS);
    shm_semaphores->sem_sim_settings = create_a_semaphore(SEM_ID_SIM_SETTINGS);
};

void sem_destroy() {
    del_a_semaphore(shm_semaphores->sem_queues);
    del_a_semaphore(shm_semaphores->sem_store_data);
    del_a_semaphore(shm_semaphores->sem_sim_settings);
    del_a_semaphore(shm_semaphores->sem_checkouts);
};

void msq_create() {
    int msq_logger_id = msgget(MSQ_ID_LOGGER, IPC_CREAT | 0600);
    if (msq_logger_id == -1) exit(1);

    int msq_ss_checkouts_id = msgget(MSQ_ID_SS_CHECKOUTS, IPC_CREAT | 0600);
    if (msq_ss_checkouts_id == -1) exit(1);

    operation_wait(shm_semaphores->sem_queues);
    shm_queues->msq_logger = msq_logger_id;
    shm_queues->msq_ss_checkouts = msq_ss_checkouts_id;
    operation_signal(shm_semaphores->sem_queues);
};

void msq_destroy() {
    msgctl(shm_queues->msq_logger, IPC_RMID, NULL);
    msgctl(shm_queues->msq_ss_checkouts, IPC_RMID, NULL);
    msgctl(shm_queues->msq_checkout_one, IPC_RMID, NULL);
    msgctl(shm_queues->msq_checkout_two, IPC_RMID, NULL);
};

