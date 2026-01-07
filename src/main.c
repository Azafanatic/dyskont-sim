#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include "dyskont_utils.h"

volatile sig_atomic_t koniec = 0;
int globalny_id_klienta = 0;
char shm_id_str[8];

void obsluga_sygnalu(int sig) {
    zapisz_wiadomosc(COL_RED, "Otrzymano SIGINT\n");
    if (sig == SIGINT) {
        koniec = 1;
    }
}

int main(int argc, char *argv[]) {
    pid_t pids[3];
    int pid_index = 3;

    int shm_id = shmget(IPC_PRIVATE, sizeof(SHM), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    SHM *shared = (SHM *)shmat(shm_id, NULL, 0);
    if (shared == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    sprintf(shm_id_str, "%d", shm_id);

    shared->sem_kolejka_samoobslugowa = utworz_semafor(SEM_ID_KOLEJKA_SAMOOBSLUGOWA);
    shared->sem_kolejka_stacjonarna = utworz_semafor(SEM_ID_KOLEJKA_STACJONARNA);
    shared->sem_otwieranie_kasy = utworz_semafor(SEM_ID_OTWIERANIE_KASY);
    shared->sem_zamykanie_kasy = utworz_semafor(SEM_ID_ZAMYKANIE_KASY);
    shared->sem_raport = utworz_semafor(SEM_ID_RAPORT);

    // Logger
    pids[0] = fork();
    if (pids[0] < 0) {
        zapisz_wiadomosc(COL_RED, "Błąd forka loggera\n");
        exit(1);
    } else if (pids[0] == 0) {
        execlp("./logger", "logger", (char *)NULL);
        zapisz_wiadomosc(COL_RED, "Błąd exec dla loggera\n");
        exit(1);
    }

    sleep(1);

    int shm_logger = shmget(SHM_KOLEJKA_LOG, sizeof(KolejkaLogger), 0666);
    if (shm_logger == -1) {
        perror("shmget");
        exit(1);
    }

    KolejkaLogger *shm_logger_kolejka_id = (KolejkaLogger *)shmat(shm_logger, NULL, 0);
    if (shm_logger_kolejka_id == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    shared->logger_kolejka = shm_logger_kolejka_id->kolejka_id;

    zapisz_log(LOG_INFO, "ROZPOCZYNAM SYMULACJE\n", shared->logger_kolejka);

    char * argumenty[6] = {"logger", "kierownik", "klient", "kasa_samoobslugowa", "kasa_stacjonarna", "obsluga"};
    char argZero[32];
    char blad[128];

    for (int i = 1; i < 6; i++) {
        pids[i] = fork();

        sprintf(argZero, "./%s", argumenty[i]);

        if (pids[i] < 0) {
            zapisz_wiadomosc(COL_RED, "Błąd forka\n");
            exit(1);
        } else if (pids[i] == 0) {
            execlp(argZero, argumenty[i], shm_id_str,(char *)NULL);
            zapisz_wiadomosc(COL_RED, "Błąd exec\n");
            exit(1);
        }
    }

    signal(SIGINT, obsluga_sygnalu);

    while (!koniec) {
        sleep(1);
    }

    zapisz_log(LOG_INFO, "ZAKAŃCZAM SYMULACJE\n", shared->logger_kolejka);

    sleep(1);

    for (int i = 0; i < pid_index; i++) {
        kill(pids[i], SIGINT);
    }

    for (int i = 0; i < pid_index; i++) {
        waitpid(pids[i], NULL, 0);
    }

    usun_semafor(shared->sem_kolejka_samoobslugowa);
    usun_semafor(shared->sem_kolejka_stacjonarna);
    usun_semafor(shared->sem_otwieranie_kasy);
    usun_semafor(shared->sem_zamykanie_kasy);
    usun_semafor(shared->sem_raport);

    return 0;
}
