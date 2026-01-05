#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "kierownik.h"
#include "shared.h"

void wykonaj_prace(int semID, int kolejka_loggera_ID) {
    operacja_wait(semID);
    zapisz_log(LOG_INFO, "Tu kierownik!\n", kolejka_loggera_ID);
    sleep(8);
    zapisz_log(LOG_OSTRZEZENIE, "Zamykam sklep.\n", kolejka_loggera_ID);
    kill(getppid(), SIGINT);
    operacja_signal(semID);
};

SHM *shared;

int main(int argc, char *argv[]) {
    int shm_id = atoi(argv[1]);
    shared = (SHM *)shmat(shm_id, NULL, 0);

    wykonaj_prace(shared->sem_kolejka_samoobslugowa, shared->logger_kolejka);
    exit(0);
}
