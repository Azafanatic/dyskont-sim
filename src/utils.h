#ifndef DYSKONT_UTILS_H
#define DYSKONT_UTILS_H

#include <errno.h>
#include <libintl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#define _(STRING) gettext(STRING)
#define LOCALEDIR "./locale"

#define IPC_KEY_FILE "/tmp/dyskont_sim.ipc"

#define MAX_SIM_LENGTH 86400
#define MAX_SIM_SPEED 1800
#define MAIN_PROCESSES 6
#define MAX_CLIENTS 128
#define MIN_PRODUCTS 3
#define MAX_PRODUCTS 10
#define PRODUCTS_AVAILABLE 32
#define MAX_SS_CHECKOUTS 6
#define MAX_CHECKOUTS 2
#define SIM_SPEED 600
#define SIM_LENGTH 7200

#define SEM_ID_SS_QUEUE 1
#define SEM_ID_QUEUE 2
#define SEM_ID_CHECKOUTS 3
#define SEM_ID_QUEUES 4
#define SEM_ID_STORE_DATA 5
#define SEM_ID_SIM_SETTINGS 6

#define SHM_SEMAPHORES 1
#define SHM_QUEUES 2
#define SHM_STORE_DATA 3
#define SHM_SS_CHECKOUTS 4
#define SHM_CHECKOUTS 5
#define SHM_SIM_SETTINGS 6

#define MSQ_ID_LOGGER 1
#define MSQ_ID_SS_CHECKOUTS 2
#define MSQ_ID_CHECKOUT_ONE 3
#define MSQ_ID_CHECKOUT_TWO 4
#define MSQ_ID_RECEIPTS 5
#define MSQ_ID_STAFF 6
#define MSQ_ID_CLIENT_RESP 7
#define MSQ_ID_SS_STAFF 8

/** @brief Generates a key using ftok */
key_t generate_key(int proj_id);

/** @brief IPC section types */
typedef enum {
    QUEUES, /**< Queues section */
    SEMAPHORES, /**< Semaphores section */
    STORE_DATA, /**< Store data section */
    SIM_SETTINGS, /**< Simulation settings section */
    SS_CHECKOUTS, /**< Self-service checkouts section */
    CHECKOUTS /**< Checkouts section */
} SectionsIPC;

/** @brief Log message types */
typedef enum {
    LOG_DEF, /**< Default log */
    LOG_SIM_INFO, /**< Simulation info */
    LOG_SIM_WARN, /**< Simulation warning */
    LOG_SIM_ERR, /**< Simulation error */
    LOG_SS_CHECKOUT, /**< Self-service checkout log */
    LOG_CHECKOUT, /**< Checkout log */
    LOG_MANAGER, /**< Manager log */
    LOG_CLIENT, /**< Client log */
    LOG_STAFF, /**< Staff log */
} LogType;

/** @brief Product categories */
typedef enum {
    ALCOHOL, /**< Alcohol products */
    COLD_CUTS, /**< Cold cuts */
    FRUIT, /**< Fruits */
    VEGETABLES, /**< Vegetables */
    BREAD, /**< Bread */
    DAIRY, /**< Dairy products */
    JUICES, /**< Juices */
    CARBONATED_DRINKS, /**< Carbonated drinks */
    SWEETS, /**< Sweets */
    DRY_GOODS, /**< Dry goods */
    OTHER /**< Other products */
} ProductCategory;

/** @brief Product structure */
typedef struct {
    char name[32]; /**< Product name */
    float price; /**< Product price */
    ProductCategory category; /**< Product category */
} Product;

/** @brief Client structure */
typedef struct {
    int id; /**< Client ID */
    int number_of_products; /**< Number of products */
    int age; /**< Client age */
    double shopping_time; /**< Shopping time */
    Product products[MAX_PRODUCTS]; /**< Client's products */
} Client;

/** @brief Checkout structure */
typedef struct {
    sig_atomic_t open; /**< Is checkout open */
    sig_atomic_t blocked; /**< Is checkout blocked */
    pid_t pid; /**< Process ID */
    int id; /**< Checkout ID */
    int clients_served; /**< Number of clients served */
    time_t last_client; /**< Last client time */
} Checkout;

/** @brief Log message structure */
typedef struct {
    long message_type; /**< Message type */
    LogType log_type; /**< Log type */
    char message[1024]; /**< Log message */
} LogMessage;

/** @brief Client message structure */
typedef struct {
    long message_type; /**< Message type */
    Client client; /**< Client data */
} ClientMessage;

/** @brief Receipt message structure */
typedef struct {
    long message_type; /**< Message type */
    char message[1024]; /**< Receipt message */
} ReceiptMessage;

/** @brief Age verification response structure */
typedef struct {
    long message_type; /**< Message type */
    int approved; /**< Approval status */
} AgeVerificationResponse;

/** @brief Client response structure */
typedef struct {
    long message_type; /**< Message type */
    int approved; /**< Approval status */
} ClientResponse;

/** @brief Age verification request structure */
typedef struct {
    long message_type; /**< Message type */
    Client client; /**< Client data */
    Checkout checkout; /**< Checkout data */
} AgeVerificationRequest;

/** @brief Self-service block message structure */
typedef struct {
    long message_type; /**< Message type */
    int checkout_id; /**< Checkout ID */
    char reason[240]; /**< Block reason */
} SSBlockMessage;

/** @brief Self-service checkouts structure */
typedef struct {
    int checkouts_opened; /**< Number of checkouts opened */
    Checkout checkout[MAX_SS_CHECKOUTS]; /**< Checkouts array */
} SelfServiceCheckouts;

/** @brief Checkouts structure */
typedef struct {
    int checkouts_opened; /**< Number of checkouts opened */
    Checkout checkout[MAX_CHECKOUTS]; /**< Checkouts array */
} Checkouts;

/** @brief Semaphores structure */
typedef struct {
    int sem_checkouts; /**< Checkouts semaphore */
    int sem_queues; /**< Queues semaphore */
    int sem_store_data; /**< Store data semaphore */
    int sem_sim_settings; /**< Simulation settings semaphore */
} Semaphores;

/** @brief Simulation settings structure */
typedef struct {
    int sim_length; /**< Simulation length */
    int sim_speed; /**< Simulation speed */
    volatile sig_atomic_t stop_sim; /**< Stop simulation flag */
    int evacuation; /**< Evacuation flag */
} SimSettings;

/** @brief Store data structure */
typedef struct {
    int num_of_clients_in_the_store; /**< Number of clients in store */
    int all_clients; /**< Total clients */
    int clients_not_served; /**< Clients not served */
    float money; /**< Money earned */
    int products_sold; /**< Products sold */
    bool open; /**< Is store open */
} StoreData;

/** @brief Queues structure */
typedef struct {
    int msq_logger; /**< Logger message queue */
    int msq_ss_checkouts; /**< Self-service checkouts queue */
    int msq_checkout_one; /**< Checkout one queue */
    int msq_checkout_two; /**< Checkout two queue */
    int msq_receipts; /**< Receipts queue */
    int msq_staff; /**< Staff queue */
    int msq_client_resp; /**< Client response queue */
    int msq_ss_staff; /**< Self-service staff queue */
} Queues;

/** @brief Creates a semaphore
 * @param key Semaphore key
 * @return Semaphore ID
 */
int create_a_semaphore(int key);

/** @brief Deletes a semaphore
 * @param semid Semaphore ID
 */
void del_a_semaphore(int semid);

/** @brief Waits on semaphore
 * @param semid Semaphore ID
 */
void operation_wait(int semid);

/** @brief Signals semaphore
 * @param semid Semaphore ID
 */
void operation_signal(int semid);

/** @brief Attaches to shared memory
 * @param id Shared memory ID
 * @param section_type Section type
 * @return Pointer to shared memory
 */
void* shm_att(int* id, SectionsIPC section_type);

/** @brief Creates shared memory
 * @param id Shared memory ID
 * @param section_type Section type
 * @return Pointer to shared memory
 */
void* shm_create(int* id, SectionsIPC section_type);

/** @brief Detaches from shared memory
 * @param data Pointer to shared memory
 */
void shm_det(void* data);

/** @brief Destroys shared memory
 * @param id Shared memory ID
 * @param data Pointer to shared memory
 */
void shm_destroy(int id, void* data);

/** @brief Saves a log message
 * @param log_type Log type
 * @param format Log message format
 * @param msq_id Message queue ID
 */
void save_a_log(LogType log_type, const char* format, int msq_id);

/** @brief Gets queue length
 * @param msq_id Message queue ID
 * @return Queue length
 */
int queue_length(int msq_id);

/** @brief Initializes internationalization
 */
void init_i18n();

#endif
