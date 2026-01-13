#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "utils.h"

pid_t pids[MAX_CLIENTS];

volatile sig_atomic_t stop_sim = 0;

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

int active = 0;
double lambda;
double u;
double wait_time;

bool open;

volatile sig_atomic_t done = 0;
bool success;

Product available_products[PRODUCTS_AVAILABLE] =
{{"Piwo", 2.99, ALKOHOLE}, {"Wino", 39.99, ALKOHOLE}, {"Wodka", 44.99, ALKOHOLE}, {"Jagermeister", 69.99, ALKOHOLE},
{"Whisky", 74.99, ALKOHOLE}, {"Maslo", 9.99, NABIAL}, {"Sok jablkowy", 5.49, SOKI}, {"Sok pomaranczowy", 6.99, SOKI},
{"Chleb", 4.29, PIECZYWO}, {"Mleko", 3.19, NABIAL}, {"Jablko", 1.99, OWOCE}, {"Ser zolty", 19.99, NABIAL},
{"Szynka", 24.99, WEDLINY}, {"Kawa", 15.99, SUCHE}, {"Herbata", 6.49, INNE}, {"Cukier", 3.79, SUCHE},
{"Makaron", 5.29, SUCHE}, {"Ryz", 4.79, SUCHE}, {"Olej", 12.99, INNE}, {"Woda mineralna", 2.49, INNE},
{"Cola", 4.99, NAPOJE_GAZOWANE}, {"Czekolada", 5.99, SLODYCZE}, {"Batonik", 2.79, SLODYCZE}, {"Jogurt", 2.29, NABIAL},
{"Dzem", 7.49, INNE}, {"Ketchup", 6.99, INNE}, {"Musztarda", 4.99, INNE}, {"Ciasteczka", 5.49, SLODYCZE},
{"Mak", 3.99, INNE}, {"Orzechy", 14.99, INNE}, {"Miod", 18.99, INNE}, {"Przyprawy", 3.49, SUCHE}};


void do_some_shopping();

void shm_init();
void shm_close();

void sig_handler(int sig);

int main(int argc, char *argv[]) {

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
                perror("fork");
            }
            else if (pid == 0) {
                do_some_shopping();
            }
            else {
                pids[active++] = pid;
            }
        }

        operation_wait(shm_semaphores->sem_store_data);
        shm_store_data->all_clients = active;
        operation_signal(shm_semaphores->sem_store_data);


        //TODO; znaleźć lepszy sposób na wprowadzanie klientów ze zmiennym tempem
        wait_time = ((6 + (cos(time(NULL) * 10) + 1) * 5 + (rand() % 7)) * 150000 )/ shm_sim_settings->sim_speed;
        usleep(wait_time * 3);
    }

    shm_close();

    exit(0);
}

void do_some_shopping() {
    char logger_message[240];

    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);

    Client client;
    client.number_of_products = MIN_PRODUCTS + rand() % (MAX_PRODUCTS - MIN_PRODUCTS + 1);
    client.shopping_time = (double) (60 + 30 * client.number_of_products) / shm_sim_settings->sim_speed * 1000000;
    client.id = getpid();
    client.age = 5 + rand() % 90;

    for (int i = 0; i < client.number_of_products; i++) {
        client.products[i] = available_products[rand() % PRODUCTS_AVAILABLE];
    }

    sprintf(logger_message, "(%d): Dzien dobry!\n",client.id);
    save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);

    sprintf(logger_message, "(%d): Kupie %d rzeczy.\n",client.id, client.number_of_products);
    save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);

    usleep(client.shopping_time);

    sprintf(logger_message, "Staje w kolejce. Moje miesce ma nr. %d\n", queue_length(shm_queues->msq_ss_checkouts));
    save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);
    stand_in_the_queue(client, shm_queues->msq_ss_checkouts);

    while (!done) {
        sleep(1);
    }

    if (success) {
        sprintf(logger_message, "(%d): Dowidzenia!\n",getpid());
    } else {
        sprintf(logger_message, "(%d): :C\n",getpid());
    }

    save_a_log(LOG_CLIENT, logger_message, shm_queues->msq_logger);

    exit(0);
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

void sig_handler(int sig) {
    if (sig == SIGUSR1) {
        success = false;
    } else {
        success = true;
    }
    done = 1;
}
