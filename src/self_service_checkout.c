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

/** @brief Shared memory IDs */
int shm_sim_settings_id;
int shm_store_data_id;
int shm_semaphores_id;
int shm_queues_id;
int shm_ss_checkouts_id;

/** @brief Shared memory pointers */
SimSettings* shm_sim_settings;
StoreData* shm_store_data;
Semaphores* shm_semaphores;
Queues* shm_queues;
SelfServiceCheckouts* shm_ss_checkouts;

/** @brief Checkout ID */
int id;
/** @brief Logger message buffers */
char logger_message[480];
char logger_message_buf[80];

/** @brief Unblock signal handler */
void unblock_handler(int sig);
/** @brief Alarm signal handler */
void sigalrm_handler(int sig);

pid_t pids[MAX_SS_CHECKOUTS];

void serve_the_customer(int id_kasy);
void shm_init();
void shm_close();
void am_i_blcked();

int main(int argc, char* argv[])
{

    init_i18n();

    shm_init();

    operation_wait(shm_semaphores->sem_checkouts);
    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror(_("Fork error\n"));
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

    signal(SIGALRM, sigalrm_handler);

    while (!shm_sim_settings->stop_sim) {
        usleep(10000000. / shm_sim_settings->sim_speed);
    }

    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    shm_close();
    exit(0);
}

/** @brief Serves a customer at self-service checkout */
void serve_the_customer(int new_id)
{
    signal(SIGALRM, SIG_DFL);
    id = new_id;

    srand(time(NULL) + id);

    signal(SIGUSR1, unblock_handler);

    shm_ss_checkouts->checkout[id].open = (id < 3) ? 1 : 0;

    ClientMessage msg;

    while (!shm_sim_settings->stop_sim) {
        if (shm_ss_checkouts->checkout[id].open == 0) {
            usleep(500000 / shm_sim_settings->sim_speed);
            continue;
        };

        if (msgrcv(shm_queues->msq_ss_checkouts, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            if (errno != EINTR) {
                perror(_("Msgrcv error\n"));
            }
            break;
        }

        strcpy(logger_message, "");
        strcpy(logger_message_buf, "");

        am_i_blcked();

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

        bool approved = true;

        if (alcohol) {
            AgeVerificationRequest sreq;
            sreq.message_type = 1;
            sreq.client = msg.client;
            if (msgsnd(shm_queues->msq_staff, &sreq, sizeof(sreq) - sizeof(long), 0) == -1) {
                perror(_("Msgsnd error\n"));
            }

            AgeVerificationResponse sresp;
            if (msgrcv(shm_queues->msq_staff, &sresp, sizeof(sresp) - sizeof(long), (long)msg.client.id, 0) == -1) {
                perror(_("Msgrcv error\n"));
                approved = false;
            } else {
                approved = sresp.approved;
            }
        }

        if (!approved) {
            sprintf(logger_message_buf, _("You're only %d years old, I can't sell you that!\n"), msg.client.age);
            strcat(logger_message, logger_message_buf);
            save_a_log(LOG_SS_CHECKOUT, logger_message, shm_queues->msq_logger);

            ClientResponse cresp;
            cresp.message_type = msg.client.id;
            cresp.approved = 0;
            if (msgsnd(shm_queues->msq_client_resp, &cresp, sizeof(cresp) - sizeof(long), 0) == -1) {
                perror(_("Msgsnd error\n"));
            }
        } else {
            float cena = 0.0;
            for (int i = 0; i < msg.client.number_of_products; i++) {
                cena += msg.client.products[i].price;
            }
            sprintf(logger_message_buf, _("It'll be: %.2f PLN.\nThank you!\n"), cena);
            strcat(logger_message, logger_message_buf);
            save_a_log(LOG_SS_CHECKOUT, logger_message, shm_queues->msq_logger);

            ReceiptMessage receipt;
            receipt.message_type = msg.client.id;
            snprintf(receipt.message, sizeof(receipt.message), _("Receipt\nClient PID: %d\nCheckout ID: SS_%d\nItems bought:\n"), msg.client.id, id);

            for (int i = 0; i < msg.client.number_of_products; i++) {
                char buf[80];
                sprintf(buf, _("%s - %.2f PLN\n"), _(msg.client.products[i].name), msg.client.products[i].price);
                strncat(receipt.message, buf, sizeof(receipt.message) - strlen(receipt.message) - 1);
            }

            char buf_tot[80];

            sprintf(buf_tot, _("Total: %.2f PLN\n"), cena);
            strncat(receipt.message, buf_tot, sizeof(receipt.message) - strlen(receipt.message) - 1);

            if (msgsnd(shm_queues->msq_receipts, &receipt, sizeof(receipt) - sizeof(long), 0) == -1) {
                perror(_("Msgsnd error\n"));
            }

            ClientResponse cresp;
            cresp.message_type = msg.client.id;
            cresp.approved = 1;
            if (msgsnd(shm_queues->msq_client_resp, &cresp, sizeof(cresp) - sizeof(long), 0) == -1) {
                perror(_("Msgsnd error\n"));
            }
        }

        operation_wait(shm_semaphores->sem_checkouts);
        shm_ss_checkouts->checkout[id].clients_served++;
        operation_signal(shm_semaphores->sem_checkouts);
    }
};

/** @brief Initializes shared memory implementation */
void shm_init()
{
    shm_queues = (Queues*)shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*)shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_store_data = (StoreData*)shm_att(&shm_store_data_id, STORE_DATA);
    shm_sim_settings = (SimSettings*)shm_att(&shm_sim_settings_id, SIM_SETTINGS);
    shm_ss_checkouts = (SelfServiceCheckouts*)shm_att(&shm_ss_checkouts_id, SS_CHECKOUTS);
};

/** @brief Closes shared memory implementation */
void shm_close()
{
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_store_data);
    shm_det(shm_sim_settings);
    shm_det(shm_ss_checkouts);
};

/** @brief Checks if checkout is blocked */
void am_i_blcked()
{
    if (rand() % 100 < 5) {
        shm_ss_checkouts->checkout[id].blocked = 1;
        const char* reasons[4] = {
            _("Invalid product weight"),
            _("Invalid barcode"),
            _("Payment problem"),
            _("Other error")
        };
        char reason[240];
        strcpy(reason, reasons[rand() % 4]);

        SSBlockMessage block_msg;
        block_msg.message_type = 1;
        block_msg.checkout_id = id;
        strcpy(block_msg.reason, reason);

        if (msgsnd(shm_queues->msq_ss_staff, &block_msg, sizeof(block_msg) - sizeof(long), 0) == -1) {
            perror(_("Msgsnd error\n"));
        }

        sprintf(logger_message_buf, _("Staff assistance required: %s\n"), reason);
        strcat(logger_message, logger_message_buf);
        save_a_log(LOG_SS_CHECKOUT, logger_message, shm_queues->msq_logger);

        while (shm_ss_checkouts->checkout[id].blocked) {
            usleep(1000000 / shm_sim_settings->sim_speed);
        }
    }
}

/** @brief Unblock handler */
void unblock_handler(int sig)
{
    shm_ss_checkouts->checkout[id].blocked = 0;
}

/** @brief SIGALRM handler */
void sigalrm_handler(int sig)
{
    for (int i = 0; i < MAX_SS_CHECKOUTS; i++) {
        if (kill(pids[i], SIGTERM) == -1) {
            perror(_("Kill error\n"));
        }
    }
}
