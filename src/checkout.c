#include "utils.h"
#include <errno.h>
#include <libintl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int shm_sim_settings_id;
int shm_store_data_id;
int shm_semaphores_id;
int shm_queues_id;
int shm_checkouts_id;
SimSettings* shm_sim_settings;
StoreData* shm_store_data;
Semaphores* shm_semaphores;
Queues* shm_queues;
Checkouts* shm_checkouts;
int id;
char logger_message[480];
char logger_message_buf[80];

void sigusr_handler(int sig);

pid_t pids[MAX_SS_CHECKOUTS];

void serve_the_customer(int new_id);
void shm_init();
void shm_close();

int main(int argc, char* argv[])
{

    init_i18n();

    shm_init();

    operation_wait(shm_semaphores->sem_checkouts);
    for (int i = 0; i < MAX_CHECKOUTS; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror(_("Fork error\n"));
            exit(1);
        } else if (pids[i] == 0) {
            serve_the_customer(i);
            exit(1);
        } else {
            shm_checkouts->checkout[i].pid = pids[i];
            shm_checkouts->checkout[i].clients_served = 0;
        }
    }
    operation_signal(shm_semaphores->sem_checkouts);

    while (!shm_sim_settings->stop_sim) {
        usleep(10000000. / shm_sim_settings->sim_speed);
    }

    shm_close();
    exit(0);
}

void serve_the_customer(int new_id)
{
    id = new_id;

    srand(time(NULL) + id);

    shm_checkouts->checkout[id].open = 0;

    ClientMessage msg;

    signal(SIGUSR1, sigusr_handler);
    signal(SIGUSR2, sigusr_handler);

    while (!shm_sim_settings->stop_sim) {
        if (shm_checkouts->checkout[id].open == 0) {
            usleep(500000 / shm_sim_settings->sim_speed);
            continue;
        };

        if (id == 0) {
            if (msgrcv(shm_queues->msq_checkout_one, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    usleep(500000 / shm_sim_settings->sim_speed);
                    continue;
                } else {
                    break;
                }
            }
        } else if (id == 1) {
            if (msgrcv(shm_queues->msq_checkout_two, &msg, sizeof(msg) - sizeof(long), 0, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    usleep(500000 / shm_sim_settings->sim_speed);
                    continue;
                } else {
                    break;
                }
            }
        } else {
            break;
        };
        shm_checkouts->checkout[id].last_client = time(NULL);

        strcpy(logger_message, "");
        strcpy(logger_message_buf, "");

        sprintf(logger_message_buf, "(%d) ", id);
        strcat(logger_message, logger_message_buf);
        sprintf(logger_message_buf, _("Hello (%d)!\nYour shopping list:\n"), msg.client.id);
        strcat(logger_message, logger_message_buf);

        for (int i = 0; i < msg.client.number_of_products; i++) {
            sprintf(logger_message_buf, "%s ", _(msg.client.products[i].name));
            strcat(logger_message, logger_message_buf);
        }
        sprintf(logger_message_buf, "\n");
        strcat(logger_message, logger_message_buf);

        usleep((5 + 2 * msg.client.number_of_products) * 1000000 / shm_sim_settings->sim_speed);

        bool alcohol = false;

        for (int i = 0; i < msg.client.number_of_products; i++) {
            if (msg.client.products[i].category == ALCOHOL) {
                alcohol = true;
                break;
            }
        }

        bool approved;

        if (alcohol) {
            approved = (msg.client.age >= 18) ? true : false;
        }

        if (!approved) {
            sprintf(logger_message_buf, _("You're only %d years old, I can't sell you that!\n"), msg.client.age);
            strcat(logger_message, logger_message_buf);
            save_a_log(LOG_CHECKOUT, logger_message, shm_queues->msq_logger);

            ClientResponse cresp;
            cresp.message_type = msg.client.id;
            cresp.approved = 0;
            msgsnd(shm_queues->msq_client_resp, &cresp, sizeof(cresp) - sizeof(long), 0);
        } else {
            float cena = 0.0;
            for (int i = 0; i < msg.client.number_of_products; i++) {
                cena += msg.client.products[i].price;
            }
            sprintf(logger_message_buf, _("It'll be: %.2f PLN.\nThank you!\n"), cena);
            strcat(logger_message, logger_message_buf);
            save_a_log(LOG_CHECKOUT, logger_message, shm_queues->msq_logger);

            ReceiptMessage receipt;
            receipt.message_type = msg.client.id;
            snprintf(receipt.message, sizeof(receipt.message), _("Receipt\nClient PID: %d\nCheckout ID: %d\nItems bought:\n"), msg.client.id, id);

            for (int i = 0; i < msg.client.number_of_products; i++) {
                char buf[80];
                sprintf(buf, _("%s - %.2f PLN\n"), _(msg.client.products[i].name), msg.client.products[i].price);
                strncat(receipt.message, buf, sizeof(receipt.message) - strlen(receipt.message) - 1);
            }

            char buf_tot[80];

            sprintf(buf_tot, _("Total: %.2f PLN\n"), cena);
            strncat(receipt.message, buf_tot, sizeof(receipt.message) - strlen(receipt.message) - 1);

            msgsnd(shm_queues->msq_receipts, &receipt, sizeof(receipt) - sizeof(long), 0);

            ClientResponse cresp;
            cresp.message_type = msg.client.id;
            cresp.approved = 1;
            msgsnd(shm_queues->msq_client_resp, &cresp, sizeof(cresp) - sizeof(long), 0);
        }

        operation_wait(shm_semaphores->sem_checkouts);
        shm_checkouts->checkout[id].clients_served++;
        operation_signal(shm_semaphores->sem_checkouts);
    }
    exit(0);
};

void shm_init()
{
    shm_queues = (Queues*)shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*)shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_store_data = (StoreData*)shm_att(&shm_store_data_id, STORE_DATA);
    shm_sim_settings = (SimSettings*)shm_att(&shm_sim_settings_id, SIM_SETTINGS);
    shm_checkouts = (Checkouts*)shm_att(&shm_checkouts_id, CHECKOUTS);
};

void shm_close()
{
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_store_data);
    shm_det(shm_sim_settings);
    shm_det(shm_checkouts);
};

void sigusr_handler(int sig)
{
    if (sig == SIGUSR1) {
        shm_checkouts->checkout[id].open = 1;
    } else {
        shm_checkouts->checkout[id].open = 0;
    }
}
