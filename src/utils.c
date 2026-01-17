#include "utils.h"
#include <errno.h>
#include <libgen.h>
#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>

int create_a_semaphore(int key)
{
    int semID = semget(key, 1, 0666 | IPC_CREAT);
    if (semID == -1) {
        exit(EXIT_FAILURE);
    }
    semctl(semID, 0, SETVAL, 1);
    return semID;
}

void del_a_semaphore(int semID)
{
    semctl(semID, 0, IPC_RMID);
}

void operation_wait(int semID)
{
    struct sembuf sb = { 0, -1, SEM_UNDO };

    while (semop(semID, &sb, 1) == -1) {
        if (errno == EINTR)
            continue;
        perror(_("Semop wait error\n"));
        exit(EXIT_FAILURE);
    }
}

void operation_signal(int semID)
{
    struct sembuf sb = { 0, 1, SEM_UNDO };
    while (semop(semID, &sb, 1) == -1) {
        if (errno == EINTR) {
            continue;
        }
        perror(_("Semop signal error\n"));
        exit(EXIT_FAILURE);
    }
}

void save_a_log(LogType log_type, const char* format, int msq_id)
{
    if (msq_id == -1)
        return;

    LogMessage msg;
    msg.message_type = 1;
    msg.log_type = log_type;
    strncpy(msg.message, format, sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';

    if (msgsnd(msq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror(_("Msgsnd error\n"));
        return;
    }
}

int queue_length(int msq_id)
{
    struct msqid_ds buf;
    if (msgctl(msq_id, IPC_STAT, &buf) == -1) {
        perror(_("Msgctl error\n"));
        return -1;
    }
    return buf.msg_qnum;
};

void* shm_att(int* id, SectionsIPC section_type)
{
    key_t key;
    size_t size;
    void* pointer;

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
        break;
    case CHECKOUTS:
        key = SHM_CHECKOUTS;
        size = sizeof(Checkouts);
        break;
    default:
        break;
    }

    *id = shmget(key, size, 0666);
    if (*id == -1) {
        perror(_("Shmget error\n"));
        exit(1);
    }

    pointer = shmat(*id, NULL, 0);
    if (pointer == (void*)-1) {
        perror(_("Shmat error\n"));
        exit(1);
    }
    return pointer;
};

void* shm_create(int* id, SectionsIPC section_type)
{
    key_t key;
    size_t size;
    void* pointer;

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
        break;
    case CHECKOUTS:
        key = SHM_CHECKOUTS;
        size = sizeof(Checkouts);
        break;
    default:
        break;
    }

    *id = shmget(key, size, IPC_CREAT | 0666);
    if (*id == -1) {
        perror(_("Shmget error\n"));
        exit(1);
    }

    pointer = shmat(*id, NULL, 0);
    if (pointer == (void*)-1) {
        perror(_("Shmat error\n"));
        exit(1);
    }
    return pointer;
};

void shm_destroy(int id, void* data)
{
    shmdt(data);
    shmctl(id, IPC_RMID, NULL);
};

void shm_det(void* data)
{
    shmdt(data);
};

void init_i18n()
{
    setlocale(LC_ALL, "");

    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
        return;

    exe_path[len] = '\0';
    char* dir = dirname(exe_path);

    char locale_path[PATH_MAX];
    snprintf(locale_path, sizeof(locale_path), "%s/locale", dir);

    bindtextdomain("messages", locale_path);
    textdomain("messages");
}
