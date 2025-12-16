#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "shared.h"
#include "wiadomosc.h"

int sem_kolejka_samoobslugowa;
int sem_kolejka_stacjonarna;
int sem_otwieranie_kasy;
int sem_zamykanie_kasy;
int sem_raport;

volatile sig_atomic_t koniec = 0;
int globalny_id_klienta = 0;

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
}

int main(int argc, char *argv[]) {
    pid_t pids[3];
    int pid_index = 3;

    sem_kolejka_samoobslugowa = utworz_semafor(SEM_ID_KOLEJKA_SAMOOBSLUGOWA);
    sem_kolejka_stacjonarna = utworz_semafor(SEM_ID_KOLEJKA_STACJONARNA);
    sem_otwieranie_kasy = utworz_semafor(SEM_ID_OTWIERANIE_KASY);
    sem_zamykanie_kasy = utworz_semafor(SEM_ID_ZAMYKANIE_KASY);
    sem_raport = utworz_semafor(SEM_ID_RAPORT);

    // Logger
    pids[0] = fork();
    if (pids[0] < 0) {
        //TODO: zmien na system logowania
        msg(COL_RED, "Błąd forka loggera\n");
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(1);
    } else if (pids[0] == 0) {
        execlp("./logger", "logger", (char *)NULL);
        //TODO: zmien na system logowania
        msg(COL_RED, "Błąd exec dla loggera\n");
        exit(1);
    }

    // Kierownik
    pids[1] = fork();
    if (pids[1] < 0) {
        //TODO: zmien na system logowania
        msg(COL_RED, "Błąd forka kierownik\n");
        exit(1);
    } else if (pids[1] == 0) {
        execlp("./kierownik", "kierownik", (char *)NULL);
        //TODO: zmien na system logowania
        msg(COL_RED, "Błąd exec dla kierownik\n");
        exit(1);
    }

    // Klient
    pids[2] = fork();
    if (pids[2] < 0) {
        //TODO: zmien na system logowania
        msg(COL_RED, "Błąd forka klient\n");
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(1);
    } else if (pids[2] == 0) {
        execlp("./klient", "klient", (char *)NULL);
        //TODO: zmien na system logowania
        msg(COL_RED, "Błąd exec dla klient\n");
        exit(1);
    }

    signal(SIGINT, obsluga_sygnalu);

    // WORKAROUND: program czasem zwracał błąd, bo kolejka nie byłą jeszcze utworzona
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

    int kolejka_id = shm_logger_kolejka_id->kolejka_id;

    zapisz_log(LOG_INFO, "LOG TEST INFO\n", kolejka_id);
    zapisz_log(LOG_OSTRZEZENIE, "LOG TEST OSTRZEZENIE\n", kolejka_id);
    zapisz_log(LOG_ERR, "LOG TEST BLAD\n", kolejka_id);

    while (!koniec) {
        sleep(1);
    }

    for (int i = 0; i < pid_index; i++) {
        kill(pids[i], SIGINT);
    }

    for (int i = 0; i < pid_index; i++) {
        waitpid(pids[i], NULL, 0);
    }

    usun_semafor(sem_kolejka_samoobslugowa);
    usun_semafor(sem_kolejka_stacjonarna);
    usun_semafor(sem_otwieranie_kasy);
    usun_semafor(sem_zamykanie_kasy);
    usun_semafor(sem_raport);

    return 0;
}
