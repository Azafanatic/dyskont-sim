#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <math.h>
#include "dyskont_utils.h"

time_t czas_koniec;
int shm_kolejki_id;
int shm_semafory_id;
int shm_dane_id;
int shm_raport_id;
int shm_kasy_sam_id;
Kolejki *shm_kolejki;
Semafory *shm_semafory;
Dane *shm_dane;
Raport *shm_raport;
KasySam *shm_kasy_sam;

int dlugosc_symulacji;
int szybkosc_symulacji;

char wiadomosc[320];

int ilosc_klientow_w_kolejce_sam;
float czas_oczekiwania;

void wykonaj_prace();
void shm_init();
void shm_close();
void przelicz_otwarte_kasy();
void dostosuj_ilosc_otwartych_kas();

int main(int argc, char *argv[]) {

    shm_init();
    wykonaj_prace();
    shm_close();

    exit(0);
}

void wykonaj_prace() {
    operacja_wait(shm_semafory->sem_sklep_dane);
    shm_dane->stan_sklepu = true;
    dlugosc_symulacji = shm_dane->dlugosc_symulacji;
    szybkosc_symulacji = shm_dane->szybkosc_symulacji;
    operacja_signal(shm_semafory->sem_sklep_dane);

    czas_koniec = time(NULL) + (time_t)ceil(dlugosc_symulacji / (double) szybkosc_symulacji);

    zapisz_log(LOG_KIEROWNIK, "Otwieram sklep!\n", shm_kolejki->kol_logger);


    while (time(NULL) < czas_koniec) {
        ilosc_klientow_w_kolejce_sam = ilu_w_kolejce(shm_kolejki->kol_kasy_sam);

        operacja_wait(shm_semafory->sem_kasy);

        przelicz_otwarte_kasy();

        czas_oczekiwania = (10 + 3.0 * 6.5) * ilosc_klientow_w_kolejce_sam / shm_kasy_sam->otwarte_kasy / 60.;

        dostosuj_ilosc_otwartych_kas();

        sprintf(wiadomosc,"Czas oczekiwania %.2f \tOtwarte kasy: %d\n", czas_oczekiwania, shm_kasy_sam->otwarte_kasy);
        zapisz_log(LOG_KIEROWNIK, wiadomosc, shm_kolejki->kol_logger);

        operacja_signal(shm_semafory->sem_kasy);

        usleep(30.0 * 1000000 /szybkosc_symulacji);

    }

    zapisz_log(LOG_KIEROWNIK, "Zamykam sklep.\n", shm_kolejki->kol_logger);

    operacja_wait(shm_semafory->sem_sklep_dane);
    shm_dane->stan_sklepu = false;
    operacja_signal(shm_semafory->sem_sklep_dane);

    int ilosc_klientow_w_sklepie;

    while (true) {
        operacja_wait(shm_semafory->sem_sklep_dane);
        ilosc_klientow_w_sklepie = shm_dane->ilosc_klientow;
        operacja_signal(shm_semafory->sem_sklep_dane);
        if (ilosc_klientow_w_sklepie <= 0) {
            zapisz_log(LOG_KIEROWNIK, "Sklep zamkniety.\n", shm_kolejki->kol_logger);
            kill(getppid(), SIGINT);
            exit(0);
        }
        sleep(1);
    }
};

void przelicz_otwarte_kasy() {
    if (shm_kasy_sam->kasa[5].otwarta) {
        shm_kasy_sam->otwarte_kasy = 6;
    } else if (shm_kasy_sam->kasa[4].otwarta) {
        shm_kasy_sam->otwarte_kasy = 5;
    } else if (shm_kasy_sam->kasa[3].otwarta) {
        shm_kasy_sam->otwarte_kasy = 4;
    } else {
        shm_kasy_sam->otwarte_kasy = 3;
    }
};

void dostosuj_ilosc_otwartych_kas() {
    if (czas_oczekiwania > 2.0) {
        shm_kasy_sam->kasa[3].otwarta = true;
    } else {
        shm_kasy_sam->kasa[3].otwarta = false;
    }

    if (czas_oczekiwania > 4.0) {
        shm_kasy_sam->kasa[4].otwarta = true;
    } else {
        shm_kasy_sam->kasa[4].otwarta = false;
    }

    if (czas_oczekiwania > 6.0) {
        shm_kasy_sam->kasa[5].otwarta = true;
    } else {
        shm_kasy_sam->kasa[5].otwarta = false;
    }
};

void shm_init() {
    shm_kolejki = (Kolejki*) shm_att(&shm_kolejki_id, KOLEJKI);
    shm_semafory = (Semafory*) shm_att(&shm_semafory_id, SEMAFORY);
    shm_dane = (Dane*) shm_att(&shm_dane_id, DANE);
    shm_raport = (Raport*) shm_att(&shm_raport_id, RAPORT);
    shm_kasy_sam = (KasySam*) shm_create(&shm_kasy_sam_id, KASY_SAM);
};

void shm_close() {
    shm_destroy(shm_kolejki_id, shm_kolejki);
    shm_destroy(shm_semafory_id, shm_semafory);
    shm_destroy(shm_dane_id, shm_dane);
    shm_destroy(shm_raport_id, shm_raport);
    shm_destroy(shm_kasy_sam_id, shm_kasy_sam);
};
