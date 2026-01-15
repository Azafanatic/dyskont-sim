#include "logger.h"
#include "utils.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int shm_semaphores_id;
int shm_queues_id;
int shm_sim_settings_id;
Semaphores* shm_semaphores;
Queues* shm_queues;
SimSettings* shm_sim_settings;
LogMessage msg;

char path[64];
int file;

volatile sig_atomic_t stop_sim;

void shm_init();
void shm_close();
void sigint_handler(int sig);
void logger();

int main()
{
    init_i18n();
    mkdir("logs", 0755);
    stop_sim = 0;
    signal(SIGINT, sigint_handler);

    shm_init();

    sprintf(path, "logs/log_%d.txt", (int)time(NULL));

    file = open(path, O_WRONLY | O_CREAT, 0644);
    if (file == -1) {
        exit(1);
    }

    while (!stop_sim) {
        logger();
    }

    close(file);

    shm_close();

    exit(0);
}

void shm_init()
{
    shm_queues = (Queues*)shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*)shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_sim_settings = (SimSettings*)shm_att(&shm_sim_settings_id, SIM_SETTINGS);
};

void shm_close()
{
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_sim_settings);
};

void sigint_handler(int sig)
{
    if (sig == SIGINT)
        stop_sim = 1;
}

void logger()
{
    if (msgrcv(shm_queues->msq_logger, &msg, sizeof(msg) - sizeof(long), 0, 0) == -1) {
        return;
    }

    const char* prefix = "";
    const char* colour = COL_INFO;

    switch (msg.log_type) {
    case LOG_SIM_INFO:
        prefix = _("[INFO] ");
        colour = COL_INFO;
        break;
    case LOG_SIM_WARN:
        prefix = _("[WARNING] ");
        colour = COL_WARN;
        break;
    case LOG_SIM_ERR:
        prefix = _("[ERROR] ");
        colour = COL_ERR;
        break;
    case LOG_DEF:
        prefix = "";
        colour = COL_DEF;
        break;
    case LOG_MANAGER:
        prefix = _("[MANAGER] ");
        colour = COL_MANAGER;
        break;
    case LOG_CLIENT:
        prefix = _("[CLIENT] ");
        colour = COL_CLIENT;
        break;
    case LOG_STAFF:
        prefix = _("[STAFF] ");
        colour = COL_STAFF;
        break;
    case LOG_SS_CHECKOUT:
        prefix = _("[SELF SERVICE CHECKOUT] ");
        colour = COL_SS_CHECKOUT;
        break;
    case LOG_CHECKOUT:
        prefix = _("[CHECKOUT] ");
        colour = COL_CHECKOUT;
        break;
    }

    printf("%s%s%s%s", colour, prefix, msg.message, COL_DEF);

    char message_buf[512];
    sprintf(message_buf, "%s%s", prefix, msg.message);
    write(file, message_buf, strlen(message_buf));
}
