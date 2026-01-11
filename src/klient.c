#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "dyskont_utils.h"

#define ILOSC_DOSTEPNYCH_PRODUKTOW 32

pid_t pids[MAX_KLIENCI];

volatile sig_atomic_t koniec = 0;

int shm_kolejki_id;
int shm_semafory_id;
int shm_dane_id;
int shm_raport_id;
Kolejki *shm_kolejki;
Semafory *shm_semafory;
Dane *shm_dane;
Raport *shm_raport;

int szybkosc_symulacji;
int aktywni = 0;

bool stan_sklepu;

volatile sig_atomic_t skasowano = 0;
bool sukces;

Produkt dostepne_produkty[32] =
{{"Piwo", 2.99, true}, {"Wino", 39.99, true}, {"Wodka", 44.99, true}, {"Jagermeister", 69.99, true},
{"Whisky", 74.99, true}, {"Maslo", 9.99, false}, {"Sok jablkowy", 5.49, false}, {"Sok pomaranczowy", 6.99, false},
{"Chleb", 4.29, false}, {"Mleko", 3.19, false}, {"Jajka", 8.99, false}, {"Ser zolty", 19.99, false},
{"Szynka", 24.99, false}, {"Kawa", 15.99, false}, {"Herbata", 6.49, false}, {"Cukier", 3.79, false},
{"Makaron", 5.29, false}, {"Ryz", 4.79, false}, {"Olej", 12.99, false}, {"Woda mineralna", 2.49, false},
{"Cola", 4.99, false}, {"Czekolada", 5.99, false}, {"Batonik", 2.79, false}, {"Jogurt", 2.29, false},
{"Dzem", 7.49, false}, {"Ketchup", 6.99, false}, {"Musztarda", 4.99, false}, {"Ciasteczka", 5.49, false},
{"Mak", 3.99, false}, {"Orzechy", 14.99, false}, {"Miod", 18.99, false}, {"Przyprawy", 3.49, false}};


void wykonaj_prace();

void shm_init();
void shm_close();

void obsluga_sygnalu(int sig);

int main(int argc, char *argv[]) {

    shm_init();

    srand(time(NULL));

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

        // 6 + 0-10(zmienne z czasem) + 0-6 (losowo)
        usleep( (6 + (cos(time(NULL) * 10) + 1) * 5 + (rand() % 7)) * 150000 / szybkosc_symulacji);
    }

    shm_close();

    exit(0);
}

void wykonaj_prace() {
    char wiadomosc[240];

    signal(SIGUSR1, obsluga_sygnalu);
    signal(SIGUSR2, obsluga_sygnalu);

    Klient klient;
    klient.liczba_produktow = MIN_PRODUKTY + rand() % (MAX_PRODUKTY - MIN_PRODUKTY + 1);
    klient.czas_zakupow = (double) (120 + rand() % 30 * klient.liczba_produktow) / szybkosc_symulacji * 1000000;
    klient.id = getpid();
    klient.wiek = 5 + rand() % 90;
    klient.ma_alkohol = false;

    for (int i = 0; i < klient.liczba_produktow; i++) {
        klient.produkty[i] = dostepne_produkty[rand() % ILOSC_DOSTEPNYCH_PRODUKTOW];
    }

    for (int i = 0; i < klient.liczba_produktow; i++) {
        if (klient.produkty[i].alkohol == true) {
            klient.ma_alkohol = true;
            break;
        }
    }

    sprintf(wiadomosc, "(%d): Dzien dobry!\n",klient.id);
    zapisz_log(LOG_KLIENT, wiadomosc, shm_kolejki->kol_logger);


    sprintf(wiadomosc, "(%d): Kupie %d rzeczy.\n",klient.id, klient.liczba_produktow);
    zapisz_log(LOG_KLIENT, wiadomosc, shm_kolejki->kol_logger);

    usleep(klient.czas_zakupow);

    sprintf(wiadomosc, "Staje w kolejce. Moje miesce ma nr. %d\n", ilu_w_kolejce(shm_kolejki->kol_kasy_sam));
    zapisz_log(LOG_KLIENT, wiadomosc, shm_kolejki->kol_logger);
    stan_w_kolejce(klient, shm_kolejki->kol_kasy_sam);

    while (!skasowano) {
        sleep(1);
    }

    if (sukces) {
        sprintf(wiadomosc, "(%d): Dowidzenia!\n",getpid());
    } else {
        sprintf(wiadomosc, "(%d): :C\n",getpid());
    }

    zapisz_log(LOG_KLIENT, wiadomosc, shm_kolejki->kol_logger);

    exit(0);
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

void obsluga_sygnalu(int sig) {
    if (sig == SIGUSR1) {
        sukces = false;
    } else {
        sukces = true;
    }
    skasowano = 1;
}
