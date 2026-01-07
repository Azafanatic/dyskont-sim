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
    if (sig == SIGINT) {
        koniec = 1;
    }
}

int main(int argc, char *argv[]) {
    pid_t pids[6];
    int pid_index = 6;

    int shm_semafory_id = shmget(SHM_SEMAFORY, sizeof(Semafory), IPC_CREAT | 0666);
    if (shm_semafory_id == -1) {
        perror("shmget");
        exit(1);
    }

    Semafory *shm_semafory = (Semafory *)shmat(shm_semafory_id, NULL, 0);
    if (shm_semafory == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    shm_semafory->sem_kolejka_samoobslugowa = utworz_semafor(SEM_ID_KOLEJKA_SAMOOBSLUGOWA);
    shm_semafory->sem_kolejka_stacjonarna = utworz_semafor(SEM_ID_KOLEJKA_STACJONARNA);
    shm_semafory->sem_otwieranie_kasy = utworz_semafor(SEM_ID_OTWIERANIE_KASY);
    shm_semafory->sem_zamykanie_kasy = utworz_semafor(SEM_ID_ZAMYKANIE_KASY);
    shm_semafory->sem_raport = utworz_semafor(SEM_ID_RAPORT);
    shm_semafory->sem_kolejka_logger = utworz_semafor(SEM_ID_KOLEJKA_LOGGER);
    shm_semafory->sem_sklep_dane = utworz_semafor(SEM_ID_SKLEP_DANE);

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

    int shm_kolejki_id = shmget(SHM_KOLEJKI, sizeof(Kolejki), 0666);
    if (shm_kolejki_id == -1) {
        perror("shmget");
        exit(1);
    }

    Kolejki *shm_kolejki = (Kolejki *)shmat(shm_kolejki_id, NULL, 0);
    if (shm_kolejki == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    zapisz_log(LOG_INFO, "ROZPOCZYNAM SYMULACJE\n", shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);

    char * argumenty[5] = {"kierownik", "klient", "kasa_samoobslugowa", "kasa_stacjonarna", "obsluga"};
    char argZero[32];
    char blad[128];

    for (int i = 0; i < 5; i++) {
        pids[i] = fork();

        sprintf(argZero, "./%s", argumenty[i]);

        if (pids[i] < 0) {
            zapisz_wiadomosc(COL_RED, "Błąd forka\n");
            exit(1);
        } else if (pids[i] == 0) {
            execlp(argZero, argumenty[i], (char *)NULL);
            zapisz_wiadomosc(COL_RED, "Błąd exec\n");
            exit(1);
        }
    }

    signal(SIGINT, obsluga_sygnalu);

    while (!koniec) {
        sleep(1);
    }

    zapisz_log(LOG_INFO, "KONCZE SYMULACJE\n", shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);

    sleep(1);

    for (int i = 0; i < pid_index; i++) {
        kill(pids[i], SIGINT);
    }

    for (int i = 0; i < pid_index; i++) {
        waitpid(pids[i], NULL, 0);
    }

    usun_semafor(shm_semafory->sem_kolejka_samoobslugowa);
    usun_semafor(shm_semafory->sem_kolejka_stacjonarna);
    usun_semafor(shm_semafory->sem_otwieranie_kasy);
    usun_semafor(shm_semafory->sem_zamykanie_kasy);
    usun_semafor(shm_semafory->sem_raport);
    usun_semafor(shm_semafory->sem_kolejka_logger);
    usun_semafor(shm_semafory->sem_sklep_dane);

    shmdt(shm_kolejki);
    shmctl(shm_kolejki_id, IPC_RMID, NULL);

    shmdt(shm_semafory);
    shmctl(shm_semafory_id, IPC_RMID, NULL);

    return 0;
}
