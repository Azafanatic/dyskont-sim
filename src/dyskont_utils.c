#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include "dyskont_utils.h"

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

void zapisz_log(TypLogu typ_logu, const char* format, int msq_id) {
    if (msq_id == -1) return;

    Log msg;
    msg.typ_komunikatu = 1;
    msg.typ_logu = typ_logu;
    strncpy(msg.wiadomosc, format, 255);

    if (msgsnd(msq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Blad msgsnd");
        return;
    }
}

int ilu_w_kolejce(int msq_id){
     struct msqid_ds buf;
     if (msgctl(msq_id, IPC_STAT, &buf) == -1) {
         perror("msgctl");
         return -1;
    }
    return buf.msg_qnum;
};

void stan_w_kolejce(Klient klient, int msq_id) {
    if (msq_id == -1) return;

    KlientMSQ msg;
    msg.typ_komunikatu = 1;
    msg.klient = klient;

    if (msgsnd(msq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Blad msgsnd");
        return;
    }
}


void * shm_att(int * id, SekcjeIPC typ_sekcji) {
    key_t klucz;
    size_t rozmiar;
    void *wskaznik;

    switch (typ_sekcji) {
        case SEMAFORY:
            klucz = SHM_SEMAFORY;
            rozmiar = sizeof(Semafory);
            break;
        case KOLEJKI:
            klucz = SHM_KOLEJKI;
            rozmiar = sizeof(Kolejki);
            break;
        case DANE:
            klucz = SHM_DANE;
            rozmiar = sizeof(Dane);
            break;
        case RAPORT:
            klucz = SHM_RAPORT;
            rozmiar = sizeof(Raport);
            break;
        case KASY_SAM:
            klucz = SHM_KASY_SAM;
            rozmiar = sizeof(KasySam);
        case KASY_STAC:
            klucz = SHM_KASY_STAC;
            rozmiar = sizeof(KasyStac);
        default:
            break;
    }

    *id = shmget(klucz, rozmiar, 0666);
    if (*id == -1) {
        perror("shmget");
        exit(1);
    }

    wskaznik = shmat(*id, NULL, 0);
    if (wskaznik == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    return wskaznik;
};

void * shm_create(int * id, SekcjeIPC typ_sekcji) {
    key_t klucz;
    size_t rozmiar;
    void *wskaznik;

    switch (typ_sekcji) {
        case SEMAFORY:
            klucz = SHM_SEMAFORY;
            rozmiar = sizeof(Semafory);
            break;
        case KOLEJKI:
            klucz = SHM_KOLEJKI;
            rozmiar = sizeof(Kolejki);
            break;
        case DANE:
            klucz = SHM_DANE;
            rozmiar = sizeof(Dane);
            break;
        case RAPORT:
            klucz = SHM_RAPORT;
            rozmiar = sizeof(Raport);
            break;
        case KASY_SAM:
            klucz = SHM_KASY_SAM;
            rozmiar = sizeof(KasySam);
        case KASY_STAC:
            klucz = SHM_KASY_STAC;
            rozmiar = sizeof(KasyStac);
        default:
            break;
    }

    *id = shmget(klucz, rozmiar, IPC_CREAT | 0666);
    if (*id == -1) {
        perror("shmget");
        exit(1);
    }

    wskaznik = shmat(*id, NULL, 0);
    if (wskaznik == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    return wskaznik;
};

void shm_destroy(int id, void * data) {
    shmdt(data);
    shmctl(id, IPC_RMID, NULL);
};

void shm_det(void * data) {
    shmdt(data);
};
