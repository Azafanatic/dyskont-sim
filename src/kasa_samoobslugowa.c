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
Kolejki *shm_kolejki;
Semafory *shm_semafory;
int shm_dane_id;
Dane *shm_dane;
int shm_raport_id;
Raport *shm_raport;
int szybkosc_symulacji;
int kolejka_id;
char wiadomosc[320];
char wiadomosc_buf[80];

void obsluga_sygnalu(int sig);
void shm_init();
void shm_destroy();

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

    shm_destroy();
    exit(0);
}

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
}

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
}

void shm_destroy() {
    shmdt(shm_kolejki);

    shmdt(shm_semafory);

    shmdt(shm_dane);

    shmdt(shm_raport);
}
