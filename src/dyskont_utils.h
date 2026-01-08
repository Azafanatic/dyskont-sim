#ifndef DYSKONT_UTILS_H
#define DYSKONT_UTILS_H

#include <stdbool.h>
#include <sys/shm.h>

#define MAX_KLIENCI 128
#define MIN_PRODUKTY 3
#define MAX_PRODUKTY 10
#define MAX_KASY_SAMOOBSLUGOWE 6
#define MAX_KASY_STACJONARNE 2

#define SEM_ID_KOLEJKA_SAMOOBSLUGOWA 6841
#define SEM_ID_KOLEJKA_STACJONARNA 6842
#define SEM_ID_OTWIERANIE_KASY 6843
#define SEM_ID_ZAMYKANIE_KASY 6844
#define SEM_ID_RAPORT 6845
#define SEM_ID_KOLEJKA_LOGGER 6846
#define SEM_ID_SKLEP_DANE 6847

#define SHM_SEMAFORY 4581
#define SHM_KOLEJKI 4582
#define SHM_DANE 4583
#define SHM_RAPORT 4584

typedef enum {
    LOG_INFO,
    LOG_OSTRZEZENIE,
    LOG_ERR
} TypLogu;

typedef enum {
    COL_RED,
    COL_GREEN,
    COL_BLUE,
    COL_YELLOW,
    COL_CYAN,
    COL_MAGENTA,
    COL_DEFAULT
} KolorWiadomosci;

struct Log {
    long typ_komunikatu;
    TypLogu typ_logu;
    char wiadomosc[256];
};

typedef struct {
    int id;
    int liczba_produktow;
    bool ma_alkohol;
    int czas_zakupow;
} Klient;

typedef struct {
    int id;
    bool otwarta;
    int kolejka;
} Kasa;

typedef struct {
    int sem_kolejka_samoobslugowa;
    int sem_kolejka_stacjonarna;
    int sem_otwieranie_kasy;
    int sem_zamykanie_kasy;
    int sem_raport;
    int sem_kolejka_logger;
    int sem_sklep_dane;
} Semafory;

typedef struct {
    int dlugosc_symulacji;
    int szybkosc_symulacji;
    int ilosc_klientow;
    bool stan_sklepu;
} Dane;

typedef struct {
    int wszyscy_klienci;
    int sprzedane_produkty;
} Raport;

typedef struct {
    int kol_logger;
    int kol_kasy_samoobslugowe;
    int kol_kasy_stacjonarne;
} Kolejki;

int utworz_semafor(int key);
void usun_semafor(int semid);
void operacja_wait(int semid);
void operacja_signal(int semid);
void zapisz_log(TypLogu typ_logu, const char* format, int kolejka_id, int semafor_logger);

void zapisz_wiadomosc(KolorWiadomosci color, const char *message);


#endif
