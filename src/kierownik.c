#include <unistd.h>
#include <stdlib.h>
#include "kierownik.h"
#include "shared.h"
#include "wiadomosc.h"

void wykonaj_prace(int semID, int kolejka_loggera_ID) {
    operacja_wait(semID);
    zapisz_log(LOG_INFO, "Tu kierownik!\n", kolejka_loggera_ID);
    sleep(1);
    operacja_signal(semID);
};

pid_t pids[128];
Semafory *shared;

int main(int argc, char *argv[]) {

    int shm_id = atoi(argv[1]);
    shared = (Semafory *)shmat(shm_id, NULL, 0);

    for (int i = 0; i < 5; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            zapisz_wiadomosc(COL_RED, "Błąd forka kierownik\n");
            exit(0);
        } else if (pids[i] == 0) {
            wykonaj_prace(shared->sem_kolejka_samoobslugowa, shared->logger_kolejka);
            exit(0);
        }
    }
}
