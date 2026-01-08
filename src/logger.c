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
#include "dyskont_utils.h"

int shm_kolejki_id;
int shm_semafory_id;
Kolejki *shm_kolejki;
Semafory *shm_semafory;

sig_atomic_t koniec = 0;

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
}

int main() {

    signal(SIGINT, obsluga_sygnalu);

    mkdir("logi", 0755);

    shm_kolejki_id = shmget(SHM_KOLEJKI, sizeof(Kolejki), IPC_CREAT | 0666);
    if (shm_kolejki_id == -1) {
        perror("shmget");
        exit(1);
    }

    Kolejki *shm_kolejki = (Kolejki *)shmat(shm_kolejki_id, NULL, 0);
    if (shm_kolejki == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    int kolejka_id = msgget(MSQ_LOG_ID, IPC_CREAT | 0600);
    if (kolejka_id == -1) {
        exit(1);
    }

    shm_kolejki->kol_logger = kolejka_id;

    shm_semafory_id = shmget(SHM_SEMAFORY, sizeof(Semafory), 0666);
    if (shm_semafory_id == -1) {
        exit(1);
    }

    shm_semafory = (Semafory *)shmat(shm_semafory_id, NULL, 0);
    if (shm_semafory == (void *)-1) {
        exit(1);
    }

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
            case LOG_SYM_INFO:
                prefix = "[INFO] ";
                kolor = KOLOR_INFO;
                break;
            case LOG_SYM_OSTRZEZENIE: prefix = "[OSTRZERZENIE] ";
                kolor = KOLOR_OSTRZEZENIE;
                break;
            case LOG_SYM_ERR:
                prefix = "[ERR] ";
                kolor = KOLOR_ERR;
                break;
            case LOG_DOMYSLNY:
                prefix = "";
                kolor = KOLOR_DOMYSLNY;
                break;
            case LOG_KIEROWNIK:
                prefix = "[KIEROWNIK] ";
                kolor = KOLOR_KIEROWNIK;
                break;
            case LOG_KLIENT:
                prefix = "[KLIENT] ";
                kolor = KOLOR_KLIENT;
                break;
            case LOG_OBSLUGA:
                prefix = "[OBSLUGA] ";
                kolor = KOLOR_OBSLUGA;
                break;
            case LOG_KASA_SAM:
                prefix = "[KASA SAMOOBSLUGOWA] ";
                kolor = KOLOR_KASA_SAM;
                break;
            case LOG_KASA_STAC:
                prefix = "[KASA STACJONARNA] ";
                kolor = KOLOR_KASA_STAC;
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
    shmdt(shm_kolejki);
    shmctl(shm_kolejki_id, IPC_RMID, NULL);

    exit(0);
}

