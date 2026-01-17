#include "utils.h"
#include <libintl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int shm_sim_settings_id;
int shm_store_data_id;
int shm_semaphores_id;
int shm_queues_id;
int shm_ss_checkouts_id;
int shm_checkouts_id;
SimSettings* shm_sim_settings;
StoreData* shm_store_data;
Semaphores* shm_semaphores;
Queues* shm_queues;
SelfServiceCheckouts* shm_ss_checkouts;
Checkouts* shm_checkouts;

pid_t pids[MAIN_PROCESSES];
char logger_message[240];

void sig_handler(int sig);

void shm_init();
void sem_create();
void msq_create();

void shm_close();
void sem_destroy();
void msq_destroy();

int main(int argc, char* argv[])
{
    init_i18n();

    shm_init();
    sem_create();
    msq_create();

    operation_wait(shm_semaphores->sem_store_data);
    shm_store_data->products_sold = 0;
    shm_store_data->all_clients = 0;
    operation_signal(shm_semaphores->sem_store_data);

    operation_wait(shm_semaphores->sem_sim_settings);
    int evacuate = (rand() % 100 < 10) ? 1 : 0;
    shm_sim_settings->stop_sim = 0;
    shm_sim_settings->sim_length = (argc >= 2) ? atoi(argv[1]) : SIM_LENGTH;
    shm_sim_settings->sim_speed = (argc >= 3) ? atoi(argv[2]) : SIM_SPEED;
    shm_sim_settings->evacuation = (argc >= 4) ? atoi(argv[3]) : evacuate;
    operation_signal(shm_semaphores->sem_sim_settings);

    save_a_log(LOG_SIM_INFO, _("Starting the simulation!\n"), shm_queues->msq_logger);
    sprintf(logger_message, _("Sim settings:\nTime:%d (sec)\t Speed: %d\n"), shm_sim_settings->sim_length, shm_sim_settings->sim_speed);
    save_a_log(LOG_SIM_INFO, logger_message, shm_queues->msq_logger);

    {
        char path[PATH_MAX];
        char* res = realpath(argv[0], path);
        (void)argc;

        if (!res) {
            perror(_("Path error\n"));
            exit(0);
        };

        char bin_name[] = "sim";
        path[strlen(path) - strlen(bin_name)] = '\0';

        char* process_names[MAIN_PROCESSES] = { "logger", "manager", "client", "self_service_checkout", "checkout", "staff" };
        char buf[PATH_MAX];

        for (int i = 0; i < MAIN_PROCESSES; i++) {
            pids[i] = fork();

            sprintf(buf, "%s%s", path, process_names[i]);

            if (pids[i] < 0) {
                perror(_("Fork error\n"));
                exit(1);
            } else if (pids[i] == 0) {
                execlp(buf, process_names[i], (char*)NULL);
                perror(_("Exec error\n"));
                exit(1);
            }
        }
    }

    signal(SIGINT, sig_handler);
    signal(SIGALRM, sig_handler);

    while (!shm_sim_settings->stop_sim) {
        sprintf(logger_message, _("Customers in the store: %d\t SSC queue: %d\t SSC open: %d\n"), shm_store_data->all_clients, queue_length(shm_queues->msq_ss_checkouts), shm_ss_checkouts->checkouts_opened);
        save_a_log(LOG_SIM_INFO, logger_message, shm_queues->msq_logger);

        sprintf(logger_message, _("Checkout one queue: %d\t Checkout two queue: %d\t Checkouts open: %d\n"), queue_length(shm_queues->msq_checkout_one), queue_length(shm_queues->msq_checkout_two), shm_checkouts->checkouts_opened);
        save_a_log(LOG_SIM_INFO, logger_message, shm_queues->msq_logger);

        usleep(10000000 / shm_sim_settings->sim_speed);
    }

    save_a_log(LOG_SIM_INFO, _("Stopping the simulation...\n"), shm_queues->msq_logger);

    save_a_log(LOG_SIM_INFO, _("Clients served:\nSelf service checkouts:\n"), shm_queues->msq_logger);
    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        sprintf(logger_message, _("(%d) : %d clients.\n"), i, shm_ss_checkouts->checkout[i].clients_served);
        save_a_log(LOG_SIM_INFO, logger_message, shm_queues->msq_logger);
    }
    save_a_log(LOG_SIM_INFO, _("Checkouts:\n"), shm_queues->msq_logger);
    for (int i = MAX_SS_CHECKOUTS; i < MAX_SS_CHECKOUTS + MAX_CHECKOUTS; i++) {
        sprintf(logger_message, _("(%d) : %d clients.\n"), i - MAX_SS_CHECKOUTS, shm_checkouts->checkout[i - MAX_SS_CHECKOUTS].clients_served);
        save_a_log(LOG_SIM_INFO, logger_message, shm_queues->msq_logger);
    }

    usleep(1000000);

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

void sig_handler(int sig)
{
    operation_wait(shm_semaphores->sem_sim_settings);
    shm_sim_settings->stop_sim = 1;
    if (sig == SIGALRM) {
        for (int i = 2; i < MAIN_PROCESSES; i++) {
            kill(pids[i], SIGALRM);
        };
    };
    operation_signal(shm_semaphores->sem_sim_settings);
};

void shm_init()
{
    shm_queues = (Queues*)shm_create(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*)shm_create(&shm_semaphores_id, SEMAPHORES);
    shm_store_data = (StoreData*)shm_create(&shm_store_data_id, STORE_DATA);
    shm_sim_settings = (SimSettings*)shm_create(&shm_sim_settings_id, SIM_SETTINGS);
    shm_ss_checkouts = (SelfServiceCheckouts*)shm_create(&shm_ss_checkouts_id, SS_CHECKOUTS);
    shm_checkouts = (Checkouts*)shm_create(&shm_checkouts_id, CHECKOUTS);
};

void shm_close()
{
    shm_destroy(shm_queues_id, shm_queues);
    shm_destroy(shm_semaphores_id, shm_semaphores);
    shm_destroy(shm_store_data_id, shm_store_data);
    shm_destroy(shm_sim_settings_id, shm_sim_settings);
    shm_destroy(shm_ss_checkouts_id, shm_ss_checkouts);
    shm_destroy(shm_checkouts_id, shm_checkouts);
};

void sem_create()
{
    shm_semaphores->sem_queues = create_a_semaphore(SEM_ID_QUEUES);
    shm_semaphores->sem_store_data = create_a_semaphore(SEM_ID_STORE_DATA);
    shm_semaphores->sem_checkouts = create_a_semaphore(SEM_ID_CHECKOUTS);
    shm_semaphores->sem_sim_settings = create_a_semaphore(SEM_ID_SIM_SETTINGS);
};

void sem_destroy()
{
    del_a_semaphore(shm_semaphores->sem_queues);
    del_a_semaphore(shm_semaphores->sem_store_data);
    del_a_semaphore(shm_semaphores->sem_sim_settings);
    del_a_semaphore(shm_semaphores->sem_checkouts);
};

void msq_create()
{
    int msq_logger_id = msgget(MSQ_ID_LOGGER, IPC_CREAT | 0600);
    if (msq_logger_id == -1)
        exit(1);

    int msq_ss_checkouts_id = msgget(MSQ_ID_SS_CHECKOUTS, IPC_CREAT | 0600);
    if (msq_ss_checkouts_id == -1)
        exit(1);

    int msq_receipts_id = msgget(MSQ_ID_RECEIPTS, IPC_CREAT | 0600);
    if (msq_receipts_id == -1)
        exit(1);

    int msq_staff_id = msgget(MSQ_ID_STAFF, IPC_CREAT | 0600);
    if (msq_staff_id == -1)
        exit(1);

    int msq_client_resp_id = msgget(MSQ_ID_CLIENT_RESP, IPC_CREAT | 0600);
    if (msq_client_resp_id == -1)
        exit(1);

    int msq_ss_staff_id = msgget(MSQ_ID_SS_STAFF, IPC_CREAT | 0600);
    if (msq_ss_staff_id == -1)
        exit(1);

    int msq_checkout_one_id = msgget(MSQ_ID_CHECKOUT_ONE, IPC_CREAT | 0600);
    if (msq_checkout_one_id == -1)
        exit(1);

    int msq_checkout_two_id = msgget(MSQ_ID_CHECKOUT_TWO, IPC_CREAT | 0600);
    if (msq_checkout_two_id == -1)
        exit(1);

    operation_wait(shm_semaphores->sem_queues);
    shm_queues->msq_logger = msq_logger_id;
    shm_queues->msq_ss_checkouts = msq_ss_checkouts_id;
    shm_queues->msq_receipts = msq_receipts_id;
    shm_queues->msq_staff = msq_staff_id;
    shm_queues->msq_client_resp = msq_client_resp_id;
    shm_queues->msq_ss_staff = msq_ss_staff_id;
    shm_queues->msq_checkout_one = msq_checkout_one_id;
    shm_queues->msq_checkout_two = msq_checkout_two_id;
    operation_signal(shm_semaphores->sem_queues);
};

void msq_destroy()
{
    msgctl(shm_queues->msq_logger, IPC_RMID, NULL);
    msgctl(shm_queues->msq_ss_checkouts, IPC_RMID, NULL);
    msgctl(shm_queues->msq_checkout_one, IPC_RMID, NULL);
    msgctl(shm_queues->msq_checkout_two, IPC_RMID, NULL);
    msgctl(shm_queues->msq_receipts, IPC_RMID, NULL);
    msgctl(shm_queues->msq_staff, IPC_RMID, NULL);
    msgctl(shm_queues->msq_client_resp, IPC_RMID, NULL);
    msgctl(shm_queues->msq_ss_staff, IPC_RMID, NULL);
};
