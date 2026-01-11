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
Kolejki *shm_kolejki;
Semafory *shm_semafory;
Dane *shm_dane;
Raport *shm_raport;

int szybkosc_symulacji;
int kolejka_id;

char wiadomosc[320];
char wiadomosc_buf[80];

void obsluga_sygnalu(int sig);
void shm_init();
void shm_close();

int main(int argc, char *argv[]) {
    signal(SIGINT, obsluga_sygnalu);

    shm_init();
    operacja_wait(shm_semafory->sem_kolejki);
    kolejka_id = shm_kolejki->kol_kasy_sam;
    operacja_signal(shm_semafory->sem_kolejki);

    operacja_wait(shm_semafory->sem_sklep_dane);
    szybkosc_symulacji = shm_dane->szybkosc_symulacji;
    operacja_signal(shm_semafory->sem_sklep_dane);

    KlientMSQ msg;
    while (!koniec) {
        if (msgrcv(kolejka_id, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            break;
        }
        strcpy(wiadomosc, " ");
        strcpy(wiadomosc_buf, " ");
        sprintf(wiadomosc_buf, "Witaj kliencie (%d)!\nTwoja lista zakupow:\n", msg.klient.id);
        strcat(wiadomosc,wiadomosc_buf);
        for (int i = 0; i < msg.klient.liczba_produktow; i++) {
            sprintf(wiadomosc_buf, "%s ", msg.klient.produkty[i].nazwa);
            strcat(wiadomosc,wiadomosc_buf);
        }
        sprintf(wiadomosc_buf, "\n");
        strcat(wiadomosc,wiadomosc_buf);

        usleep(msg.klient.liczba_produktow * 1000000 / szybkosc_symulacji);

        if (msg.klient.ma_alkohol == true && msg.klient.wiek < 18) {
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

    }

    shm_close();
    exit(0);
}

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
}

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
