#include <unistd.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "dyskont_utils.h"

sig_atomic_t koniec = 0;

int shm_kolejki_id;
int shm_semafory_id;
Kolejki *shm_kolejki;
Semafory *shm_semafory;
int kolejka_id;

void obsluga_sygnalu(int sig);
void shm_init();

int main(int argc, char *argv[]) {
    signal(SIGINT, obsluga_sygnalu);

    while (!koniec) {
        sleep(1);
    }
    exit(0);
}

void obsluga_sygnalu(int sig) {
    if (sig == SIGINT) {
        koniec = 1;
    }
}
