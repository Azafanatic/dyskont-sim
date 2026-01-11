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
int globalny_id_klienta = 0;
char shm_id_str[8];
int shm_dane_id;
Dane *shm_dane;
int shm_semafory_id;
Semafory *shm_semafory;
int shm_kolejki_id;
Kolejki *shm_kolejki;
int shm_raport_id;
Raport *shm_raport;
pid_t pids[PROCESY_GLOWNE];
char wiadomosc[240];

void obsluga_sygnalu(int sig);
void shm_init();
void msq_init();
void shm_destroy();

int main(int argc, char *argv[]) {

    shm_init();
    msq_init();

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

    shm_destroy();

    return 0;
}

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
}

void shm_init() {
    shm_semafory_id = shmget(SHM_SEMAFORY, sizeof(Semafory), IPC_CREAT | 0666);
    if (shm_semafory_id == -1) {
        perror("shmget");
        exit(1);
    }
    shm_semafory = (Semafory *)shmat(shm_semafory_id, NULL, 0);
    if (shm_semafory == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    shm_semafory->sem_otwieranie_kasy = utworz_semafor(SEM_ID_OTWIERANIE_KASY);
    shm_semafory->sem_zamykanie_kasy = utworz_semafor(SEM_ID_ZAMYKANIE_KASY);
    shm_semafory->sem_raport = utworz_semafor(SEM_ID_RAPORT);
    shm_semafory->sem_kolejki = utworz_semafor(SEM_ID_KOLEJKI);
    shm_semafory->sem_sklep_dane = utworz_semafor(SEM_ID_SKLEP_DANE);



    shm_kolejki_id = shmget(SHM_KOLEJKI, sizeof(Kolejki), IPC_CREAT | 0666);
    if (shm_kolejki_id == -1) {
        perror("shmget");
        exit(1);
    }
    shm_kolejki = (Kolejki *)shmat(shm_kolejki_id, NULL, 0);
    if (shm_kolejki == (void *)-1) {
        perror("shmat");
        exit(1);
    }



    shm_dane_id = shmget(SHM_DANE, sizeof(Dane), IPC_CREAT | 0666);
    if (shm_dane_id == -1) {
        perror("shmget");
        exit(1);
    }
    shm_dane = (Dane *)shmat(shm_dane_id, NULL, 0);
    if (shm_dane == (void *)-1) {
        perror("shmat");
        exit(1);
    }


    shm_raport_id = shmget(SHM_RAPORT, sizeof(Raport), IPC_CREAT | 0666);
    if (shm_raport_id == -1) {
        exit(1);
    }

    shm_raport = (Raport *)shmat(shm_raport_id, NULL, 0);
    if (shm_raport == (void *)-1) {
        exit(1);
    }
}

void msq_init() {
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
}

void shm_destroy() {
    usun_semafor(shm_semafory->sem_otwieranie_kasy);
    usun_semafor(shm_semafory->sem_zamykanie_kasy);
    usun_semafor(shm_semafory->sem_raport);
    usun_semafor(shm_semafory->sem_kolejki);
    usun_semafor(shm_semafory->sem_sklep_dane);

    msgctl(shm_kolejki->kol_logger, IPC_RMID, NULL);
    msgctl(shm_kolejki->kol_kasy_sam, IPC_RMID, NULL);

    shmdt(shm_kolejki);
    shmctl(shm_kolejki_id, IPC_RMID, NULL);

    shmdt(shm_semafory);
    shmctl(shm_semafory_id, IPC_RMID, NULL);

    shmdt(shm_dane);
    shmctl(shm_dane_id, IPC_RMID, NULL);

    shmdt(shm_raport);
    shmctl(shm_raport_id, IPC_RMID, NULL);
};

