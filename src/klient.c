#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <time.h>
#include "dyskont_utils.h"

#define ILOSC_DOSTEPNYCH_PRODUKTOW 32

pid_t pids[MAX_KLIENCI];
volatile sig_atomic_t koniec = 0;
int shm_kolejki_id;
int shm_semafory_id;
Kolejki *shm_kolejki;
Semafory *shm_semafory;
int shm_dane_id;
Dane *shm_dane;
int shm_raport_id;
Raport *shm_raport;
int szybkosc_symulacji;


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

    Klient klient;
    klient.liczba_produktow = MIN_PRODUKTY + rand() % (MAX_PRODUKTY - MIN_PRODUKTY + 1);
    klient.czas_zakupow = (double) (120 + rand() % 120) / szybkosc_symulacji * 1000000;
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

    char wiadomosc[220];

    sprintf(wiadomosc, "(%d): Dzien dobry!\n",klient.id);
    zapisz_log(LOG_KLIENT, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);


    sprintf(wiadomosc, "(%d): Kupie %d rzeczy.\n",klient.id, klient.liczba_produktow);
    zapisz_log(LOG_KLIENT, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);

    usleep(klient.czas_zakupow);

    /*
    if (klient.ma_alkohol == true && klient.wiek < 18) {
        sprintf(wiadomosc, "(%d): Mam %d lat i chciałbym kupić alkohol :)\n",klient.id, klient.wiek);
        zapisz_log(LOG_SYM_OSTRZEZENIE, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);
    }

    sprintf(wiadomosc, "(%d): Moje zakupy to: ",klient.id);
    for (int i = 0; i < klient.liczba_produktow; i++) {
        strcat(wiadomosc, klient.produkty[i].nazwa);
        strcat(wiadomosc, " ");
    }
    strcat(wiadomosc, "\n");

    if (klient.ma_alkohol == true) {
        zapisz_log(LOG_SYM_ERR, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);
    } else {
        zapisz_log(LOG_DOMYSLNY, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);
    }

    */

    operacja_wait(shm_semafory->sem_raport);
    shm_raport->sprzedane_produkty = shm_raport->sprzedane_produkty + klient.liczba_produktow;
    shm_raport->wszyscy_klienci = shm_raport->wszyscy_klienci + 1;
    operacja_signal(shm_semafory->sem_raport);

    sprintf(wiadomosc, "(%d): Dowidzenia!\n",getpid());
    zapisz_log(LOG_KLIENT, wiadomosc, shm_kolejki->kol_logger, shm_semafory->sem_kolejka_logger);

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

    shm_raport_id = shmget(SHM_RAPORT, sizeof(Raport), 0666);
    if (shm_raport_id == -1) {
        exit(1);
    }

    shm_raport = (Raport *)shmat(shm_raport_id, NULL, 0);
    if (shm_raport == (void *)-1) {
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

    shmdt(shm_raport);
    shmctl(shm_raport_id, IPC_RMID, NULL);
};
