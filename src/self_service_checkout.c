#include <unistd.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"

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
int id;

pid_t pids[MAX_SS_CHECKOUTS];

void serve_the_customer(int id_kasy);
void shm_init();
void shm_close();

int main(int argc, char *argv[]) {

    shm_init();

    operation_wait(shm_semaphores->sem_checkouts);
    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("Blad forka\n");
            exit(1);
        } else if (pids[i] == 0) {
            serve_the_customer(i);
            exit(1);
        } else {
            shm_ss_checkouts->checkout[i].pid = pids[i];
            shm_ss_checkouts->checkout[i].clients_served = 0;
        }
    }
    operation_signal(shm_semaphores->sem_checkouts);

    char logger_message[320];

    while (!shm_sim_settings->stop_sim) {
        sprintf(logger_message, "Osoby w sklepie: %d\t Kolejka: %d\t Kasy otwarte: %d\n",shm_store_data->all_clients, queue_length(shm_queues->msq_ss_checkouts), shm_ss_checkouts->checkouts_opened);
        save_a_log(LOG_SIM_INFO, logger_message, shm_queues->msq_logger);

        usleep(10000000. / shm_sim_settings->sim_speed);
    }

    shm_close();
    exit(0);
}

void serve_the_customer(int new_id) {
    id = new_id;
    char logger_message[320];
    char logger_message_buf[80];

    shm_ss_checkouts->checkout[id].open = (id < 3) ? 1 : 0;

    sprintf(logger_message, "Tu kasa nr. %d, pid %d\n", id, shm_ss_checkouts->checkout[id].pid);
    save_a_log(LOG_SIM_WARN, logger_message, shm_queues->msq_logger);

    ClientMessage msg;

    while (!shm_sim_settings->stop_sim) {
        if (shm_ss_checkouts->checkout[id].open == 0) {
            usleep(500000 / shm_sim_settings->sim_speed);
            continue;
        };

        if (msgrcv(shm_queues->msq_ss_checkouts, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            break;
        ;}

        strcpy(logger_message, "");
        strcpy(logger_message_buf, "");
        sprintf(logger_message_buf, "(%d) ", id);
        strcat(logger_message,logger_message_buf);
        sprintf(logger_message_buf, "Witaj kliencie (%d)!\nTwoja lista zakupow:\n", msg.client.id);
        strcat(logger_message,logger_message_buf);
        for (int i = 0; i < msg.client.number_of_products; i++) {
            sprintf(logger_message_buf, "%s ", msg.client.products[i].name);
            strcat(logger_message,logger_message_buf);
        }
        sprintf(logger_message_buf, "\n");
        strcat(logger_message,logger_message_buf);

        usleep((10 + 2 * msg.client.number_of_products) * 1000000 / shm_sim_settings->sim_speed);

        bool alcohol = false;

        for (int i = 0; i < msg.client.number_of_products; i++) {
            if (msg.client.products[i].category == ALKOHOLE) {
                alcohol = true;
                break;
            }
        }

        if (alcohol && msg.client.age < 18) {
            sprintf(logger_message_buf, "Masz tylko %d lat, nie moge Ci tego sprzedac.\n", msg.client.age);
            strcat(logger_message,logger_message_buf);
            save_a_log(LOG_SS_CHECKOUT, logger_message, shm_queues->msq_logger);

            kill(msg.client.id, SIGUSR1);
        } else {
            float cena = 0.0;
            for (int i = 0; i < msg.client.number_of_products; i++) {
                cena += msg.client.products[i].price;
            }
            sprintf(logger_message_buf, "Calosc kosztuje %.2f zł.\nDziekujemy i zapraszamy ponownie!\n", cena);
            strcat(logger_message,logger_message_buf);
            save_a_log(LOG_SS_CHECKOUT, logger_message, shm_queues->msq_logger);

            ReceiptMessage receipt;
            receipt.message_type = msg.client.id;
            snprintf(receipt.message, sizeof(receipt.message), "PARAGON\nKlient PID: %d\nKasa ID: %d\nTwoja lista zakupow:\n",id, msg.client.id);

            for (int i = 0; i < msg.client.number_of_products; i++) {
                char buf[80];
                sprintf(buf, "%s - %.2f zl\n", msg.client.products[i].name, msg.client.products[i].price);
                strncat(receipt.message, buf, sizeof(receipt.message) - strlen(receipt.message) - 1);
            }

            char buf_tot[80];

            sprintf(buf_tot, "Razem: %.2f zl\n", cena);
            strncat(receipt.message, buf_tot, sizeof(receipt.message) - strlen(receipt.message) - 1);

            msgsnd(shm_queues->msq_receipts, &receipt, sizeof(receipt) - sizeof(long), 0);

            kill(msg.client.id, SIGUSR2);
        }

        operation_wait(shm_semaphores->sem_checkouts);
        shm_ss_checkouts->checkout[id].clients_served++;
        operation_signal(shm_semaphores->sem_checkouts);
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
