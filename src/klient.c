#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "dyskont_utils.h"

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

int aktywni = 0;
double lambda;
double u;
double czas_oczekiwania;

bool stan_sklepu;

volatile sig_atomic_t skasowano = 0;
bool sukces;

Produkt dostepne_produkty[ILOSC_DOSTEPNYCH_PRODUKTOW] =
{{"Piwo", 2.99, ALKOHOLE}, {"Wino", 39.99, ALKOHOLE}, {"Wodka", 44.99, ALKOHOLE}, {"Jagermeister", 69.99, ALKOHOLE},
{"Whisky", 74.99, ALKOHOLE}, {"Maslo", 9.99, NABIAL}, {"Sok jablkowy", 5.49, SOKI}, {"Sok pomaranczowy", 6.99, SOKI},
{"Chleb", 4.29, PIECZYWO}, {"Mleko", 3.19, NABIAL}, {"Jablko", 1.99, OWOCE}, {"Ser zolty", 19.99, NABIAL},
{"Szynka", 24.99, WEDLINY}, {"Kawa", 15.99, SUCHE}, {"Herbata", 6.49, INNE}, {"Cukier", 3.79, SUCHE},
{"Makaron", 5.29, SUCHE}, {"Ryz", 4.79, SUCHE}, {"Olej", 12.99, INNE}, {"Woda mineralna", 2.49, INNE},
{"Cola", 4.99, NAPOJE_GAZOWANE}, {"Czekolada", 5.99, SLODYCZE}, {"Batonik", 2.79, SLODYCZE}, {"Jogurt", 2.29, NABIAL},
{"Dzem", 7.49, INNE}, {"Ketchup", 6.99, INNE}, {"Musztarda", 4.99, INNE}, {"Ciasteczka", 5.49, SLODYCZE},
{"Mak", 3.99, INNE}, {"Orzechy", 14.99, INNE}, {"Miod", 18.99, INNE}, {"Przyprawy", 3.49, SUCHE}};


void zrob_zakupy();

void shm_init();
void shm_close();

void obsluga_sygnalu(int sig);

int main(int argc, char *argv[]) {

    shm_init();

    srand(time(NULL));

    lambda = 1.0 / MAX_KLIENCI;

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
                zrob_zakupy();
            }
            else {
                pids[aktywni++] = pid;
            }
        }

        operacja_wait(shm_semafory->sem_sklep_dane);
        shm_dane->ilosc_klientow = aktywni;
        operacja_signal(shm_semafory->sem_sklep_dane);


        //TODO; znaleźć lepszy sposób na wprowadzanie klientów ze zmiennym tempem
        u = (double)rand() / (double)RAND_MAX;
        czas_oczekiwania = -log(1.0 - u) / lambda;
        if (czas_oczekiwania < 0.1) {
            czas_oczekiwania = 0.1;
        }

        czas_oczekiwania = czas_oczekiwania * 8500 / shm_dane->szybkosc_symulacji;
        usleep(czas_oczekiwania);
    }

    shm_close();

    exit(0);
}

void zrob_zakupy() {
    char wiadomosc[240];

    signal(SIGUSR1, obsluga_sygnalu);
    signal(SIGUSR2, obsluga_sygnalu);

    Klient klient;
    klient.liczba_produktow = MIN_PRODUKTY + rand() % (MAX_PRODUKTY - MIN_PRODUKTY + 1);
    klient.czas_zakupow = (double) (60 + rand() % 30 * klient.liczba_produktow) / shm_dane->szybkosc_symulacji * 1000000;
    klient.id = getpid();
    klient.wiek = 5 + rand() % 90;

    for (int i = 0; i < klient.liczba_produktow; i++) {
        klient.produkty[i] = dostepne_produkty[rand() % ILOSC_DOSTEPNYCH_PRODUKTOW];
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
