#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dyskont_utils.h"

int shm_kolejki_id;
int shm_semafory_id;
int shm_dane_id;
int shm_raport_id;
Kolejki *shm_kolejki;
Semafory *shm_semafory;
Dane *shm_dane;
Raport *shm_raport;

int szybkosc_symulacji;

char wiadomosc[320];

void wykonaj_prace();
void shm_init();
void shm_close();

int main(int argc, char *argv[]) {

    shm_init();
    wykonaj_prace();
    shm_close();

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

    zapisz_log(LOG_KIEROWNIK, "Otwieram sklep!\n", shm_kolejki->kol_logger);

    int ilosc_pomiarow = 1000;
    double czas_pomiedzy_pomiarami = dlugosc_symulacji / (double) szybkosc_symulacji * 1000000 / ilosc_pomiarow;
    int zmierzeni_klienci = 0;

    for (int i = 0; i < ilosc_pomiarow; i++) {
        usleep(czas_pomiedzy_pomiarami);
        operacja_wait(shm_semafory->sem_sklep_dane);
        zmierzeni_klienci += shm_dane->ilosc_klientow;
        operacja_signal(shm_semafory->sem_sklep_dane);
    }

    operacja_wait(shm_semafory->sem_raport);
    shm_raport->prod_na_klienta = shm_raport->sprzedane_produkty / (float) shm_raport->wszyscy_klienci;
    shm_raport->klienci_w_sklepie = zmierzeni_klienci / (float) ilosc_pomiarow;
    operacja_signal(shm_semafory->sem_raport);

    zapisz_log(LOG_KIEROWNIK, "Zamykam sklep.\n", shm_kolejki->kol_logger);

    operacja_wait(shm_semafory->sem_sklep_dane);
    shm_dane->stan_sklepu = false;
    operacja_signal(shm_semafory->sem_sklep_dane);

    int ilosc_klientow;

    while (true) {
        operacja_wait(shm_semafory->sem_sklep_dane);
        ilosc_klientow = shm_dane->ilosc_klientow;
        operacja_signal(shm_semafory->sem_sklep_dane);

        if (ilosc_klientow <= 0) {
            zapisz_log(LOG_KIEROWNIK, "Sklep zamkniety.\n", shm_kolejki->kol_logger);
            kill(getppid(), SIGINT);

            operacja_wait(shm_semafory->sem_raport);
            sprintf(wiadomosc, "Raport:\nKlienci: %d\t Sprzedane produkty: %d\t Prod./Klient: %.2f\t Średnio klientów: %.2f\nWszystkie pieniadze: %.2f \t Klienci nieobslozeni: %d\n",shm_raport->wszyscy_klienci, shm_raport->sprzedane_produkty,  shm_raport->prod_na_klienta, shm_raport->klienci_w_sklepie, shm_raport->skasowane_pieniadze, shm_raport->klienci_nieobslozeni);
            operacja_signal(shm_semafory->sem_raport);
            zapisz_log(LOG_KIEROWNIK, wiadomosc, shm_kolejki->kol_logger);

            exit(0);
        }

        sleep(1);
    }
};

void shm_init() {
    shm_kolejki = (Kolejki*) shm_att(&shm_kolejki_id, KOLEJKI);
    shm_semafory = (Semafory*) shm_att(&shm_semafory_id, SEMAFORY);
    shm_dane = (Dane*) shm_att(&shm_dane_id, DANE);
    shm_raport = (Raport*) shm_att(&shm_raport_id, RAPORT);
};

void shm_close() {
    shm_destroy(shm_kolejki_id, shm_kolejki);
    shm_destroy(shm_semafory_id, shm_semafory);
    shm_destroy(shm_dane_id, shm_dane);
    shm_destroy(shm_raport_id, shm_raport);
};
