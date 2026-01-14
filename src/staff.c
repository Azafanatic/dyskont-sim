#include <unistd.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"

int shm_sim_settings_id;
int shm_semaphores_id;
int shm_queues_id;
SimSettings *shm_sim_settings;
Semaphores *shm_semaphores;
Queues *shm_queues;

void shm_init();
void shm_close();

int main(int argc, char *argv[]) {
    shm_init();

    while (!shm_sim_settings->stop_sim) {
        StaffRequest req;
        if (msgrcv(shm_queues->msq_staff, &req, sizeof(req) - sizeof(long), 1, 0) == -1) {
            usleep(100000);
            continue;
        }

        char msgbuf[240];
        snprintf(msgbuf, sizeof(msgbuf), "Weryfikuje klienta %d... Wiek %d lat(a).\n", req.client.id, req.client.age);
        save_a_log(LOG_STAFF, msgbuf, shm_queues->msq_logger);

        StaffResponse resp;
        resp.message_type = req.client.id;
        resp.approved = (req.client.age >= 18) ? 1 : 0;

        if (msgsnd(shm_queues->msq_staff, &resp, sizeof(resp) - sizeof(long), 0) == -1) {
            perror("Blad msgsnd\n");
        }

        snprintf(msgbuf, sizeof(msgbuf), "Klient %d %s\n", req.client.id, resp.approved ? "ma >= 18 lat." : "ma < 18 lat.");
        save_a_log(LOG_STAFF, msgbuf, shm_queues->msq_logger);
    }

    shm_close();
    return 0;
}

void shm_init() {
    shm_queues = (Queues*) shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*) shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_sim_settings = (SimSettings*) shm_att(&shm_sim_settings_id, SIM_SETTINGS);
}

void shm_close() {
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_sim_settings);
}
