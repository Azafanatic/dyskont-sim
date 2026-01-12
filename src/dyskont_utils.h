#ifndef DYSKONT_UTILS_H
#define DYSKONT_UTILS_H

#include <stdbool.h>
#include <sys/shm.h>

#define MAX_KLIENCI 50
#define MIN_PRODUKTY 3
#define MAX_PRODUKTY 10
#define ILOSC_DOSTEPNYCH_PRODUKTOW 32
#define MAX_KASY_SAM 6
#define MAX_KASY_STAC 2

#define SEM_ID_KOLEJKA_SAM 6841
#define SEM_ID_KOLEJKA_STAC 6842
#define SEM_ID_KASY 6843
#define SEM_ID_RAPORT 6844
#define SEM_ID_KOLEJKI 6845
#define SEM_ID_SKLEP_DANE 6846

#define SHM_SEMAFORY 4581
#define SHM_KOLEJKI 4582
#define SHM_DANE 4583
#define SHM_RAPORT 4584
#define SHM_KASY_SAM 4585
#define SHM_KASY_STAC 4586

#define MSQ_LOG_ID 6840
#define MSQ_KASY_SAM_ID 6841
#define MSQ_KASA_STAC_1_ID 6842
#define MSQ_KASA_STAC_2_ID 6842

typedef enum {
    KOLEJKI,
    SEMAFORY,
    DANE,
    RAPORT,
    KASY_SAM,
    KASY_STAC
} SekcjeIPC;

typedef enum {
    LOG_DOMYSLNY,
    LOG_SYM_INFO,
    LOG_SYM_OSTRZEZENIE,
    LOG_SYM_ERR,
    LOG_KASA_SAM,
    LOG_KASA_STAC,
    LOG_KIEROWNIK,
    LOG_KLIENT,
    LOG_OBSLUGA,
} TypLogu;

typedef struct {
    long typ_komunikatu;
    TypLogu typ_logu;
    char wiadomosc[320];
} Log;

typedef enum {
    ALKOHOLE,
    WEDLINY,
    OWOCE,
    WAZYWA,
    PIECZYWO,
    NABIAL,
    SOKI,
    NAPOJE_GAZOWANE,
    SLODYCZE,
    SUCHE,
    INNE
}KategorieProduktow;

typedef struct {
    char nazwa[32];
    float cena;
    KategorieProduktow kategoria;
} Produkt;

typedef struct {
    int id;
    int liczba_produktow;
    int wiek;
    double czas_zakupow;
    Produkt produkty[MAX_PRODUKTY];
} Klient;

typedef struct {
    long typ_komunikatu;
    Klient klient;
} KlientMSQ;

typedef struct {
    bool otwarta;
    int obsluzeni_klienci;
} Kasa;

typedef struct {
    int otwarte_kasy;
    Kasa kasa[MAX_KASY_SAM];
} KasySam;

typedef struct {
    int otwarte_kasy;
    Kasa kasa[MAX_KASY_STAC];
} KasyStac;

typedef struct {
    int sem_kasy;
    int sem_raport;
    int sem_kolejki;
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
    int klienci_nieobslozeni;
    float skasowane_pieniadze;
    int sprzedane_produkty;
    float prod_na_klienta;
    float klienci_w_sklepie;
} Raport;

typedef struct {
    int kol_logger;
    int kol_kasy_sam;
    int kol_kasy_stac;
} Kolejki;

int utworz_semafor(int key);
void usun_semafor(int semid);

void operacja_wait(int semid);
void operacja_signal(int semid);

void * shm_att(int * id, SekcjeIPC typ_sekcji);
void * shm_create(int * id, SekcjeIPC typ_sekcji);
void shm_det(void * data);
void shm_destroy(int id, void * data);

void zapisz_log(TypLogu typ_logu, const char* format, int msq_id);

int ilu_w_kolejce(int msq_id);
void stan_w_kolejce(Klient klient, int msq_id);

#endif
