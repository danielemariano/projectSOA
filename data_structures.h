#define LEVELS 32
#define MAX_TAG_NUMBER 256
#define MSG_MAX_SIZE 4096

#ifndef PROGETTO_SOA_DATA_STRUCTURES_H
#define PROGETTO_SOA_DATA_STRUCTURES_H


/**
 * Struttura a supporto dello sviluppo e l'utilizzo dei tag
 */
struct tag{
    int exist;
    // campo per capire se un tag esiste o meno
    int key;
    // campo che contiene la chiave del tag
    int command;
    // campo che contiene il comando associato all'operazione sul tag
    int permission;
    // flag che ci consente di capire se il tag è privato o meno
    int private;
    // campo che ci permette di salvare l'id del thread che ha aperto
    // il servizio in modalità privata, in modo da permettere solo
    // a lui di poterlo riaprire
    int tag_id;
    // campo ridondante con key
    int opened;
    // campo per capire se un tag è aperto o meno
    struct level *structlevels;
    // puntatore alla struttura livello associata a quella del tag
};


/**
 * Struttura a supporto dello sviluppo e l'utilizzo dei livelli
 */
struct level{
    char *bufs;
    // buffer per salvare il messaggio corrispondente alla coppia livello/tag
    int lvl;
    // descrittore univoco (per quel tag) del livello
    int tag;
    // descrittore univoco del tag
    int is_empty;
    // flag che ci permette di capire se un livello è vuoto o meno
    int is_queued;
    // flag che ci permette di eliminare il tag nel caso volessimo
    // eliminare solo un thread che è stato messo in coda
    wait_queue_head_t wq;
    // struttura che ci permette di mettere in attesa i thread e di
    // tenere traccia e gestire questi thread
    int reader;
    // campo che tiene traccia del numero di lettori (aggiornati atomicamente)
};

/**
 * Struttura a supporto dello sviluppo e l'utilizzo del device driver
 */
struct dev_struct{
    int tag;
    // campo che contiene il descrittore univoco del tag
    int thread;
    // campo che contiene il descrittore univoco del thread
    int sleepers;
    // campo che contiene il numero di thread che stanno aspettando
};

#endif //PROGETTO_SOA_DATA_STRUCTURES_H