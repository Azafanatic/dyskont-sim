#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include "dyskont_utils.h"

#define PROCESY_GLOWNE 6

volatile sig_atomic_t koniec = 0;

int shm_dane_id;
int shm_semafory_id;
int shm_kolejki_id;
int shm_raport_id;
int shm_kasy_sam_id;
Dane * shm_dane;
Semafory *shm_semafory;
Kolejki *shm_kolejki;
Raport *shm_raport;
KasySam *shm_kasy_sam;

pid_t pids[PROCESY_GLOWNE];
char wiadomosc[240];

void obsluga_sygnalu(int sig);

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

    operacja_wait(shm_semafory->sem_raport);
    shm_raport->sprzedane_produkty = 0;
    shm_raport->wszyscy_klienci = 0;
    operacja_signal(shm_semafory->sem_raport);

    operacja_wait(shm_semafory->sem_sklep_dane);
    shm_dane->dlugosc_symulacji = (argc >= 2) ? atoi(argv[1]) : 3600;
    shm_dane->szybkosc_symulacji = (argc >= 3) ? atoi(argv[2]) : 60;
    operacja_signal(shm_semafory->sem_sklep_dane);

    zapisz_log(LOG_SYM_INFO, "ROZPOCZYNAM SYMULACJE\n", shm_kolejki->kol_logger);
    sprintf(wiadomosc, "Dane symulacji:\nCzas trwania:%d (sek)\t Predkosc: %d\t\n", shm_dane->dlugosc_symulacji, shm_dane->szybkosc_symulacji);
    zapisz_log(LOG_SYM_INFO, wiadomosc, shm_kolejki->kol_logger);

    {
        char * argumenty[PROCESY_GLOWNE] = {"logger", "kierownik", "klient", "kasa_samoobslugowa", "kasa_stacjonarna", "obsluga"};
        char argZero[32];
        char blad[128];

        for (int i = 0; i < PROCESY_GLOWNE; i++) {
            pids[i] = fork();

            sprintf(argZero, "./%s", argumenty[i]);

            if (pids[i] < 0) {
                perror("Błąd forka\n");
                exit(1);
            } else if (pids[i] == 0) {
                execlp(argZero, argumenty[i], (char *)NULL);
                perror("Błąd exec\n");
                exit(1);
            }
        }
    }

    signal(SIGINT, obsluga_sygnalu);

    while (!koniec) {
        sleep(1);
    }

    zapisz_log(LOG_SYM_INFO, "KONCZE SYMULACJE\n", shm_kolejki->kol_logger);

    sleep(1);
    for (int i = 0; i < PROCESY_GLOWNE; i++) {
        kill(pids[i], SIGINT);
    }
    for (int i = 0; i < PROCESY_GLOWNE; i++) {
        waitpid(pids[i], NULL, 0);
    }

    msq_destroy();
    sem_destroy();
    shm_close();

    return 0;
}

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
};

void shm_init() {
    shm_kolejki = (Kolejki*) shm_create(&shm_kolejki_id, KOLEJKI);
    shm_semafory = (Semafory*) shm_create(&shm_semafory_id, SEMAFORY);
    shm_dane = (Dane*) shm_create(&shm_dane_id, DANE);
    shm_raport = (Raport*) shm_create(&shm_raport_id, RAPORT);
    shm_kasy_sam = (KasySam*) shm_create(&shm_kasy_sam_id, KASY_SAM);

    for (int i = 0; i < MAX_KASY_SAM; i++) {
        shm_kasy_sam[i].kasa->otwarta = false;
    }
};

void shm_close() {
    shm_destroy(shm_kolejki_id, shm_kolejki);
    shm_destroy(shm_semafory_id, shm_semafory);
    shm_destroy(shm_dane_id, shm_dane);
    shm_destroy(shm_raport_id, shm_raport);
    shm_destroy(shm_kasy_sam_id, shm_kasy_sam);
};

void sem_create() {
    shm_semafory->sem_kasy = utworz_semafor(SEM_ID_KASY);
    shm_semafory->sem_raport = utworz_semafor(SEM_ID_RAPORT);
    shm_semafory->sem_kolejki = utworz_semafor(SEM_ID_KOLEJKI);
    shm_semafory->sem_sklep_dane = utworz_semafor(SEM_ID_SKLEP_DANE);
};

void sem_destroy() {
    usun_semafor(shm_semafory->sem_kasy);
    usun_semafor(shm_semafory->sem_raport);
    usun_semafor(shm_semafory->sem_kolejki);
    usun_semafor(shm_semafory->sem_sklep_dane);
};

void msq_create() {
    int msq_logger_id = msgget(MSQ_LOG_ID, IPC_CREAT | 0600);
    if (msq_logger_id == -1) {
        exit(1);
    }
    int msq_kasy_sam_id = msgget(MSQ_KASY_SAM_ID, IPC_CREAT | 0600);
    if (msq_kasy_sam_id == -1) {
        exit(1);
    }

    operacja_wait(shm_semafory->sem_kolejki);
    shm_kolejki->kol_logger = msq_logger_id;
    shm_kolejki->kol_kasy_sam = msq_kasy_sam_id;
    operacja_signal(shm_semafory->sem_kolejki);
};

void msq_destroy() {
    msgctl(shm_kolejki->kol_logger, IPC_RMID, NULL);
    msgctl(shm_kolejki->kol_kasy_sam, IPC_RMID, NULL);
};

