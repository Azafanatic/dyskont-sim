#include "shared.h"

//TODO: lepsze błędy

int utworz_semafor(int key) {
    int semid = semget(key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        exit(EXIT_FAILURE);
    }
    semctl(semid, 0, SETVAL, 1);
    return semid;
}

void usun_semafor(int semid) {
    semctl(semid, 0, IPC_RMID);
}

void operacja_wait(int semid) {
    struct sembuf sb = {0, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        exit(EXIT_FAILURE);
    }
}

void operacja_signal(int semid) {
    struct sembuf sb = {0, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        exit(EXIT_FAILURE);
    }
}
