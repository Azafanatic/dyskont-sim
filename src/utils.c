#include "utils.h"

#include <errno.h>
#include <libgen.h>
#include <libintl.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

/** @brief Generates a System V IPC key using ftok */
key_t generate_key(int proj_id)
{
    if (proj_id < 1 || proj_id > 255) {
        fprintf(stderr, "Invalid proj_id for ftok(): %d\n", proj_id);
        exit(EXIT_FAILURE);
    }

    /* Ensure key file exists */
    FILE* f = fopen(IPC_KEY_FILE, "a");
    if (!f) {
        perror("fopen IPC_KEY_FILE");
        exit(EXIT_FAILURE);
    }
    fclose(f);

    key_t key = ftok(IPC_KEY_FILE, proj_id);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    return key;
}

/** @brief Creates a semaphore */
int create_a_semaphore(int proj_id)
{
    key_t key = generate_key(proj_id);

    int semID = semget(key, 1, 0666 | IPC_CREAT);
    if (semID == -1) {
        perror(_("semget error"));
        exit(EXIT_FAILURE);
    }

    if (semctl(semID, 0, SETVAL, 1) == -1) {
        perror(_("semctl SETVAL error"));
        exit(EXIT_FAILURE);
    }

    return semID;
}

/** @brief Deletes a semaphore */
void del_a_semaphore(int semID)
{
    if (semctl(semID, 0, IPC_RMID) == -1) {
        perror(_("semctl IPC_RMID error"));
    }
}

/** @brief Waits on semaphore */
void operation_wait(int semID)
{
    struct sembuf sb = { 0, -1, SEM_UNDO };

    while (semop(semID, &sb, 1) == -1) {
        if (errno == EINTR)
            continue;

        perror(_("semop wait error"));
        exit(EXIT_FAILURE);
    }
}

/** @brief Signals semaphore */
void operation_signal(int semID)
{
    struct sembuf sb = { 0, 1, SEM_UNDO };

    while (semop(semID, &sb, 1) == -1) {
        if (errno == EINTR)
            continue;

        perror(_("semop signal error"));
        exit(EXIT_FAILURE);
    }
}

void wait_if_queue_near_full(int msq_id, size_t msg_size)
{
    struct msqid_ds buf;

    while (1) {
        if (msgctl(msq_id, IPC_STAT, &buf) == -1)
            return;

        double usage = (double)buf.__msg_cbytes / (double)buf.msg_qbytes;

        if (usage < QUEUE_LIMIT)
            return;

        usleep(WAIT_USEC);
    }
}


/** @brief Saves a log message */
void save_a_log(LogType log_type, const char* format, int msq_id)
{
    if (msq_id == -1)
        return;

    struct msqid_ds buf;
    int max_msgs;
    int current_msgs;

    while (1) {
        if (msgctl(msq_id, IPC_STAT, &buf) == -1) {
            perror("msgctl IPC_STAT error");
            return;
        }

        max_msgs = buf.msg_qbytes / sizeof(LogMessage);
        current_msgs = buf.msg_qnum;

        if (current_msgs < (int)(0.7 * max_msgs)) {
            break;
        }

        usleep(100);
    }

    LogMessage msg;
    msg.message_type = 1;
    msg.log_type = log_type;

    strncpy(msg.message, format, sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';

    if (msgsnd(msq_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd error");
    }
}


/** @brief Gets queue length */
int queue_length(int msq_id)
{
    struct msqid_ds buf;

    if (msgctl(msq_id, IPC_STAT, &buf) == -1) {
        perror(_("msgctl IPC_STAT error"));
        return -1;
    }

    return buf.msg_qnum;
}

/** @brief Resolves shared memory parameters for given section */
static void get_shm_params(
    SectionsIPC section_type,
    int* proj_id,
    size_t* size)
{
    switch (section_type) {
    case SEMAPHORES:
        *proj_id = SHM_SEMAPHORES;
        *size = sizeof(Semaphores);
        break;
    case QUEUES:
        *proj_id = SHM_QUEUES;
        *size = sizeof(Queues);
        break;
    case STORE_DATA:
        *proj_id = SHM_STORE_DATA;
        *size = sizeof(StoreData);
        break;
    case SIM_SETTINGS:
        *proj_id = SHM_SIM_SETTINGS;
        *size = sizeof(SimSettings);
        break;
    case SS_CHECKOUTS:
        *proj_id = SHM_SS_CHECKOUTS;
        *size = sizeof(SelfServiceCheckouts);
        break;
    case CHECKOUTS:
        *proj_id = SHM_CHECKOUTS;
        *size = sizeof(Checkouts);
        break;
    default:
        fprintf(stderr, "Unknown shared memory section\n");
        exit(EXIT_FAILURE);
    }
}

/** @brief Creates shared memory */
void* shm_create(int* id, SectionsIPC section_type)
{
    int proj_id;
    size_t size;

    get_shm_params(section_type, &proj_id, &size);

    key_t key = generate_key(proj_id);

    *id = shmget(key, size, IPC_CREAT | 0666);
    if (*id == -1) {
        perror(_("shmget create error"));
        exit(EXIT_FAILURE);
    }

    void* ptr = shmat(*id, NULL, 0);
    if (ptr == (void*)-1) {
        perror(_("shmat error"));
        exit(EXIT_FAILURE);
    }

    return ptr;
}

/** @brief Attaches shared memory */
void* shm_att(int* id, SectionsIPC section_type)
{
    int proj_id;
    size_t size;

    get_shm_params(section_type, &proj_id, &size);

    key_t key = generate_key(proj_id);

    *id = shmget(key, size, 0666);
    if (*id == -1) {
        perror(_("shmget attach error"));
        exit(EXIT_FAILURE);
    }

    void* ptr = shmat(*id, NULL, 0);
    if (ptr == (void*)-1) {
        perror(_("shmat error"));
        exit(EXIT_FAILURE);
    }

    return ptr;
}

/** @brief Detaches shared memory */
void shm_det(void* data)
{
    if (shmdt(data) == -1) {
        perror(_("shmdt error"));
    }
}

/** @brief Destroys shared memory */
void shm_destroy(int id, void* data)
{
    shm_det(data);

    if (shmctl(id, IPC_RMID, NULL) == -1) {
        perror(_("shmctl IPC_RMID error"));
    }
}

/** @brief Initializes internationalization */
void init_i18n(void)
{
    setlocale(LC_ALL, "");

    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) {
        perror(_("readlink error"));
        return;
    }

    exe_path[len] = '\0';
    char* dir = dirname(exe_path);

    char locale_path[PATH_MAX];
    snprintf(locale_path, sizeof(locale_path), "%s/locale", dir);

    bindtextdomain("messages", locale_path);
    textdomain("messages");
}
