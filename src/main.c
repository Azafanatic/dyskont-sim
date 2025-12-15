#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include "shared.h"
#include "wiadomosc.h"

int sem_kolejka_samoobslugowa;
int sem_kolejka_stacjonarna;
int sem_otwieranie_kasy;
int sem_zamykanie_kasy;
int sem_raport;

volatile sig_atomic_t koniec = 0;
int globalny_id_klienta = 0;

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        printf("\nOtrzymano sygnał SIGINT. Kończenie symulacji...\n");
        koniec = 1;
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, obsluga_sygnalu);

    msg(COL_BLUE,"~=~=~=~ DYSKONT ~=~=~=~\n");

    pid_t pids[2];
    int pid_index = 2;

    sem_kolejka_samoobslugowa = utworz_semafor(SEM_KEY_KolejkaSamoobslugowa);
    sem_kolejka_stacjonarna = utworz_semafor(SEM_KEY_KolejkaStacjonarna);
    sem_otwieranie_kasy = utworz_semafor(SEM_KEY_OtwieranieKasy);
    sem_zamykanie_kasy = utworz_semafor(SEM_KEY_ZamykanieKasy);
    sem_raport = utworz_semafor(SEM_KEY_Raport);

    pids[0] = fork();
    if (pids[0] < 0) {
        msg(COL_RED, "Błąd forka kierownik\n");
        exit(1);
    } else if (pids[0] == 0) {
        execlp("./kierownik", "kierownik", (char *)NULL);
        msg(COL_RED, "Błąd exec dla kierownik\n");
        exit(1);
    }

    pids[1] = fork();
    if (pids[1] < 0) {
        msg(COL_RED, "Błąd forka klient\n");
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(1);
    } else if (pids[1] == 0) {
        execlp("./klient", "klient", (char *)NULL);
        msg(COL_RED, "Błąd exec dla klient\n");
        exit(1);
    }

    while (!koniec) {
        sleep(1);
    }

    for (int i = 0; i < pid_index; i++) {
        kill(pids[i], SIGTERM);
    }

    for (int i = 0; i < pid_index; i++) {
        waitpid(pids[i], NULL, 0);
    }

    usun_semafor(sem_kolejka_samoobslugowa);
    usun_semafor(sem_kolejka_stacjonarna);
    usun_semafor(sem_otwieranie_kasy);
    usun_semafor(sem_zamykanie_kasy);
    usun_semafor(sem_raport);

    msg(COL_BLUE,"~=~=~=~ KONIEC ~=~=~=~\n");

    return 0;
}
