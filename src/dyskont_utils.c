#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

void zapisz_wiadomosc(KolorWiadomosci color, const char *message) {
    const char *color_code = "\033[0m";

    switch (color) {
        case COL_RED:
            color_code = "\033[91m";
            break;
        case COL_GREEN:
            color_code = "\033[92m";
            break;
        case COL_BLUE:
            color_code = "\033[94m";
            break;
        case COL_YELLOW:
            color_code = "\033[93m";
            break;
        case COL_CYAN:
            color_code = "\033[96m";
            break;
        case COL_MAGENTA:
            color_code = "\033[95m";
            break;
        case COL_DEFAULT:
            color_code = "\033[0m";
            break;
    }

    char formatted_message[1024];
    int len = snprintf(formatted_message, sizeof(formatted_message), "%s%s\033[0m", color_code, message);

    write(STDOUT_FILENO, formatted_message, len);
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

