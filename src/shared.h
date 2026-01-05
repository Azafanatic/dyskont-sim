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

#define SHM_KOLEJKA_LOG 4581

typedef enum {
    LOG_INFO,
    LOG_OSTRZEZENIE,
    LOG_ERR
} TypLogu;

struct Log {
    long typ_komunikatu;
    TypLogu typ_logu;
    char wiadomosc[256];
};

typedef struct {
    int kolejka_id;
} KolejkaLogger;

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
    int logger_kolejka;
} SHM;

int utworz_semafor(int key);
void usun_semafor(int semid);
void operacja_wait(int semid);
void operacja_signal(int semid);
void zapisz_log(TypLogu typ_logu, const char* format, int kolejka_id);

