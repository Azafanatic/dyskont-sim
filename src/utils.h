#ifndef DYSKONT_UTILS_H
#define DYSKONT_UTILS_H

#include <signal.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CLIENTS 100
#define MIN_PRODUCTS 3
#define MAX_PRODUCTS 10
#define PRODUCTS_AVAILABLE 32
#define MAX_SS_CHECKOUTS 6
#define MAX_CHECKOUTS 2

#define SEM_ID_SS_QUEUE 6841
#define SEM_ID_QUEUE 6842
#define SEM_ID_CHECKOUTS 6843
#define SEM_ID_QUEUES 6844
#define SEM_ID_STORE_DATA 6845
#define SEM_ID_SIM_SETTINGS 6846

#define SHM_SEMAPHORES 4581
#define SHM_QUEUES 4582
#define SHM_STORE_DATA 4583
#define SHM_SS_CHECKOUTS 4584
#define SHM_CHECKOUTS 4585
#define SHM_SIM_SETTINGS 4586

#define MSQ_ID_LOGGER 6840
#define MSQ_ID_SS_CHECKOUTS 6841
#define MSQ_ID_CHECKOUT_ONE 6842
#define MSQ_ID_CHECKOUT_TWO 6842

typedef enum {
    QUEUES,
    SEMAPHORES,
    STORE_DATA,
    SIM_SETTINGS,
    SS_CHECKOUTS,
    CHECKOUTS
} SectionsIPC;

typedef enum {
    LOG_DEF,
    LOG_SIM_INFO,
    LOG_SIM_WARN,
    LOG_SIM_ERR,
    LOG_SS_CHECKOUT,
    LOG_CHECKOUT,
    LOG_MANAGER,
    LOG_CLIENT,
    LOG_STAFF,
} LogType;

typedef struct {
    long message_type;
    LogType log_type;
    char message[320];
} LogMessage;

typedef enum {
    ALKOHOLE,
    WEDLINY,
    OWOCE,
    WAZYWA,
    PIECZYWO,
    NABIAL,
    SOKI,
    NAPOJE_GAZOWANE,
    SLODYCZE,
    SUCHE,
    INNE
} ProductCategory;

typedef struct {
    char name[32];
    float price;
    ProductCategory category;
} Product;

typedef struct {
    int id;
    int number_of_products;
    int age;
    double shopping_time;
    Product products[MAX_PRODUCTS];
} Client;

typedef struct {
    long message_type;
    Client client;
} ClientMessage;

typedef struct {
    sig_atomic_t open;
    pid_t pid;
    int clients_served;
} Checkout;

typedef struct {
    int checkouts_opened;
    Checkout checkout[MAX_SS_CHECKOUTS];
} SelfServiceCheckouts;

typedef struct {
    int checkouts_opened;
    Checkout checkout[MAX_CHECKOUTS];
} Checkouts;

typedef struct {
    int sem_checkouts;
    int sem_queues;
    int sem_store_data;
    int sem_sim_settings;
} Semaphores;

typedef struct {
    int sim_length;
    int sim_speed;
    sig_atomic_t stop_sim;
} SimSettings;

typedef struct {
    int num_of_clients_in_the_store;
    int all_clients;
    int clients_not_served;
    float money;
    int products_sold;
    bool open;
} StoreData;

typedef struct {
    int msq_logger;
    int msq_ss_checkouts;
    int msq_checkout_one;
    int msq_checkout_two;
} Queues;

int create_a_semaphore(int key);
void del_a_semaphore(int semid);

void operation_wait(int semid);
void operation_signal(int semid);

void * shm_att(int * id, SectionsIPC section_type);
void * shm_create(int * id, SectionsIPC section_type);
void shm_det(void * data);
void shm_destroy(int id, void * data);

void save_a_log(LogType log_type, const char* format, int msq_id);

int queue_length(int msq_id);
void stand_in_the_queue(Client client, int msq_id);

#endif
