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

/** @brief Creates a semaphore */
int create_a_semaphore(int key)
{
    int semID = semget(key, 1, 0666 | IPC_CREAT);
    if (semID == -1) {
        perror(_("Semget error\n"));
        exit(EXIT_FAILURE);
    }
    if (semctl(semID, 0, SETVAL, 1) == -1) {
        perror(_("Semctl error\n"));
        exit(EXIT_FAILURE);
    }
    return semID;
}

/** @brief Deletes a semaphore */
void del_a_semaphore(int semID)
{
    if (semctl(semID, 0, IPC_RMID) == -1) {
        perror(_("Semctl error\n"));
    }
}

/** @brief Waits on semaphore */
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

/** @brief Signals semaphore */
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

/** @brief Saves a log message */
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

/** @brief Gets queue length */
int queue_length(int msq_id)
{
    struct msqid_ds buf;
    if (msgctl(msq_id, IPC_STAT, &buf) == -1) {
        perror(_("Msgctl error\n"));
        return -1;
    }
    return buf.msg_qnum;
};

/** @brief Attaches shared memory */
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

/** @brief Creates shared memory */
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

/** @brief Destroys shared memory */
void shm_destroy(int id, void* data)
{
    if (shmdt(data) == -1) {
        perror(_("Shmdt error\n"));
    }
    if (shmctl(id, IPC_RMID, NULL) == -1) {
        perror(_("Shmctl error\n"));
    }
};

/** @brief Detaches shared memory */
void shm_det(void* data)
{
    if (shmdt(data) == -1) {
        perror(_("Shmdt error\n"));
    }
};

/** @brief Initializes internationalization */
void init_i18n()
{
    setlocale(LC_ALL, "");

    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror(_("Readlink error\n"));
        return;
    }

    exe_path[len] = '\0';
    char* dir = dirname(exe_path);

    char locale_path[PATH_MAX];
    snprintf(locale_path, sizeof(locale_path), "%s/locale", dir);

    bindtextdomain("messages", locale_path);
    textdomain("messages");
}
