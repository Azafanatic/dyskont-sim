#include "utils.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/msg.h>
#include <unistd.h>

/** @brief Shared memory IDs */
int shm_sim_settings_id;
int shm_semaphores_id;
int shm_queues_id;
int shm_ss_checkouts_id;

/** @brief Shared memory pointers */
SimSettings* shm_sim_settings;
Semaphores* shm_semaphores;
Queues* shm_queues;
SelfServiceCheckouts* shm_ss_checkouts;

/** @brief Initializes shared memory */
void shm_init();
/** @brief Closes shared memory */
void shm_close();

int main(int argc, char* argv[])
{
    init_i18n();
    shm_init();

    while (!shm_sim_settings->stop_sim) {
        SSBlockMessage block_msg;
        if (msgrcv(shm_queues->msq_ss_staff, &block_msg, sizeof(block_msg) - sizeof(long), 1, IPC_NOWAIT) != -1) {
            char msgbuf[240];
            snprintf(msgbuf, sizeof(msgbuf), _("Self service checkout %d blocked: %s\n"), block_msg.checkout_id, block_msg.reason);
            save_a_log(LOG_STAFF, msgbuf, shm_queues->msq_logger);

            if (kill(shm_ss_checkouts->checkout[block_msg.checkout_id].pid, SIGUSR1) == -1) {
                perror(_("Kill error\n"));
            }

            snprintf(msgbuf, sizeof(msgbuf), _("Self service checkout %d unblocked.\n"), block_msg.checkout_id);
            save_a_log(LOG_STAFF, msgbuf, shm_queues->msq_logger);
        }

        AgeVerificationRequest req;
        if (msgrcv(shm_queues->msq_staff, &req, sizeof(req) - sizeof(long), 1, IPC_NOWAIT) == -1) {
            if (errno != ENOMSG && errno != EINTR) {
                perror(_("Msgrcv error\n"));
            }
        } else {
            char msgbuf[240];
            snprintf(msgbuf, sizeof(msgbuf), _("Verifying client %d... They are %d yo.\n"), req.client.id, req.client.age);
            save_a_log(LOG_STAFF, msgbuf, shm_queues->msq_logger);

            AgeVerificationResponse resp;
            resp.message_type = req.client.id;
            resp.approved = (req.client.age >= 18) ? 1 : 0;

            if (msgsnd(shm_queues->msq_staff, &resp, sizeof(resp) - sizeof(long), 0) == -1) {
                perror(_("Msgsnd error\n"));
            }

            snprintf(msgbuf, sizeof(msgbuf), _("Client %d %s\n"), req.client.id, resp.approved ? _("is >= 18 yo.") : _("is < 18 yo."));
            save_a_log(LOG_STAFF, msgbuf, shm_queues->msq_logger);
        }

        usleep(5000000 / shm_sim_settings->sim_speed);
    }

    shm_close();
    return 0;
}

/** @brief Initializes shared memory implementation */
void shm_init()
{
    shm_queues = (Queues*)shm_att(&shm_queues_id, QUEUES);
    shm_semaphores = (Semaphores*)shm_att(&shm_semaphores_id, SEMAPHORES);
    shm_sim_settings = (SimSettings*)shm_att(&shm_sim_settings_id, SIM_SETTINGS);
    shm_ss_checkouts = (SelfServiceCheckouts*)shm_att(&shm_ss_checkouts_id, SS_CHECKOUTS);
}

/** @brief Closes shared memory implementation */
void shm_close()
{
    shm_det(shm_queues);
    shm_det(shm_semaphores);
    shm_det(shm_sim_settings);
    shm_det(shm_ss_checkouts);
}
