#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include "dyskont_utils.h"

int shm_kolejki_id;
int shm_semafory_id;
Kolejki *shm_kolejki;
Semafory *shm_semafory;
int shm_dane_id;
Dane *shm_dane;
int szybkosc_symulacji;

void wykonaj_prace();
void shm_init();
void shm_destroy();

int main(int argc, char *argv[]) {

    shm_init();
    wykonaj_prace();
    shm_destroy();

    exit(0);

}

void wykonaj_prace() {
    int dlugosc_symulacji;
    int szybkosc_symulacji;

    operacja_wait(shm_semafory->sem_sklep_dane);
    shm_dane->stan_sklepu = true;
    dlugosc_symulacji = shm_dane->dlugosc_symulacji;
    szybkosc_symulacji = shm_dane->szybkosc_symulacji;
    operacja_signal(shm_semafory->sem_sklep_dane);

    zapisz_log(LOG_INFO, "Tu kierownik!\n", shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);
    usleep(dlugosc_symulacji / szybkosc_symulacji * 1000000);
    zapisz_log(LOG_INFO, "Zamykam sklep.\n", shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);

    operacja_wait(shm_semafory->sem_sklep_dane);
    shm_dane->stan_sklepu = false;
    operacja_signal(shm_semafory->sem_sklep_dane);

    int ilosc_klientow;
    while (true) {
        operacja_wait(shm_semafory->sem_sklep_dane);
        ilosc_klientow = shm_dane->ilosc_klientow;
        operacja_signal(shm_semafory->sem_sklep_dane);

        if (ilosc_klientow <= 0) {
            zapisz_log(LOG_INFO, "Sklep zamkniety.\n", shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);
            kill(getppid(), SIGINT);
            exit(0);
        }

        sleep(1);
    }
};

void shm_init() {

    shm_kolejki_id = shmget(SHM_KOLEJKI, sizeof(Kolejki), 0666);
    if (shm_kolejki_id == -1) {
        exit(1);
    }
    shm_kolejki = (Kolejki *)shmat(shm_kolejki_id, NULL, 0);
    if (shm_kolejki == (void *)-1) {
        exit(1);
    }

    shm_semafory_id = shmget(SHM_SEMAFORY, sizeof(Semafory), 0666);
    if (shm_semafory_id == -1) {
        exit(1);
    }

    shm_semafory = (Semafory *)shmat(shm_semafory_id, NULL, 0);
    if (shm_semafory == (void *)-1) {
        exit(1);
    }

    shm_dane_id = shmget(SHM_DANE, sizeof(Dane), 0666);
    if (shm_dane_id == -1) {
        exit(1);
    }

    shm_dane = (Dane *)shmat(shm_dane_id, NULL, 0);
    if (shm_dane == (void *)-1) {
        exit(1);
    }
};
void shm_destroy() {
    shmdt(shm_kolejki);
    shmctl(shm_kolejki_id, IPC_RMID, NULL);

    shmdt(shm_semafory);
    shmctl(shm_semafory_id, IPC_RMID, NULL);

    shmdt(shm_dane);
    shmctl(shm_dane_id, IPC_RMID, NULL);

};
