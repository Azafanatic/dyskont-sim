#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "logger.h"

static int kolejka_id = -1;
sig_atomic_t koniec = 0;

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
}

int main() {

    signal(SIGINT, obsluga_sygnalu);

    mkdir("logi", 0755);

    int shmid = shmget(SHM_KOLEJKA_LOG, sizeof(KolejkaLogger), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    KolejkaLogger *shared_data = (KolejkaLogger *)shmat(shmid, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    kolejka_id = msgget(MSQ_LOG_ID, IPC_CREAT | 0600);
    if (kolejka_id == -1) {
        exit(1);
    }

    shared_data->kolejka_id = kolejka_id;

    char sciezka[64];
    sprintf(sciezka, "logi/test.log");

    int deskryptor_pliku = open(sciezka, O_WRONLY | O_CREAT, 0644);
    if (deskryptor_pliku == -1) {
        exit(1);
    }

    struct Log msg;

    while (!koniec) {
        if (msgrcv(kolejka_id, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            break;
        }

        const char* prefix = "";
        const char* kolor = KOLOR_INFO;

        switch (msg.typ_logu) {
            case LOG_INFO:
                prefix = "[INFO] ";
                kolor = KOLOR_INFO;
                break;
            case LOG_OSTRZEZENIE: prefix = "[OSTRZERZENIE] ";
                kolor = KOLOR_OSTRZEZENIE;
                break;
            case LOG_ERR:
                prefix = "[ERR] ";
                kolor = KOLOR_ERR;
                break;
        }

        // TODO: Usunąć resetowanie koloru przy każdym logu
        printf("%s%s%s%s", kolor, prefix, msg.wiadomosc, KOLOR_DOMYSLNY);

        char bufor_wiadomosci[512];
        sprintf(bufor_wiadomosci, "%s%s", prefix, msg.wiadomosc);
        write(deskryptor_pliku, bufor_wiadomosci, strlen(bufor_wiadomosci));

    }

    close(deskryptor_pliku);
    msgctl(kolejka_id, IPC_RMID, NULL);
    shmdt(shared_data);
    shmctl(shmid, IPC_RMID, NULL);

    exit(0);
}

