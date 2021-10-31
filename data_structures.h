#define LEVELS 32
#define MAX_TAG_NUMBER 256
#define MSG_MAX_SIZE 4096

#ifndef PROGETTO_SOA_DATA_STRUCTURES_H
#define PROGETTO_SOA_DATA_STRUCTURES_H

/**
 * Struttura a supporto dello sviluppo e l'utilizzo dei tag
 * */
struct tag{
    int exist;
    int key;
    int command;
    int permission;
    int private;
    int tag_id;
    int opened;
    struct level *structlevels;
};

/**
 * Struttura a supporto dello sviluppo e l'utilizzo dei livelli
 * */
struct level{
    char *bufs;
    int lvl;
    int tag;
    int is_empty;
    int is_queued;
    wait_queue_head_t wq;
    int reader;
};

/**
 * Struttura a supporto dello sviluppo e l'utilizzo del device driver
 * */
struct dev_struct{
    int tag;
    int thread;
    int sleepers;
};

#endif //PROGETTO_SOA_DATA_STRUCTURES_H