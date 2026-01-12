#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include "utils.h"

int create_a_semaphore(int key) {
    int semID = semget(key, 1, 0666 | IPC_CREAT);
    if (semID == -1) {
        exit(EXIT_FAILURE);
    }
    semctl(semID, 0, SETVAL, 1);
    return semID;
}

void del_a_semaphore(int semID) {
    semctl(semID, 0, IPC_RMID);
}

void operation_wait(int semID) {
    struct sembuf sb = {0, -1, SEM_UNDO};

    while (semop(semID, &sb, 1) == -1) {
        if (errno == EINTR) continue;
        perror("semop wait");
        exit(EXIT_FAILURE);
    }
}

void operation_signal(int semID) {
    struct sembuf sb = {0, 1, SEM_UNDO};
    while (semop(semID, &sb, 1) == -1) {
        if(errno == EINTR) {
            continue;
        }
        exit(EXIT_FAILURE);
    }
}

void save_a_log(LogType log_type, const char* format, int msq_id) {
    if (msq_id == -1) return;

    LogMessage msg;
    msg.message_type = 1;
    msg.log_type = log_type;
    strncpy(msg.message, format, 255);

    if (msgsnd(msq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        return;
    }
}

int queue_length(int msq_id){
     struct msqid_ds buf;
     if (msgctl(msq_id, IPC_STAT, &buf) == -1) {
         perror("msgctl");
         return -1;
    }
    return buf.msg_qnum;
};

void stand_in_the_queue(Client client, int msq_id) {
    if (msq_id == -1) return;

    ClientMessage msg;
    msg.message_type = 1;
    msg.client = client;

    if (msgsnd(msq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Blad msgsnd");
        return;
    }
}


void * shm_att(int * id, SectionsIPC section_type) {
    key_t key;
    size_t size;
    void *pointer;

    switch (section_type) {
        case SEMAPHORES:
            key = SHM_SEMAPHORES;
            size = sizeof(Semaphores);
            break;
        case QUEUES:
            key = SHM_QUEUES;
            size = sizeof(Queues);
            break;
        case STORE_DATA:
            key = SHM_STORE_DATA;
            size = sizeof(StoreData);
            break;
        case SIM_SETTINGS:
            key = SHM_SIM_SETTINGS;
            size = sizeof(SimSettings);
            break;
        case SS_CHECKOUTS:
            key = SHM_SS_CHECKOUTS;
            size = sizeof(SelfServiceCheckouts);
        case CHECKOUTS:
            key = SHM_CHECKOUTS;
            size = sizeof(Checkout);
        default:
            break;
    }

    *id = shmget(key, size, 0666);
    if (*id == -1) {
        perror("shmget");
        exit(1);
    }

    pointer = shmat(*id, NULL, 0);
    if (pointer == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    return pointer;
};

void * shm_create(int * id, SectionsIPC section_type) {
    key_t key;
    size_t size;
    void *pointer;

    switch (section_type) {
        case SEMAPHORES:
            key = SHM_SEMAPHORES;
            size = sizeof(Semaphores);
            break;
        case QUEUES:
            key = SHM_QUEUES;
            size = sizeof(Queues);
            break;
        case STORE_DATA:
            key = SHM_STORE_DATA;
            size = sizeof(StoreData);
            break;
        case SIM_SETTINGS:
            key = SHM_SIM_SETTINGS;
            size = sizeof(SimSettings);
            break;
        case SS_CHECKOUTS:
            key = SHM_SS_CHECKOUTS;
            size = sizeof(SelfServiceCheckouts);
        case CHECKOUTS:
            key = SHM_CHECKOUTS;
            size = sizeof(Checkout);
        default:
            break;
    }

    *id = shmget(key, size, IPC_CREAT | 0666);
    if (*id == -1) {
        perror("shmget");
        exit(1);
    }

    pointer = shmat(*id, NULL, 0);
    if (pointer == (void *)-1) {
        perror("shmat");
        exit(1);
    }
    return pointer;
};

void shm_destroy(int id, void * data) {
    shmdt(data);
    shmctl(id, IPC_RMID, NULL);
};

void shm_det(void * data) {
    shmdt(data);
};
