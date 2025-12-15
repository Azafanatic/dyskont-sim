#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

#define MAX_KLIENCI 100
#define MAX_KASY_SAMOOBSLUGOWE 6
#define MAX_KASY_STACJONARNE 2

#define SEM_KEY_KolejkaSamoobslugowa 6841
#define SEM_KEY_KolejkaStacjonarna 6842
#define SEM_KEY_OtwieranieKasy 6843
#define SEM_KEY_ZamykanieKasy 6844
#define SEM_KEY_Raport 6845

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

// Deklaracje semaforów
extern int sem_kolejka_samoobslugowa;
extern int sem_kolejka_stacjonarna;
extern int sem_otwieranie_kasy;
extern int sem_zamykanie_kasy;
extern int sem_raport;

// Deklaracje funkcji
int utworz_semafor(int key);
void usun_semafor(int semid);
void operacja_wait(int semid);
void operacja_signal(int semid);
void klient(int id);
void kasa_samoobslugowa(int id);
void kasa_stacjonarna(int id);
void pracownik_obslugi();
void kierownik();
void zapis_raportu(Klient klient, Kasa kasa);
