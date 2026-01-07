#include <unistd.h>
#include <stdlib.h>
#include "dyskont_utils.h"

void wykonaj_prace(int semID, int kolejka_loggera_ID);

pid_t pids[MAX_KLIENCI];
SHM *shared;

int main(int argc, char *argv[]) {

    int shm_id = atoi(argv[1]);
    shared = (SHM *)shmat(shm_id, NULL, 0);

    for (int i = 0; i < 5; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            zapisz_wiadomosc(COL_RED, "Błąd forka kierownik\n");
            exit(0);
        } else if (pids[i] == 0) {
            wykonaj_prace(shared->sem_kolejka_stacjonarna, shared->logger_kolejka);
            exit(0);
        }
    }
}

void wykonaj_prace(int semID, int kolejka_loggera_ID) {
    operacja_wait(semID);
    zapisz_log(LOG_OSTRZEZENIE, "Tu klient!\n", kolejka_loggera_ID);
    sleep(1);
    operacja_signal(semID);
};
