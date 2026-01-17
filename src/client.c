#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

pid_t pids[MAX_CLIENTS];

volatile sig_atomic_t stop_sim = 0;

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

int active = 0;
double lambda;
double u;
double wait_time;

bool open;

Product available_products[PRODUCTS_AVAILABLE] = {
    { "Beer", 2.99, ALCOHOL },
    { "Wine", 39.99, ALCOHOL },
    { "Vodka", 44.99, ALCOHOL },
    { "Jagermeister", 69.99, ALCOHOL },
    { "Whisky", 74.99, ALCOHOL },

    { "Butter", 9.99, DAIRY },
    { "Apple juice", 5.49, JUICES },
    { "Orange juice", 6.99, JUICES },

    { "Bread", 4.29, BREAD },
    { "Milk", 3.19, DAIRY },
    { "Apple", 1.99, FRUIT },
    { "Yellow cheese", 19.99, DAIRY },

    { "Ham", 24.99, COLD_CUTS },
    { "Coffee", 15.99, DRY_GOODS },
    { "Tea", 6.49, OTHER },
    { "Sugar", 3.79, DRY_GOODS },

    { "Pasta", 5.29, DRY_GOODS },
    { "Rice", 4.79, DRY_GOODS },
    { "Cooking oil", 12.99, OTHER },
    { "Mineral water", 2.49, OTHER },

    { "Cola", 4.99, CARBONATED_DRINKS },
    { "Chocolate", 5.99, SWEETS },
    { "Chocolate bar", 2.79, SWEETS },
    { "Yogurt", 2.29, DAIRY },

    { "Jam", 7.49, OTHER },
    { "Ketchup", 6.99, OTHER },
    { "Mustard", 4.99, OTHER },
    { "Cookies", 5.49, SWEETS },

    { "Poppy seeds", 3.99, OTHER },
    { "Nuts", 14.99, OTHER },
    { "Honey", 18.99, OTHER },
    { "Spices", 3.49, DRY_GOODS }
};

void do_some_shopping();

void shm_init();
void shm_close();

int main(int argc, char* argv[])
{
    init_i18n();

    shm_init();

    srand(time(NULL));

    while (!stop_sim) {

        for (int i = 0; i < active; i++) {
            pid_t ret = waitpid(pids[i], NULL, WNOHANG);
            if (ret > 0) {
                pids[i] = pids[active - 1];
                active--;
                i--;
            }
        }

        open = shm_store_data->open;

        if (active < MAX_CLIENTS && open) {
            pid_t pid = fork();

            if (pid < 0) {
                perror(_("Fork error\n"));
            } else if (pid == 0) {
                do_some_shopping();
            } else {
                pids[active++] = pid;
            }
        }

        operation_wait(shm_semaphores->sem_store_data);
        shm_store_data->all_clients = active;
        operation_signal(shm_semaphores->sem_store_data);

        wait_time = ((6 + (cos(time(NULL) * 10) + 1) * 5 + (rand() % 7)) * 150000) / shm_sim_settings->sim_speed;
        usleep(wait_time * 3);
    }

    shm_close();

    exit(0);
}

void do_some_shopping()
{
    char logger_message[480];

    Client client;
    client.number_of_products = MIN_PRODUCTS + rand() % (MAX_PRODUCTS - MIN_PRODUCTS + 1);
    client.shopping_time = (double)(60 + 30 * client.number_of_products) / shm_sim_settings->sim_speed * 1000000;
    client.id = getpid();
    client.age = 5 + rand() % 90;

    for (int i = 0; i < client.number_of_products; i++) {
        client.products[i] = available_products[rand() % PRODUCTS_AVAILABLE];
    }

    sprintf(logger_message, _("(%d): Hello!\n"), client.id);
    save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);

    sprintf(logger_message, _("(%d): I'll buy %d items.\n"), client.id, client.number_of_products);
    save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);

    usleep(client.shopping_time);

    if (rand() % 100 < 5) {

        int msq;
        if (shm_checkouts->checkout[1].open == 1) {
            msq = (queue_length(shm_queues->msq_checkout_one) <= queue_length(shm_queues->msq_checkout_two)) ? shm_queues->msq_checkout_one : shm_queues->msq_checkout_two;
        } else {
            msq = shm_queues->msq_checkout_one;
        }
        sprintf(logger_message, _("(%d) I'll get in line for checkout. My number is %d.\n"), client.id, queue_length(msq));
        save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);
        stand_in_the_queue(client, msq);

    } else {
        sprintf(logger_message, _("(%d) I'll get in line for self-service. My number is %d.\n"), client.id, queue_length(shm_queues->msq_ss_checkouts));
        save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);
        stand_in_the_queue(client, shm_queues->msq_ss_checkouts);
    }

    ClientResponse cresp;
    if (msgrcv(shm_queues->msq_client_resp, &cresp, sizeof(cresp) - sizeof(long), (long)client.id, 0) != -1) {
        if (cresp.approved) {
            ReceiptMessage receipt;
            if (msgrcv(shm_queues->msq_receipts, &receipt, sizeof(receipt) - sizeof(long), (long)client.id, 0) != -1) {
                save_a_log(LOG_CLIENT, receipt.message, shm_queues->msq_logger);
            }
            sprintf(logger_message, _("(%d): Goodbye!\n"), getpid());
        } else {
            sprintf(logger_message, _("(%d): :C\n"), getpid());
        }
    } else {
        sprintf(logger_message, _("(%d): :C\n"), getpid());
    }

    save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);

    exit(0);
};

void shm_init()
{
    shm_queues = (Queues*)shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*)shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_store_data = (StoreData*)shm_att(&shm_store_data_id, STORE_DATA);
    shm_sim_settings = (SimSettings*)shm_att(&shm_sim_settings_id, SIM_SETTINGS);
    shm_ss_checkouts = (SelfServiceCheckouts*)shm_att(&shm_ss_checkouts_id, SS_CHECKOUTS);
    shm_checkouts = (Checkouts*)shm_att(&shm_checkouts_id, CHECKOUTS);
};

void shm_close()
{
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_store_data);
    shm_det(shm_sim_settings);
    shm_det(shm_ss_checkouts);
    shm_det(shm_checkouts);
};
