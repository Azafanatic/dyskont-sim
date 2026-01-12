#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "logger.h"
#include "utils.h"

int shm_semaphores_id;
int shm_queues_id;
Semaphores *shm_semaphores;
Queues *shm_queues;

sig_atomic_t stop_sim = 0;

void sigint_handler(int sig);

void shm_init();
void shm_close();

int main() {

    signal(SIGINT, sigint_handler);

    mkdir("logi", 0755);

    shm_init();

    char sciezka[64];
    sprintf(sciezka, "logi/log_%d.log", (int)time(NULL));

    int deskryptor_pliku = open(sciezka, O_WRONLY | O_CREAT, 0644);
    if (deskryptor_pliku == -1) {
        exit(1);
    }

    LogMessage msg;

    while (!stop_sim) {
        if (msgrcv(shm_queues->msq_logger, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            break;
        }

        const char* prefix = "";
        const char* colour = COL_INFO;

        switch (msg.log_type) {
            case LOG_SIM_INFO:
                prefix = "[INFO] ";
                colour = COL_INFO;
                break;
            case LOG_SIM_WARN: prefix = "[OSTRZERZENIE] ";
                colour = COL_WARN;
                break;
            case LOG_SIM_ERR:
                prefix = "[ERR] ";
                colour = COL_ERR;
                break;
            case LOG_DEF:
                prefix = "";
                colour = COL_DEF;
                break;
            case LOG_MANAGER:
                prefix = "[KIEROWNIK] ";
                colour = COL_MANAGER;
                break;
            case LOG_CLIENT:
                prefix = "[KLIENT] ";
                colour = COL_CLIENT;
                break;
            case LOG_STAFF:
                prefix = "[OBSLUGA] ";
                colour = COL_STAFF;
                break;
            case LOG_SS_CHECKOUT:
                prefix = "[KASA SAMOOBSLUGOWA] ";
                colour = COL_SS_CHECKOUT;
                break;
            case LOG_CHECKOUT:
                prefix = "[KASA STACJONARNA] ";
                colour = COL_CHECKOUT;
                break;
        }

        printf("%s%s%s%s", colour, prefix, msg.message, COL_DEF);

        char bufor_wiadomosci[512];
        sprintf(bufor_wiadomosci, "%s%s", prefix, msg.message);
        write(deskryptor_pliku, bufor_wiadomosci, strlen(bufor_wiadomosci));
    }

    close(deskryptor_pliku);

    shm_close();

    exit(0);
}

void sigint_handler(int sig) {
    if (sig == SIGINT) {
        stop_sim = 1;
    }
}

void shm_init() {
    shm_queues = (Queues*) shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*) shm_att(&shm_semaphores_id, SEMAPHORES);
};

void shm_close() {
    shm_det(shm_queues);
    shm_det(shm_semaphores);
};


