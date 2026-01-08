#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>
#include "dyskont_utils.h"

pid_t pids[MAX_KLIENCI];
volatile sig_atomic_t koniec = 0;
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

    srand(time(NULL));

    int aktywni = 0;
    bool stan_sklepu;

    operacja_wait(shm_semafory->sem_sklep_dane);
    szybkosc_symulacji = shm_dane->szybkosc_symulacji;
    operacja_signal(shm_semafory->sem_sklep_dane);

    while (!koniec) {

        for (int i = 0; i < aktywni; i++) {
            pid_t ret = waitpid(pids[i], NULL, WNOHANG);
            if (ret > 0) {
                pids[i] = pids[aktywni - 1];
                aktywni--;
                i--;
            }
        }

        operacja_wait(shm_semafory->sem_sklep_dane);
        stan_sklepu = shm_dane->stan_sklepu;
        operacja_signal(shm_semafory->sem_sklep_dane);

        if (aktywni < MAX_KLIENCI && stan_sklepu) {
            pid_t pid = fork();

            if (pid < 0) {
                perror("fork");
            }
            else if (pid == 0) {
                wykonaj_prace();
            }
            else {
                pids[aktywni++] = pid;
            }
        }

        operacja_wait(shm_semafory->sem_sklep_dane);
        shm_dane->ilosc_klientow = aktywni;
        operacja_signal(shm_semafory->sem_sklep_dane);

        usleep((rand() % 3) * 1000000 / szybkosc_symulacji);
    }

    shm_destroy();

    exit(0);
}

void wykonaj_prace() {
    int ilosc_rzeczy = 3 + rand() % 7;
    char wiadomosc[256];

    sprintf(wiadomosc, "[Klient %d]: Dzien dobry!\n",getpid());
    zapisz_log(LOG_OSTRZEZENIE, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);
    sprintf(wiadomosc, "[Klient %d]: Kupie %d rzeczy.\n",getpid(), ilosc_rzeczy);
    zapisz_log(LOG_OSTRZEZENIE, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);
    usleep((30 + rand() % 30) * 1000000 / szybkosc_symulacji);
    sprintf(wiadomosc, "[Klient %d]: Dowidzenia!\n",getpid());
    zapisz_log(LOG_OSTRZEZENIE, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);

    exit(0);
};

void shm_init(){
    shm_kolejki_id = shmget(SHM_KOLEJKI, sizeof(Kolejki), 0666);
    if (shm_kolejki_id == -1) {
        exit(1);
    }

    shm_kolejki = (Kolejki *)shmat(shm_kolejki_id, NULL, 0);
    if (shm_kolejki == (void *)-1) {
        exit(1);
    }

    shm_semafory_id = shmget(SHM_SEMAFORY, sizeof(Semafory), 0666);
    if (shm_kolejki_id == -1) {
        exit(1);
    }

    shm_semafory = (Semafory *)shmat(shm_semafory_id, NULL, 0);
    if (shm_kolejki == (void *)-1) {
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
};
