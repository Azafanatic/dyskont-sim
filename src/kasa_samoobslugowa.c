#include <unistd.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "dyskont_utils.h"

sig_atomic_t koniec = 0;

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

pid_t pids[MAX_KASY_SAM];

void obsluga_sygnalu(int sig);
void obsloz_klienta(int id_kasy);
void shm_init();
void shm_close();

int main(int argc, char *argv[]) {
    signal(SIGINT, obsluga_sygnalu);

    shm_init();

    for (int i = 0; i < 3; i++) {
        shm_kasy_sam[i].kasa->otwarta = true;
    }

    for (int i = 0; i < MAX_KASY_SAM; i++) {
        pids[i] = fork();

        if (pids[i] < 0) {
            perror("Blad forka\n");
            exit(1);
        } else if (pids[i] == 0) {
            obsloz_klienta(i);
            exit(1);
        }
    }

    char wiadomosc[320];

    while (!koniec) {
        sprintf(wiadomosc, "Kolejka %d\t Kasy otwarte %d\n", ilu_w_kolejce(shm_kolejki->kol_kasy_sam), shm_kasy_sam->otwarte_kasy);
        zapisz_log(LOG_KASA_SAM, wiadomosc, shm_kolejki->kol_logger);

        usleep(1000000);
    }

    shm_close();
    exit(0);
}

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
}

void obsloz_klienta(int id_kasy) {
    char wiadomosc[320];
    char wiadomosc_buf[80];
    KlientMSQ msg;
    bool otwarta;
    operacja_wait(shm_semafory->sem_kasy);
    shm_kasy_sam->kasa[id_kasy].obsluzeni_klienci = 0;
    operacja_signal(shm_semafory->sem_kasy);

    while (!koniec) {
        otwarta = shm_kasy_sam->otwarte_kasy > id_kasy;

        if (!otwarta) {
            usleep(5000000 / shm_dane->szybkosc_symulacji);
            continue;
        };

        if (msgrcv(shm_kolejki->kol_kasy_sam, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            break;
        }
        strcpy(wiadomosc, " ");
        strcpy(wiadomosc_buf, " ");
        sprintf(wiadomosc_buf, "(%d) ", id_kasy);
        strcat(wiadomosc,wiadomosc_buf);
        sprintf(wiadomosc_buf, "Witaj kliencie (%d)!\nTwoja lista zakupow:\n", msg.klient.id);
        strcat(wiadomosc,wiadomosc_buf);
        for (int i = 0; i < msg.klient.liczba_produktow; i++) {
            sprintf(wiadomosc_buf, "%s ", msg.klient.produkty[i].nazwa);
            strcat(wiadomosc,wiadomosc_buf);
        }
        sprintf(wiadomosc_buf, "\n");
        strcat(wiadomosc,wiadomosc_buf);

        usleep((10 + 3 * msg.klient.liczba_produktow) * 1000000 / shm_dane->szybkosc_symulacji);

        bool alkohol = false;

        for (int i = 0; i < msg.klient.liczba_produktow; i++) {
            if (msg.klient.produkty[i].kategoria == ALKOHOLE) {
                alkohol = true;
                break;
            }
        }

        if (alkohol && msg.klient.wiek < 18) {
            sprintf(wiadomosc_buf, "Masz tylko %d lat, nie moge Ci tego sprzedac.\n", msg.klient.wiek);
            strcat(wiadomosc,wiadomosc_buf);
            zapisz_log(LOG_KASA_SAM, wiadomosc, shm_kolejki->kol_logger);

            operacja_wait(shm_semafory->sem_raport);
            shm_raport->sprzedane_produkty = shm_raport->sprzedane_produkty + msg.klient.liczba_produktow;
            shm_raport->wszyscy_klienci = shm_raport->wszyscy_klienci + 1;
            shm_raport->klienci_nieobslozeni = shm_raport->klienci_nieobslozeni + 1;
            operacja_signal(shm_semafory->sem_raport);

            kill(msg.klient.id, SIGUSR1);
        } else {
            float cena = 0.0;
            for (int i = 0; i < msg.klient.liczba_produktow; i++) {
                cena += msg.klient.produkty[i].cena;
            }
            sprintf(wiadomosc_buf, "Calosc kosztuje %.2f zł.\nDziekujemy i zapraszamy ponownie!\n", cena);
            strcat(wiadomosc,wiadomosc_buf);
            zapisz_log(LOG_KASA_SAM, wiadomosc, shm_kolejki->kol_logger);

            operacja_wait(shm_semafory->sem_raport);
            shm_raport->sprzedane_produkty = shm_raport->sprzedane_produkty + msg.klient.liczba_produktow;
            shm_raport->wszyscy_klienci = shm_raport->wszyscy_klienci + 1;
            shm_raport->skasowane_pieniadze += cena;
            operacja_signal(shm_semafory->sem_raport);

            kill(msg.klient.id, SIGUSR2);
        }

        operacja_wait(shm_semafory->sem_kasy);
        shm_kasy_sam->kasa[id_kasy].obsluzeni_klienci++;
        operacja_signal(shm_semafory->sem_kasy);
    }

    //printf("Kasa (%d) - obsluzeni klienci: %d\n", id_kasy, shm_kasy_sam->kasa[id_kasy].obsluzeni_klienci);
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
