#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include "shared.h"

int utworz_semafor(int key) {
    int semID = semget(key, 1, 0666 | IPC_CREAT);
    if (semID == -1) {
        exit(EXIT_FAILURE);
    }
    semctl(semID, 0, SETVAL, 1);
    return semID;
}

void usun_semafor(int semID) {
    semctl(semID, 0, IPC_RMID);
}

void operacja_wait(int semID) {
    struct sembuf sb = {0, -1, SEM_UNDO};

    while (semop(semID, &sb, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop wait");
        exit(EXIT_FAILURE);
    }
}

void operacja_signal(int semID) {
    struct sembuf sb = {0, 1, SEM_UNDO};
    while (semop(semID, &sb, 1) == -1) {
        if(errno == EINTR) {
            continue;
        }
        exit(EXIT_FAILURE);
    }
}

void zapisz_log(TypLogu typ_logu, const char* format, int kolejka_id) {
    if (kolejka_id == -1) return;

    struct Log msg;
    msg.typ_komunikatu = 1;
    msg.typ_logu = typ_logu;
    strncpy(msg.wiadomosc, format, 255);
    msg.wiadomosc[255] = '\0';

    if (msgsnd(kolejka_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Blad msgsnd");
    }
}
