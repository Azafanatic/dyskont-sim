#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include "wiadomosc.h"

int main(int argc, char *argv[]) {
    msg(COL_BLUE,"~=~=~=~ DYSKONT ~=~=~=~\n");

    pid_t kierownik, klient;

    kierownik = fork();
    if (kierownik < 0) {
        msg(COL_RED, "Błąd forka kierownik\n");
        exit(1);
    } else if (kierownik == 0) {
        execlp("./kierownik", "kierownik", (char *)NULL);
        msg(COL_RED, "Błąd exec dla kierownik\n");
        exit(1);
    }

    klient = fork();
    if (klient < 0) {
        msg(COL_RED, "Błąd forka klient\n");
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(1);
    } else if (klient == 0) {
        execlp("./klient", "klient", (char *)NULL);
        msg(COL_RED, "Błąd exec dla klient\n");
        exit(1);
    }

    wait(NULL);
    wait(NULL);
    msg(COL_BLUE,"~=~=~=~ KONIEC ~=~=~=~\n");

    return 0;
}
